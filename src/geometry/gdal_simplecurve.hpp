#ifndef __NODE_OGR_CURVE_H__
#define __NODE_OGR_CURVE_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_curvebase.hpp"
#include "../collections/linestring_points.hpp"


namespace node_gdal {

class SimpleCurve : public CurveBase<SimpleCurve, OGRSimpleCurve, LineStringPoints> {

    public:
  static Napi::FunctionReference constructor;
  // Abstract in JS
  SimpleCurve(const Napi::CallbackInfo &info);

  static void Initialize(Napi::Object target);
  static NAPI_METHOD(toString);
  static NAPI_METHOD(value);
  static NAPI_METHOD(getLength);
  static NAPI_METHOD(addSubLineString);

  static NAPI_GETTER(pointsGetter);
};

} // namespace node_gdal

#endif
