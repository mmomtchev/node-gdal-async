#include "layer_features.hpp"
#include "../gdal_common.hpp"
#include "../gdal_feature.hpp"
#include "../gdal_layer.hpp"

namespace node_gdal {

Napi::FunctionReference LayerFeatures::constructor;

void LayerFeatures::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(LayerFeatures);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "LayerFeatures",
    {
        METHOD(toString)
        METHOD_ASYNCABLE(count)
        METHOD_ASYNCABLE(add)
        METHOD_ASYNCABLE(get)
        METHOD_ASYNCABLE(set)
        METHOD_ASYNCABLE(first)
        METHOD_ASYNCABLE(next)
        METHOD_ASYNCABLE(remove)
        ATTR_DONT_ENUM(lcons, "layer", layerGetter, READ_ONLY_SETTER)
    });

  target.Set("LayerFeatures", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

LayerFeatures::LayerFeatures(const Napi::CallbackInfo &info) : GDALObject<LayerFeatures>(info) {
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::Error::New(info.Env(), "Cannot create LayerFeatures directly").ThrowAsJavaScriptException();
    return;
  }
  GDAL_SET_PRIVATE(info.This(), "parent_", info[0]);
}

LayerFeatures::~LayerFeatures() {
}

/**
 * An encapsulation of a {@link Layer}
 * features.
 *
 * @class LayerFeatures
 */

Napi::Value LayerFeatures::New(Napi::Value layer_obj) {

  std::vector<napi_value> args = {layer_obj};
  Napi::Object obj = LayerFeatures::constructor.Value().New(args);

  return obj;
}

NAPI_METHOD(LayerFeatures::toString) {
  return Napi::String::New(node_gdal::napi_env(), "LayerFeatures");
}

/**
 * Fetch a feature by its identifier.
 *
 * **Important:** The `id` argument is not an index. In most cases it will be
 * zero-based, but in some cases it will not. If iterating, it's best to use the
 * `next()` method.
 *
 * @method get
 * @instance
 * @memberof LayerFeatures
 * @param {number} id The feature ID of the feature to read.
 * @throws {Error}
 * @return {Feature}
 */

/**
 * Fetch a feature by its identifier.
 *
 * **Important:** The `id` argument is not an index. In most cases it will be
 * zero-based, but in some cases it will not. If iterating, it's best to use the
 * `next()` method.
 * @async
 *
 * @method getAsync
 * @instance
 * @memberof LayerFeatures
 * @param {number} id The feature ID of the feature to read.
 * @param {callback<Feature>} [callback=undefined]
 * @throws {Error}
 * @return {Promise<Feature>}
 */
GDAL_ASYNCABLE_DEFINE(LayerFeatures::get) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  int feature_id;
  NODE_ARG_INT(0, "feature id", feature_id);
  OGRLayer *gdal_layer = layer->get();
  GDALAsyncableJob<OGRFeature *> job(layer->parent_uid);
  job.persist(layer->Value());
  job.main = [gdal_layer, feature_id](const GDALExecutionProgress &) {
    CPLErrorReset();
    OGRFeature *feature = gdal_layer->GetFeature(feature_id);
    if (feature == nullptr) throw CPLGetLastErrorMsg();
    return feature;
  };
  job.rval = [](OGRFeature *feature, const GetFromPersistentFunc &) { return Feature::New(feature); };
  return job.run(info, async, 1);
}

/**
 * Resets the feature pointer used by `next()` and
 * returns the first feature in the layer.
 *
 * @method first
 * @instance
 * @memberof LayerFeatures
 * @return {Feature}
 */

/**
 * Resets the feature pointer used by `next()` and
 * returns the first feature in the layer.
 * @async
 *
 * @method firstAsync
 * @instance
 * @memberof LayerFeatures
 * @param {callback<Feature>} [callback=undefined]
 * @return {Promise<Feature>}
 */
GDAL_ASYNCABLE_DEFINE(LayerFeatures::first) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  OGRLayer *gdal_layer = layer->get();
  GDALAsyncableJob<OGRFeature *> job(layer->parent_uid);
  job.persist(layer->Value());
  job.main = [gdal_layer](const GDALExecutionProgress &) {
    gdal_layer->ResetReading();
    OGRFeature *feature = gdal_layer->GetNextFeature();
    return feature;
  };
  job.rval = [](OGRFeature *feature, const GetFromPersistentFunc &) { return Feature::New(feature); };
  return job.run(info, async, 0);
}

/**
 * Returns the next feature in the layer. Returns null if no more features.
 *
 * @example
 *
 * while (feature = layer.features.next()) { ... }
 *
 * @method next
 * @instance
 * @memberof LayerFeatures
 * @return {Feature}
 */

/**
 * Returns the next feature in the layer. Returns null if no more features.
 * @async
 *
 * @example
 *
 * while (feature = await layer.features.nextAsync()) { ... }
 *
 * @method nextAsync
 * @instance
 * @memberof LayerFeatures
 * @param {callback<Feature>} [callback=undefined]
 * @return {Promise<Feature>}
 */
GDAL_ASYNCABLE_DEFINE(LayerFeatures::next) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  OGRLayer *gdal_layer = layer->get();
  GDALAsyncableJob<OGRFeature *> job(layer->parent_uid);
  job.persist(layer->Value());
  job.main = [gdal_layer](const GDALExecutionProgress &) {
    OGRFeature *feature = gdal_layer->GetNextFeature();
    return feature;
  };
  job.rval = [](OGRFeature *feature, const GetFromPersistentFunc &) { return Feature::New(feature); };
  return job.run(info, async, 0);
}

/**
 * Adds a feature to the layer. The feature should be created using the current
 * layer as the definition.
 *
 * @example
 *
 * var feature = new gdal.Feature(layer);
 * feature.setGeometry(new gdal.Point(0, 1));
 * feature.fields.set('name', 'somestring');
 * layer.features.add(feature);
 *
 * @method add
 * @instance
 * @memberof LayerFeatures
 * @throws {Error}
 * @param {Feature} feature
 */

/**
 * Adds a feature to the layer. The feature should be created using the current
 * layer as the definition.
 * @async
 *
 * @example
 *
 * var feature = new gdal.Feature(layer);
 * feature.setGeometry(new gdal.Point(0, 1));
 * feature.fields.set('name', 'somestring');
 * await layer.features.addAsync(feature);
 *
 * @method addAsync
 * @instance
 * @memberof LayerFeatures
 * @throws {Error}
 * @param {Feature} feature
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 */

GDAL_ASYNCABLE_DEFINE(LayerFeatures::add) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  Feature *f;
  NODE_ARG_WRAPPED(0, "feature", Feature, f)

  OGRLayer *gdal_layer = layer->get();
  OGRFeature *gdal_f = f->get();
  GDALAsyncableJob<int> job(layer->parent_uid);
  job.persist(layer->Value());
  job.main = [gdal_layer, gdal_f](const GDALExecutionProgress &) {
    int err = gdal_layer->CreateFeature(gdal_f);
    if (err != CE_None) throw getOGRErrMsg(err);
    return err;
  };
  job.rval = [](int, const GetFromPersistentFunc &) { return node_gdal::napi_env().Undefined(); };
  return job.run(info, async, 1);
}

/**
 * Returns the number of features in the layer.
 *
 * @method count
 * @instance
 * @memberof LayerFeatures
 * @param {boolean} [force=true]
 * @return {number} number of features in the layer.
 */

/**
 * Returns the number of features in the layer.
 * @async
 *
 * @method countAsync
 * @instance
 * @memberof LayerFeatures
 * @param {boolean} [force=true]
 * @param {callback<number>} [callback=undefined]
 * @return {Promise<number>} number of features in the layer.
 */

GDAL_ASYNCABLE_DEFINE(LayerFeatures::count) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  Napi::Object ds;
  if (object_store.has(layer->getParent())) {
    ds = object_store.get(layer->getParent());
  } else {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  int force = 1;
  NODE_ARG_BOOL_OPT(0, "force", force);

  OGRLayer *gdal_layer = layer->get();
  GDALAsyncableJob<GIntBig> job(layer->parent_uid);
  job.persist(layer->Value());
  job.main = [gdal_layer, force](const GDALExecutionProgress &) {
    GIntBig count = gdal_layer->GetFeatureCount(force);
    return count;
  };
  job.rval = [](GIntBig count, const GetFromPersistentFunc &) { return Napi::Number::New(node_gdal::napi_env(), count); };
  return job.run(info, async, 1);
}

/**
 * Sets a feature in the layer.
 *
 * @method set
 * @instance
 * @memberof LayerFeatures
 * @throws {Error}
 * @param {Feature} feature
 */

/**
 * Sets a feature in the layer.
 *
 * @method set
 * @instance
 * @memberof LayerFeatures
 * @throws {Error}
 * @param {number} id
 * @param {Feature} feature
 */

/**
 * Sets a feature in the layer.
 * @async
 *
 * @method setAsync
 * @instance
 * @memberof LayerFeatures
 * @throws {Error}
 * @param {number} id
 * @param {Feature} feature
 * @param {callback<Feature>} [callback=undefined]
 * @return {Promise<Feature>}
 */
GDAL_ASYNCABLE_DEFINE(LayerFeatures::set) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  Napi::Object ds;
  if (object_store.has(layer->getParent())) { ds = object_store.get(layer->getParent()); }
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  int err;
  Feature *f;

  Napi::Object feature;
  if (info[0].IsObject()) {
    NODE_ARG_WRAPPED(0, "feature", Feature, f);
    feature = info[0].As<Napi::Object>();
  } else if (info[0].IsNumber()) {
    int i = 0;
    NODE_ARG_INT(0, "feature id", i);
    NODE_ARG_WRAPPED(1, "feature", Feature, f);
    feature = info[1].As<Napi::Object>();
    err = f->get()->SetFID(i);
    if (err) {
      Napi::Error::New(node_gdal::napi_env(), "Error setting feature id").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }
  } else {
    Napi::Error::New(node_gdal::napi_env(), "Invalid arguments").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (!f->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  OGRLayer *gdal_layer = layer->get();
  OGRFeature *gdal_feature = f->get();
  GDALAsyncableJob<OGRErr> job(layer->parent_uid);
  job.persist(layer->Value(), f->Value());
  job.main = [gdal_layer, gdal_feature](const GDALExecutionProgress &) {
    OGRErr err = gdal_layer->SetFeature(gdal_feature);
    if (err != CE_None) throw getOGRErrMsg(err);
    return err;
  };

  job.rval = [](int, const GetFromPersistentFunc &) { return node_gdal::napi_env().Undefined(); };
  return job.run(info, async, 2);
}

/**
 * Removes a feature from the layer.
 *
 * @method remove
 * @instance
 * @memberof LayerFeatures
 * @throws {Error}
 * @param {number} id
 */

/**
 * Removes a feature from the layer.
 * @async
 *
 * @method removeAsync
 * @instance
 * @memberof LayerFeatures
 * @throws {Error}
 * @param {number} id
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 */

GDAL_ASYNCABLE_DEFINE(LayerFeatures::remove) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(parent);
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  int i;
  NODE_ARG_INT(0, "feature id", i);

  OGRLayer *gdal_layer = layer->get();
  GDALAsyncableJob<int> job(layer->parent_uid);
  job.persist(layer->Value());
  job.main = [gdal_layer, i](const GDALExecutionProgress &) {
    int err = gdal_layer->DeleteFeature(i);
    if (err) { throw getOGRErrMsg(err); }
    return err;
  };
  job.rval = [](int, const GetFromPersistentFunc &) { return node_gdal::napi_env().Undefined(); };
  return job.run(info, async, 1);

  return node_gdal::napi_env().Undefined();
}

/**
 * Returns the parent layer.
 *
 * @kind member
 * @name layer
 * @instance
 * @memberof LayerFeatures
 * @type {Layer}
 */
NAPI_GETTER(LayerFeatures::layerGetter) {
  return GDAL_GET_PRIVATE(info.This(), "parent_");
}

} // namespace node_gdal
