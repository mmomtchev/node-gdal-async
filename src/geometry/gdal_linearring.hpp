#ifndef __NODE_OGR_LINEARRING_H__
#define __NODE_OGR_LINEARRING_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_curvebase.hpp"
#include "../collections/linestring_points.hpp"


namespace node_gdal {

class LinearRing : public CurveBase<LinearRing, OGRLinearRing, LineStringPoints> {

    public:
  static Napi::FunctionReference constructor;
  using CurveBase<LinearRing, OGRLinearRing, LineStringPoints>::CurveBase;

  static void Initialize(Napi::Object target);
  using CurveBase<LinearRing, OGRLinearRing, LineStringPoints>::New;
  static Napi::Value New(OGRLinearRing *geom, bool owned);
  static NAPI_METHOD(toString);
  static NAPI_METHOD(getArea);
  static NAPI_METHOD(addSubLineString);
};

} // namespace node_gdal
#endif
