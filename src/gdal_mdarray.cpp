#include "gdal_mdarray.hpp"
#include "gdal_group.hpp"
#include "gdal_common.hpp"
#include "gdal_driver.hpp"
#include "collections/array_dimensions.hpp"
#include "collections/array_attributes.hpp"
#include "geometry/gdal_geometry.hpp"
#include "gdal_layer.hpp"
#include "gdal_majorobject.hpp"
#include "gdal_spatial_reference.hpp"
#include "utils/typed_array.hpp"

namespace node_gdal {

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

Nan::Persistent<FunctionTemplate> MDArray::constructor;

void MDArray::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(MDArray::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("MDArray").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan__SetPrototypeAsyncableMethod(lcons, "read", read);
  Nan::SetPrototypeMethod(lcons, "getView", getView);
  Nan::SetPrototypeMethod(lcons, "getMask", getMask);
  Nan::SetPrototypeMethod(lcons, "asDataset", asDataset);

  ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER);
  ATTR(lcons, "srs", srsGetter, READ_ONLY_SETTER);
  ATTR(lcons, "dataType", typeGetter, READ_ONLY_SETTER);
  ATTR(lcons, "length", lengthGetter, READ_ONLY_SETTER);
  ATTR(lcons, "unitType", unitTypeGetter, READ_ONLY_SETTER);
  ATTR(lcons, "scale", scaleGetter, READ_ONLY_SETTER);
  ATTR(lcons, "offset", offsetGetter, READ_ONLY_SETTER);
  ATTR(lcons, "noDataValue", noDataValueGetter, READ_ONLY_SETTER);
  ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER);
  ATTR(lcons, "dimensions", dimensionsGetter, READ_ONLY_SETTER);
  ATTR(lcons, "attributes", attributesGetter, READ_ONLY_SETTER);

  Nan::Set(target, Nan::New("MDArray").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

MDArray::MDArray(std::shared_ptr<GDALMDArray> md) : Nan::ObjectWrap(), uid(0), this_(md) {
  LOG("Created MDArray [%p]", md.get());
}

MDArray::~MDArray() {
  dispose();
}

void MDArray::dispose() {
  if (this_) {

    LOG("Disposing array [%p]", this_.get());

    object_store.dispose(uid);

    LOG("Disposed array [%p]", this_.get());
  }
};

/**
 * A representation of an array with access methods.
 *
 * @class MDArray
 */
NAN_METHOD(MDArray::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }

  if (info.Length() == 2 && info[0]->IsExternal() && info[1]->IsObject()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    MDArray *f = static_cast<MDArray *>(ptr);
    f->Wrap(info.This());

    Local<Value> dims = ArrayDimensions::New(info.This(), info[1]);
    Nan::SetPrivate(info.This(), Nan::New("dims_").ToLocalChecked(), dims);
    Local<Value> attrs = ArrayAttributes::New(info.This(), info[1]);
    Nan::SetPrivate(info.This(), Nan::New("attrs_").ToLocalChecked(), attrs);

    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create MDArray directly. Create with dataset instead.");
    return;
  }

  info.GetReturnValue().Set(info.This());
}

Local<Value> MDArray::New(std::shared_ptr<GDALMDArray> raw, GDALDataset *parent_ds) {
  Nan::EscapableHandleScope scope;

  if (!raw) { return scope.Escape(Nan::Null()); }
  if (object_store.has(raw)) { return scope.Escape(object_store.get(raw)); }

  MDArray *wrapped = new MDArray(raw);

  // add reference to datasource so datasource doesnt get GC'ed while group is
  // alive
  Local<Object> ds;
  if (object_store.has(parent_ds)) {
    ds = object_store.get(parent_ds);
  } else {
    LOG("MDArray's parent dataset disappeared from cache (array = %p, dataset = %p)", raw.get(), parent_ds);
    Nan::ThrowError("MDArray's parent dataset disappeared from cache");
    return scope.Escape(Nan::Undefined());
  }

  Local<Value> ext = Nan::New<External>(wrapped);
  Local<Value> argv[] = {ext, ds};
  Local<Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(MDArray::constructor)).ToLocalChecked(), 2, argv).ToLocalChecked();

  size_t dim = raw->GetDimensionCount();

  Dataset *unwrapped_ds = Nan::ObjectWrap::Unwrap<Dataset>(ds);
  long parent_uid = unwrapped_ds->uid;

  wrapped->uid = object_store.add(raw, wrapped->persistent(), parent_uid);
  wrapped->parent_ds = parent_ds;
  wrapped->parent_uid = parent_uid;
  wrapped->dimensions = dim;

  Nan::SetPrivate(obj, Nan::New("ds_").ToLocalChecked(), ds);

  return scope.Escape(obj);
}

NAN_METHOD(MDArray::toString) {
  info.GetReturnValue().Set(Nan::New("MDArray").ToLocalChecked());
}

/* Find the lowest possible element index for the given spans and strides */
static inline int
findLowest(int dimensions, std::shared_ptr<size_t> span, std::shared_ptr<GPtrDiff_t> stride, GPtrDiff_t offset) {
  GPtrDiff_t dimStride = 1;
  GPtrDiff_t lowest = 0;
  for (int dim = 0; dim < dimensions; dim++) {
    if (stride != nullptr) {
      // strides are given
      dimStride = stride.get()[dim];
    } else {
      // default strides: 1 on the first dimension -or- (size of the current dimensions) x (stride of the previous dimension)
      if (dim > 0) dimStride = dimStride * span.get()[dim - 1];
    }

    // Lowest address element on this dimension
    size_t element;
    if (dimStride < 0)
      element = span.get()[dim] - 1;
    else
      element = 0;

    lowest += element * dimStride;
  }

  return offset + lowest;
}

/* Find the highest possible element index for the given spans and strides */
static inline int
findHighest(int dimensions, std::shared_ptr<size_t> span, std::shared_ptr<GPtrDiff_t> stride, GPtrDiff_t offset) {
  GPtrDiff_t dimStride = 1;
  GPtrDiff_t highest = 0;
  for (int dim = 0; dim < dimensions; dim++) {
    if (stride != nullptr) {
      // strides are given
      dimStride = stride.get()[dim];
    } else {
      // default strides: 1 on the first dimension -or- (size of the current dimensions) x (stride of the previous dimension)
      if (dim > 0) dimStride = dimStride * span.get()[dim - 1];
    }

    // Highest address element on this dimension
    size_t element;
    if (dimStride > 0)
      element = span.get()[dim] - 1;
    else
      element = 0;

    highest += element * dimStride;
  }

  return offset + highest;
}

/**
 * @typedef {object} MDArrayOptions<T extends TypedArray<number> | TypedArray<bigint> = TypedArray<number>>
 * @property {number[]} origin
 * @property {number[]} span
 * @property {number[]} [stride]
 * @property {string} [data_type]
 * @property {T} [data]
 * @property {number} [_offset]
 */

/**
 * Read data from the MDArray.
 *
 * This will extract the context of a (hyper-)rectangle from the array into a buffer.
 * If the buffer can be passed as an argument or it can be allocated by the function.
 * Generalized n-dimensional strides are supported.
 *
 * Although this method can be used in its raw form, it works best when used with the ndarray plugin.
 *
 * @method read<T extends TypedArray<number> | TypedArray<bigint> = TypedArray<number>>
 * @instance
 * @memberof MDArray
 * @throws {Error}
 * @param {MDArrayOptions<T>} options
 * @param {number[]} options.origin An array of the starting indices
 * @param {number[]} options.span An array specifying the number of elements to read in each dimension
 * @param {number[]} [options.stride] An array of strides for the output array, mandatory if the array is specified
 * @param {string} [options.data_type] See {@link GDT|GDT constants}
 * @param {T} [options.data] The `TypedArray` to put the data in. A new array is created if not given.
 * @return {T}
 */

/**
 * Read data from the MDArray.
 * @async
 *
 * This will extract the context of a (hyper-)rectangle from the array into a buffer.
 * If the buffer can be passed as an argument or it can be allocated by the function.
 * Generalized n-dimensional strides are supported.
 *
 * Although this method can be used in its raw form, it works best when used with the ndarray plugin.
 *
 * @method readAsync<T extends TypedArray<number> | TypedArray<bigint> = TypedArray<number>>
 * @instance
 * @memberof MDArray
 * @throws {Error}
 * @param {MDArrayOptions<T>} options
 * @param {number[]} options.origin An array of the starting indices
 * @param {number[]} options.span An array specifying the number of elements to read in each dimension
 * @param {number[]} [options.stride] An array of strides for the output array, mandatory if the array is specified
 * @param {string} [options.data_type] See {@link GDT|GDT constants}
 * @param {T} [options.data] The `TypedArray` to put the data in. A new array is created if not given.
 * @param {ProgressCb} [options.progress_cb]
 * @param {callback<T>} [callback=undefined]
 * @return {Promise<T>} A `TypedArray` of values.
 */
GDAL_ASYNCABLE_DEFINE(MDArray::read) {

  NODE_UNWRAP_CHECK(MDArray, info.This(), self);

  Local<Object> options;
  Local<Array> origin, span, stride;
  std::string type_name;
  GDALDataType type = GDT_Byte;
  GPtrDiff_t offset = 0;

  NODE_ARG_OBJECT(0, "options", options);
  NODE_ARRAY_FROM_OBJ(options, "origin", origin);
  NODE_ARRAY_FROM_OBJ(options, "span", span);
  NODE_ARRAY_FROM_OBJ_OPT(options, "stride", stride);
  NODE_STR_FROM_OBJ_OPT(options, "data_type", type_name);
  NODE_INT64_FROM_OBJ_OPT(options, "_offset", offset);
  if (!type_name.empty()) { type = GDALGetDataTypeByName(type_name.c_str()); }

  std::shared_ptr<GUInt64> gdal_origin;
  std::shared_ptr<size_t> gdal_span;
  std::shared_ptr<GPtrDiff_t> gdal_stride;
  try {
    gdal_origin = NumberArrayToSharedPtr<int64_t, GUInt64>(origin, self->dimensions);
    gdal_span = NumberArrayToSharedPtr<int64_t, size_t>(span, self->dimensions);
    gdal_stride = NumberArrayToSharedPtr<int64_t, GPtrDiff_t>(stride, self->dimensions);
  } catch (const char *e) {
    Nan::ThrowError(e);
    return;
  }
  GPtrDiff_t highest = findHighest(self->dimensions, gdal_span, gdal_stride, offset);
  GPtrDiff_t lowest = findLowest(self->dimensions, gdal_span, gdal_stride, offset);
  size_t length = (highest - (lowest < 0 ? lowest : 0)) + 1;

  Local<String> sym = Nan::New("data").ToLocalChecked();
  Local<Value> data;
  Local<Object> array;
  if (Nan::HasOwnProperty(options, sym).FromMaybe(false)) {
    data = Nan::Get(options, sym).ToLocalChecked();
    if (!data->IsUndefined() && !data->IsNull()) {
      array = data.As<Object>();
      type = node_gdal::TypedArray::Identify(array);
      if (type == GDT_Unknown) {
        Nan::ThrowError("Invalid array");
        return;
      }
    }
  }

  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, self, gdal_mdarray);

  // create array if no array was passed
  if (data.IsEmpty()) {
    if (type_name.empty()) {
      auto exType = gdal_mdarray->GetDataType();
      if (exType.GetClass() != GEDTC_NUMERIC) {
        Nan::ThrowTypeError("Reading of extended data types is not supported yet");
        return;
      }
      type = exType.GetNumericDataType();
    }
    data = node_gdal::TypedArray::New(type, length);
    if (data.IsEmpty() || !data->IsObject()) {
      Nan::ThrowError("Failed to allocate array");
      return; // TypedArray::New threw an error
    }
    array = data.As<Object>();
  }

  if (lowest < 0) {
    Nan::ThrowRangeError("Will have to read before the start of the array");
    return;
  }

  void *buffer = node_gdal::TypedArray::Validate(array, type, length);
  if (!buffer) {
    Nan::ThrowError("Failed to allocate array");
    return; // TypedArray::Validate threw an error
  }

  GDALAsyncableJob<bool> job(self->parent_uid);
  job.persist("array", array);

  job.main =
    [buffer, gdal_mdarray, gdal_origin, gdal_span, gdal_stride, type, length, offset](const GDALExecutionProgress &) {
      int bytes_per_pixel = GDALGetDataTypeSize(type) / 8;
      if (bytes_per_pixel == 0) { throw "Invalid GDAL data type"; }
      CPLErrorReset();
      GDALExtendedDataType gdal_type = GDALExtendedDataType::Create(type);
      bool success = gdal_mdarray->Read(
        gdal_origin.get(),
        gdal_span.get(),
        nullptr,
        gdal_stride.get(),
        gdal_type,
        (void *)((uint8_t *)buffer + offset * bytes_per_pixel),
        buffer,
        length * bytes_per_pixel);
      if (!success) { throw CPLGetLastErrorMsg(); }
      return success;
    };
  job.rval = [](bool success, const GetFromPersistentFunc &getter) { return getter("array"); };
  job.run(info, async, 1);
}

/**
 * Get a partial view of the MDArray.
 *
 * The slice expression uses the same syntax as NumPy basic slicing and indexing. See (https://www.numpy.org/devdocs/reference/arrays.indexing.html#basic-slicing-and-indexing). Or it can use field access by name. See (https://www.numpy.org/devdocs/reference/arrays.indexing.html#field-access).
 *
 * @method getView
 * @instance
 * @memberof MDArray
 * @throws {Error}
 * @param {string} view
 * @return {MDArray}
 */
NAN_METHOD(MDArray::getView) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, array, raw);

  std::string viewExpr;
  NODE_ARG_STR(0, "view", viewExpr);
  GDAL_LOCK_PARENT(array);
  CPLErrorReset();
  std::shared_ptr<GDALMDArray> view = raw->GetView(viewExpr);
  if (view == nullptr) {
    Nan::ThrowError(CPLGetLastErrorMsg());
    return;
  }
  Local<Value> obj = New(view, array->parent_ds);
  info.GetReturnValue().Set(obj);
}

/**
 * Return an array that is a mask for the current array.
 *
 * This array will be of type Byte, with values set to 0 to indicate invalid pixels of the current array, and values set to 1 to indicate valid pixels.
 *
 * The generic implementation honours the NoDataValue, as well as various netCDF CF attributes: missing_value, _FillValue, valid_min, valid_max and valid_range.
 *
 * @method getMask
 * @instance
 * @memberof MDArray
 * @throws {Error}
 * @return {MDArray}
 */
NAN_METHOD(MDArray::getMask) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, array, raw);

  GDAL_LOCK_PARENT(array);
  CPLErrorReset();
  std::shared_ptr<GDALMDArray> mask = raw->GetMask(NULL);
  if (mask == nullptr) {
    Nan::ThrowError(CPLGetLastErrorMsg());
    return;
  }
  Local<Value> obj = New(mask, array->parent_ds);
  info.GetReturnValue().Set(obj);
}

/**
 * Return a view of this array as a gdal.Dataset (ie 2D)
 *
 * In the case of > 2D arrays, additional dimensions will be represented as raster bands.
 *
 * @method asDataset
 * @instance
 * @memberof MDArray
 * @param {number|string} x dimension to be used as X axis
 * @param {number|string} y dimension to be used as Y axis
 * @throws {Error}
 * @return {Dataset}
 */
NAN_METHOD(MDArray::asDataset) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, array, raw);

  int x = -1, y = -1;
  std::string dim;
  NODE_ARG_STR_INT(0, "x", dim, x, isXString);
  if (isXString) x = ArrayDimensions::__getIdx(raw, dim);
  NODE_ARG_STR_INT(1, "y", dim, y, isYString);
  if (isYString) y = ArrayDimensions::__getIdx(raw, dim);

  GDAL_LOCK_PARENT(array);
  CPLErrorReset();
  GDALDataset *ds = raw->AsClassicDataset(x, y);
  if (ds == nullptr) {
    Nan::ThrowError(CPLGetLastErrorMsg());
    return;
  }
  Local<Value> obj = Dataset::New(ds, array->parent_ds);
  info.GetReturnValue().Set(obj);
}

/**
 * Spatial reference associated with MDArray.
 *
 * @throws {Error}
 * @kind member
 * @name srs
 * @instance
 * @memberof MDArray
 * @type {SpatialReference|null}
 */
NAN_GETTER(MDArray::srsGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, array, raw);
  GDAL_LOCK_PARENT(array);
  std::shared_ptr<OGRSpatialReference> srs = raw->GetSpatialRef();
  if (srs == nullptr) {
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  info.GetReturnValue().Set(SpatialReference::New(srs.get(), false));
}

/**
 * Raster value offset.
 *
 * @kind member
 * @name offset
 * @instance
 * @memberof MDArray
 * @type {number}
 */
NAN_GETTER(MDArray::offsetGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  bool hasOffset = false;
  GDAL_LOCK_PARENT(array);
  double result = array->this_->GetOffset(&hasOffset);
  if (hasOffset)
    info.GetReturnValue().Set(Nan::New<Number>(result));
  else
    info.GetReturnValue().Set(Nan::New<Number>(0));
}

/**
 * Raster value scale.
 *
 * @kind member
 * @name scale
 * @instance
 * @memberof MDArray
 * @type {number}
 */
NAN_GETTER(MDArray::scaleGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  bool hasScale = false;
  GDAL_LOCK_PARENT(array);
  double result = array->this_->GetScale(&hasScale);
  if (hasScale)
    info.GetReturnValue().Set(Nan::New<Number>(result));
  else
    info.GetReturnValue().Set(Nan::New<Number>(1));
}

/**
 * No data value for this array.
 *
 * @kind member
 * @name noDataValue
 * @instance
 * @memberof MDArray
 * @type {number|null}
 */
NAN_GETTER(MDArray::noDataValueGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  bool hasNoData = false;
  GDAL_LOCK_PARENT(array);
  double result = array->this_->GetNoDataValueAsDouble(&hasNoData);

  if (hasNoData && !std::isnan(result)) {
    info.GetReturnValue().Set(Nan::New<Number>(result));
    return;
  } else {
    info.GetReturnValue().Set(Nan::Null());
    return;
  }
}

/**
 * Raster unit type (name for the units of this raster's values).
 * For instance, it might be `"m"` for an elevation model in meters,
 * or `"ft"` for feet. If no units are available, a value of `""`
 * will be returned.
 *
 * @kind member
 * @name unitType
 * @instance
 * @memberof MDArray
 * @type {string}
 */
NAN_GETTER(MDArray::unitTypeGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_LOCK_PARENT(array);
  std::string unit = array->this_->GetUnit();
  info.GetReturnValue().Set(SafeString::New(unit.c_str()));
}

/**
 * @readonly
 * @kind member
 * @name dataType
 * @instance
 * @memberof MDArray
 * @type {string}
 */
NAN_GETTER(MDArray::typeGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, array, raw);
  GDAL_LOCK_PARENT(array);
  GDALExtendedDataType type = raw->GetDataType();
  const char *r;
  switch (type.GetClass()) {
    case GEDTC_NUMERIC: r = GDALGetDataTypeName(type.GetNumericDataType()); break;
    case GEDTC_STRING: r = "String"; break;
    case GEDTC_COMPOUND: r = "Compound"; break;
    default: Nan::ThrowError("Invalid attribute type"); return;
  }
  info.GetReturnValue().Set(SafeString::New(r));
}

/**
 * @readonly
 * @kind member
 * @name dimensions
 * @instance
 * @memberof MDArray
 * @type {GroupDimensions}
 */
NAN_GETTER(MDArray::dimensionsGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("dims_").ToLocalChecked()).ToLocalChecked());
}

/**
 * @readonly
 * @kind member
 * @name attributes
 * @instance
 * @memberof MDArray
 * @type {ArrayAttributes}
 */
NAN_GETTER(MDArray::attributesGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("attrs_").ToLocalChecked()).ToLocalChecked());
}

/**
 * @readonly
 * @kind member
 * @name description
 * @instance
 * @memberof MDArray
 * @type {string}
 */
NAN_GETTER(MDArray::descriptionGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, array, raw);
  GDAL_LOCK_PARENT(array);
  std::string description = raw->GetFullName();
  info.GetReturnValue().Set(SafeString::New(description.c_str()));
}

/**
 * The flattened length of the array.
 *
 * @readonly
 * @kind member
 * @name length
 * @instance
 * @memberof MDArray
 * @type {number}
 */
NODE_WRAPPED_GETTER_WITH_RESULT_LOCKED(MDArray, lengthGetter, Number, GetTotalElementsCount);

NAN_GETTER(MDArray::uidGetter) {
  MDArray *ds = Nan::ObjectWrap::Unwrap<MDArray>(info.This());
  info.GetReturnValue().Set(Nan::New((int)ds->uid));
}

#endif

} // namespace node_gdal
