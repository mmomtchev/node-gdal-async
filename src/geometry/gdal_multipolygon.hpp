#ifndef __NODE_OGR_MULTIPOLYGON_H__
#define __NODE_OGR_MULTIPOLYGON_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_geometrycollectionbase.hpp"


namespace node_gdal {

class MultiPolygon : public GeometryCollectionBase<MultiPolygon, OGRMultiPolygon> {

    public:
  static Napi::FunctionReference constructor;
  using GeometryCollectionBase<MultiPolygon, OGRMultiPolygon>::GeometryCollectionBase;

  static void Initialize(Napi::Object target);
  using GeometryCollectionBase<MultiPolygon, OGRMultiPolygon>::New;
  static NAPI_METHOD(toString);
  static NAPI_METHOD(unionCascaded);
  static NAPI_METHOD(getArea);
};

} // namespace node_gdal
#endif
