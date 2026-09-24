#include <memory>
#include "dataset_layers.hpp"
#include "../gdal_common.hpp"
#include "../gdal_dataset.hpp"
#include "../gdal_layer.hpp"
#include "../gdal_spatial_reference.hpp"
#include "../utils/string_list.hpp"

namespace node_gdal {

Napi::FunctionReference DatasetLayers::constructor;

void DatasetLayers::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(DatasetLayers);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "DatasetLayers",
    {
        METHOD(toString)
        METHOD_ASYNCABLE(count)
        METHOD_ASYNCABLE(create)
        METHOD_ASYNCABLE(copy)
        METHOD_ASYNCABLE(get)
        METHOD_ASYNCABLE(remove)
        ATTR_DONT_ENUM(lcons, "ds", dsGetter, READ_ONLY_SETTER)
    });

  target.Set("DatasetLayers", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

DatasetLayers::DatasetLayers(const Napi::CallbackInfo &info) : GDALObject<DatasetLayers>(info) {
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::Error::New(info.Env(), "Cannot create DatasetLayers directly").ThrowAsJavaScriptException();
    return;
  }
  GDAL_SET_PRIVATE(info.This(), "parent_", info[0]);
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

Napi::Value DatasetLayers::New(Napi::Value ds_obj) {

  std::vector<napi_value> args = {ds_obj};
  Napi::Object obj = DatasetLayers::constructor.Value().New(args);


  return obj;
}

NAPI_METHOD(DatasetLayers::toString) {
  return Napi::String::New(node_gdal::napi_env(), "DatasetLayers");
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

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(parent);

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();

  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env(), "method must be given integer or string").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALAsyncableJob<OGRLayer *> job(ds->uid);
  job.persist(parent);
  if (info[0].IsString()) {
    std::string *layer_name = new std::string(info[0].As<Napi::String>().Utf8Value());
    job.main = [raw, layer_name](const GDALExecutionProgress &) {
      std::unique_ptr<std::string> layer_name_ptr(layer_name);
      CPLErrorReset();
      OGRLayer *lyr = raw->GetLayerByName(layer_name->c_str());
      if (lyr == nullptr) { throw CPLGetLastErrorMsg(); }
      return lyr;
    };
  } else if (info[0].IsNumber()) {
    int64_t id = info[0].As<Napi::Number>().Int64Value();
    job.main = [raw, id](const GDALExecutionProgress &) {
      CPLErrorReset();
      OGRLayer *lyr = raw->GetLayer(id);
      if (lyr == nullptr) { throw CPLGetLastErrorMsg(); }
      return lyr;
    };
  } else {
    Napi::TypeError::New(node_gdal::napi_env(), "method must be given integer or string").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  job.rval = [raw](OGRLayer *lyr, const GetFromPersistentFunc &) { return Layer::New(lyr, raw); };
  return job.run(info, async, 1);
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

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(parent);

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
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
    return node_gdal::napi_env().Undefined(); // error parsing string list
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

  return job.run(info, async, 4);
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

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(parent);

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();

  GDALAsyncableJob<int> job(ds->uid);
  job.persist(parent);
  job.main = [raw](const GDALExecutionProgress &) {
    int count = raw->GetLayerCount();
    return count;
  };

  job.rval = [](int count, const GetFromPersistentFunc &) { return Napi::Number::New(node_gdal::napi_env(), count); };
  return job.run(info, async, 0);
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

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(parent);

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();

  Layer *layer_to_copy;
  std::string *new_name = new std::string("");
  StringList *options = new StringList;

  NODE_ARG_WRAPPED(0, "layer to copy", Layer, layer_to_copy);
  NODE_ARG_STR(1, "new layer name", *new_name);
  if (info.Length() > 2 && options->parse(info[2])) { node_gdal::ThrowOverPending(node_gdal::napi_env(), "Error parsing string list"); }

  OGRLayer *src = layer_to_copy->get();
  GDALAsyncableJob<OGRLayer *> job(ds->uid);
  job.persist(parent, info[0].As<Napi::Object>());
  job.main = [raw, src, new_name, options](const GDALExecutionProgress &) {
    std::unique_ptr<StringList> options_ptr(options);
    std::unique_ptr<std::string> new_name_ptr(new_name);
    CPLErrorReset();
    OGRLayer *layer = raw->CopyLayer(src, new_name->c_str(), options->get());
    if (layer == nullptr) throw CPLGetLastErrorMsg();
    return layer;
  };

  job.rval = [raw](OGRLayer *layer, const GetFromPersistentFunc &) { return Layer::New(layer, raw); };

  return job.run(info, async, 3);
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

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(parent);

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
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

  job.rval = [](int count, const GetFromPersistentFunc &) { return node_gdal::napi_env().Undefined().As<Napi::Value>(); };
  return job.run(info, async, 1);
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
NAPI_GETTER(DatasetLayers::dsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "parent_");
}

} // namespace node_gdal
