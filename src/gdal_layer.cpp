
#include "gdal_layer.hpp"
#include "collections/layer_features.hpp"
#include "collections/layer_fields.hpp"
#include "gdal_common.hpp"
#include "gdal_dataset.hpp"
#include "gdal_feature.hpp"
#include "gdal_feature_defn.hpp"
#include "gdal_field_defn.hpp"
#include "geometry/gdal_geometry.hpp"
#include "gdal_spatial_reference.hpp"

#include <sstream>
#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference Layer::constructor;

void Layer::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(Layer);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "Layer",
    {
        METHOD(toString)
        METHOD(getExtent)
        METHOD(setAttributeFilter)
        METHOD(setSpatialFilter)
        METHOD(getSpatialFilter)
        METHOD(testCapability)
        METHOD_ASYNCABLE_AS("flush", syncToDisk)
        ATTR_DONT_ENUM(lcons, "ds", dsGetter, READ_ONLY_SETTER)
        ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER)
        ATTR(lcons, "srs", srsGetter, READ_ONLY_SETTER)
        ATTR(lcons, "features", featuresGetter, READ_ONLY_SETTER)
        ATTR(lcons, "fields", fieldsGetter, READ_ONLY_SETTER)
        ATTR(lcons, "name", nameGetter, READ_ONLY_SETTER)
        ATTR(lcons, "geomType", geomTypeGetter, READ_ONLY_SETTER)
        ATTR(lcons, "geomColumn", geomColumnGetter, READ_ONLY_SETTER)
        ATTR(lcons, "fidColumn", fidColumnGetter, READ_ONLY_SETTER)
    });

  target.Set("Layer", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}


Layer::Layer(const Napi::CallbackInfo &info) : GDALObject<Layer>(info), uid(0), this_(nullptr), parent_ds(0) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = static_cast<OGRLayer *>(info[0].As<Napi::External<void>>().Data());
    LOG("Created layer [%p]", this_);
  } else {
    Napi::Error::New(info.Env(), "Cannot create Layer directly").ThrowAsJavaScriptException();
    return;
  }

  GDAL_SET_PRIVATE(info.This(), "features_", LayerFeatures::New(info.This()));
  GDAL_SET_PRIVATE(info.This(), "fields_", LayerFields::New(info.This()));
}

Layer::~Layer() {
  dispose();
}

void Layer::dispose() {
  if (this_) {

    LOG("Disposing layer [%p]", this_);

    object_store.dispose(uid);

    LOG("Disposed layer [%p]", this_);
    this_ = NULL;
  }
};

/**
 * A representation of a layer of simple vector features, with access methods.
 *
 * @class Layer
 */

Napi::Value Layer::New(OGRLayer *raw, GDALDataset *raw_parent) {
  return Layer::New(raw, raw_parent, false);
}

Napi::Value Layer::New(OGRLayer *raw, GDALDataset *raw_parent, bool result_set) {

  if (!raw) { return node_gdal::napi_env().Null(); }
  if (object_store.has(raw)) { return object_store.get(raw); }

  std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env(), raw)};
  Napi::Object obj = Layer::constructor.Value().New(args);
  Layer *wrapped = node_gdal::UnwrapWrapped<Layer>(obj);

  // add reference to datasource so datasource doesnt get GC'ed while layer is
  // alive
  Napi::Object ds;
  if (object_store.has(raw_parent)) {
    ds = object_store.get(raw_parent);
  } else {
    LOG("Layer's parent dataset disappeared from cache (layer = %p, dataset = %p)", raw, raw_parent);
    Napi::Error::New(node_gdal::napi_env(), "Layer's parent dataset disappeared from cache").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
    // ds = Dataset::New(raw_parent); //should never happen
  }

  Dataset *unwrapped = node_gdal::UnwrapWrapped<Dataset>(ds);
  long parent_uid = unwrapped->uid;

  wrapped->uid = object_store.add(raw, *wrapped, parent_uid, result_set);
  wrapped->parent_ds = raw_parent;
  wrapped->parent_uid = parent_uid;

  // the layer reads it back in dsGetter, and it also keeps the dataset alive
  GDAL_SET_PRIVATE(obj, "ds_", ds);

  return obj;
}

NAPI_METHOD(Layer::toString) {

  Layer *layer = node_gdal::UnwrapWrapped<Layer>(info.This().As<Napi::Object>());
  if (!layer->this_) {
    return Napi::String::New(node_gdal::napi_env(), "Null layer");
    return node_gdal::napi_env().Undefined();
  }

  std::ostringstream ss;
  GDAL_LOCK_PARENT(layer);
  ss << "Layer (" << layer->this_->GetName() << ")";

  return SafeString::New(ss.str().c_str());
}

/**
 * Flush pending changes to disk.
 *
 * @throws {Error}
 * @method flush
 * @instance
 * @memberof Layer
 */

/**
 * Flush pending changes to disk.
 * @async
 *
 * @throws {Error}
 * @method flushAsync
 * @instance
 * @memberof Layer
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 *
 */
NODE_WRAPPED_ASYNC_METHOD_WITH_OGRERR_RESULT_LOCKED(Layer, syncToDisk, SyncToDisk);

/**
 * Determines if the dataset supports the indicated operation.
 *
 * @method testCapability
 * @instance
 * @memberof Layer
 * @param {string} capability (see {@link OLC|capability list}
 * @return {boolean}
 */
NODE_WRAPPED_METHOD_WITH_RESULT_1_STRING_PARAM_LOCKED(Layer, testCapability, Boolean, TestCapability, "capability");

/**
 * Fetch the extent of this layer.
 *
 * @throws {Error}
 * @method getExtent
 * @instance
 * @memberof Layer
 * @param {boolean} [force=true]
 * @return {Envelope} Bounding envelope
 */
NAPI_METHOD(Layer::getExtent) {

  Layer *layer = node_gdal::UnwrapWrapped<Layer>(info.This().As<Napi::Object>());
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  int force = 1;
  NODE_ARG_BOOL_OPT(0, "force", force);

  std::unique_ptr<OGREnvelope> envelope(new OGREnvelope());
  GDAL_LOCK_PARENT(layer);
  OGRErr err = layer->this_->GetExtent(envelope.get(), force);

  if (err) {
    Napi::Error::New(node_gdal::napi_env(), "Can't get layer extent without computing it").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  Napi::Object obj = Napi::Object::New(node_gdal::napi_env());
  obj.Set( Napi::String::New(node_gdal::napi_env(), "minX"), Napi::Number::New(node_gdal::napi_env(), envelope->MinX));
  obj.Set( Napi::String::New(node_gdal::napi_env(), "maxX"), Napi::Number::New(node_gdal::napi_env(), envelope->MaxX));
  obj.Set( Napi::String::New(node_gdal::napi_env(), "minY"), Napi::Number::New(node_gdal::napi_env(), envelope->MinY));
  obj.Set( Napi::String::New(node_gdal::napi_env(), "maxY"), Napi::Number::New(node_gdal::napi_env(), envelope->MaxY));

  return obj;
}

/**
 * This method returns the current spatial filter for this layer.
 *
 * @throws {Error}
 * @method getSpatialFilter
 * @instance
 * @memberof Layer
 * @return {Geometry}
 */
NAPI_METHOD(Layer::getSpatialFilter) {

  Layer *layer = node_gdal::UnwrapWrapped<Layer>(info.This().As<Napi::Object>());
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDAL_LOCK_PARENT(layer);
  auto r = layer->this_->GetSpatialFilter();

  return Geometry::New(r, false);
}

/**
 * This method sets the geometry to be used as a spatial filter when fetching
 * features via the `layer.features.next()` method. Only features that
 * geometrically intersect the filter geometry will be returned.
 *
 * Alernatively you can pass it envelope bounds as individual arguments.
 *
 * @example
 *
 * layer.setSpatialFilter(geometry);
 *
 * @throws {Error}
 * @method setSpatialFilter
 * @instance
 * @memberof Layer
 * @param {Geometry|null} filter
 */

/**
 * This method sets the geometry to be used as a spatial filter when fetching
 * features via the `layer.features.next()` method. Only features that
 * geometrically intersect the filter geometry will be returned.
 *
 * Alernatively you can pass it envelope bounds as individual arguments.
 *
 * @example
 *
 * layer.setSpatialFilter(minX, minY, maxX, maxY);
 *
 * @throws {Error}
 * @method setSpatialFilter
 * @instance
 * @memberof Layer
 * @param {number} minxX
 * @param {number} minyY
 * @param {number} maxX
 * @param {number} maxY
 */
NAPI_METHOD(Layer::setSpatialFilter) {

  Layer *layer = node_gdal::UnwrapWrapped<Layer>(info.This().As<Napi::Object>());
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (info.Length() == 1) {
    Geometry *filter = NULL;
    NODE_ARG_WRAPPED_OPT(0, "filter", Geometry, filter);

    GDAL_LOCK_PARENT(layer);
    if (filter) {
      layer->this_->SetSpatialFilter(filter->get());
    } else {
      layer->this_->SetSpatialFilter(NULL);
    }
  } else if (info.Length() == 4) {
    double minX, minY, maxX, maxY;
    NODE_ARG_DOUBLE(0, "minX", minX);
    NODE_ARG_DOUBLE(1, "minY", minY);
    NODE_ARG_DOUBLE(2, "maxX", maxX);
    NODE_ARG_DOUBLE(3, "maxY", maxY);

    GDAL_LOCK_PARENT(layer);
    layer->this_->SetSpatialFilterRect(minX, minY, maxX, maxY);
  } else {
    Napi::Error::New(node_gdal::napi_env(), "Invalid number of arguments").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  return node_gdal::napi_env().Undefined();
}

/**
 * Sets the attribute query string to be used when fetching features via the
 * `layer.features.next()` method. Only features for which the query evaluates
 * as `true` will be returned.
 *
 * The query string should be in the format of an SQL WHERE clause. For instance
 * "population > 1000000 and population < 5000000" where `population` is an
 * attribute in the layer. The query format is normally a restricted form of
 * SQL WHERE clause as described in the "WHERE" section of the [OGR SQL
 * tutorial](https://gdal.org/user/ogr_sql_dialect.html). In some cases (RDBMS backed
 * drivers) the native capabilities of the database may be used to interprete
 * the WHERE clause in which case the capabilities will be broader than those
 * of OGR SQL.
 *
 * @example
 *
 * layer.setAttributeFilter('population > 1000000 and population < 5000000');
 *
 * @throws {Error}
 * @method setAttributeFilter
 * @instance
 * @memberof Layer
 * @param {string|null} [filter=null]
 */
NAPI_METHOD(Layer::setAttributeFilter) {

  Layer *layer = node_gdal::UnwrapWrapped<Layer>(info.This().As<Napi::Object>());
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  std::string filter = "";
  NODE_ARG_OPT_STR(0, "filter", filter);

  OGRErr err;
  GDAL_LOCK_PARENT(layer);
  if (filter.empty()) {
    err = layer->this_->SetAttributeFilter(NULL);
  } else {
    err = layer->this_->SetAttributeFilter(filter.c_str());
  }

  if (err) {
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }

  return node_gdal::napi_env().Undefined();
}

/*
NAPI_METHOD(Layer::getLayerDefn)
{
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(info.This().As<Napi::Object>());

  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  info.GetReturnValue().Set(FeatureDefn::New(layer->this_->GetLayerDefn(),
false));
}*/

/**
 * @readonly
 * @kind member
 * @name ds
 * @instance
 * @memberof Layer
 * @type {Dataset}
 */
NAPI_GETTER(Layer::dsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "ds_");
}

/**
 * @readonly
 * @kind member
 * @name srs
 * @instance
 * @memberof Layer
 * @type {SpatialReference|null}
 */
NAPI_GETTER(Layer::srsGetter) {
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(info.This().As<Napi::Object>());
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  GDAL_LOCK_PARENT(layer);
  auto r = layer->this_->GetSpatialRef();
  return SpatialReference::New(r);
}

/**
 * @readonly
 * @kind member
 * @name name
 * @instance
 * @memberof Layer
 * @type {string}
 */
NAPI_GETTER(Layer::nameGetter) {
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(info.This().As<Napi::Object>());
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  GDAL_LOCK_PARENT(layer);
  auto r = layer->this_->GetName();
  return SafeString::New(r);
}

/**
 * @readonly
 * @kind member
 * @name geomColumn
 * @instance
 * @memberof Layer
 * @type {string}
 */
NAPI_GETTER(Layer::geomColumnGetter) {
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(info.This().As<Napi::Object>());
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  GDAL_LOCK_PARENT(layer);
  auto r = layer->this_->GetGeometryColumn();
  return SafeString::New(r);
}

/**
 * @readonly
 * @kind member
 * @name fidColumn
 * @instance
 * @memberof Layer
 * @type {string}
 */
NAPI_GETTER(Layer::fidColumnGetter) {
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(info.This().As<Napi::Object>());
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  GDAL_LOCK_PARENT(layer);
  auto r = layer->this_->GetFIDColumn();
  return SafeString::New(r);
}

/**
 * @readonly
 * @kind member
 * @name geomType
 * @instance
 * @memberof Layer
 * @type {number} (see {@link wkbGeometry|geometry types}
 */
NAPI_GETTER(Layer::geomTypeGetter) {
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(info.This().As<Napi::Object>());
  if (!layer->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Layer object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  GDAL_LOCK_PARENT(layer);
  auto r = layer->this_->GetGeomType();
  return Napi::Number::New(node_gdal::napi_env(), r);
}

/**
 * @readonly
 * @kind member
 * @name features
 * @instance
 * @memberof Layer
 * @type {LayerFeatures}
 */
NAPI_GETTER(Layer::featuresGetter) {
  return GDAL_GET_PRIVATE(info.This(), "features_");
}

/**
 * @readonly
 * @kind member
 * @name fields
 * @instance
 * @memberof Layer
 * @type {LayerFields}
 */
NAPI_GETTER(Layer::fieldsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "fields_");
}

NAPI_GETTER(Layer::uidGetter) {
  Layer *layer = node_gdal::UnwrapWrapped<Layer>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env(), (int)layer->uid);
}

} // namespace node_gdal
