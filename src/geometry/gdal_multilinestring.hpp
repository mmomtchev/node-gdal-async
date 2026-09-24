#ifndef __NODE_OGR_MULTILINESTRING_H__
#define __NODE_OGR_MULTILINESTRING_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_geometrycollectionbase.hpp"


namespace node_gdal {

class MultiLineString : public GeometryCollectionBase<MultiLineString, OGRMultiLineString> {

    public:
  static Napi::FunctionReference constructor;
  using GeometryCollectionBase<MultiLineString, OGRMultiLineString>::GeometryCollectionBase;

  static void Initialize(Napi::Object target);
  using GeometryCollectionBase<MultiLineString, OGRMultiLineString>::New;
  static NAPI_METHOD(toString);
  static NAPI_METHOD(polygonize);
};

} // namespace node_gdal
#endif
