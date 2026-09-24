#ifndef __NODE_GDAL_BAND_PIXELS_H__
#define __NODE_GDAL_BAND_PIXELS_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

#include "../gdal_rasterband.hpp"
#include "../async.hpp"

namespace node_gdal {

class RasterBandPixels : public GDALObject<RasterBandPixels> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(Napi::Value band_obj);
  static NAPI_METHOD(toString);

  GDAL_ASYNCABLE_DECLARE(get);
  GDAL_ASYNCABLE_DECLARE(set);
  GDAL_ASYNCABLE_DECLARE(read);
  GDAL_ASYNCABLE_DECLARE(write);
  GDAL_ASYNCABLE_DECLARE(readBlock);
  GDAL_ASYNCABLE_DECLARE(writeBlock);
  GDAL_ASYNCABLE_DECLARE(clampBlock);

  static NAPI_GETTER(bandGetter);

  static RasterBand *parent(const Napi::CallbackInfo &info);

  RasterBandPixels(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~RasterBandPixels();
    private:
};

} // namespace node_gdal
#endif
