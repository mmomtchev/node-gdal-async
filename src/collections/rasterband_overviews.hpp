#ifndef __NODE_GDAL_BAND_OVERVIEWS_H__
#define __NODE_GDAL_BAND_OVERVIEWS_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

#include "../async.hpp"

namespace node_gdal {

class RasterBandOverviews : public GDALObject<RasterBandOverviews> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(Napi::Value band_obj);
  static NAPI_METHOD(toString);

  GDAL_ASYNCABLE_DECLARE(get);
  GDAL_ASYNCABLE_DECLARE(getBySampleCount);
  GDAL_ASYNCABLE_DECLARE(count);

  RasterBandOverviews(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~RasterBandOverviews();
    private:
};

} // namespace node_gdal
#endif
