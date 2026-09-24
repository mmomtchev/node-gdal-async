#ifndef __NODE_OGR_CIRCULARSTRING_H__
#define __NODE_OGR_CIRCULARSTRING_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_curvebase.hpp"
#include "../collections/linestring_points.hpp"


namespace node_gdal {

class CircularString : public CurveBase<CircularString, OGRCircularString, LineStringPoints> {

    public:
  static Napi::FunctionReference constructor;
  using CurveBase<CircularString, OGRCircularString, LineStringPoints>::CurveBase;

  static void Initialize(Napi::Object target);
  using CurveBase<CircularString, OGRCircularString, LineStringPoints>::New;
  static NAPI_METHOD(toString);
};

} // namespace node_gdal
#endif
