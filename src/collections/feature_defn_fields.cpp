#include "feature_defn_fields.hpp"
#include "../gdal_common.hpp"
#include "../gdal_feature_defn.hpp"
#include "../gdal_field_defn.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> FeatureDefnFields::constructor;

void FeatureDefnFields::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(FeatureDefnFields::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("FeatureDefnFields").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan::SetPrototypeMethod(lcons, "count", count);
  Nan::SetPrototypeMethod(lcons, "get", get);
  Nan::SetPrototypeMethod(lcons, "remove", remove);
  Nan::SetPrototypeMethod(lcons, "getNames", getNames);
  Nan::SetPrototypeMethod(lcons, "indexOf", indexOf);
  Nan::SetPrototypeMethod(lcons, "reorder", reorder);
  Nan::SetPrototypeMethod(lcons, "add", add);
  // Nan::SetPrototypeMethod(lcons, "alter", alter);

  ATTR_DONT_ENUM(lcons, "featureDefn", featureDefnGetter, READ_ONLY_SETTER);

  Nan::Set(target, Nan::New("FeatureDefnFields").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

FeatureDefnFields::FeatureDefnFields() : Nan::ObjectWrap() {
}

FeatureDefnFields::~FeatureDefnFields() {
}

/**
 * An encapsulation of a {@link FeatureDefn}'s fields.
 *
 * @class FeatureDefnFields
 */
NAN_METHOD(FeatureDefnFields::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }
  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    FeatureDefnFields *feature_def = static_cast<FeatureDefnFields *>(ptr);
    feature_def->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create FeatureDefnFields directly");
    return;
  }
}

Local<Value> FeatureDefnFields::New(Local<Value> feature_defn) {
  Nan::EscapableHandleScope scope;

  FeatureDefnFields *wrapped = new FeatureDefnFields();

  v8::Local<v8::Value> ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(FeatureDefnFields::constructor)).ToLocalChecked(), 1, &ext)
      .ToLocalChecked();
  Nan::SetPrivate(obj, Nan::New("parent_").ToLocalChecked(), feature_defn);

  return scope.Escape(obj);
}

NAN_METHOD(FeatureDefnFields::toString) {
  info.GetReturnValue().Set(Nan::New("FeatureDefnFields").ToLocalChecked());
}

/**
 * Returns the number of fields.
 *
 * @method count
 * @instance
 * @memberof FeatureDefnFields
 * @return {number}
 */
NAN_METHOD(FeatureDefnFields::count) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  FeatureDefn *feature_def = Nan::ObjectWrap::Unwrap<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Nan::ThrowError("FeatureDefn object already destroyed");
    return;
  }

  info.GetReturnValue().Set(Nan::New<Integer>(feature_def->get()->GetFieldCount()));
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
NAN_METHOD(FeatureDefnFields::indexOf) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  FeatureDefn *feature_def = Nan::ObjectWrap::Unwrap<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Nan::ThrowError("FeatureDefn object already destroyed");
    return;
  }

  std::string name("");
  NODE_ARG_STR(0, "field name", name);

  info.GetReturnValue().Set(Nan::New<Integer>(feature_def->get()->GetFieldIndex(name.c_str())));
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
NAN_METHOD(FeatureDefnFields::get) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  FeatureDefn *feature_def = Nan::ObjectWrap::Unwrap<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Nan::ThrowError("FeatureDefn object already destroyed");
    return;
  }

  if (info.Length() < 1) {
    Nan::ThrowError("Field index or name must be given");
    return;
  }

  int field_index;
  ARG_FIELD_ID(0, feature_def->get(), field_index);

  CPLErrorReset();
  auto r = feature_def->get()->GetFieldDefn(field_index);
  if (r == nullptr) { throw CPLGetLastErrorMsg(); }
  info.GetReturnValue().Set(FieldDefn::New(r));
}

/**
 * Returns a list of field names.
 *
 * @method getNames
 * @instance
 * @memberof FeatureDefnFields
 * @return {string[]} List of field names.
 */
NAN_METHOD(FeatureDefnFields::getNames) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  FeatureDefn *feature_def = Nan::ObjectWrap::Unwrap<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Nan::ThrowError("FeatureDefn object already destroyed");
    return;
  }

  int n = feature_def->get()->GetFieldCount();
  Local<Array> result = Nan::New<Array>(n);

  for (int i = 0; i < n; i++) {
    OGRFieldDefn *field_def = feature_def->get()->GetFieldDefn(i);
    Nan::Set(result, i, SafeString::New(field_def->GetNameRef()));
  }

  info.GetReturnValue().Set(result);
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
NAN_METHOD(FeatureDefnFields::remove) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  FeatureDefn *feature_def = Nan::ObjectWrap::Unwrap<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Nan::ThrowError("FeatureDefn object already destroyed");
    return;
  }

  if (info.Length() < 1) {
    Nan::ThrowError("Field index or name must be given");
    return;
  }

  int field_index;
  ARG_FIELD_ID(0, feature_def->get(), field_index);

  int err = feature_def->get()->DeleteFieldDefn(field_index);
  if (err) {
    NODE_THROW_OGRERR(err);
    return;
  }

  return;
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
NAN_METHOD(FeatureDefnFields::add) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  FeatureDefn *feature_def = Nan::ObjectWrap::Unwrap<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Nan::ThrowError("FeatureDefn object already destroyed");
    return;
  }
  if (info.Length() < 1) {
    Nan::ThrowError("field definition(s) must be given");
    return;
  }

  FieldDefn *field_def;

  if (info[0]->IsArray()) {
    Local<Array> array = info[0].As<Array>();
    int n = array->Length();
    for (int i = 0; i < n; i++) {
      Local<Value> element = Nan::Get(array, i).ToLocalChecked();
      if (IS_WRAPPED(element, FieldDefn)) {
        field_def = Nan::ObjectWrap::Unwrap<FieldDefn>(element.As<Object>());
        feature_def->get()->AddFieldDefn(field_def->get());
      } else {
        Nan::ThrowError("All array elements must be FieldDefn objects");
        return;
      }
    }
  } else if (IS_WRAPPED(info[0], FieldDefn)) {
    field_def = Nan::ObjectWrap::Unwrap<FieldDefn>(info[0].As<Object>());
    feature_def->get()->AddFieldDefn(field_def->get());
  } else {
    Nan::ThrowError("field definition(s) must be a FieldDefn object or array of FieldDefn objects");
    return;
  }

  return;
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
NAN_METHOD(FeatureDefnFields::reorder) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  FeatureDefn *feature_def = Nan::ObjectWrap::Unwrap<FeatureDefn>(parent);
  if (!feature_def->isAlive()) {
    Nan::ThrowError("FeatureDefn object already destroyed");
    return;
  }

  Local<Array> field_map = Nan::New<Array>(0);
  NODE_ARG_ARRAY(0, "field map", field_map);

  int n = feature_def->get()->GetFieldCount();
  OGRErr err = 0;

  if ((int)field_map->Length() != n) {
    Nan::ThrowError("Array length must match field count");
    return;
  }

  int *field_map_array = new int[n];

  for (int i = 0; i < n; i++) {
    Local<Value> val = Nan::Get(field_map, i).ToLocalChecked();
    if (!val->IsNumber()) {
      delete[] field_map_array;
      Nan::ThrowError("Array must only contain integers");
      return;
    }

    int key = Nan::To<int64_t>(val).ToChecked();
    if (key < 0 || key >= n) {
      delete[] field_map_array;
      Nan::ThrowError("Values must be between 0 and field count - 1");
      return;
    }

    field_map_array[i] = key;
  }

  err = feature_def->get()->ReorderFieldDefns(field_map_array);

  delete[] field_map_array;

  if (err) {
    NODE_THROW_OGRERR(err);
    return;
  }
  return;
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
NAN_GETTER(FeatureDefnFields::featureDefnGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked());
}

} // namespace node_gdal
