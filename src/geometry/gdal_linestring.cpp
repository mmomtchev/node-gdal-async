
#include "gdal_linestring.hpp"
#include "../collections/linestring_points.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"
#include "gdal_simplecurve.hpp"
#include "gdal_point.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference LineString::constructor;

void LineString::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(LineString);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "LineString",
    {
        METHOD(toString)
    });

  // lcons->Inherit() has no DefineClass equivalent
  node_gdal::Inherit(lcons, SimpleCurve::constructor.Value());

  target.Set("LineString", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

/**
 * Concrete representation of a multi-vertex line.
 *
 * @example
 *
 * var lineString = new gdal.LineString();
 * lineString.points.add(new gdal.Point(0,0));
 * lineString.points.add(new gdal.Point(0,10));
 *
 * @constructor
 * @class LineString
 * @extends SimpleCurve
 */

NAPI_METHOD(LineString::toString) {
  return Napi::String::New(node_gdal::napi_env(), "LineString");
}

} // namespace node_gdal
