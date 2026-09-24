#include "gdal_point.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference Point::constructor;

void Point::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(Point);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "Point",
    {
        METHOD(toString)
        ATTR(lcons, "x", xGetter, xSetter)
        ATTR(lcons, "y", yGetter, ySetter)
        ATTR(lcons, "z", zGetter, zSetter)
    });

  // lcons->Inherit() has no DefineClass equivalent
  node_gdal::Inherit(lcons, Geometry::constructor.Value());

  // properties

  target.Set("Point", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

/**
 * Point class.
 *
 * @constructor
 * @class Point
 * @extends Geometry
 * @param {number} x
 * @param {number} y
 * @param {number} [z]
 */
Point::Point(const Napi::CallbackInfo &info) : GeometryBase<Point, OGRPoint>(info) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = static_cast<OGRPoint *>(info[0].As<Napi::External<void>>().Data());
    return;
  }

  if (info.Length() == 1) {
    Napi::Error::New(info.Env(), "Point constructor must be given 0, 2, or 3 arguments").ThrowAsJavaScriptException();
    return;
  }

  // GeometryBase<Point, OGRPoint>() has already created an empty OGRPoint,
  // which is (0, 0) - apply the arguments to it
  if (info.Length() > 0) this_->setX(info[0].As<Napi::Number>().DoubleValue());
  if (info.Length() > 1) this_->setY(info[1].As<Napi::Number>().DoubleValue());
  if (info.Length() > 2) this_->setZ(info[2].As<Napi::Number>().DoubleValue());
}


NAPI_METHOD(Point::toString) {
  return Napi::String::New(node_gdal::napi_env(), "Point");
}

/**
 * @kind member
 * @name x
 * @instance
 * @memberof Point
 * @type {number}
 */
NAPI_GETTER(Point::xGetter) {
  Point *geom = node_gdal::UnwrapWrapped<Point>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env(), (geom->this_)->getX());
}

NAPI_SETTER(Point::xSetter) {
  Point *geom = node_gdal::UnwrapWrapped<Point>(info.This().As<Napi::Object>());

  if (!value.IsNumber()) {
    Napi::Error::New(node_gdal::napi_env(), "y must be a number").ThrowAsJavaScriptException();
    return;
  }
  double x = value.As<Napi::Number>().DoubleValue();

  ((OGRPoint *)geom->this_)->setX(x);
}

/**
 * @kind member
 * @name y
 * @instance
 * @memberof Point
 * @type {number}
 */
NAPI_GETTER(Point::yGetter) {
  Point *geom = node_gdal::UnwrapWrapped<Point>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env(), (geom->this_)->getY());
}

NAPI_SETTER(Point::ySetter) {
  Point *geom = node_gdal::UnwrapWrapped<Point>(info.This().As<Napi::Object>());

  if (!value.IsNumber()) {
    Napi::Error::New(node_gdal::napi_env(), "y must be a number").ThrowAsJavaScriptException();
    return;
  }
  double y = value.As<Napi::Number>().DoubleValue();

  ((OGRPoint *)geom->this_)->setY(y);
}

/**
 * @kind member
 * @name z
 * @instance
 * @memberof Point
 * @type {number}
 */
NAPI_GETTER(Point::zGetter) {
  Point *geom = node_gdal::UnwrapWrapped<Point>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env(), (geom->this_)->getZ());
}

NAPI_SETTER(Point::zSetter) {
  Point *geom = node_gdal::UnwrapWrapped<Point>(info.This().As<Napi::Object>());

  if (!value.IsNumber()) {
    Napi::Error::New(node_gdal::napi_env(), "z must be a number").ThrowAsJavaScriptException();
    return;
  }
  double z = value.As<Napi::Number>().DoubleValue();

  ((OGRPoint *)geom->this_)->setZ(z);
}

} // namespace node_gdal
