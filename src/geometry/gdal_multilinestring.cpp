
#include "gdal_multilinestring.hpp"
#include "../collections/geometry_collection_children.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"
#include "gdal_geometrycollection.hpp"
#include "gdal_linestring.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference MultiLineString::constructor;

void MultiLineString::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(MultiLineString);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "MultiLineString",
    {
        METHOD(toString)
        METHOD(polygonize)
    });

  // lcons->Inherit() has no DefineClass equivalent
  node_gdal::Inherit(lcons, GeometryCollection::constructor.Value());

  target.Set("MultiLineString", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

/**
 * @constructor
 * @class MultiLineString
 * @extends GeometryCollection
 */

NAPI_METHOD(MultiLineString::toString) {
  return Napi::String::New(node_gdal::napi_env(), "MultiLineString");
}

/**
 * Converts it to a polygon.
 *
 * @method polygonize
 * @instance
 * @memberof MultiLineString
 * @return {Polygon}
 */
NAPI_METHOD(MultiLineString::polygonize) {

  MultiLineString *geom = node_gdal::UnwrapWrapped<MultiLineString>(info.This().As<Napi::Object>());

  return Geometry::New(geom->this_->Polygonize());
}

} // namespace node_gdal
