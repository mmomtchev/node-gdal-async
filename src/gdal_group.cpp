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

Napi::FunctionReference Group::constructor;

void Group::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(Group);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "Group",
    {
        METHOD(toString)
        ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER)
        ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER)
        ATTR(lcons, "groups", groupsGetter, READ_ONLY_SETTER)
        ATTR(lcons, "arrays", arraysGetter, READ_ONLY_SETTER)
        ATTR(lcons, "dimensions", dimensionsGetter, READ_ONLY_SETTER)
        ATTR(lcons, "attributes", attributesGetter, READ_ONLY_SETTER)
    });

  target.Set("Group", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

Group::Group(const Napi::CallbackInfo &info) : GDALObject<Group>(info), uid(0), this_(nullptr), parent_ds(0) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = node_gdal::ImportShared<GDALGroup>(info);
    LOG("Created group [%p]", this_.get());
  } else {
    Napi::Error::New(info.Env(), "Cannot create Group directly").ThrowAsJavaScriptException();
    return;
  }

  // the parent dataset travels as the second argument, for the sub-collections
  Napi::Value parent_ds = info.Length() > 1 ? info[1] : info.Env().Undefined();
  GDAL_SET_PRIVATE(info.This(), "groups_", GroupGroups::New(info.This(), parent_ds));
  GDAL_SET_PRIVATE(info.This(), "arrays_", GroupArrays::New(info.This(), parent_ds));
  GDAL_SET_PRIVATE(info.This(), "dims_", GroupDimensions::New(info.This(), parent_ds));
  GDAL_SET_PRIVATE(info.This(), "attrs_", GroupAttributes::New(info.This(), parent_ds));
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

Napi::Value Group::New(std::shared_ptr<GDALGroup> raw, GDALDataset *parent_ds) {

  if (object_store.has(parent_ds)) {
    Napi::Object ds = object_store.get(parent_ds);
    return Group::New(raw, ds);
  } else {
    LOG("Group's parent dataset disappeared from cache (group = %p, dataset = %p)", raw.get(), parent_ds);
    Napi::Error::New(node_gdal::napi_env(), "Group's parent dataset disappeared from cache").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
}

Napi::Value Group::New(std::shared_ptr<GDALGroup> raw, Napi::Object parent_ds) {

  if (!raw) { return node_gdal::napi_env().Null(); }
  if (object_store.has(raw)) { return object_store.get(raw); }

  std::vector<napi_value> args = {
    Napi::External<void>::New(node_gdal::napi_env(), node_gdal::ExportShared(raw)), parent_ds};
  Napi::Object obj = Group::constructor.Value().New(args);
  Group *wrapped = node_gdal::UnwrapWrapped<Group>(obj);

  long parent_group_uid = 0;
  Napi::Object parent;

  Dataset *unwrapped_ds = node_gdal::UnwrapWrapped<Dataset>(parent_ds);
  long parent_uid = unwrapped_ds->uid;

  wrapped->uid = object_store.add(raw, *wrapped, parent_uid);
  wrapped->parent_ds = unwrapped_ds->get();
  wrapped->parent_uid = parent_uid;
  if (parent_group_uid != 0) GDAL_SET_PRIVATE(obj, "parent_", parent);

  return obj;
}

NAPI_METHOD(Group::toString) {
  return Napi::String::New(node_gdal::napi_env(), "Group");
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
NAPI_GETTER(Group::groupsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "groups_");
}

/**
 * @readonly
 * @kind member
 * @name arrays
 * @instance
 * @memberof Group
 * @type {GroupArrays}
 */
NAPI_GETTER(Group::arraysGetter) {
  return GDAL_GET_PRIVATE(info.This(), "arrays_");
}

/**
 * @readonly
 * @kind member
 * @name dimensions
 * @instance
 * @memberof Group
 * @type {GroupDimensions}
 */
NAPI_GETTER(Group::dimensionsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "dims_");
}

/**
 * @readonly
 * @kind member
 * @name attributes
 * @instance
 * @memberof Group
 * @type {GroupAttributes}
 */
NAPI_GETTER(Group::attributesGetter) {
  return GDAL_GET_PRIVATE(info.This(), "attrs_");
}

NAPI_GETTER(Group::uidGetter) {
  Group *group = node_gdal::UnwrapWrapped<Group>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env(), (int)group->uid);
}

#endif

} // namespace node_gdal
