#ifndef __NODE_GDAL_DRIVER_H__
#define __NODE_GDAL_DRIVER_H__

// gdal
#include <gdal_priv.h>

// ogr
#include <ogrsf_frmts.h>

#include "async.hpp"
#include "gdal_common.hpp"

namespace node_gdal {

// > GDAL 2.0 : a wrapper for GDALDriver
// < GDAL 2.0 : a wrapper for either a GDALDriver or OGRSFDriver that behaves
// like a 2.0 Driver
//
class Driver : public GDALObject<Driver> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(GDALDriver *driver);
  static NAPI_METHOD(toString);
  GDAL_ASYNCABLE_DECLARE(open);
  GDAL_ASYNCABLE_DECLARE(create);
  GDAL_ASYNCABLE_DECLARE(createCopy);
  static NAPI_METHOD(deleteDataset);
  static NAPI_METHOD(rename);
  GDAL_ASYNCABLE_DECLARE(copyFiles);
  static NAPI_METHOD(getMetadata);

  static NAPI_GETTER(descriptionGetter);

  Driver(const Napi::CallbackInfo &info);
  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~Driver();

  inline GDALDriver *getGDALDriver() {
    return this_gdaldriver;
  }
  void dispose();
  long uid;

  inline bool isAlive() {
    return this_gdaldriver;
  }

    private:
  GDALDriver *this_gdaldriver;
};

} // namespace node_gdal
#endif
