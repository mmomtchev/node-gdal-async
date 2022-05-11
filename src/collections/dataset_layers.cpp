#include <memory>
#include "dataset_layers.hpp"
#include "../gdal_common.hpp"
#include "../gdal_dataset.hpp"
#include "../gdal_layer.hpp"
#include "../gdal_spatial_reference.hpp"
#include "../utils/string_list.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> DatasetLayers::constructor;

void DatasetLayers::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(DatasetLayers::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("DatasetLayers").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan__SetPrototypeAsyncableMethod(lcons, "count", count);
  Nan__SetPrototypeAsyncableMethod(lcons, "create", create);
  Nan__SetPrototypeAsyncableMethod(lcons, "copy", copy);
  Nan__SetPrototypeAsyncableMethod(lcons, "get", get);
  Nan__SetPrototypeAsyncableMethod(lcons, "remove", remove);

  ATTR_DONT_ENUM(lcons, "ds", dsGetter, READ_ONLY_SETTER);

  Nan::Set(target, Nan::New("DatasetLayers").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

DatasetLayers::DatasetLayers() : Nan::ObjectWrap() {
}

DatasetLayers::~DatasetLayers() {
}

/**
 * An encapsulation of a {@link Dataset}
 * vector layers.
 *
 * @example
 * var layers = dataset.layers;
 *
 * @class DatasetLayers
 */
NAN_METHOD(DatasetLayers::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }
  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    DatasetLayers *f = static_cast<DatasetLayers *>(ptr);
    f->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create DatasetLayers directly");
    return;
  }
}

Local<Value> DatasetLayers::New(Local<Value> ds_obj) {
  Nan::EscapableHandleScope scope;

  DatasetLayers *wrapped = new DatasetLayers();

  v8::Local<v8::Value> ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(DatasetLayers::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();

  Nan::SetPrivate(obj, Nan::New("parent_").ToLocalChecked(), ds_obj);

  return scope.Escape(obj);
}

NAN_METHOD(DatasetLayers::toString) {
  info.GetReturnValue().Set(Nan::New("DatasetLayers").ToLocalChecked());
}

/**
 * Returns the layer with the given name or identifier.
 *
 * @method get
 * @instance
 * @memberof DatasetLayers
 * @param {string|number} key Layer name or ID.
 * @throws {Error}
 * @return {Layer}
 */

/**
 * Returns the layer with the given name or identifier.
 * @async
 *
 * @method getAsync
 * @instance
 * @memberof DatasetLayers
 * @param {string|number} key Layer name or ID.
 * @param {callback<Layer>} [callback=undefined]
 * @throws {Error}
 * @return {Promise<Layer>}
 */

GDAL_ASYNCABLE_DEFINE(DatasetLayers::get) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(parent);

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();

  if (info.Length() < 1) {
    Nan::ThrowError("method must be given integer or string");
    return;
  }

  GDALAsyncableJob<OGRLayer *> job(ds->uid);
  job.persist(parent);
  if (info[0]->IsString()) {
    std::string *layer_name = new std::string(*Nan::Utf8String(info[0]));
    job.main = [raw, layer_name](const GDALExecutionProgress &) {
      std::unique_ptr<std::string> layer_name_ptr(layer_name);
      CPLErrorReset();
      OGRLayer *lyr = raw->GetLayerByName(layer_name->c_str());
      if (lyr == nullptr) { throw CPLGetLastErrorMsg(); }
      return lyr;
    };
  } else if (info[0]->IsNumber()) {
    int64_t id = Nan::To<int64_t>(info[0]).ToChecked();
    job.main = [raw, id](const GDALExecutionProgress &) {
      CPLErrorReset();
      OGRLayer *lyr = raw->GetLayer(id);
      if (lyr == nullptr) { throw CPLGetLastErrorMsg(); }
      return lyr;
    };
  } else {
    Nan::ThrowTypeError("method must be given integer or string");
    return;
  }

  job.rval = [raw](OGRLayer *lyr, const GetFromPersistentFunc &) { return Layer::New(lyr, raw); };
  job.run(info, async, 1);
}

/**
 * Adds a new layer.
 *
 * @example
 *
 * dataset.layers.create('layername', null, gdal.Point);
 *
 *
 * @method create
 * @instance
 * @memberof DatasetLayers
 * @throws {Error}
 * @param {string} name Layer name
 * @param {SpatialReference|null} [srs=null] Layer projection
 * @param {number|Function|null} [geomType=null] Geometry type or constructor ({@link wkbGeometryType|see geometry types})
 * @param {string[]|object} [creation_options] driver-specific layer creation
 * options
 * @return {Layer}
 */

/**
 * Adds a new layer.
 * @async
 *
 * @example
 *
 * await dataset.layers.createAsync('layername', null, gdal.Point);
 * dataset.layers.createAsync('layername', null, gdal.Point, (e, r) => console.log(e, r));
 *
 *
 * @method createAsync
 * @instance
 * @memberof DatasetLayers
 * @throws {Error}
 * @param {string} name Layer name
 * @param {SpatialReference|null} [srs=null] Layer projection
 * @param {number|Function|null} [geomType=null] Geometry type or constructor ({@link wkbGeometryType|see geometry types})
 * @param {string[]|object} [creation_options] driver-specific layer creation
 * options
 * @param {callback<Layer>} [callback=undefined]
 * @return {Promise<Layer>}
 */

GDAL_ASYNCABLE_DEFINE(DatasetLayers::create) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(parent);

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();

  std::string *layer_name = new std::string;
  SpatialReference *spatial_ref = NULL;
  OGRwkbGeometryType geom_type = wkbUnknown;
  StringList *options = new StringList;

  NODE_ARG_STR(0, "layer name", *layer_name);
  NODE_ARG_WRAPPED_OPT(1, "spatial reference", SpatialReference, spatial_ref);
  NODE_ARG_ENUM_OPT(2, "geometry type", OGRwkbGeometryType, geom_type);
  if (info.Length() > 3 && options->parse(info[3])) {
    return; // error parsing string list
  }

  OGRSpatialReference *srs = NULL;
  if (spatial_ref) srs = spatial_ref->get();

  GDALAsyncableJob<OGRLayer *> job(ds->uid);
  job.persist(parent);
  job.main = [raw, layer_name, srs, geom_type, options](const GDALExecutionProgress &) {
    std::unique_ptr<StringList> options_ptr(options);
    std::unique_ptr<std::string> layer_name_ptr(layer_name);
    CPLErrorReset();
    OGRLayer *layer = raw->CreateLayer(layer_name->c_str(), srs, geom_type, options->get());
    if (layer == nullptr) throw CPLGetLastErrorMsg();
    return layer;
  };

  job.rval = [raw](OGRLayer *layer, const GetFromPersistentFunc &) { return Layer::New(layer, raw, false); };

  job.run(info, async, 4);
}

/**
 * Returns the number of layers.
 *
 * @method count
 * @instance
 * @memberof DatasetLayers
 * @return {number}
 */

/**
 * Returns the number of layers.
 * @async
 *
 * @method countAsync
 * @instance
 * @memberof DatasetLayers
 * @param {callback<number>} [callback=undefined]
 * @return {Promise<number>}
 */

GDAL_ASYNCABLE_DEFINE(DatasetLayers::count) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(parent);

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();

  GDALAsyncableJob<int> job(ds->uid);
  job.persist(parent);
  job.main = [raw](const GDALExecutionProgress &) {
    int count = raw->GetLayerCount();
    return count;
  };

  job.rval = [](int count, const GetFromPersistentFunc &) { return Nan::New<Integer>(count); };
  job.run(info, async, 0);
}

/**
 * Copies a layer.
 *
 * @method copy
 * @instance
 * @memberof DatasetLayers
 * @param {Layer} src_lyr_name
 * @param {string} dst_lyr_name
 * @param {object|string[]} [options=null] layer creation options
 * @return {Layer}
 */

/**
 * Copies a layer.
 * @async
 *
 * @method copyAsync
 * @instance
 * @memberof DatasetLayers
 * @param {Layer} src_lyr_name
 * @param {string} dst_lyr_name
 * @param {object|string[]} [options=null] layer creation options
 * @param {callback<Layer>} [callback=undefined]
 * @return {Promise<Layer>}
 */

GDAL_ASYNCABLE_DEFINE(DatasetLayers::copy) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(parent);

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();

  Layer *layer_to_copy;
  std::string *new_name = new std::string("");
  StringList *options = new StringList;

  NODE_ARG_WRAPPED(0, "layer to copy", Layer, layer_to_copy);
  NODE_ARG_STR(1, "new layer name", *new_name);
  if (info.Length() > 2 && options->parse(info[2])) { Nan::ThrowError("Error parsing string list"); }

  OGRLayer *src = layer_to_copy->get();
  GDALAsyncableJob<OGRLayer *> job(ds->uid);
  job.persist(parent, info[0].As<Object>());
  job.main = [raw, src, new_name, options](const GDALExecutionProgress &) {
    std::unique_ptr<StringList> options_ptr(options);
    std::unique_ptr<std::string> new_name_ptr(new_name);
    CPLErrorReset();
    OGRLayer *layer = raw->CopyLayer(src, new_name->c_str(), options->get());
    if (layer == nullptr) throw CPLGetLastErrorMsg();
    return layer;
  };

  job.rval = [raw](OGRLayer *layer, const GetFromPersistentFunc &) { return Layer::New(layer, raw); };

  job.run(info, async, 3);
}

/**
 * Removes a layer.
 *
 * @method remove
 * @instance
 * @memberof DatasetLayers
 * @throws {Error}
 * @param {number} index
 */

/**
 * Removes a layer.
 * @async
 *
 * @method removeAsync
 * @instance
 * @memberof DatasetLayers
 * @throws {Error}
 * @param {number} index
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 */

GDAL_ASYNCABLE_DEFINE(DatasetLayers::remove) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(parent);

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();

  int i;
  NODE_ARG_INT(0, "layer index", i);
  GDALAsyncableJob<OGRErr> job(ds->uid);
  job.persist(parent);
  job.main = [raw, i](const GDALExecutionProgress &) {
    OGRErr err = raw->DeleteLayer(i);
    if (err) throw getOGRErrMsg(err);
    return err;
  };

  job.rval = [](int count, const GetFromPersistentFunc &) { return Nan::Undefined().As<Value>(); };
  job.run(info, async, 1);
}

/**
 * Returns the parent dataset.
 *
 * @readonly
 * @kind member
 * @name ds
 * @instance
 * @memberof DatasetLayers
 * @type {Dataset}
 */
NAN_GETTER(DatasetLayers::dsGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked());
}

} // namespace node_gdal
