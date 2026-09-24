
#include "gdal_multicurve.hpp"
#include "../collections/geometry_collection_children.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"
#include "gdal_geometrycollection.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference MultiCurve::constructor;

void MultiCurve::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(MultiCurve);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "MultiCurve",
    {
        METHOD(toString)
        METHOD(polygonize)
    });

  // lcons->Inherit() has no DefineClass equivalent
  node_gdal::Inherit(lcons, GeometryCollection::constructor.Value());

  target.Set("MultiCurve", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

/**
 * @constructor
 * @class MultiCurve
 * @extends GeometryCollection
 */

NAPI_METHOD(MultiCurve::toString) {
  return Napi::String::New(node_gdal::napi_env(), "MultiCurve");
}

/**
 * Converts it to a polygon.
 *
 * @method polygonize
 * @instance
 * @memberof MultiCurve
 * @return {Polygon}
 */
NAPI_METHOD(MultiCurve::polygonize) {

  MultiCurve *geom = node_gdal::UnwrapWrapped<MultiCurve>(info.This().As<Napi::Object>());

  return Geometry::New(geom->this_->Polygonize());
}

} // namespace node_gdal
