#include "gdal_attribute.hpp"
#include "gdal_common.hpp"
#include "gdal_driver.hpp"
#include "gdal_group.hpp"
#include "collections/group_groups.hpp"
#include "collections/group_arrays.hpp"
#include "geometry/gdal_geometry.hpp"
#include "gdal_layer.hpp"
#include "gdal_majorobject.hpp"
#include "gdal_rasterband.hpp"
#include "gdal_spatial_reference.hpp"

namespace node_gdal {

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

Napi::FunctionReference Attribute::constructor;

void Attribute::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(Attribute);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "Attribute",
    {
        METHOD(toString)
        ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER)
        ATTR(lcons, "dataType", typeGetter, READ_ONLY_SETTER)
        ATTR(lcons, "value", valueGetter, READ_ONLY_SETTER)
    });

  target.Set("Attribute", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

Attribute::Attribute(const Napi::CallbackInfo &info) : GDALObject<Attribute>(info), uid(0), this_(nullptr), parent_ds(0) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = node_gdal::ImportShared<GDALAttribute>(info);
    LOG("Created attribute [%p]", this_.get());
    return;
  }
  Napi::Error::New(info.Env(), "Cannot create Attribute directly").ThrowAsJavaScriptException();
}

Attribute::~Attribute() {
  dispose();
}

void Attribute::dispose() {
  if (this_) {

    LOG("Disposing attribute [%p]", this_.get());

    object_store.dispose(uid);

    LOG("Disposed attribute [%p]", this_.get());
  }
};

/**
 * A representation of a group with access methods.
 *
 * @class Attribute
 */

Napi::Value Attribute::New(std::shared_ptr<GDALAttribute> raw, GDALDataset *parent_ds) {

  if (!raw) { return node_gdal::napi_env().Null(); }
  if (object_store.has(raw)) { return object_store.get(raw); }

  std::vector<napi_value> args = {
    Napi::External<void>::New(node_gdal::napi_env(), node_gdal::ExportShared(raw))};
  Napi::Object obj = Attribute::constructor.Value().New(args);
  Attribute *wrapped = node_gdal::UnwrapWrapped<Attribute>(obj);

  Dataset *unwrapped_ds = node_gdal::UnwrapWrapped<Dataset>(object_store.get(parent_ds));
  long parent_uid = unwrapped_ds->uid;

  wrapped->uid = object_store.add(raw, *wrapped, parent_uid);
  wrapped->parent_ds = parent_ds;
  wrapped->parent_uid = parent_uid;


  return obj;
}

NAPI_METHOD(Attribute::toString) {
  return Napi::String::New(node_gdal::napi_env(), "Attribute");
}

/**
 * Complex GDAL data types introduced in 3.1 are not yet supported
 * @readonly
 * @kind member
 * @name value
 * @instance
 * @memberof Attribute
 * @throws {Error}
 * @type {string|number}
 */
NAPI_GETTER(Attribute::valueGetter) {
  NODE_UNWRAP_CHECK(Attribute, info.This(), attribute);
  GDAL_RAW_CHECK(std::shared_ptr<GDALAttribute>, attribute, raw);
  GDAL_LOCK_PARENT(attribute);
  GDALExtendedDataType type = raw->GetDataType();
  Napi::Value r;
  switch (type.GetClass()) {
    case GEDTC_NUMERIC: r = Napi::Number::New(node_gdal::napi_env(), raw->ReadAsDouble()); break;
    case GEDTC_STRING: r = SafeString::New(raw->ReadAsString()); break;
    default: Napi::Error::New(node_gdal::napi_env(), "Compound attributes are not supported yet").ThrowAsJavaScriptException(); return node_gdal::napi_env().Undefined();
  }

  return r;
}

/**
 * @readonly
 * @kind member
 * @name dataType
 * @instance
 * @memberof Attribute
 * @type {string}
 */
NAPI_GETTER(Attribute::typeGetter) {
  NODE_UNWRAP_CHECK(Attribute, info.This(), attribute);
  GDAL_RAW_CHECK(std::shared_ptr<GDALAttribute>, attribute, raw);
  GDAL_LOCK_PARENT(attribute);
  GDALExtendedDataType type = raw->GetDataType();
  const char *r;
  switch (type.GetClass()) {
    case GEDTC_NUMERIC: r = GDALGetDataTypeName(type.GetNumericDataType()); break;
    case GEDTC_STRING: r = "String"; break;
    case GEDTC_COMPOUND: r = "Compound"; break;
    default: Napi::Error::New(node_gdal::napi_env(), "Invalid attribute type").ThrowAsJavaScriptException(); return node_gdal::napi_env().Undefined();
  }

  return SafeString::New(r);
}

NAPI_GETTER(Attribute::uidGetter) {
  Attribute *group = node_gdal::UnwrapWrapped<Attribute>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env(), (int)group->uid);
}

#endif

} // namespace node_gdal
