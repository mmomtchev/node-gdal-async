#include "gdal_point.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"

#include <stdlib.h>

namespace node_gdal {

Nan::Persistent<FunctionTemplate> Point::constructor;

void Point::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(Point::New);
  lcons->Inherit(Nan::New(Geometry::constructor));
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("Point").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);

  // properties
  ATTR(lcons, "x", xGetter, xSetter);
  ATTR(lcons, "y", yGetter, ySetter);
  ATTR(lcons, "z", zGetter, zSetter);

  Nan::Set(target, Nan::New("Point").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
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
NAN_METHOD(Point::New) {
  Point *f;
  OGRPoint *geom;
  double x = 0, y = 0, z = 0;

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }

  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value(V8_TYPE_TAG);
    f = static_cast<Point *>(ptr);

  } else {
    NODE_ARG_DOUBLE_OPT(0, "x", x);
    NODE_ARG_DOUBLE_OPT(1, "y", y);
    NODE_ARG_DOUBLE_OPT(2, "z", z);

    if (info.Length() == 1) {
      Nan::ThrowError("Point constructor must be given 0, 2, or 3 arguments");
      return;
    }

    if (info.Length() == 3) {
      geom = new OGRPoint(x, y, z);
    } else {
      geom = new OGRPoint(x, y);
    }

    f = new Point(geom);
  }

  f->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Point::toString) {
  info.GetReturnValue().Set(Nan::New("Point").ToLocalChecked());
}

/**
 * @kind member
 * @name x
 * @instance
 * @memberof Point
 * @type {number}
 */
NAN_GETTER(Point::xGetter) {
  Point *geom = Nan::ObjectWrap::Unwrap<Point>(info.This());
  info.GetReturnValue().Set(Nan::New<Number>((geom->this_)->getX()));
}

NAN_SETTER(Point::xSetter) {
  Point *geom = Nan::ObjectWrap::Unwrap<Point>(info.This());

  if (!value->IsNumber()) {
    Nan::ThrowError("y must be a number");
    return;
  }
  double x = Nan::To<double>(value).ToChecked();

  ((OGRPoint *)geom->this_)->setX(x);
}

/**
 * @kind member
 * @name y
 * @instance
 * @memberof Point
 * @type {number}
 */
NAN_GETTER(Point::yGetter) {
  Point *geom = Nan::ObjectWrap::Unwrap<Point>(info.This());
  info.GetReturnValue().Set(Nan::New<Number>((geom->this_)->getY()));
}

NAN_SETTER(Point::ySetter) {
  Point *geom = Nan::ObjectWrap::Unwrap<Point>(info.This());

  if (!value->IsNumber()) {
    Nan::ThrowError("y must be a number");
    return;
  }
  double y = Nan::To<double>(value).ToChecked();

  ((OGRPoint *)geom->this_)->setY(y);
}

/**
 * @kind member
 * @name z
 * @instance
 * @memberof Point
 * @type {number}
 */
NAN_GETTER(Point::zGetter) {
  Point *geom = Nan::ObjectWrap::Unwrap<Point>(info.This());
  info.GetReturnValue().Set(Nan::New<Number>((geom->this_)->getZ()));
}

NAN_SETTER(Point::zSetter) {
  Point *geom = Nan::ObjectWrap::Unwrap<Point>(info.This());

  if (!value->IsNumber()) {
    Nan::ThrowError("z must be a number");
    return;
  }
  double z = Nan::To<double>(value).ToChecked();

  ((OGRPoint *)geom->this_)->setZ(z);
}

} // namespace node_gdal
