#ifndef __NODE_OGR_MULTICURVE_H__
#define __NODE_OGR_MULTICURVE_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_geometrycollectionbase.hpp"


namespace node_gdal {

class MultiCurve : public GeometryCollectionBase<MultiCurve, OGRMultiCurve> {

    public:
  static Napi::FunctionReference constructor;
  using GeometryCollectionBase<MultiCurve, OGRMultiCurve>::GeometryCollectionBase;

  static void Initialize(Napi::Object target);
  using GeometryCollectionBase<MultiCurve, OGRMultiCurve>::New;
  static NAPI_METHOD(toString);
  static NAPI_METHOD(polygonize);
};

} // namespace node_gdal
#endif
