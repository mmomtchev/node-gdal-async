
#include "gdal_field_defn.hpp"
#include "gdal_common.hpp"
#include "utils/field_types.hpp"

namespace node_gdal {

Napi::FunctionReference FieldDefn::constructor;

void FieldDefn::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(FieldDefn);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "FieldDefn",
    {
        ATTR(lcons, "name", nameGetter, nameSetter)
        ATTR(lcons, "type", typeGetter, typeSetter)
        ATTR(lcons, "justification", justificationGetter, justificationSetter)
        ATTR(lcons, "width", widthGetter, widthSetter)
        ATTR(lcons, "precision", precisionGetter, precisionSetter)
        ATTR(lcons, "ignored", ignoredGetter, ignoredSetter)
        METHOD(toString)
    });

  target.Set("FieldDefn", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

FieldDefn::FieldDefn(const Napi::CallbackInfo &info) : GDALObject<FieldDefn>(info), this_(nullptr), owned_(false) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = static_cast<OGRFieldDefn *>(info[0].As<Napi::External<void>>().Data());
    LOG("Created FieldDefn [%p]", this_);
    return;
  }

  // Constructed from JS: FieldDefn(name, type). The NODE_ARG_* macros cannot be
  // used here - they return a value on error, which a constructor cannot do
  if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
    Napi::Error::New(info.Env(), "FieldDefn requires a field name and a field type").ThrowAsJavaScriptException();
    return;
  }
  std::string field_name = info[0].As<Napi::String>().Utf8Value();
  std::string type_name = info[1].As<Napi::String>().Utf8Value();

  int field_type = getFieldTypeByName(type_name);
  if (field_type < 0) {
    Napi::Error::New(info.Env(), "Unrecognized field type").ThrowAsJavaScriptException();
    return;
  }

  this_ = new OGRFieldDefn(field_name.c_str(), static_cast<OGRFieldType>(field_type));
  owned_ = true;
}

FieldDefn::~FieldDefn() {
  if (this_) {
    LOG("Disposing FieldDefn [%p] (%s)", this_, owned_ ? "owned" : "unowned");
    if (owned_) delete this_;
    LOG("Disposed FieldDefn [%p]", this_);
    this_ = NULL;
  }
}

// Currently read-only field definitions are copied.
// Modifying a field definition that should have been
// read-only modifies a shadow copy without any real effect.
// TODO: Implement proper read-only objects that throw

Napi::Value FieldDefn::New(const OGRFieldDefn *def) {
  if (!def) { return node_gdal::napi_env().Null(); }
  OGRFieldDefn *copy = new OGRFieldDefn(def);
  return FieldDefn::New(copy, true);
}

Napi::Value FieldDefn::New(OGRFieldDefn *def, bool owned) {

  if (!def) { return node_gdal::napi_env().Null(); }
  if (!owned) { def = new OGRFieldDefn(def); }

  std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env(), def)};
  Napi::Object obj = FieldDefn::constructor.Value().New(args);
  FieldDefn *wrapped = node_gdal::UnwrapWrapped<FieldDefn>(obj);

  return obj;
}

NAPI_METHOD(FieldDefn::toString) {
  return Napi::String::New(node_gdal::napi_env(), "FieldDefn");
}

/**
 * @kind member
 * @name name
 * @instance
 * @memberof FieldDefn
 * @type {string}
 */
NAPI_GETTER(FieldDefn::nameGetter) {
  FieldDefn *def = node_gdal::UnwrapWrapped<FieldDefn>(info.This().As<Napi::Object>());
  return SafeString::New(def->this_->GetNameRef());
}

/**
 * Data type (see {@link OFT|OFT constants}
 *
 * @kind member
 * @name type
 * @instance
 * @memberof FieldDefn
 * @type {string}
 */
NAPI_GETTER(FieldDefn::typeGetter) {
  FieldDefn *def = node_gdal::UnwrapWrapped<FieldDefn>(info.This().As<Napi::Object>());
  return SafeString::New(getFieldTypeName(def->this_->GetType()));
}

/**
 * @kind member
 * @name ignored
 * @instance
 * @memberof FieldDefn
 * @type {boolean}
 */
NAPI_GETTER(FieldDefn::ignoredGetter) {
  FieldDefn *def = node_gdal::UnwrapWrapped<FieldDefn>(info.This().As<Napi::Object>());
  return Napi::Boolean::New(node_gdal::napi_env(), def->this_->IsIgnored());
}

/**
 * Field justification (see {@link OJ|OJ constants})
 *
 * @kind member
 * @name justification
 * @instance
 * @memberof FieldDefn
 * @type {string}
 */
NAPI_GETTER(FieldDefn::justificationGetter) {
  FieldDefn *def = node_gdal::UnwrapWrapped<FieldDefn>(info.This().As<Napi::Object>());
  OGRJustification justification = def->this_->GetJustify();
  if (justification == OJRight) {
    return Napi::String::New(node_gdal::napi_env(), "Right");
    return node_gdal::napi_env().Undefined();
  }
  if (justification == OJLeft) {
    return Napi::String::New(node_gdal::napi_env(), "Left");
    return node_gdal::napi_env().Undefined();
  }
  return node_gdal::napi_env().Undefined();
}

/**
 * @kind member
 * @name width
 * @instance
 * @memberof FieldDefn
 * @type {number}
 */
NAPI_GETTER(FieldDefn::widthGetter) {
  FieldDefn *def = node_gdal::UnwrapWrapped<FieldDefn>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env(), def->this_->GetWidth());
}

/**
 * @kind member
 * @name precision
 * @instance
 * @memberof FieldDefn
 * @type {number}
 */
NAPI_GETTER(FieldDefn::precisionGetter) {
  FieldDefn *def = node_gdal::UnwrapWrapped<FieldDefn>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env(), def->this_->GetPrecision());
}

NAPI_SETTER(FieldDefn::nameSetter) {
  FieldDefn *def = node_gdal::UnwrapWrapped<FieldDefn>(info.This().As<Napi::Object>());
  if (!value.IsString()) {
    Napi::Error::New(node_gdal::napi_env(), "Name must be string").ThrowAsJavaScriptException();
    return;
  }
  std::string name = value.As<Napi::String>().Utf8Value();
  def->this_->SetName(name.c_str());
}

NAPI_SETTER(FieldDefn::typeSetter) {
  FieldDefn *def = node_gdal::UnwrapWrapped<FieldDefn>(info.This().As<Napi::Object>());
  if (!value.IsString()) {
    Napi::Error::New(node_gdal::napi_env(), "type must be a string").ThrowAsJavaScriptException();
    return;
  }
  std::string name = value.As<Napi::String>().Utf8Value();
  int type = getFieldTypeByName(name.c_str());
  if (type < 0) {
    Napi::Error::New(node_gdal::napi_env(), "Unrecognized field type").ThrowAsJavaScriptException();
  } else {
    def->this_->SetType(OGRFieldType(type));
  }
}

NAPI_SETTER(FieldDefn::justificationSetter) {
  FieldDefn *def = node_gdal::UnwrapWrapped<FieldDefn>(info.This().As<Napi::Object>());

  OGRJustification justification;
  if (value.IsString()) {
    const std::string str = value.As<Napi::String>().Utf8Value();
    if (str == "Left") {
      justification = OJLeft;
    } else if (str == "Right") {
      justification = OJRight;
    } else if (str == "Undefined") {
      justification = OJUndefined;
    } else {
      Napi::Error::New(node_gdal::napi_env(), "Unrecognized justification").ThrowAsJavaScriptException();
      return;
    }
  } else if (value.IsNull() || value.IsUndefined()) {
    justification = OJUndefined;
  } else {
    Napi::Error::New(node_gdal::napi_env(), "justification must be a string or undefined").ThrowAsJavaScriptException();
    return;
  }

  def->this_->SetJustify(justification);
}

NAPI_SETTER(FieldDefn::widthSetter) {
  FieldDefn *def = node_gdal::UnwrapWrapped<FieldDefn>(info.This().As<Napi::Object>());
  if (!value.IsNumber()) {
    Napi::Error::New(node_gdal::napi_env(), "width must be an integer").ThrowAsJavaScriptException();
    return;
  }
  def->this_->SetWidth(value.As<Napi::Number>().Int64Value());
}

NAPI_SETTER(FieldDefn::precisionSetter) {
  FieldDefn *def = node_gdal::UnwrapWrapped<FieldDefn>(info.This().As<Napi::Object>());
  if (!value.IsNumber()) {
    Napi::Error::New(node_gdal::napi_env(), "precision must be an integer").ThrowAsJavaScriptException();
    return;
  }
  def->this_->SetPrecision(value.As<Napi::Number>().Int64Value());
}

NAPI_SETTER(FieldDefn::ignoredSetter) {
  FieldDefn *def = node_gdal::UnwrapWrapped<FieldDefn>(info.This().As<Napi::Object>());
  if (!value.IsBoolean()) {
    Napi::Error::New(node_gdal::napi_env(), "ignored must be a boolean").ThrowAsJavaScriptException();
    return;
  }
  def->this_->SetIgnored(value.As<Napi::Boolean>().Value());
}

} // namespace node_gdal
