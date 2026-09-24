#ifndef __NODE_OGR_LINESTRING_H__
#define __NODE_OGR_LINESTRING_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_curvebase.hpp"
#include "../collections/linestring_points.hpp"


namespace node_gdal {

class LineString : public CurveBase<LineString, OGRLineString, LineStringPoints> {

    public:
  static Napi::FunctionReference constructor;
  using CurveBase<LineString, OGRLineString, LineStringPoints>::CurveBase;

  static void Initialize(Napi::Object target);
  using CurveBase<LineString, OGRLineString, LineStringPoints>::New;
  static NAPI_METHOD(toString);
};

} // namespace node_gdal
#endif
