#include "gdal_dimension.hpp"
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

Nan::Persistent<FunctionTemplate> Dimension::constructor;

void Dimension::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(Dimension::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("Dimension").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);

  ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER);
  ATTR(lcons, "size", sizeGetter, READ_ONLY_SETTER);
  ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER);
  ATTR(lcons, "type", typeGetter, READ_ONLY_SETTER);
  ATTR(lcons, "direction", directionGetter, READ_ONLY_SETTER);

  Nan::Set(target, Nan::New("Dimension").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

Dimension::Dimension(std::shared_ptr<GDALDimension> dimension)
  : Nan::ObjectWrap(), uid(0), this_(dimension), parent_ds(0) {
  LOG("Created dimension [%p]", dimension.get());
}

Dimension::Dimension() : Nan::ObjectWrap(), uid(0), this_(0), parent_ds(0) {
}

Dimension::~Dimension() {
  dispose();
}

void Dimension::dispose() {
  if (this_) {

    LOG("Disposing dimension [%p]", this_.get());

    object_store.dispose(uid);

    LOG("Disposed dimension [%p]", this_.get());
  }
};

/**
 * A representation of a group with access methods.
 *
 * @class Dimension
 */
NAN_METHOD(Dimension::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }

  if (info.Length() == 1 && info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value(V8_TYPE_TAG);
    Dimension *f = static_cast<Dimension *>(ptr);
    f->Wrap(info.This());

    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create dimension directly. Create with dataset instead.");
    return;
  }

  info.GetReturnValue().Set(info.This());
}

Local<Value> Dimension::New(std::shared_ptr<GDALDimension> raw, GDALDataset *parent_ds) {
  Nan::EscapableHandleScope scope;

  if (!raw) { return scope.Escape(Nan::Null()); }
  if (object_store.has(raw)) { return scope.Escape(object_store.get(raw)); }

  Dimension *wrapped = new Dimension(raw);

  Local<Object> ds;
  if (object_store.has(parent_ds)) {
    ds = object_store.get(parent_ds);
  } else {
    LOG("Dimension's parent dataset disappeared from cache (array = %p, dataset = %p)", raw.get(), parent_ds);
    Nan::ThrowError("Dimension's parent dataset disappeared from cache");
    return scope.Escape(Nan::Undefined());
  }

  Local<Value> ext = Nan::New<External>(wrapped);
  Local<Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(Dimension::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();

  Dataset *unwrapped_ds = Nan::ObjectWrap::Unwrap<Dataset>(ds);
  long parent_uid = unwrapped_ds->uid;

  wrapped->uid = object_store.add(raw, wrapped->persistent(), parent_uid);
  wrapped->parent_ds = parent_ds;
  wrapped->parent_uid = parent_uid;

  Nan::SetPrivate(obj, Nan::New("ds_").ToLocalChecked(), ds);

  return scope.Escape(obj);
}

NAN_METHOD(Dimension::toString) {
  info.GetReturnValue().Set(Nan::New("Dimension").ToLocalChecked());
}

/**
 * @readonly
 * @kind member
 * @name size
 * @instance
 * @memberof Dimension
 * @type {number}
 */
NODE_WRAPPED_GETTER_WITH_RESULT_LOCKED(Dimension, sizeGetter, Number, GetSize);

/**
 * @readonly
 * @kind member
 * @name description
 * @instance
 * @memberof Dimension
 * @type {string}
 */
NODE_WRAPPED_GETTER_WITH_STRING_LOCKED(Dimension, descriptionGetter, GetFullName);

/**
 * @readonly
 * @kind member
 * @name direction
 * @instance
 * @memberof Dimension
 * @type {string}
 */
NODE_WRAPPED_GETTER_WITH_STRING_LOCKED(Dimension, directionGetter, GetDirection);

/**
 * @readonly
 * @kind member
 * @name type
 * @instance
 * @memberof Dimension
 * @type {string}
 */
NODE_WRAPPED_GETTER_WITH_STRING_LOCKED(Dimension, typeGetter, GetType);

NAN_GETTER(Dimension::uidGetter) {
  Dimension *group = Nan::ObjectWrap::Unwrap<Dimension>(info.This());
  info.GetReturnValue().Set(Nan::New((int)group->uid));
}

#endif

} // namespace node_gdal
