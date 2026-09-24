#ifndef __NODE_GDAL_DRIVERS_H__
#define __NODE_GDAL_DRIVERS_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

namespace node_gdal {

class GDALDrivers : public GDALObject<GDALDrivers> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New();
  static NAPI_METHOD(toString);

  static NAPI_METHOD(get);
  static NAPI_METHOD(getNames);
  static NAPI_METHOD(count);

  GDALDrivers(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~GDALDrivers();
    private:
};

} // namespace node_gdal
#endif
