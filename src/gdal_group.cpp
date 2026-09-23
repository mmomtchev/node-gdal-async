#include "gdal_group.hpp"
#include "gdal_common.hpp"
#include "gdal_driver.hpp"
#include "collections/group_groups.hpp"
#include "collections/group_arrays.hpp"
#include "collections/group_dimensions.hpp"
#include "collections/group_attributes.hpp"
#include "geometry/gdal_geometry.hpp"
#include "gdal_layer.hpp"
#include "gdal_majorobject.hpp"
#include "gdal_rasterband.hpp"
#include "gdal_spatial_reference.hpp"

namespace node_gdal {

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

Nan::Persistent<FunctionTemplate> Group::constructor;

void Group::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(Group::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("Group").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);

  ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER);
  ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER);
  ATTR(lcons, "groups", groupsGetter, READ_ONLY_SETTER);
  ATTR(lcons, "arrays", arraysGetter, READ_ONLY_SETTER);
  ATTR(lcons, "dimensions", dimensionsGetter, READ_ONLY_SETTER);
  ATTR(lcons, "attributes", attributesGetter, READ_ONLY_SETTER);

  Nan::Set(target, Nan::New("Group").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

Group::Group(std::shared_ptr<GDALGroup> group) : Nan::ObjectWrap(), uid(0), this_(group), parent_ds(0) {
  LOG("Created group [%p]", group.get());
}

Group::Group() : Nan::ObjectWrap(), uid(0), this_(0), parent_ds(0) {
}

Group::~Group() {
  dispose();
}

void Group::dispose() {
  if (this_) {

    LOG("Disposing group [%p]", this_.get());

    object_store.dispose(uid);

    LOG("Disposed group [%p]", this_.get());
  }
};

/**
 * A representation of a group with access methods.
 *
 * @class Group
 */
NAN_METHOD(Group::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }

  if (info.Length() > 1 && info[0]->IsExternal() && info[1]->IsObject()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value(V8_TYPE_TAG);
    Group *f = static_cast<Group *>(ptr);
    f->Wrap(info.This());

    Local<Value> groups = GroupGroups::New(info.This(), info[1]);
    Nan::SetPrivate(info.This(), Nan::New("groups_").ToLocalChecked(), groups);
    Local<Value> arrays = GroupArrays::New(info.This(), info[1]);
    Nan::SetPrivate(info.This(), Nan::New("arrays_").ToLocalChecked(), arrays);
    Local<Value> dims = GroupDimensions::New(info.This(), info[1]);
    Nan::SetPrivate(info.This(), Nan::New("dims_").ToLocalChecked(), dims);
    Local<Value> attrs = GroupAttributes::New(info.This(), info[1]);
    Nan::SetPrivate(info.This(), Nan::New("attrs_").ToLocalChecked(), attrs);

    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create group directly. Create with dataset instead.");
    return;
  }

  info.GetReturnValue().Set(info.This());
}

Local<Value> Group::New(std::shared_ptr<GDALGroup> raw, GDALDataset *parent_ds) {
  Nan::EscapableHandleScope scope;

  if (object_store.has(parent_ds)) {
    Local<Object> ds = object_store.get(parent_ds);
    return Group::New(raw, ds);
  } else {
    LOG("Group's parent dataset disappeared from cache (group = %p, dataset = %p)", raw.get(), parent_ds);
    Nan::ThrowError("Group's parent dataset disappeared from cache");
    return scope.Escape(Nan::Undefined());
  }
}

Local<Value> Group::New(std::shared_ptr<GDALGroup> raw, Local<Object> parent_ds) {
  Nan::EscapableHandleScope scope;

  if (!raw) { return scope.Escape(Nan::Null()); }
  if (object_store.has(raw)) { return scope.Escape(object_store.get(raw)); }

  Group *wrapped = new Group(raw);

  long parent_group_uid = 0;
  Local<Object> parent;

  Local<Value> ext = Nan::New<External>(wrapped);
  v8::Local<v8::Value> argv[] = {ext, parent_ds};
  Local<Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(Group::constructor)).ToLocalChecked(), 2, argv).ToLocalChecked();

  Dataset *unwrapped_ds = Nan::ObjectWrap::Unwrap<Dataset>(parent_ds);
  long parent_uid = unwrapped_ds->uid;

  wrapped->uid = object_store.add(raw, wrapped->persistent(), parent_uid);
  wrapped->parent_ds = unwrapped_ds->get();
  wrapped->parent_uid = parent_uid;
  Nan::SetPrivate(obj, Nan::New("ds_").ToLocalChecked(), parent_ds);
  if (parent_group_uid != 0) Nan::SetPrivate(obj, Nan::New("parent_").ToLocalChecked(), parent);

  return scope.Escape(obj);
}

NAN_METHOD(Group::toString) {
  info.GetReturnValue().Set(Nan::New("Group").ToLocalChecked());
}

/**
 * @readonly
 * @kind member
 * @name description
 * @instance
 * @memberof Group
 * @type {string}
 */
NODE_WRAPPED_GETTER_WITH_STRING_LOCKED(Group, descriptionGetter, GetFullName);

/**
 * @readonly
 * @kind member
 * @name groups
 * @instance
 * @memberof Group
 * @type {GroupGroups}
 */
NAN_GETTER(Group::groupsGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("groups_").ToLocalChecked()).ToLocalChecked());
}

/**
 * @readonly
 * @kind member
 * @name arrays
 * @instance
 * @memberof Group
 * @type {GroupArrays}
 */
NAN_GETTER(Group::arraysGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("arrays_").ToLocalChecked()).ToLocalChecked());
}

/**
 * @readonly
 * @kind member
 * @name dimensions
 * @instance
 * @memberof Group
 * @type {GroupDimensions}
 */
NAN_GETTER(Group::dimensionsGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("dims_").ToLocalChecked()).ToLocalChecked());
}

/**
 * @readonly
 * @kind member
 * @name attributes
 * @instance
 * @memberof Group
 * @type {GroupAttributes}
 */
NAN_GETTER(Group::attributesGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("attrs_").ToLocalChecked()).ToLocalChecked());
}

NAN_GETTER(Group::uidGetter) {
  Group *group = Nan::ObjectWrap::Unwrap<Group>(info.This());
  info.GetReturnValue().Set(Nan::New((int)group->uid));
}

#endif

} // namespace node_gdal
