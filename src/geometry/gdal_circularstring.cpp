
#include "gdal_circularstring.hpp"
#include "gdal_linestring.hpp"
#include "../collections/linestring_points.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"
#include "gdal_simplecurve.hpp"
#include "gdal_point.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference CircularString::constructor;

void CircularString::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(CircularString);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "CircularString",
    {
        METHOD(toString)
    });

  // lcons->Inherit() has no DefineClass equivalent
  node_gdal::Inherit(lcons, SimpleCurve::constructor.Value());

  target.Set("CircularString", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

/**
 * Concrete representation of an arc.
 *
 * @example
 *
 * var CircularString = new gdal.CircularString();
 * CircularString.points.add(new gdal.Point(0,0));
 * CircularString.points.add(new gdal.Point(0,10));
 *
 * @constructor
 * @class CircularString
 * @extends SimpleCurve
 */

NAPI_METHOD(CircularString::toString) {
  return Napi::String::New(node_gdal::napi_env(), "CircularString");
}

} // namespace node_gdal
