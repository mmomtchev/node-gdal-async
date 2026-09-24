#ifndef __NODE_GDAL_ATTRIBUTE_H__
#define __NODE_GDAL_ATTRIBUTE_H__

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

class Attribute : public GDALObject<Attribute> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(std::shared_ptr<GDALAttribute> group, GDALDataset *parent_ds);
  static NAPI_METHOD(toString);
  static NAPI_GETTER(typeGetter);
  static NAPI_GETTER(valueGetter);
  static NAPI_GETTER(uidGetter);

  Attribute(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~Attribute();
  inline std::shared_ptr<GDALAttribute> get() {
    return this_;
  }

  void dispose();
  long uid;
  long parent_uid;

  inline bool isAlive() {
    return this_ && object_store.isAlive(uid);
  }

    private:
  std::shared_ptr<GDALAttribute> this_;
  GDALDataset *parent_ds;
};

} // namespace node_gdal
#endif
#endif
