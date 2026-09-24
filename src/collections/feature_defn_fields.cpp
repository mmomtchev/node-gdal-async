#include "feature_defn_fields.hpp"
#include "../gdal_common.hpp"
#include "../gdal_feature_defn.hpp"
#include "../gdal_field_defn.hpp"

namespace node_gdal {

Napi::FunctionReference FeatureDefnFields::constructor;

void FeatureDefnFields::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(FeatureDefnFields);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "FeatureDefnFields",
    {
        METHOD(toString)
        METHOD(count)
        METHOD(get)
        METHOD(remove)
        METHOD(getNames)
        METHOD(indexOf)
        METHOD(reorder)
        METHOD(add)
        ATTR_DONT_ENUM(lcons, "featureDefn", featureDefnGetter, READ_ONLY_SETTER)
    });

  // METHOD(alter);

  target.Set("FeatureDefnFields", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

FeatureDefnFields::FeatureDefnFields(const Napi::CallbackInfo &info) : GDALObject<FeatureDefnFields>(info) {
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::Error::New(info.Env(), "Cannot create FeatureDefnFields directly").ThrowAsJavaScriptException();
    return;
  }
  GDAL_SET_PRIVATE(info.This(), "parent_", info[0]);
}

FeatureDefnFields::~FeatureDefnFields() {
}

/**
 * An encapsulation of a {@link FeatureDefn}'s fields.
 *
 * @class FeatureDefnFields
 */

Napi::Value FeatureDefnFields::New(Napi::Value feature_defn) {

  std::vector<napi_value> args = {feature_defn};
  Napi::Object obj = FeatureDefnFields::constructor.Value().New(args);

  return obj;
}

NAPI_METHOD(FeatureDefnFields::toString) {
  return Napi::String::New(node_gdal::napi_env(), "FeatureDefnFields");
}

/**
 * Returns the number of fields.
 *
 * @method count
 * @instance
 * @memberof FeatureDefnFields
 * @return {number}
 */
NAPI_METHOD(FeatureDefnFields::count) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  FeatureDefn *feature_def = node_gdal::UnwrapWrapped<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "FeatureDefn object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  return Napi::Number::New(node_gdal::napi_env(), feature_def->get()->GetFieldCount());
}

/**
 * Returns the index of field definition.
 *
 * @method indexOf
 * @instance
 * @memberof FeatureDefnFields
 * @param {string} name
 * @return {number} Index or `-1` if not found.
 */
NAPI_METHOD(FeatureDefnFields::indexOf) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  FeatureDefn *feature_def = node_gdal::UnwrapWrapped<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "FeatureDefn object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  std::string name("");
  NODE_ARG_STR(0, "field name", name);

  return Napi::Number::New(node_gdal::napi_env(), feature_def->get()->GetFieldIndex(name.c_str()));
}

/**
 * Returns a field definition.
 *
 * @method get
 * @instance
 * @memberof FeatureDefnFields
 * @param {string|number} key Field name or index
 * @throws {Error}
 * @return {FieldDefn}
 */
NAPI_METHOD(FeatureDefnFields::get) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  FeatureDefn *feature_def = node_gdal::UnwrapWrapped<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "FeatureDefn object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env(), "Field index or name must be given").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  int field_index;
  ARG_FIELD_ID(0, feature_def->get(), field_index);

  CPLErrorReset();
  auto r = feature_def->get()->GetFieldDefn(field_index);
  if (r == nullptr) { throw CPLGetLastErrorMsg(); }
  return FieldDefn::New(r);
}

/**
 * Returns a list of field names.
 *
 * @method getNames
 * @instance
 * @memberof FeatureDefnFields
 * @return {string[]} List of field names.
 */
NAPI_METHOD(FeatureDefnFields::getNames) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  FeatureDefn *feature_def = node_gdal::UnwrapWrapped<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "FeatureDefn object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  int n = feature_def->get()->GetFieldCount();
  Napi::Array result = Napi::Array::New(node_gdal::napi_env(), n);

  for (int i = 0; i < n; i++) {
    OGRFieldDefn *field_def = feature_def->get()->GetFieldDefn(i);
    result.Set( i, SafeString::New(field_def->GetNameRef()));
  }

  return result;
}

/**
 * Removes a field definition.
 *
 * @method remove
 * @instance
 * @memberof FeatureDefnFields
 * @throws {Error}
 * @param {string|number} key Field name or index
 */
NAPI_METHOD(FeatureDefnFields::remove) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  FeatureDefn *feature_def = node_gdal::UnwrapWrapped<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "FeatureDefn object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env(), "Field index or name must be given").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  int field_index;
  ARG_FIELD_ID(0, feature_def->get(), field_index);

  int err = feature_def->get()->DeleteFieldDefn(field_index);
  if (err) {
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }

  return node_gdal::napi_env().Undefined();
}

/**
 * Adds field definition(s).
 *
 * @method add
 * @instance
 * @memberof FeatureDefnFields
 * @throws {Error}
 * @param {FieldDefn|FieldDefn[]} fields
 */
NAPI_METHOD(FeatureDefnFields::add) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  FeatureDefn *feature_def = node_gdal::UnwrapWrapped<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "FeatureDefn object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env(), "field definition(s) must be given").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  FieldDefn *field_def;

  if (info[0].IsArray()) {
    Napi::Array array = info[0].As<Napi::Array>();
    int n = array.Length();
    for (int i = 0; i < n; i++) {
      Napi::Value element = array.As<Napi::Object>().Get(i);
      if (IS_WRAPPED(element, FieldDefn)) {
        field_def = node_gdal::UnwrapWrapped<FieldDefn>(element.As<Napi::Object>());
        feature_def->get()->AddFieldDefn(field_def->get());
      } else {
        Napi::Error::New(node_gdal::napi_env(), "All array elements must be FieldDefn objects").ThrowAsJavaScriptException();
        return node_gdal::napi_env().Undefined();
      }
    }
  } else if (IS_WRAPPED(info[0], FieldDefn)) {
    field_def = node_gdal::UnwrapWrapped<FieldDefn>(info[0].As<Napi::Object>());
    feature_def->get()->AddFieldDefn(field_def->get());
  } else {
    Napi::Error::New(node_gdal::napi_env(), "field definition(s) must be a FieldDefn object or array of FieldDefn objects").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  return node_gdal::napi_env().Undefined();
}

/**
 * Reorders the fields.
 *
 * @example
 *
 * // reverse fields:
 * featureDef.fields.reorder([2, 1, 0]);
 *
 * @method reorder
 * @instance
 * @memberof FeatureDefnFields
 * @throws {Error}
 * @param {number[]} map An array representing the new field order.
 */
NAPI_METHOD(FeatureDefnFields::reorder) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  FeatureDefn *feature_def = node_gdal::UnwrapWrapped<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "FeatureDefn object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  Napi::Array field_map = Napi::Array::New(node_gdal::napi_env(), 0);
  NODE_ARG_ARRAY(0, "field map", field_map);

  int n = feature_def->get()->GetFieldCount();
  OGRErr err = 0;

  if ((int)field_map.Length() != n) {
    Napi::Error::New(node_gdal::napi_env(), "Array length must match field count").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  int *field_map_array = new int[n];

  for (int i = 0; i < n; i++) {
    Napi::Value val = field_map.As<Napi::Object>().Get(i);
    if (!val.IsNumber()) {
      delete[] field_map_array;
      Napi::Error::New(node_gdal::napi_env(), "Array must only contain integers").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }

    int key = val.As<Napi::Number>().Int64Value();
    if (key < 0 || key >= n) {
      delete[] field_map_array;
      Napi::Error::New(node_gdal::napi_env(), "Values must be between 0 and field count - 1").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }

    field_map_array[i] = key;
  }

  err = feature_def->get()->ReorderFieldDefns(field_map_array);

  delete[] field_map_array;

  if (err) {
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }
  return node_gdal::napi_env().Undefined();
}

/**
 * Returns the parent feature definition.
 *
 * @readonly
 * @kind member
 * @name featureDefn
 * @instance
 * @memberof FeatureDefnFields
 * @type {FeatureDefn}
 */
NAPI_GETTER(FeatureDefnFields::featureDefnGetter) {
  return GDAL_GET_PRIVATE(info.This(), "parent_");
}

} // namespace node_gdal
