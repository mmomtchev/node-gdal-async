#include "../gdal_common.hpp"
#include "../gdal_feature.hpp"
#include "feature_fields.hpp"

namespace node_gdal {

Napi::FunctionReference FeatureFields::constructor;

void FeatureFields::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(FeatureFields);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "FeatureFields",
    {
        METHOD(toString)
        METHOD(toObject)
        METHOD(toArray)
        METHOD(count)
        METHOD(get)
        METHOD(getNames)
        METHOD(set)
        METHOD(reset)
        METHOD(indexOf)
        ATTR_DONT_ENUM(lcons, "feature", featureGetter, READ_ONLY_SETTER)
    });

  target.Set("FeatureFields", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

FeatureFields::FeatureFields(const Napi::CallbackInfo &info) : GDALObject<FeatureFields>(info) {
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::Error::New(info.Env(), "Cannot create FeatureFields directly").ThrowAsJavaScriptException();
    return;
  }
  GDAL_SET_PRIVATE(info.This(), "parent_", info[0]);
}

FeatureFields::~FeatureFields() {
}

/**
 * An encapsulation of all field data that makes up a {@link Feature}.
 *
 * @class FeatureFields
 */

Napi::Value FeatureFields::New(Napi::Value layer_obj) {

  std::vector<napi_value> args = {layer_obj};
  Napi::Object obj = FeatureFields::constructor.Value().New(args);

  return obj;
}

NAPI_METHOD(FeatureFields::toString) {
  return Napi::String::New(node_gdal::napi_env(), "FeatureFields");
}

inline bool setField(OGRFeature *f, int field_index, Napi::Value val) {
  if (val.IsNumber()) {
    // V8's IsInt32 (what the original used) is "the value survives a round trip
    // through int32"; OGR keeps whatever precision the field type allows
    const double d = val.As<Napi::Number>().DoubleValue();
    const int32_t i = val.As<Napi::Number>().Int32Value();
    if (static_cast<double>(i) == d) {
      f->SetField(field_index, i);
    } else {
      f->SetField(field_index, d);
    }
  } else if (val.IsString()) {
    std::string str = val.As<Napi::String>().Utf8Value();
    f->SetField(field_index, str.c_str());
  } else if (val.IsNull() || val.IsUndefined()) {
    f->UnsetField(field_index);
  } else {
    return true;
  }
  return false;
}

/**
 * Sets feature field(s).
 *
 * @example
 *
 * // most-efficient, least flexible. requires you to know the ordering of the
 * fields: feature.fields.set(['Something']); feature.fields.set(0,
 * 'Something');
 *
 * // most flexible.
 * feature.fields.set({name: 'Something'});
 * feature.fields.set('name', 'Something');
 *
 *
 * @method set
 * @instance
 * @memberof FeatureFields
 * @throws {Error}
 * @param {string|number} key Field name or index
 * @param {any} value
 */

/**
 * @method set
 * @instance
 * @memberof FeatureFields
 * @throws {Error}
 * @param {object} fields
 */
NAPI_METHOD(FeatureFields::set) {
  int field_index;
  unsigned int i, n, n_fields_set;

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Feature *f = node_gdal::UnwrapWrapped<Feature>(parent);
  if (!f->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (info.Length() == 1) {
    if (info[0].IsArray()) {
      // set([])
      Napi::Array values = info[0].As<Napi::Array>();

      n = f->get()->GetFieldCount();
      if (values.Length() < n) { n = values.Length(); }

      for (i = 0; i < n; i++) {
        Napi::Value val = values.As<Napi::Object>().Get(i);
        if (setField(f->get(), i, val)) {
          Napi::Error::New(node_gdal::napi_env(), "Unsupported type of field value").ThrowAsJavaScriptException();
          return node_gdal::napi_env().Undefined();
        }
      }

      return Napi::Number::New(node_gdal::napi_env(), n);
      return node_gdal::napi_env().Undefined();
    } else if (info[0].IsObject()) {
      // set({})
      Napi::Object values = info[0].As<Napi::Object>();

      n = f->get()->GetFieldCount();
      n_fields_set = 0;

      for (i = 0; i < n; i++) {
        // iterate through field names from field defn,
        // grabbing values from passed object, if not undefined

        const OGRFieldDefn *field_def = f->get()->GetFieldDefnRef(i);

        const char *field_name = field_def->GetNameRef();

        field_index = f->get()->GetFieldIndex(field_name);

        // skip value if field name doesnt exist
        // both in the feature definition and the passed object
        if (field_index == -1 || !values.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), field_name))) {
          continue;
        }

        Napi::Value val = values.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), field_name));
        if (setField(f->get(), field_index, val)) {
          Napi::Error::New(node_gdal::napi_env(), "Unsupported type of field value").ThrowAsJavaScriptException();
          return node_gdal::napi_env().Undefined();
        }

        n_fields_set++;
      }

      return Napi::Number::New(node_gdal::napi_env(), n_fields_set);
      return node_gdal::napi_env().Undefined();
    } else {
      Napi::Error::New(node_gdal::napi_env(), "Method expected an object or array").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }

  } else if (info.Length() == 2) {
    // set(name|index, value)
    ARG_FIELD_ID(0, f->get(), field_index);

    // set field value
    if (setField(f->get(), field_index, info[1])) {
      Napi::Error::New(node_gdal::napi_env(), "Unsupported type of field value").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }

    return Napi::Number::New(node_gdal::napi_env(), 1);
    return node_gdal::napi_env().Undefined();
  } else {
    Napi::Error::New(node_gdal::napi_env(), "Invalid number of arguments").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
}

/**
 * Resets all fields.
 *
 * @example
 *
 * feature.fields.reset();
 *
 * @method reset
 * @instance
 * @memberof FeatureFields
 * @throws {Error}
 * @param {object} [values]
 * @return {void}
 */
NAPI_METHOD(FeatureFields::reset) {
  int field_index;
  unsigned int i, n;

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Feature *f = node_gdal::UnwrapWrapped<Feature>(parent);
  if (!f->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  n = f->get()->GetFieldCount();

  if (info.Length() == 0) {
    for (i = 0; i < n; i++) { f->get()->UnsetField(i); }
    return Napi::Number::New(node_gdal::napi_env(), n);
    return node_gdal::napi_env().Undefined();
  }

  if (!info[0].IsObject()) {
    Napi::Error::New(node_gdal::napi_env(), "fields must be an object").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  Napi::Object values = info[0].As<Napi::Object>();

  for (i = 0; i < n; i++) {
    // iterate through field names from field defn,
    // grabbing values from passed object

    const OGRFieldDefn *field_def = f->get()->GetFieldDefnRef(i);

    const char *field_name = field_def->GetNameRef();

    field_index = f->get()->GetFieldIndex(field_name);
    if (field_index == -1) continue;

    Napi::Value val = values.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), field_name));
    if (setField(f->get(), field_index, val)) {
      Napi::Error::New(node_gdal::napi_env(), "Unsupported type of field value").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }
  }

  return Napi::Number::New(node_gdal::napi_env(), n);
}

/**
 * Returns the number of fields.
 *
 * @example
 *
 * feature.fields.count();
 *
 * @method count
 * @instance
 * @memberof FeatureFields
 * @return {number}
 */
NAPI_METHOD(FeatureFields::count) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Feature *f = node_gdal::UnwrapWrapped<Feature>(parent);
  if (!f->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  return Napi::Number::New(node_gdal::napi_env(), f->get()->GetFieldCount());
}

/**
 * Returns the index of a field, given its name.
 *
 * @example
 *
 * var index = feature.fields.indexOf('field');
 *
 * @method indexOf
 * @instance
 * @memberof FeatureFields
 * @param {string} name
 * @return {number} Index or, `-1` if it cannot be found.
 */
NAPI_METHOD(FeatureFields::indexOf) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Feature *f = node_gdal::UnwrapWrapped<Feature>(parent);
  if (!f->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  std::string name("");
  NODE_ARG_STR(0, "field name", name);

  return Napi::Number::New(node_gdal::napi_env(), f->get()->GetFieldIndex(name.c_str()));
}

/**
 * Outputs the field data as a pure JS object.
 *
 * @throws {Error}
 * @method toObject
 * @instance
 * @memberof FeatureFields
 * @return {any}
 */
NAPI_METHOD(FeatureFields::toObject) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Feature *f = node_gdal::UnwrapWrapped<Feature>(parent);
  if (!f->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  Napi::Object obj = Napi::Object::New(node_gdal::napi_env());

  int n = f->get()->GetFieldCount();
  for (int i = 0; i < n; i++) {

    // get field name
    const OGRFieldDefn *field_def = f->get()->GetFieldDefnRef(i);
    const char *key = field_def->GetNameRef();
    if (!key) {
      Napi::Error::New(node_gdal::napi_env(), "Error getting field name").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }

    // get field value
    try {
      Napi::Value val = FeatureFields::get(f->get(), i);
      obj.Set( Napi::String::New(node_gdal::napi_env(), key), val);
    } catch (const char *err) {
      Napi::Error::New(node_gdal::napi_env(), err).ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }
  }
  return obj;
}

/**
 * Outputs the field values as a pure JS array.
 *
 * @throws {Error}
 * @method toArray
 * @instance
 * @memberof FeatureFields
 * @return {any[]}
 */
NAPI_METHOD(FeatureFields::toArray) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Feature *f = node_gdal::UnwrapWrapped<Feature>(parent);
  if (!f->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  int n = f->get()->GetFieldCount();
  Napi::Array array = Napi::Array::New(node_gdal::napi_env(), n);

  for (int i = 0; i < n; i++) {
    // get field value
    try {
      Napi::Value val = FeatureFields::get(f->get(), i);
      array.Set( i, val);
    } catch (const char *err) {
      Napi::Error::New(node_gdal::napi_env(), err).ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }
  }
  return array;
}

Napi::Value FeatureFields::get(OGRFeature *f, int field_index) {
  //throws

  if (field_index < 0 || field_index >= f->GetFieldCount()) throw "Invalid field";
  if (!f->IsFieldSet(field_index)) return node_gdal::napi_env().Null();
  if (f->IsFieldNull(field_index)) return node_gdal::napi_env().Null();

  const OGRFieldDefn *field_def = f->GetFieldDefnRef(field_index);
  switch (field_def->GetType()) {
    case OFTInteger: return Napi::Number::New(node_gdal::napi_env(), f->GetFieldAsInteger(field_index));
    case OFTInteger64: return Napi::Number::New(node_gdal::napi_env(), f->GetFieldAsInteger64(field_index));
    case OFTInteger64List: return getFieldAsInteger64List(f, field_index);
    case OFTReal: return Napi::Number::New(node_gdal::napi_env(), f->GetFieldAsDouble(field_index));
    case OFTString: return SafeString::New(f->GetFieldAsString(field_index));
    case OFTIntegerList: return getFieldAsIntegerList(f, field_index);
    case OFTRealList: return getFieldAsDoubleList(f, field_index);
    case OFTStringList: return getFieldAsStringList(f, field_index);
    case OFTBinary: return getFieldAsBinary(f, field_index);
    case OFTDate:
    case OFTTime:
    case OFTDateTime: return getFieldAsDateTime(f, field_index);
    default: throw "Unsupported field type";
  }
}

/**
 * Returns a field's value.
 *
 * @example
 *
 * value = feature.fields.get(0);
 * value = feature.fields.get('field');
 *
 * @method get
 * @instance
 * @memberof FeatureFields
 * @param {string|number} key Feature name or index.
 * @throws {Error}
 * @return {any}
 */
NAPI_METHOD(FeatureFields::get) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Feature *f = node_gdal::UnwrapWrapped<Feature>(parent);
  if (!f->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env(), "Field index or name must be given").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  int field_index;
  ARG_FIELD_ID(0, f->get(), field_index);

  try {
    Napi::Value result = FeatureFields::get(f->get(), field_index);
    return result;
  } catch (const char *err) { Napi::Error::New(node_gdal::napi_env(), err).ThrowAsJavaScriptException(); }
  return node_gdal::napi_env().Undefined();
}

/**
 * Returns a list of field name.
 *
 * @method getNames
 * @instance
 * @memberof FeatureFields
 * @throws {Error}
 * @return {string[]} List of field names.
 */
NAPI_METHOD(FeatureFields::getNames) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Feature *f = node_gdal::UnwrapWrapped<Feature>(parent);
  if (!f->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Feature object already destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  int n = f->get()->GetFieldCount();
  Napi::Array result = Napi::Array::New(node_gdal::napi_env(), n);

  for (int i = 0; i < n; i++) {

    // get field name
    const OGRFieldDefn *field_def = f->get()->GetFieldDefnRef(i);
    const char *field_name = field_def->GetNameRef();
    if (!field_name) {
      Napi::Error::New(node_gdal::napi_env(), "Error getting field name").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }
    result.Set( i, Napi::String::New(node_gdal::napi_env(), field_name));
  }

  return result;
}

Napi::Value FeatureFields::getFieldAsIntegerList(OGRFeature *feature, int field_index) {

  int count_of_values = 0;

  const int *values = feature->GetFieldAsIntegerList(field_index, &count_of_values);

  Napi::Array return_array = Napi::Array::New(node_gdal::napi_env(), count_of_values);

  for (int index = 0; index < count_of_values; index++) {
    return_array.Set( index, Napi::Number::New(node_gdal::napi_env(), values[index]));
  }

  return return_array;
}

Napi::Value FeatureFields::getFieldAsInteger64List(OGRFeature *feature, int field_index) {

  int count_of_values = 0;

  const long long *values = feature->GetFieldAsInteger64List(field_index, &count_of_values);

  Napi::Array return_array = Napi::Array::New(node_gdal::napi_env(), count_of_values);

  for (int index = 0; index < count_of_values; index++) {
    return_array.Set( index, Napi::Number::New(node_gdal::napi_env(), values[index]));
  }

  return return_array;
}

Napi::Value FeatureFields::getFieldAsDoubleList(OGRFeature *feature, int field_index) {

  int count_of_values = 0;

  const double *values = feature->GetFieldAsDoubleList(field_index, &count_of_values);

  Napi::Array return_array = Napi::Array::New(node_gdal::napi_env(), count_of_values);

  for (int index = 0; index < count_of_values; index++) {
    return_array.Set( index, Napi::Number::New(node_gdal::napi_env(), values[index]));
  }

  return return_array;
}

Napi::Value FeatureFields::getFieldAsStringList(OGRFeature *feature, int field_index) {
  char **values = feature->GetFieldAsStringList(field_index);

  int count_of_values = CSLCount(values);

  Napi::Array return_array = Napi::Array::New(node_gdal::napi_env(), count_of_values);

  for (int index = 0; index < count_of_values; index++) {
    return_array.Set( index, SafeString::New(values[index]));
  }

  return return_array;
}

Napi::Value FeatureFields::getFieldAsBinary(OGRFeature *feature, int field_index) {

  int count_of_bytes = 0;

  char *data = (char *)feature->GetFieldAsBinary(field_index, &count_of_bytes);

  if (count_of_bytes > 0) {
    // GDAL Feature->GetFieldAsBinary returns a pointer to an internal buffer
    // that should not be freed
    // The lifetime of this internal buffer does not match the lifetime of
    // the returned buffer
    // So we copy
    return Napi::Buffer<char>::Copy(node_gdal::napi_env(), data, count_of_bytes);
  }

  return node_gdal::napi_env().Undefined();
}

Napi::Value FeatureFields::getFieldAsDateTime(OGRFeature *feature, int field_index) {

  int year, month, day, hour, minute, second, timezone;

  year = month = day = hour = minute = second = timezone = 0;

  int result = feature->GetFieldAsDateTime(field_index, &year, &month, &day, &hour, &minute, &second, &timezone);

  if (result == TRUE) {
    Napi::Object hash = Napi::Object::New(node_gdal::napi_env());

    if (year) { hash.Set( Napi::String::New(node_gdal::napi_env(), "year"), Napi::Number::New(node_gdal::napi_env(), year)); }
    if (month) { hash.Set( Napi::String::New(node_gdal::napi_env(), "month"), Napi::Number::New(node_gdal::napi_env(), month)); }
    if (day) { hash.Set( Napi::String::New(node_gdal::napi_env(), "day"), Napi::Number::New(node_gdal::napi_env(), day)); }
    if (hour) { hash.Set( Napi::String::New(node_gdal::napi_env(), "hour"), Napi::Number::New(node_gdal::napi_env(), hour)); }
    if (minute) { hash.Set( Napi::String::New(node_gdal::napi_env(), "minute"), Napi::Number::New(node_gdal::napi_env(), minute)); }
    if (second) { hash.Set( Napi::String::New(node_gdal::napi_env(), "second"), Napi::Number::New(node_gdal::napi_env(), second)); }
    if (timezone) { hash.Set( Napi::String::New(node_gdal::napi_env(), "timezone"), Napi::Number::New(node_gdal::napi_env(), timezone)); }

    return hash;
  } else {
    return node_gdal::napi_env().Undefined();
  }
}

/**
 * Returns the parent feature.
 *
 * @readonly
 * @kind member
 * @name feature
 * @instance
 * @memberof FeatureFields
 * @type {Feature}
 */
NAPI_GETTER(FeatureFields::featureGetter) {
  return GDAL_GET_PRIVATE(info.This(), "parent_");
}

} // namespace node_gdal
