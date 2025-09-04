
#include "gdal_feature.hpp"
#include "collections/feature_fields.hpp"
#include "gdal_common.hpp"
#include "gdal_feature_defn.hpp"
#include "gdal_field_defn.hpp"
#include "geometry/gdal_geometry.hpp"
#include "gdal_layer.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> Feature::constructor;

void Feature::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(Feature::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("Feature").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan::SetPrototypeMethod(lcons, "getGeometry", getGeometry);
  // Nan::SetPrototypeMethod(lcons, "setGeometryDirectly", setGeometryDirectly);
  Nan::SetPrototypeMethod(lcons, "setGeometry", setGeometry);
  // Nan::SetPrototypeMethod(lcons, "stealGeometry", stealGeometry);
  Nan::SetPrototypeMethod(lcons, "clone", clone);
  // Nan::SetPrototypeMethod(lcons, "equals", equals);
  // Nan::SetPrototypeMethod(lcons, "getFieldDefn", getFieldDefn); (use
  // defn.fields.get() instead)
  Nan::SetPrototypeMethod(lcons, "setFrom", setFrom);
  Nan::SetPrototypeMethod(lcons, "getStyleString", getStyleString);
  Nan::SetPrototypeMethod(lcons, "setStyleString", setStyleString);

  // Note: This is used mainly for testing
  // TODO: Give node more info on the amount of memory a feature is using
  //      Nan::AdjustExternalMemory()
  Nan::SetPrototypeMethod(lcons, "destroy", destroy);

  ATTR(lcons, "fields", fieldsGetter, READ_ONLY_SETTER);
  ATTR(lcons, "defn", defnGetter, READ_ONLY_SETTER);
  ATTR(lcons, "fid", fidGetter, fidSetter);

  Nan::Set(target, Nan::New("Feature").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

Feature::Feature(OGRFeature *feature) : Nan::ObjectWrap(), this_(feature), owned_(true) {
  LOG("Created Feature[%p]", feature);
}

Feature::Feature() : Nan::ObjectWrap(), this_(0), owned_(true) {
}

Feature::~Feature() {
  dispose();
}

void Feature::dispose() {
  if (this_) {
    LOG("Disposing Feature [%p] (%s)", this_, owned_ ? "owned" : "unowned");
    if (owned_) OGRFeature::DestroyFeature(this_);
    LOG("Disposed Feature [%p]", this_);
    this_ = NULL;
  }
}

/**
 * A simple feature, including geometry and attributes. Its fields and geometry
 * type is defined by the given definition.
 *
 * @example
 * //create layer and specify geometry type
 * var layer = dataset.layers.create('mylayer', null, gdal.Point);
 *
 * //setup fields for the given layer
 * layer.fields.add(new gdal.FieldDefn('elevation', gdal.OFTInteger));
 * layer.fields.add(new gdal.FieldDefn('name', gdal.OFTString));
 *
 * //create feature using layer definition and then add it to the layer
 * var feature = new gdal.Feature(layer);
 * feature.fields.set('elevation', 13775);
 * feature.fields.set('name', 'Grand Teton');
 * feature.setGeometry(new gdal.Point(43.741208, -110.802414));
 * layer.features.add(feature);
 *
 * @constructor
 * @class Feature
 * @param {Layer|FeatureDefn} definition
 */
NAN_METHOD(Feature::New) {
  Feature *f;

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }

  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    f = static_cast<Feature *>(ptr);

  } else {

    if (info.Length() < 1) {
      Nan::ThrowError("Constructor expects Layer or FeatureDefn object");
      return;
    }

    OGRFeatureDefn *def;

    if (IS_WRAPPED(info[0], Layer)) {
      Layer *layer = Nan::ObjectWrap::Unwrap<Layer>(info[0].As<Object>());
      if (!layer->isAlive()) {
        Nan::ThrowError("Layer object already destroyed");
        return;
      }
      def = layer->get()->GetLayerDefn();
    } else if (IS_WRAPPED(info[0], FeatureDefn)) {
      FeatureDefn *feature_def = Nan::ObjectWrap::Unwrap<FeatureDefn>(info[0].As<Object>());
      if (!feature_def->isAlive()) {
        Nan::ThrowError("FeatureDefn object already destroyed");
        return;
      }
      def = feature_def->get();
    } else {
      Nan::ThrowError("Constructor expects Layer or FeatureDefn object");
      return;
    }

    OGRFeature *ogr_f = new OGRFeature(def);
    f = new Feature(ogr_f);
  }

  Local<Value> fields = FeatureFields::New(info.This());
  Nan::SetPrivate(info.This(), Nan::New("fields_").ToLocalChecked(), fields);

  f->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

Local<Value> Feature::New(OGRFeature *feature) {
  Nan::EscapableHandleScope scope;
  return scope.Escape(Feature::New(feature, true));
}

Local<Value> Feature::New(OGRFeature *feature, bool owned) {
  Nan::EscapableHandleScope scope;

  if (!feature) { return scope.Escape(Nan::Null()); }

  Feature *wrapped = new Feature(feature);
  wrapped->owned_ = owned;
  Local<Value> ext = Nan::New<External>(wrapped);
  Local<Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(Feature::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();
  return scope.Escape(obj);
}

NAN_METHOD(Feature::toString) {
  info.GetReturnValue().Set(Nan::New("Feature").ToLocalChecked());
}

/**
 * Returns the geometry of the feature.
 *
 * @method getGeometry
 * @instance
 * @memberof Feature
 * @return {Geometry|null}
 */
NAN_METHOD(Feature::getGeometry) {

  Feature *feature = Nan::ObjectWrap::Unwrap<Feature>(info.This());
  if (!feature->isAlive()) {
    Nan::ThrowError("Feature object already destroyed");
    return;
  }

  OGRGeometry *geom = feature->this_->GetGeometryRef();
  if (!geom) {
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  info.GetReturnValue().Set(Geometry::New(geom, false));
}

#if 0
/*
 * Returns the definition of a particular field at an index.
 *
 * _method getFieldDefn
 * _param {number} index Field index (0-based)
 * _return {FieldDefn}
 */
NAN_METHOD(Feature::getFieldDefn) {
  int field_index;
  NODE_ARG_INT(0, "field index", field_index);

  Feature *feature = Nan::ObjectWrap::Unwrap<Feature>(info.This());
  if (!feature->isAlive()) {
    Nan::ThrowError("Feature object already destroyed");
    return;
  }

  if (field_index < 0 || field_index >= feature->this_->GetFieldCount()) {
    Nan::ThrowRangeError("Invalid field index");
    return;
  }

  info.GetReturnValue().Set(FieldDefn::New(feature->this_->GetFieldDefnRef(field_index), false));
}
#endif

// NODE_WRAPPED_METHOD_WITH_RESULT(Feature, stealGeometry, Geometry,
// StealGeometry);

/**
 * Sets the feature's geometry.
 *
 * @throws {Error}
 * @method setGeometry
 * @instance
 * @memberof Feature
 * @param {Geometry|null} geometry new geometry or null to clear the field
 */
NAN_METHOD(Feature::setGeometry) {

  Geometry *geom = NULL;
  NODE_ARG_WRAPPED_OPT(0, "geometry", Geometry, geom);

  Feature *feature = Nan::ObjectWrap::Unwrap<Feature>(info.This());
  if (!feature->isAlive()) {
    Nan::ThrowError("Feature object already destroyed");
    return;
  }

  OGRErr err = feature->this_->SetGeometry(geom ? geom->get() : NULL);
  if (err) { NODE_THROW_OGRERR(err); }

  return;
}

/**
 * Determines if the features are the same.
 *
 * @method equals
 * @instance
 * @memberof Feature
 * @param {Feature} feature
 * @return {boolean} `true` if the features are the same, `false` if different
 */
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(Feature, equals, Boolean, Equal, Feature, "feature");

/**
 * Clones the feature.
 *
 * @method clone
 * @instance
 * @memberof Feature
 * @return {Feature}
 */
NAN_METHOD(Feature::clone) {
  Feature *feature = Nan::ObjectWrap::Unwrap<Feature>(info.This());
  if (!feature->isAlive()) {
    Nan::ThrowError("Feature object already destroyed");
    return;
  }
  info.GetReturnValue().Set(Feature::New(feature->this_->Clone()));
}

/**
 * Releases the feature from memory.
 *
 * @method destroy
 * @instance
 * @memberof Feature
 */
NAN_METHOD(Feature::destroy) {
  Feature *feature = Nan::ObjectWrap::Unwrap<Feature>(info.This());
  if (!feature->isAlive()) {
    Nan::ThrowError("Feature object already destroyed");
    return;
  }
  feature->dispose();
  return;
}

/**
 * Set one feature from another. Overwrites the contents of this feature
 * from the geometry and attributes of another.
 *
 * @example
 *
 * var feature1 = new gdal.Feature(defn);
 * var feature2 = new gdal.Feature(defn);
 * feature1.setGeometry(new gdal.Point(5, 10));
 * feature1.fields.set([5, 'test', 3.14]);
 * feature2.setFrom(feature1);
 *
 * @throws {Error}
 * @method setFrom
 * @instance
 * @memberof Feature
 * @param {Feature} feature
 * @param {number[]} [index_map] Array mapping each field from the source feature
 * to the given index in the destination feature. -1 ignores the source field.
 * The field types must still match otherwise the behavior is undefined.
 * @param {boolean} [forgiving=true] `true` if the operation should continue
 * despite lacking output fields matching some of the source fields.
 */
NAN_METHOD(Feature::setFrom) {
  Feature *other_feature;
  int forgiving = 1;
  Local<Array> index_map;
  OGRErr err = 0;

  NODE_ARG_WRAPPED(0, "feature", Feature, other_feature);

  Feature *feature = Nan::ObjectWrap::Unwrap<Feature>(info.This());
  if (!feature->isAlive()) {
    Nan::ThrowError("Feature object already destroyed");
    return;
  }

  if (!info[1]->IsArray()) {
    NODE_ARG_BOOL_OPT(1, "forgiving", forgiving);

    err = feature->this_->SetFrom(other_feature->this_, forgiving ? TRUE : FALSE);
  } else {
    NODE_ARG_ARRAY(1, "index map", index_map);
    NODE_ARG_BOOL_OPT(2, "forgiving", forgiving);

    if (index_map->Length() < 1) {
      Nan::ThrowError("index map must contain at least 1 index");
      return;
    }

    int *index_map_ptr = new int[index_map->Length()];

    for (unsigned index = 0; index < index_map->Length(); index++) {
      Local<Value> field_index(Nan::Get(index_map, Nan::New<Integer>(index)).ToLocalChecked());

      if (!field_index->IsInt32()) {
        delete[] index_map_ptr;
        Nan::ThrowError("index map must contain only integer values");
        return;
      }

      int val = (int)Nan::To<int32_t>(field_index).ToChecked(); // todo: validate index? perhaps ogr already
                                                                // does this and throws an error

      index_map_ptr[index] = val;
    }

    err = feature->this_->SetFrom(other_feature->this_, index_map_ptr, forgiving ? TRUE : FALSE);

    delete[] index_map_ptr;
  }

  if (err) {
    NODE_THROW_OGRERR(err);
    return;
  }
  return;
}

/**
 * @readonly
 * @kind member
 * @name fields
 * @instance
 * @memberof Feature
 * @type {FeatureFields}
 */
NAN_GETTER(Feature::fieldsGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("fields_").ToLocalChecked()).ToLocalChecked());
}

/**
 * @kind member
 * @name fid
 * @instance
 * @memberof Feature
 * @type {number}
 */
NAN_GETTER(Feature::fidGetter) {
  Feature *feature = Nan::ObjectWrap::Unwrap<Feature>(info.This());
  if (!feature->isAlive()) {
    Nan::ThrowError("Feature object already destroyed");
    return;
  }
  info.GetReturnValue().Set(Nan::New<Number>(feature->this_->GetFID()));
}

/**
 * @readonly
 * @kind member
 * @name defn
 * @instance
 * @memberof Feature
 * @type {FeatureDefn}
 */
NAN_GETTER(Feature::defnGetter) {
  Feature *feature = Nan::ObjectWrap::Unwrap<Feature>(info.This());
  if (!feature->isAlive()) {
    Nan::ThrowError("Feature object already destroyed");
    return;
  }
  info.GetReturnValue().Set(FeatureDefn::New(feature->this_->GetDefnRef(), false));
}

/**
 * Returns the OGR style string for this feature, if any.
 *
 * @method getStyleString
 * @instance
 * @memberof Feature
 * @return {string|null}
 */
NAN_METHOD(Feature::getStyleString) {
  Feature *feature = Nan::ObjectWrap::Unwrap<Feature>(info.This());
  if (!feature->isAlive()) {
    Nan::ThrowError("Feature object already destroyed");
    return;
  }
  const char *psz = feature->this_->GetStyleString();
  if (!psz) {
    info.GetReturnValue().Set(Nan::Null());
    return;
  }
  info.GetReturnValue().Set(Nan::New(psz).ToLocalChecked());
}

/**
 * Sets the OGR style string for this feature. Pass null/undefined to clear.
 *
 * @throws {Error}
 * @method setStyleString
 * @instance
 * @memberof Feature
 * @param {string|null|undefined} style
 */
NAN_METHOD(Feature::setStyleString) {
  Feature *feature = Nan::ObjectWrap::Unwrap<Feature>(info.This());
  if (!feature->isAlive()) {
    Nan::ThrowError("Feature object already destroyed");
    return;
  }

  if (info.Length() < 1 || info[0]->IsNull() || info[0]->IsUndefined()) {
    // Clear style if null/undefined or no arg
    feature->this_->SetStyleString(nullptr);
    return;
  }

  if (!info[0]->IsString()) {
    Nan::ThrowTypeError("style must be a string, null or undefined");
    return;
  }

  Nan::Utf8String utf8(info[0]);
  feature->this_->SetStyleString(*utf8);
}

NAN_SETTER(Feature::fidSetter) {
  Feature *feature = Nan::ObjectWrap::Unwrap<Feature>(info.This());
  if (!feature->isAlive()) {
    Nan::ThrowError("Feature object already destroyed");
    return;
  }
  if (!value->IsInt32()) {
    Nan::ThrowError("fid must be an integer");
    return;
  }
  feature->this_->SetFID(Nan::To<int64_t>(value).ToChecked());
}

} // namespace node_gdal
