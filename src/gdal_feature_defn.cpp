
#include "gdal_feature_defn.hpp"
#include "collections/feature_defn_fields.hpp"
#include "gdal_common.hpp"
#include "gdal_field_defn.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> FeatureDefn::constructor;

void FeatureDefn::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(FeatureDefn::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("FeatureDefn").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan::SetPrototypeMethod(lcons, "clone", clone);

  ATTR(lcons, "name", nameGetter, READ_ONLY_SETTER);
  ATTR(lcons, "fields", fieldsGetter, READ_ONLY_SETTER);
  ATTR(lcons, "styleIgnored", styleIgnoredGetter, styleIgnoredSetter);
  ATTR(lcons, "geomIgnored", geomIgnoredGetter, geomIgnoredSetter);
  ATTR(lcons, "geomType", geomTypeGetter, geomTypeSetter);

  Nan::Set(target, Nan::New("FeatureDefn").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

FeatureDefn::FeatureDefn(OGRFeatureDefn *def) : Nan::ObjectWrap(), this_(def), owned_(true) {
  LOG("Created FeatureDefn [%p]", def);
}

FeatureDefn::FeatureDefn() : Nan::ObjectWrap(), this_(0), owned_(true) {
}

FeatureDefn::~FeatureDefn() {
  if (this_) {
    LOG("Disposing FeatureDefn [%p] (%s)", this_, owned_ ? "owned" : "unowned");
    if (owned_) this_->Release();
    this_ = NULL;
    LOG("Disposed FeatureDefn [%p]", this_);
  }
}

/**
 * Definition of a feature class or feature layer.
 *
 * @constructor
 * @class FeatureDefn
 */
NAN_METHOD(FeatureDefn::New) {
  FeatureDefn *f;

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }

  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    f = static_cast<FeatureDefn *>(ptr);
  } else {
    if (info.Length() != 0) {
      Nan::ThrowError("FeatureDefn constructor doesn't take any arguments");
      return;
    }
    f = new FeatureDefn(new OGRFeatureDefn());
    f->this_->Reference();
  }

  Local<Value> fields = FeatureDefnFields::New(info.This());
  Nan::SetPrivate(info.This(), Nan::New("fields_").ToLocalChecked(), fields);

  f->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

// Currently read-only feature definitions are copied.
// Modifying a feature definition that should have been
// read-only modifies a shadow copy without any real effect.
// TODO: Implement proper read-only objects that throw

Local<Value> FeatureDefn::New(const OGRFeatureDefn *def) {
  Nan::EscapableHandleScope scope;
  if (!def) { return scope.Escape(Nan::Null()); }
  OGRFeatureDefn *copy = def->Clone();
  return scope.Escape(FeatureDefn::New(copy, true));
}

Local<Value> FeatureDefn::New(OGRFeatureDefn *def, bool owned) {
  Nan::EscapableHandleScope scope;

  if (!def) { return scope.Escape(Nan::Null()); }
  if (!owned) { def = def->Clone(); }

  FeatureDefn *wrapped = new FeatureDefn(def);
  wrapped->owned_ = true;
  def->Reference();

  Local<Value> ext = Nan::New<External>(wrapped);
  Local<Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(FeatureDefn::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();

  return scope.Escape(obj);
}

NAN_METHOD(FeatureDefn::toString) {
  info.GetReturnValue().Set(Nan::New("FeatureDefn").ToLocalChecked());
}

/**
 * Clones the feature definition.
 *
 * @method clone
 * @instance
 * @memberof FeatureDefn
 * @return {FeatureDefn}
 */
NAN_METHOD(FeatureDefn::clone) {
  FeatureDefn *def = Nan::ObjectWrap::Unwrap<FeatureDefn>(info.This());
  info.GetReturnValue().Set(FeatureDefn::New(def->this_->Clone()));
}

/**
 * @readonly
 * @kind member
 * @name name
 * @instance
 * @memberof FeatureDefn
 * @type {string}
 */
NAN_GETTER(FeatureDefn::nameGetter) {
  FeatureDefn *def = Nan::ObjectWrap::Unwrap<FeatureDefn>(info.This());
  info.GetReturnValue().Set(SafeString::New(def->this_->GetName()));
}

/**
 * WKB geometry type ({@link wkbGeometryType|see table}
 *
 * @kind member
 * @name geomType
 * @instance
 * @memberof FeatureDefn
 * @type {number}
 */
NAN_GETTER(FeatureDefn::geomTypeGetter) {
  FeatureDefn *def = Nan::ObjectWrap::Unwrap<FeatureDefn>(info.This());
  info.GetReturnValue().Set(Nan::New<Integer>(def->this_->GetGeomType()));
}

/**
 * @kind member
 * @name geomIgnored
 * @instance
 * @memberof FeatureDefn
 * @type {boolean}
 */
NAN_GETTER(FeatureDefn::geomIgnoredGetter) {
  FeatureDefn *def = Nan::ObjectWrap::Unwrap<FeatureDefn>(info.This());
  info.GetReturnValue().Set(Nan::New<Boolean>(def->this_->IsGeometryIgnored()));
}

/**
 * @kind member
 * @name styleIgnored
 * @instance
 * @memberof FeatureDefn
 * @type {boolean}
 */
NAN_GETTER(FeatureDefn::styleIgnoredGetter) {
  FeatureDefn *def = Nan::ObjectWrap::Unwrap<FeatureDefn>(info.This());
  info.GetReturnValue().Set(Nan::New<Boolean>(def->this_->IsStyleIgnored()));
}

/**
 * @readonly
 * @kind member
 * @name fields
 * @instance
 * @memberof FeatureDefn
 * @type {FeatureDefnFields}
 */
NAN_GETTER(FeatureDefn::fieldsGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("fields_").ToLocalChecked()).ToLocalChecked());
}

NAN_SETTER(FeatureDefn::geomTypeSetter) {
  FeatureDefn *def = Nan::ObjectWrap::Unwrap<FeatureDefn>(info.This());
  if (!value->IsInt32()) {
    Nan::ThrowError("geomType must be an integer");
    return;
  }
  def->this_->SetGeomType(OGRwkbGeometryType(Nan::To<int64_t>(value).ToChecked()));
}

NAN_SETTER(FeatureDefn::geomIgnoredSetter) {
  FeatureDefn *def = Nan::ObjectWrap::Unwrap<FeatureDefn>(info.This());
  if (!value->IsBoolean()) {
    Nan::ThrowError("geomIgnored must be a boolean");
    return;
  }
  def->this_->SetGeometryIgnored(Nan::To<int64_t>(value).ToChecked());
}

NAN_SETTER(FeatureDefn::styleIgnoredSetter) {
  FeatureDefn *def = Nan::ObjectWrap::Unwrap<FeatureDefn>(info.This());
  if (!value->IsBoolean()) {
    Nan::ThrowError("styleIgnored must be a boolean");
    return;
  }
  def->this_->SetStyleIgnored(Nan::To<int64_t>(value).ToChecked());
}

} // namespace node_gdal
