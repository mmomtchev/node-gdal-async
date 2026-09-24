#ifndef __NODE_OGR_POLY_H__
#define __NODE_OGR_POLY_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_curvebase.hpp"
#include "../collections/polygon_rings.hpp"


namespace node_gdal {

class Polygon : public CurveBase<Polygon, OGRPolygon, PolygonRings> {
  friend CurveBase;

    public:
  static Napi::FunctionReference constructor;
  using CurveBase<Polygon, OGRPolygon, PolygonRings>::CurveBase;

  static void Initialize(Napi::Object target);
  using CurveBase<Polygon, OGRPolygon, PolygonRings>::New;
  static NAPI_METHOD(toString);
  static NAPI_METHOD(getArea);

  static NAPI_GETTER(ringsGetter);

    protected:
  static void SetPrivate(Napi::Object, Napi::Value);
};

} // namespace node_gdal
#endif
