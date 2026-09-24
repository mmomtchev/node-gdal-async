#ifndef __NODE_OGR_GEOMETRY_H__
#define __NODE_OGR_GEOMETRY_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "../async.hpp"
#include "gdal_geometrybase.hpp"


namespace node_gdal {

class Geometry : public GeometryBase<Geometry, OGRGeometry> {
    public:
  static Napi::FunctionReference constructor;
  // Not constructible from JS: use Geometry.fromWKT()/fromWKB() or a concrete type
  Geometry(const Napi::CallbackInfo &info);

  static void Initialize(Napi::Object target);
  using GeometryBase<Geometry, OGRGeometry>::New;
  static Napi::Value New(OGRGeometry *geom, bool owned);
  static NAPI_METHOD(toString);
  GDAL_ASYNCABLE_DECLARE(isEmpty);
  GDAL_ASYNCABLE_DECLARE(isValid);
  GDAL_ASYNCABLE_DECLARE(isSimple);
  GDAL_ASYNCABLE_DECLARE(isRing);
  static NAPI_METHOD(clone);
  GDAL_ASYNCABLE_DECLARE(empty);
  GDAL_ASYNCABLE_DECLARE(exportToKML);
  GDAL_ASYNCABLE_DECLARE(exportToGML);
  GDAL_ASYNCABLE_DECLARE(exportToJSON);
  GDAL_ASYNCABLE_DECLARE(exportToWKT);
  GDAL_ASYNCABLE_DECLARE(exportToWKB);
  GDAL_ASYNCABLE_DECLARE(closeRings);
  GDAL_ASYNCABLE_DECLARE(segmentize);
  GDAL_ASYNCABLE_DECLARE(intersects);
  GDAL_ASYNCABLE_DECLARE(equals);
  GDAL_ASYNCABLE_DECLARE(disjoint);
  GDAL_ASYNCABLE_DECLARE(touches);
  GDAL_ASYNCABLE_DECLARE(crosses);
  GDAL_ASYNCABLE_DECLARE(within);
  GDAL_ASYNCABLE_DECLARE(contains);
  GDAL_ASYNCABLE_DECLARE(overlaps);
  GDAL_ASYNCABLE_DECLARE(boundary);
  GDAL_ASYNCABLE_DECLARE(distance);
  GDAL_ASYNCABLE_DECLARE(convexHull);
  GDAL_ASYNCABLE_DECLARE(buffer);
  GDAL_ASYNCABLE_DECLARE(intersection);
  GDAL_ASYNCABLE_DECLARE(unionGeometry);
  GDAL_ASYNCABLE_DECLARE(difference);
  GDAL_ASYNCABLE_DECLARE(symDifference);
  GDAL_ASYNCABLE_DECLARE(centroid);
  GDAL_ASYNCABLE_DECLARE(simplify);
  GDAL_ASYNCABLE_DECLARE(simplifyPreserveTopology);
  GDAL_ASYNCABLE_DECLARE(polygonize);
  GDAL_ASYNCABLE_DECLARE(swapXY);
  static NAPI_METHOD(getNumGeometries);
  GDAL_ASYNCABLE_DECLARE(getEnvelope);
  GDAL_ASYNCABLE_DECLARE(getEnvelope3D);
  GDAL_ASYNCABLE_DECLARE(flattenTo2D);
  GDAL_ASYNCABLE_DECLARE(transform);
  GDAL_ASYNCABLE_DECLARE(transformTo);
#if GDAL_VERSION_MAJOR >= 3
  GDAL_ASYNCABLE_DECLARE(makeValid);
#endif

  // static constructor methods
  GDAL_ASYNCABLE_DECLARE(create);
  GDAL_ASYNCABLE_DECLARE(createFromWkt);
  GDAL_ASYNCABLE_DECLARE(createFromWkb);
  GDAL_ASYNCABLE_DECLARE(createFromGeoJson);
  GDAL_ASYNCABLE_DECLARE(createFromGeoJsonBuffer);
  static NAPI_METHOD(getName);
  static NAPI_METHOD(getConstructor);

  static NAPI_GETTER(srsGetter);
  static NAPI_GETTER(typeGetter);
  static NAPI_GETTER(nameGetter);
  static NAPI_GETTER(wkbSizeGetter);
  static NAPI_GETTER(dimensionGetter);
  static NAPI_GETTER(coordinateDimensionGetter);

  static NAPI_SETTER(srsSetter);
  static NAPI_SETTER(coordinateDimensionSetter);

  static OGRwkbGeometryType getGeometryType_fixed(OGRGeometry *geom);
  static Napi::Value getConstructor(OGRwkbGeometryType type);
};

} // namespace node_gdal
#endif
