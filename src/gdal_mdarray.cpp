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

Napi::FunctionReference MDArray::constructor;

void MDArray::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(MDArray);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "MDArray",
    {
        METHOD(toString)
        METHOD_ASYNCABLE(read)
        METHOD(getView)
        METHOD(getMask)
        METHOD(asDataset)
        ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER)
        ATTR(lcons, "srs", srsGetter, READ_ONLY_SETTER)
        ATTR(lcons, "dataType", typeGetter, READ_ONLY_SETTER)
        ATTR(lcons, "length", lengthGetter, READ_ONLY_SETTER)
        ATTR(lcons, "unitType", unitTypeGetter, READ_ONLY_SETTER)
        ATTR(lcons, "scale", scaleGetter, READ_ONLY_SETTER)
        ATTR(lcons, "offset", offsetGetter, READ_ONLY_SETTER)
        ATTR(lcons, "noDataValue", noDataValueGetter, READ_ONLY_SETTER)
        ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER)
        ATTR(lcons, "dimensions", dimensionsGetter, READ_ONLY_SETTER)
        ATTR(lcons, "attributes", attributesGetter, READ_ONLY_SETTER)
    });

  target.Set("MDArray", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

MDArray::MDArray(const Napi::CallbackInfo &info) : GDALObject<MDArray>(info), uid(0), this_(nullptr) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = node_gdal::ImportShared<GDALMDArray>(info);
    LOG("Created MDArray [%p]", this_.get());
  } else {
    Napi::Error::New(info.Env(), "Cannot create MDArray directly").ThrowAsJavaScriptException();
    return;
  }

  Napi::Value parent_ds = info.Length() > 1 ? info[1] : info.Env().Undefined();
  GDAL_SET_PRIVATE(info.This(), "dims_", ArrayDimensions::New(info.This(), parent_ds));
  GDAL_SET_PRIVATE(info.This(), "attrs_", ArrayAttributes::New(info.This(), parent_ds));
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

Napi::Value MDArray::New(std::shared_ptr<GDALMDArray> raw, GDALDataset *parent_ds) {

  if (!raw) { return node_gdal::napi_env().Null(); }
  if (object_store.has(raw)) { return object_store.get(raw); }

  // add reference to datasource so datasource doesnt get GC'ed while group is
  // alive
  Napi::Object ds;
  if (object_store.has(parent_ds)) {
    ds = object_store.get(parent_ds);
  } else {
    LOG("MDArray's parent dataset disappeared from cache (array = %p, dataset = %p)", raw.get(), parent_ds);
    Napi::Error::New(node_gdal::napi_env(), "MDArray's parent dataset disappeared from cache").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  std::vector<napi_value> args = {
    Napi::External<void>::New(node_gdal::napi_env(), node_gdal::ExportShared(raw)), ds};
  Napi::Object obj = MDArray::constructor.Value().New(args);
  MDArray *wrapped = node_gdal::UnwrapWrapped<MDArray>(obj);

  size_t dim = raw->GetDimensionCount();

  Dataset *unwrapped_ds = node_gdal::UnwrapWrapped<Dataset>(ds);
  long parent_uid = unwrapped_ds->uid;

  wrapped->uid = object_store.add(raw, *wrapped, parent_uid);
  wrapped->parent_ds = parent_ds;
  wrapped->parent_uid = parent_uid;
  wrapped->dimensions = dim;


  return obj;
}

NAPI_METHOD(MDArray::toString) {
  return Napi::String::New(node_gdal::napi_env(), "MDArray");
}

/* Find the lowest possible element index for the given spans and strides */
static inline int
findLowest(int dimensions, std::shared_ptr<size_t[]> span, std::shared_ptr<GPtrDiff_t[]> stride, GPtrDiff_t offset) {
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
findHighest(int dimensions, std::shared_ptr<size_t[]> span, std::shared_ptr<GPtrDiff_t[]> stride, GPtrDiff_t offset) {
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

  Napi::Object options;
  Napi::Array origin, span, stride;
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

  std::shared_ptr<GUInt64[]> gdal_origin;
  std::shared_ptr<size_t[]> gdal_span;
  std::shared_ptr<GPtrDiff_t[]> gdal_stride;
  try {
    gdal_origin = NumberArrayToSharedPtr<int64_t, GUInt64>(origin, self->dimensions);
    gdal_span = NumberArrayToSharedPtr<int64_t, size_t>(span, self->dimensions);
    gdal_stride = NumberArrayToSharedPtr<int64_t, GPtrDiff_t>(stride, self->dimensions);
  } catch (const char *e) {
    Napi::Error::New(node_gdal::napi_env(), e).ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  GPtrDiff_t highest = findHighest(self->dimensions, gdal_span, gdal_stride, offset);
  GPtrDiff_t lowest = findLowest(self->dimensions, gdal_span, gdal_stride, offset);
  size_t length = (highest - (lowest < 0 ? lowest : 0)) + 1;

  Napi::String sym = Napi::String::New(node_gdal::napi_env(), "data");
  Napi::Value data;
  Napi::Object array;
  if (options.As<Napi::Object>().HasOwnProperty(sym)) {
    data = options.As<Napi::Object>().Get(sym);
    if (!data.IsUndefined() && !data.IsNull()) {
      array = data.As<Napi::Object>();
      type = node_gdal::TypedArray::Identify(array);
      if (type == GDT_Unknown) {
        Napi::Error::New(node_gdal::napi_env(), "Invalid array").ThrowAsJavaScriptException();
        return node_gdal::napi_env().Undefined();
      }
    }
  }

  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, self, gdal_mdarray);

  // create array if no array was passed
  if (data.IsEmpty()) {
    if (type_name.empty()) {
      auto exType = gdal_mdarray->GetDataType();
      if (exType.GetClass() != GEDTC_NUMERIC) {
        Napi::TypeError::New(node_gdal::napi_env(), "Reading of extended data types is not supported yet").ThrowAsJavaScriptException();
        return node_gdal::napi_env().Undefined();
      }
      type = exType.GetNumericDataType();
    }
    data = node_gdal::TypedArray::New(type, length);
    if (data.IsEmpty() || !data.IsObject()) {
      node_gdal::ThrowOverPending(node_gdal::napi_env(), "Failed to allocate array");
      return node_gdal::napi_env().Undefined(); // TypedArray::New threw an error
    }
    array = data.As<Napi::Object>();
  }

  if (lowest < 0) {
    Napi::RangeError::New(node_gdal::napi_env(), "Will have to read before the start of the array").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  void *buffer = node_gdal::TypedArray::Validate(array, type, length);
  if (!buffer) {
    node_gdal::ThrowOverPending(node_gdal::napi_env(), "Failed to allocate array");
    return node_gdal::napi_env().Undefined(); // TypedArray::Validate threw an error
  }

  GDALAsyncableJob<bool> job(self->parent_uid);
  job.persist("array", array);

  job.main =
    [buffer, gdal_mdarray, gdal_origin, gdal_span, gdal_stride, type, length, offset](const GDALExecutionProgress &) {
      int bytes_per_pixel = GDALGetDataTypeSizeBytes(type);
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
  return job.run(info, async, 1);
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
NAPI_METHOD(MDArray::getView) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, array, raw);

  std::string viewExpr;
  NODE_ARG_STR(0, "view", viewExpr);
  GDAL_LOCK_PARENT(array);
  CPLErrorReset();
  std::shared_ptr<GDALMDArray> view = raw->GetView(viewExpr);
  if (view == nullptr) {
    Napi::Error::New(node_gdal::napi_env(), CPLGetLastErrorMsg()).ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  Napi::Value obj = New(view, array->parent_ds);
  return obj;
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
NAPI_METHOD(MDArray::getMask) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, array, raw);

  GDAL_LOCK_PARENT(array);
  CPLErrorReset();
  std::shared_ptr<GDALMDArray> mask = raw->GetMask(NULL);
  if (mask == nullptr) {
    Napi::Error::New(node_gdal::napi_env(), CPLGetLastErrorMsg()).ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  Napi::Value obj = New(mask, array->parent_ds);
  return obj;
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
NAPI_METHOD(MDArray::asDataset) {
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
    Napi::Error::New(node_gdal::napi_env(), CPLGetLastErrorMsg()).ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  Napi::Value obj = Dataset::New(ds, array->parent_ds);
  return obj;
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
NAPI_GETTER(MDArray::srsGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, array, raw);
  GDAL_LOCK_PARENT(array);
  std::shared_ptr<OGRSpatialReference> srs = raw->GetSpatialRef();
  if (srs == nullptr) {
    return node_gdal::napi_env().Null();
    return node_gdal::napi_env().Undefined();
  }

  return SpatialReference::New(srs.get(), false);
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
NAPI_GETTER(MDArray::offsetGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  bool hasOffset = false;
  GDAL_LOCK_PARENT(array);
  double result = array->this_->GetOffset(&hasOffset);
  if (hasOffset)
    return Napi::Number::New(node_gdal::napi_env(), result);
  else
    return Napi::Number::New(node_gdal::napi_env(), 0);
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
NAPI_GETTER(MDArray::scaleGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  bool hasScale = false;
  GDAL_LOCK_PARENT(array);
  double result = array->this_->GetScale(&hasScale);
  if (hasScale)
    return Napi::Number::New(node_gdal::napi_env(), result);
  else
    return Napi::Number::New(node_gdal::napi_env(), 1);
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
NAPI_GETTER(MDArray::noDataValueGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  bool hasNoData = false;
  GDAL_LOCK_PARENT(array);
  double result = array->this_->GetNoDataValueAsDouble(&hasNoData);

  if (hasNoData && !std::isnan(result)) {
    return Napi::Number::New(node_gdal::napi_env(), result);
    return node_gdal::napi_env().Undefined();
  } else {
    return node_gdal::napi_env().Null();
    return node_gdal::napi_env().Undefined();
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
NAPI_GETTER(MDArray::unitTypeGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_LOCK_PARENT(array);
  std::string unit = array->this_->GetUnit();
  return SafeString::New(unit.c_str());
}

/**
 * @readonly
 * @kind member
 * @name dataType
 * @instance
 * @memberof MDArray
 * @type {string}
 */
NAPI_GETTER(MDArray::typeGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, array, raw);
  GDAL_LOCK_PARENT(array);
  GDALExtendedDataType type = raw->GetDataType();
  const char *r;
  switch (type.GetClass()) {
    case GEDTC_NUMERIC: r = GDALGetDataTypeName(type.GetNumericDataType()); break;
    case GEDTC_STRING: r = "String"; break;
    case GEDTC_COMPOUND: r = "Compound"; break;
    default: Napi::Error::New(node_gdal::napi_env(), "Invalid attribute type").ThrowAsJavaScriptException(); return node_gdal::napi_env().Undefined();
  }
  return SafeString::New(r);
}

/**
 * @readonly
 * @kind member
 * @name dimensions
 * @instance
 * @memberof MDArray
 * @type {GroupDimensions}
 */
NAPI_GETTER(MDArray::dimensionsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "dims_");
}

/**
 * @readonly
 * @kind member
 * @name attributes
 * @instance
 * @memberof MDArray
 * @type {ArrayAttributes}
 */
NAPI_GETTER(MDArray::attributesGetter) {
  return GDAL_GET_PRIVATE(info.This(), "attrs_");
}

/**
 * @readonly
 * @kind member
 * @name description
 * @instance
 * @memberof MDArray
 * @type {string}
 */
NAPI_GETTER(MDArray::descriptionGetter) {
  NODE_UNWRAP_CHECK(MDArray, info.This(), array);
  GDAL_RAW_CHECK(std::shared_ptr<GDALMDArray>, array, raw);
  GDAL_LOCK_PARENT(array);
  std::string description = raw->GetFullName();
  return SafeString::New(description.c_str());
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

NAPI_GETTER(MDArray::uidGetter) {
  MDArray *ds = node_gdal::UnwrapWrapped<MDArray>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env(), (int)ds->uid);
}

#endif

} // namespace node_gdal
