
#include "gdal_multipoint.hpp"
#include "../collections/geometry_collection_children.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"
#include "gdal_geometrycollection.hpp"
#include "gdal_point.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference MultiPoint::constructor;

void MultiPoint::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(MultiPoint);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "MultiPoint",
    {
        METHOD(toString)
    });

  // lcons->Inherit() has no DefineClass equivalent
  node_gdal::Inherit(lcons, GeometryCollection::constructor.Value());

  target.Set("MultiPoint", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

/**
 * @constructor
 * @class MultiPoint
 * @extends GeometryCollection
 */

NAPI_METHOD(MultiPoint::toString) {
  return Napi::String::New(node_gdal::napi_env(), "MultiPoint");
}

} // namespace node_gdal
