
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

Nan::Persistent<FunctionTemplate> Layer::constructor;

void Layer::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(Layer::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("Layer").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan::SetPrototypeMethod(lcons, "getExtent", getExtent);
  Nan::SetPrototypeMethod(lcons, "setAttributeFilter", setAttributeFilter);
  Nan::SetPrototypeMethod(lcons, "setSpatialFilter", setSpatialFilter);
  Nan::SetPrototypeMethod(lcons, "getSpatialFilter", getSpatialFilter);
  Nan::SetPrototypeMethod(lcons, "testCapability", testCapability);
  Nan__SetPrototypeAsyncableMethod(lcons, "flush", syncToDisk);

  ATTR_DONT_ENUM(lcons, "ds", dsGetter, READ_ONLY_SETTER);
  ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER);
  ATTR(lcons, "srs", srsGetter, READ_ONLY_SETTER);
  ATTR(lcons, "features", featuresGetter, READ_ONLY_SETTER);
  ATTR(lcons, "fields", fieldsGetter, READ_ONLY_SETTER);
  ATTR(lcons, "name", nameGetter, READ_ONLY_SETTER);
  ATTR(lcons, "geomType", geomTypeGetter, READ_ONLY_SETTER);
  ATTR(lcons, "geomColumn", geomColumnGetter, READ_ONLY_SETTER);
  ATTR(lcons, "fidColumn", fidColumnGetter, READ_ONLY_SETTER);

  Nan::Set(target, Nan::New("Layer").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

Layer::Layer(OGRLayer *layer) : Nan::ObjectWrap(), uid(0), this_(layer), parent_ds(0) {
  LOG("Created layer [%p]", layer);
}

Layer::Layer() : Nan::ObjectWrap(), uid(0), this_(0), parent_ds(0) {
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
NAN_METHOD(Layer::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }

  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    Layer *f = static_cast<Layer *>(ptr);
    f->Wrap(info.This());

    Local<Value> features = LayerFeatures::New(info.This());
    Nan::SetPrivate(info.This(), Nan::New("features_").ToLocalChecked(), features);

    Local<Value> fields = LayerFields::New(info.This());
    Nan::SetPrivate(info.This(), Nan::New("fields_").ToLocalChecked(), fields);

    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create layer directly. Create with dataset instead.");
    return;
  }

  info.GetReturnValue().Set(info.This());
}

Local<Value> Layer::New(OGRLayer *raw, GDALDataset *raw_parent) {
  Nan::EscapableHandleScope scope;
  return scope.Escape(Layer::New(raw, raw_parent, false));
}

Local<Value> Layer::New(OGRLayer *raw, GDALDataset *raw_parent, bool result_set) {
  Nan::EscapableHandleScope scope;

  if (!raw) { return scope.Escape(Nan::Null()); }
  if (object_store.has(raw)) { return scope.Escape(object_store.get(raw)); }

  Layer *wrapped = new Layer(raw);

  Local<Value> ext = Nan::New<External>(wrapped);
  Local<Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(Layer::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();

  // add reference to datasource so datasource doesnt get GC'ed while layer is
  // alive
  Local<Object> ds;
  if (object_store.has(raw_parent)) {
    ds = object_store.get(raw_parent);
  } else {
    LOG("Layer's parent dataset disappeared from cache (layer = %p, dataset = %p)", raw, raw_parent);
    Nan::ThrowError("Layer's parent dataset disappeared from cache");
    return scope.Escape(Nan::Undefined());
    // ds = Dataset::New(raw_parent); //should never happen
  }

  Dataset *unwrapped = Nan::ObjectWrap::Unwrap<Dataset>(ds);
  long parent_uid = unwrapped->uid;

  wrapped->uid = object_store.add(raw, wrapped->persistent(), parent_uid, result_set);
  wrapped->parent_ds = raw_parent;
  wrapped->parent_uid = parent_uid;
  Nan::SetPrivate(obj, Nan::New("ds_").ToLocalChecked(), ds);

  return scope.Escape(obj);
}

NAN_METHOD(Layer::toString) {

  Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info.This());
  if (!layer->this_) {
    info.GetReturnValue().Set(Nan::New("Null layer").ToLocalChecked());
    return;
  }

  std::ostringstream ss;
  GDAL_LOCK_PARENT(layer);
  ss << "Layer (" << layer->this_->GetName() << ")";

  info.GetReturnValue().Set(SafeString::New(ss.str().c_str()));
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
NAN_METHOD(Layer::getExtent) {

  Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info.This());
  if (!layer->isAlive()) {
    Nan::ThrowError("Layer object has already been destroyed");
    return;
  }

  int force = 1;
  NODE_ARG_BOOL_OPT(0, "force", force);

  std::unique_ptr<OGREnvelope> envelope(new OGREnvelope());
  GDAL_LOCK_PARENT(layer);
  OGRErr err = layer->this_->GetExtent(envelope.get(), force);

  if (err) {
    Nan::ThrowError("Can't get layer extent without computing it");
    return;
  }

  Local<Object> obj = Nan::New<Object>();
  Nan::Set(obj, Nan::New("minX").ToLocalChecked(), Nan::New<Number>(envelope->MinX));
  Nan::Set(obj, Nan::New("maxX").ToLocalChecked(), Nan::New<Number>(envelope->MaxX));
  Nan::Set(obj, Nan::New("minY").ToLocalChecked(), Nan::New<Number>(envelope->MinY));
  Nan::Set(obj, Nan::New("maxY").ToLocalChecked(), Nan::New<Number>(envelope->MaxY));

  info.GetReturnValue().Set(obj);
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
NAN_METHOD(Layer::getSpatialFilter) {

  Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info.This());
  if (!layer->isAlive()) {
    Nan::ThrowError("Layer object has already been destroyed");
    return;
  }

  GDAL_LOCK_PARENT(layer);
  auto r = layer->this_->GetSpatialFilter();

  info.GetReturnValue().Set(Geometry::New(r, false));
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
NAN_METHOD(Layer::setSpatialFilter) {

  Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info.This());
  if (!layer->isAlive()) {
    Nan::ThrowError("Layer object has already been destroyed");
    return;
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
    Nan::ThrowError("Invalid number of arguments");
    return;
  }

  return;
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
NAN_METHOD(Layer::setAttributeFilter) {

  Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info.This());
  if (!layer->isAlive()) {
    Nan::ThrowError("Layer object has already been destroyed");
    return;
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
    return;
  }

  return;
}

/*
NAN_METHOD(Layer::getLayerDefn)
{
  Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info.This());

  if (!layer->isAlive()) {
    Nan::ThrowError("Layer object already destroyed");
    return;
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
NAN_GETTER(Layer::dsGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("ds_").ToLocalChecked()).ToLocalChecked());
}

/**
 * @readonly
 * @kind member
 * @name srs
 * @instance
 * @memberof Layer
 * @type {SpatialReference|null}
 */
NAN_GETTER(Layer::srsGetter) {
  Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info.This());
  if (!layer->isAlive()) {
    Nan::ThrowError("Layer object has already been destroyed");
    return;
  }
  GDAL_LOCK_PARENT(layer);
  auto r = layer->this_->GetSpatialRef();
  info.GetReturnValue().Set(SpatialReference::New(r, false));
}

/**
 * @readonly
 * @kind member
 * @name name
 * @instance
 * @memberof Layer
 * @type {string}
 */
NAN_GETTER(Layer::nameGetter) {
  Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info.This());
  if (!layer->isAlive()) {
    Nan::ThrowError("Layer object has already been destroyed");
    return;
  }
  GDAL_LOCK_PARENT(layer);
  auto r = layer->this_->GetName();
  info.GetReturnValue().Set(SafeString::New(r));
}

/**
 * @readonly
 * @kind member
 * @name geomColumn
 * @instance
 * @memberof Layer
 * @type {string}
 */
NAN_GETTER(Layer::geomColumnGetter) {
  Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info.This());
  if (!layer->isAlive()) {
    Nan::ThrowError("Layer object has already been destroyed");
    return;
  }
  GDAL_LOCK_PARENT(layer);
  auto r = layer->this_->GetGeometryColumn();
  info.GetReturnValue().Set(SafeString::New(r));
}

/**
 * @readonly
 * @kind member
 * @name fidColumn
 * @instance
 * @memberof Layer
 * @type {string}
 */
NAN_GETTER(Layer::fidColumnGetter) {
  Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info.This());
  if (!layer->isAlive()) {
    Nan::ThrowError("Layer object has already been destroyed");
    return;
  }
  GDAL_LOCK_PARENT(layer);
  auto r = layer->this_->GetFIDColumn();
  info.GetReturnValue().Set(SafeString::New(r));
}

/**
 * @readonly
 * @kind member
 * @name geomType
 * @instance
 * @memberof Layer
 * @type {number} (see {@link wkbGeometry|geometry types}
 */
NAN_GETTER(Layer::geomTypeGetter) {
  Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info.This());
  if (!layer->isAlive()) {
    Nan::ThrowError("Layer object has already been destroyed");
    return;
  }
  GDAL_LOCK_PARENT(layer);
  auto r = layer->this_->GetGeomType();
  info.GetReturnValue().Set(Nan::New<Integer>(r));
}

/**
 * @readonly
 * @kind member
 * @name features
 * @instance
 * @memberof Layer
 * @type {LayerFeatures}
 */
NAN_GETTER(Layer::featuresGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("features_").ToLocalChecked()).ToLocalChecked());
}

/**
 * @readonly
 * @kind member
 * @name fields
 * @instance
 * @memberof Layer
 * @type {LayerFields}
 */
NAN_GETTER(Layer::fieldsGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("fields_").ToLocalChecked()).ToLocalChecked());
}

NAN_GETTER(Layer::uidGetter) {
  Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info.This());
  info.GetReturnValue().Set(Nan::New((int)layer->uid));
}

} // namespace node_gdal
