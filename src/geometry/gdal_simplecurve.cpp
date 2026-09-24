
#include "gdal_simplecurve.hpp"
#include "../collections/linestring_points.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"
#include "gdal_point.hpp"
#include "gdal_linestring.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference SimpleCurve::constructor;

void SimpleCurve::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(SimpleCurve);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "SimpleCurve",
    {
        METHOD(toString)
        METHOD(getLength)
        METHOD(value)
        METHOD(addSubLineString)
        ATTR(lcons, "points", pointsGetter, READ_ONLY_SETTER)
    });

  // lcons->Inherit() has no DefineClass equivalent
  node_gdal::Inherit(lcons, Geometry::constructor.Value());

  target.Set("SimpleCurve", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

/**
 * Abstract class representing all SimpleCurves.
 *
 * @constructor
 * @class SimpleCurve
 * @extends Geometry
 */
SimpleCurve::SimpleCurve(const Napi::CallbackInfo &info)
  : CurveBase<SimpleCurve, OGRSimpleCurve, LineStringPoints>(info) {
  Napi::Error::New(info.Env(), "SimpleCurve is an abstract class and cannot be instantiated")
    .ThrowAsJavaScriptException();
}


NAPI_METHOD(SimpleCurve::toString) {
  return Napi::String::New(node_gdal::napi_env(), "SimpleCurve");
}

/**
 * Returns the point at the specified distance along the SimpleCurve.
 *
 * @method value
 * @instance
 * @memberof SimpleCurve
 * @param {number} distance
 * @return {Point}
 */
NAPI_METHOD(SimpleCurve::value) {

  SimpleCurve *geom = node_gdal::UnwrapWrapped<SimpleCurve>(info.This().As<Napi::Object>());

  OGRPoint *pt = new OGRPoint();
  double dist;

  NODE_ARG_DOUBLE(0, "distance", dist);

  geom->this_->Value(dist, pt);

  return Point::New(pt);
}

/**
 * Compute the length of a multiSimpleCurve.
 *
 * @method getLength
 * @instance
 * @memberof SimpleCurve
 * @return {number}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(SimpleCurve, getLength, Number, get_Length);

/**
 * The points that make up the SimpleCurve geometry.
 *
 * @kind member
 * @name points
 * @instance
 * @memberof Geometry
 * @type {LineStringPoints}
 */
NAPI_GETTER(SimpleCurve::pointsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "points_");
}

/**
 * Add a segment of another LineString to this SimpleCurve subtype.
 *
 * Adds the request range of vertices to the end of this compound curve in an
 * efficient manner. If the start index is larger than the end index then the
 * vertices will be reversed as they are copied.
 *
 * @method addSubLineString
 * @instance
 * @memberof SimpleCurve
 * @param {LineString} LineString to be added
 * @param {number} [start=0] the first vertex to copy, defaults to 0 to start with
 * the first vertex in the other LineString
 * @param {number} [end=-1] the last vertex to copy, defaults to -1 indicating the
 * last vertex of the other LineString
 * @return {void}
 */
NAPI_METHOD(SimpleCurve::addSubLineString) {

  SimpleCurve *geom = node_gdal::UnwrapWrapped<SimpleCurve>(info.This().As<Napi::Object>());
  LineString *other;
  int start = 0;
  int end = -1;

  NODE_ARG_WRAPPED(0, "line", LineString, other);
  NODE_ARG_INT_OPT(1, "start", start);
  NODE_ARG_INT_OPT(2, "end", end);

  int n = other->get()->getNumPoints();

  if (start < 0 || end < -1 || start >= n || end >= n) {
    Napi::RangeError::New(node_gdal::napi_env(), "Invalid start or end index for LineString").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  geom->this_->addSubLineString(other->get(), start, end);

  UPDATE_AMOUNT_OF_GEOMETRY_MEMORY(geom);

  return node_gdal::napi_env().Undefined();
}

} // namespace node_gdal
