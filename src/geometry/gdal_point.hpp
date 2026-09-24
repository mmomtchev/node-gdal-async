#ifndef __NODE_OGR_POINT_H__
#define __NODE_OGR_POINT_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_geometrybase.hpp"


namespace node_gdal {

class Point : public GeometryBase<Point, OGRPoint> {
    public:
  static Napi::FunctionReference constructor;
  // Point(), Point(x, y) and Point(x, y, z) are constructible from JS, so this
  // class reads its own arguments instead of inheriting the base constructor
  Point(const Napi::CallbackInfo &info);

  static void Initialize(Napi::Object target);
  using GeometryBase<Point, OGRPoint>::New;
  static NAPI_METHOD(toString);

  static NAPI_GETTER(xGetter);
  static NAPI_GETTER(yGetter);
  static NAPI_GETTER(zGetter);
  static NAPI_SETTER(xSetter);
  static NAPI_SETTER(ySetter);
  static NAPI_SETTER(zSetter);
};

} // namespace node_gdal
#endif
