#ifndef __NODE_GDAL_GROUP_H__
#define __NODE_GDAL_GROUP_H__

// node

// nan
#include "gdal_common.hpp"

// gdal
#include <gdal_priv.h>

// ogr
#include <ogrsf_frmts.h>

#include "async.hpp"

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

namespace node_gdal {

class Group : public GDALObject<Group> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(std::shared_ptr<GDALGroup> group, Napi::Object parent_ds);
  static Napi::Value New(std::shared_ptr<GDALGroup> group, GDALDataset *parent_ds);
  static NAPI_METHOD(toString);
  static NAPI_GETTER(descriptionGetter);
  static NAPI_GETTER(groupsGetter);
  static NAPI_GETTER(arraysGetter);
  static NAPI_GETTER(dimensionsGetter);
  static NAPI_GETTER(attributesGetter);
  static NAPI_GETTER(uidGetter);

  Group(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~Group();
  inline std::shared_ptr<GDALGroup> get() {
    return this_;
  }

  void dispose();
  long uid;
  long parent_uid;

  inline bool isAlive() {
    return this_ && object_store.isAlive(uid);
  }

    private:
  std::shared_ptr<GDALGroup> this_;
  GDALDataset *parent_ds;
};

} // namespace node_gdal
#endif
#endif
