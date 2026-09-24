#ifndef __NODE_OGR_MULTIPOINT_H__
#define __NODE_OGR_MULTIPOINT_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_geometrycollectionbase.hpp"


namespace node_gdal {

class MultiPoint : public GeometryCollectionBase<MultiPoint, OGRMultiPoint> {

    public:
  static Napi::FunctionReference constructor;
  using GeometryCollectionBase<MultiPoint, OGRMultiPoint>::GeometryCollectionBase;

  static void Initialize(Napi::Object target);
  using GeometryCollectionBase<MultiPoint, OGRMultiPoint>::New;
  static NAPI_METHOD(toString);
};

} // namespace node_gdal
#endif
