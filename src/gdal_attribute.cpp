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

Nan::Persistent<FunctionTemplate> Attribute::constructor;

void Attribute::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(Attribute::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("Attribute").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);

  ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER);
  ATTR(lcons, "dataType", typeGetter, READ_ONLY_SETTER);
  ATTR(lcons, "value", valueGetter, READ_ONLY_SETTER);

  Nan::Set(target, Nan::New("Attribute").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

Attribute::Attribute(std::shared_ptr<GDALAttribute> attribute)
  : Nan::ObjectWrap(), uid(0), this_(attribute), parent_ds(0) {
  LOG("Created attribute [%p]", attribute.get());
}

Attribute::Attribute() : Nan::ObjectWrap(), uid(0), this_(0), parent_ds(0) {
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
NAN_METHOD(Attribute::New) {
  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }

  if (info.Length() == 1 && info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    Attribute *f = static_cast<Attribute *>(ptr);
    f->Wrap(info.This());

    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create attribute directly. Create with dataset instead.");
    return;
  }

  info.GetReturnValue().Set(info.This());
}

Local<Value> Attribute::New(std::shared_ptr<GDALAttribute> raw, GDALDataset *parent_ds) {
  Nan::EscapableHandleScope scope;

  if (!raw) { return scope.Escape(Nan::Null()); }
  if (object_store.has(raw)) { return scope.Escape(object_store.get(raw)); }

  Attribute *wrapped = new Attribute(raw);

  Local<Object> ds;
  if (object_store.has(parent_ds)) {
    ds = object_store.get(parent_ds);
  } else {
    LOG("Attribute's parent dataset disappeared from cache (array = %p, dataset = %p)", raw.get(), parent_ds);
    Nan::ThrowError("Attribute's parent dataset disappeared from cache");
    return scope.Escape(Nan::Undefined());
  }

  Local<Value> ext = Nan::New<External>(wrapped);
  Local<Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(Attribute::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();

  Dataset *unwrapped_ds = Nan::ObjectWrap::Unwrap<Dataset>(ds);
  long parent_uid = unwrapped_ds->uid;

  wrapped->uid = object_store.add(raw, wrapped->persistent(), parent_uid);
  wrapped->parent_ds = parent_ds;
  wrapped->parent_uid = parent_uid;

  Nan::SetPrivate(obj, Nan::New("ds_").ToLocalChecked(), ds);

  return scope.Escape(obj);
}

NAN_METHOD(Attribute::toString) {
  info.GetReturnValue().Set(Nan::New("Attribute").ToLocalChecked());
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
NAN_GETTER(Attribute::valueGetter) {
  NODE_UNWRAP_CHECK(Attribute, info.This(), attribute);
  GDAL_RAW_CHECK(std::shared_ptr<GDALAttribute>, attribute, raw);
  GDAL_LOCK_PARENT(attribute);
  GDALExtendedDataType type = raw->GetDataType();
  Local<Value> r;
  switch (type.GetClass()) {
    case GEDTC_NUMERIC: r = Nan::New<Number>(raw->ReadAsDouble()); break;
    case GEDTC_STRING: r = SafeString::New(raw->ReadAsString()); break;
    default: Nan::ThrowError("Compound attributes are not supported yet"); return;
  }

  info.GetReturnValue().Set(r);
}

/**
 * @readonly
 * @kind member
 * @name dataType
 * @instance
 * @memberof Attribute
 * @type {string}
 */
NAN_GETTER(Attribute::typeGetter) {
  NODE_UNWRAP_CHECK(Attribute, info.This(), attribute);
  GDAL_RAW_CHECK(std::shared_ptr<GDALAttribute>, attribute, raw);
  GDAL_LOCK_PARENT(attribute);
  GDALExtendedDataType type = raw->GetDataType();
  const char *r;
  switch (type.GetClass()) {
    case GEDTC_NUMERIC: r = GDALGetDataTypeName(type.GetNumericDataType()); break;
    case GEDTC_STRING: r = "String"; break;
    case GEDTC_COMPOUND: r = "Compound"; break;
    default: Nan::ThrowError("Invalid attribute type"); return;
  }

  info.GetReturnValue().Set(SafeString::New(r));
}

NAN_GETTER(Attribute::uidGetter) {
  Attribute *group = Nan::ObjectWrap::Unwrap<Attribute>(info.This());
  info.GetReturnValue().Set(Nan::New((int)group->uid));
}

#endif

} // namespace node_gdal
