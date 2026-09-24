#ifndef __NODE_GDAL_POLYGON_RINGS_H__
#define __NODE_GDAL_POLYGON_RINGS_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

// Polygon.rings

namespace node_gdal {

class PolygonRings : public GDALObject<PolygonRings> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(Napi::Value geom);
  static NAPI_METHOD(toString);

  static NAPI_METHOD(get);
  static NAPI_METHOD(count);
  static NAPI_METHOD(add);
  static NAPI_METHOD(remove);

  PolygonRings(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~PolygonRings();
    private:
};

} // namespace node_gdal
#endif
