
#include "gdal_feature.hpp"
#include "collections/feature_fields.hpp"
#include "gdal_common.hpp"
#include "gdal_feature_defn.hpp"
#include "gdal_field_defn.hpp"
#include "geometry/gdal_geometry.hpp"
#include "gdal_layer.hpp"

namespace node_gdal {

Napi::FunctionReference Feature::constructor;

void Feature::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(Feature);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "Feature",
    {
        METHOD(toString)
        METHOD(getGeometry)
        METHOD(setGeometry)
        METHOD(clone)
        METHOD(setFrom)
        METHOD(getStyleString)
        METHOD(setStyleString)
        METHOD(destroy)
        ATTR(lcons, "fields", fieldsGetter, READ_ONLY_SETTER)
        ATTR(lcons, "defn", defnGetter, READ_ONLY_SETTER)
        ATTR(lcons, "fid", fidGetter, fidSetter)
    });

  // METHOD(setGeometryDirectly);
  // METHOD(stealGeometry);
  // METHOD(equals);
  // METHOD(getFieldDefn); (use defn.fields.get() instead)
  // Note: This is used mainly for testing
  // TODO: Give node more info on the amount of memory a feature is using
  //      Napi::MemoryManagement::AdjustExternalMemory(node_gdal::napi_env(), )

  target.Set("Feature", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
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
Feature::Feature(const Napi::CallbackInfo &info) : GDALObject<Feature>(info), this_(nullptr), owned_(true) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = static_cast<OGRFeature *>(info[0].As<Napi::External<void>>().Data());
    LOG("Created Feature[%p]", this_);
  } else {

    if (info.Length() < 1) {
      Napi::Error::New(info.Env(), "Constructor expects Layer or FeatureDefn object").ThrowAsJavaScriptException();
      return;
    }

    OGRFeatureDefn *def;

    if (IS_WRAPPED(info[0], Layer)) {
      Layer *layer = node_gdal::UnwrapWrapped<Layer>(info[0].As<Napi::Object>());
      if (!layer->isAlive()) {
        Napi::Error::New(info.Env(), "Layer object already destroyed").ThrowAsJavaScriptException();
        return;
      }
      def = layer->get()->GetLayerDefn();
    } else if (IS_WRAPPED(info[0], FeatureDefn)) {
      FeatureDefn *feature_def = node_gdal::UnwrapWrapped<FeatureDefn>(info[0].As<Napi::Object>());
      if (!feature_def->isAlive()) {
        Napi::Error::New(info.Env(), "FeatureDefn object already destroyed").ThrowAsJavaScriptException();
        return;
      }
      def = feature_def->get();
    } else {
      Napi::Error::New(info.Env(), "Constructor expects Layer or FeatureDefn object").ThrowAsJavaScriptException();
      return;
    }

    this_ = new OGRFeature(def);
    LOG("Created Feature[%p]", this_);
  }

  Napi::Value fields = FeatureFields::New(info.This());
  GDAL_SET_PRIVATE(info.This(), "fields_", fields);
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

// Currently read-only feature definitions are copied.

Napi::Value Feature::New(OGRFeature *feature) {
  return Feature::New(feature, true);
}

// Features are not tracked in the object store: the same OGRFeature can be
// handed to JS more than once
Napi::Value Feature::New(OGRFeature *feature, bool owned) {
  if (!feature) { return node_gdal::napi_env().Null(); }

  std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env(), feature)};
  Napi::Object obj = Feature::constructor.Value().New(args);
  node_gdal::UnwrapWrapped<Feature>(obj)->owned_ = owned;
  return obj;
}

NAPI_METHOD(Feature::toString) {
  return Napi::String::New(node_gdal::napi_env(), "Feature");
}

/**
 * Returns the geometry of the feature.
 *
 * @method getGeometry
 * @instance
 * @memberof Feature
 * @return {Geometry|null}
 */
NAPI_METHOD(Feature::getGeometry) {

  Feature *feature = node_gdal::UnwrapWrapped<Feature>(info.This().As<Napi::Object>());
  if (!feature->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  OGRGeometry *geom = feature->this_->GetGeometryRef();
  if (!geom) {
    return node_gdal::napi_env().Null();
    return node_gdal::napi_env().Undefined();
  }

  return Geometry::New(geom, false);
}

#if 0
/*
 * Returns the definition of a particular field at an index.
 *
 * _method getFieldDefn
 * _param {number} index Field index (0-based)
 * _return {FieldDefn}
 */
NAPI_METHOD(Feature::getFieldDefn) {
  int field_index;
  NODE_ARG_INT(0, "field index", field_index);

  Feature *feature = node_gdal::UnwrapWrapped<Feature>(info.This().As<Napi::Object>());
  if (!feature->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (field_index < 0 || field_index >= feature->this_->GetFieldCount()) {
    Napi::RangeError::New(node_gdal::napi_env(), "Invalid field index").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  return FieldDefn::New(feature->this_->GetFieldDefnRef(field_index), false);
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
NAPI_METHOD(Feature::setGeometry) {

  Geometry *geom = NULL;
  NODE_ARG_WRAPPED_OPT(0, "geometry", Geometry, geom);

  Feature *feature = node_gdal::UnwrapWrapped<Feature>(info.This().As<Napi::Object>());
  if (!feature->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  OGRErr err = feature->this_->SetGeometry(geom ? geom->get() : NULL);
  if (err) { NODE_THROW_OGRERR(err); }

  return node_gdal::napi_env().Undefined();
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
NAPI_METHOD(Feature::clone) {
  Feature *feature = node_gdal::UnwrapWrapped<Feature>(info.This().As<Napi::Object>());
  if (!feature->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  return Feature::New(feature->this_->Clone());
}

/**
 * Releases the feature from memory.
 *
 * @method destroy
 * @instance
 * @memberof Feature
 */
NAPI_METHOD(Feature::destroy) {
  Feature *feature = node_gdal::UnwrapWrapped<Feature>(info.This().As<Napi::Object>());
  if (!feature->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  feature->dispose();
  return node_gdal::napi_env().Undefined();
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
NAPI_METHOD(Feature::setFrom) {
  Feature *other_feature;
  int forgiving = 1;
  Napi::Array index_map;
  OGRErr err = 0;

  NODE_ARG_WRAPPED(0, "feature", Feature, other_feature);

  Feature *feature = node_gdal::UnwrapWrapped<Feature>(info.This().As<Napi::Object>());
  if (!feature->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (!info[1].IsArray()) {
    NODE_ARG_BOOL_OPT(1, "forgiving", forgiving);

    err = feature->this_->SetFrom(other_feature->this_, forgiving ? TRUE : FALSE);
  } else {
    NODE_ARG_ARRAY(1, "index map", index_map);
    NODE_ARG_BOOL_OPT(2, "forgiving", forgiving);

    if (index_map.Length() < 1) {
      Napi::Error::New(node_gdal::napi_env(), "index map must contain at least 1 index").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }

    int *index_map_ptr = new int[index_map.Length()];

    for (unsigned index = 0; index < index_map.Length(); index++) {
      Napi::Value field_index(index_map.As<Napi::Object>().Get(Napi::Number::New(node_gdal::napi_env(), index)));

      if (!field_index.IsNumber()) {
        delete[] index_map_ptr;
        Napi::Error::New(node_gdal::napi_env(), "index map must contain only integer values").ThrowAsJavaScriptException();
        return node_gdal::napi_env().Undefined();
      }

      int val = (int)field_index.As<Napi::Number>().Int32Value(); // todo: validate index? perhaps ogr already
                                                                // does this and throws an error

      index_map_ptr[index] = val;
    }

    err = feature->this_->SetFrom(other_feature->this_, index_map_ptr, forgiving ? TRUE : FALSE);

    delete[] index_map_ptr;
  }

  if (err) {
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }
  return node_gdal::napi_env().Undefined();
}

/**
 * @readonly
 * @kind member
 * @name fields
 * @instance
 * @memberof Feature
 * @type {FeatureFields}
 */
NAPI_GETTER(Feature::fieldsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "fields_");
}

/**
 * @kind member
 * @name fid
 * @instance
 * @memberof Feature
 * @type {number}
 */
NAPI_GETTER(Feature::fidGetter) {
  Feature *feature = node_gdal::UnwrapWrapped<Feature>(info.This().As<Napi::Object>());
  if (!feature->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  return Napi::Number::New(node_gdal::napi_env(), feature->this_->GetFID());
}

/**
 * @readonly
 * @kind member
 * @name defn
 * @instance
 * @memberof Feature
 * @type {FeatureDefn}
 */
NAPI_GETTER(Feature::defnGetter) {
  Feature *feature = node_gdal::UnwrapWrapped<Feature>(info.This().As<Napi::Object>());
  if (!feature->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  return FeatureDefn::New(feature->this_->GetDefnRef());
}

/**
 * Returns the OGR style string for this feature, if any.
 *
 * @method getStyleString
 * @instance
 * @memberof Feature
 * @return {string|null}
 */
NAPI_METHOD(Feature::getStyleString) {
  Feature *feature = node_gdal::UnwrapWrapped<Feature>(info.This().As<Napi::Object>());
  if (!feature->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  const char *psz = feature->this_->GetStyleString();
  if (!psz) {
    return node_gdal::napi_env().Null();
    return node_gdal::napi_env().Undefined();
  }
  return Napi::String::New(node_gdal::napi_env(), psz);
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
NAPI_METHOD(Feature::setStyleString) {
  Feature *feature = node_gdal::UnwrapWrapped<Feature>(info.This().As<Napi::Object>());
  if (!feature->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (info.Length() < 1 || info[0].IsNull() || info[0].IsUndefined()) {
    // Clear style if null/undefined or no arg
    feature->this_->SetStyleString(nullptr);
    return node_gdal::napi_env().Undefined();
  }

  if (!info[0].IsString()) {
    Napi::TypeError::New(node_gdal::napi_env(), "style must be a string, null or undefined").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  std::string utf8 = info[0].As<Napi::String>().Utf8Value();
  feature->this_->SetStyleString(utf8.c_str());
  return node_gdal::napi_env().Undefined();
}

NAPI_SETTER(Feature::fidSetter) {
  Feature *feature = node_gdal::UnwrapWrapped<Feature>(info.This().As<Napi::Object>());
  if (!feature->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return;
  }
  if (!value.IsNumber()) {
    Napi::Error::New(node_gdal::napi_env(), "fid must be an integer").ThrowAsJavaScriptException();
    return;
  }
  feature->this_->SetFID(value.As<Napi::Number>().Int64Value());
}

} // namespace node_gdal
