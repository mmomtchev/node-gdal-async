#include "linestring_points.hpp"
#include "../gdal_common.hpp"
#include "../geometry/gdal_geometry.hpp"
#include "../geometry/gdal_linestring.hpp"
#include "../geometry/gdal_point.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> LineStringPoints::constructor;

void LineStringPoints::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(LineStringPoints::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("LineStringPoints").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan::SetPrototypeMethod(lcons, "count", count);
  Nan::SetPrototypeMethod(lcons, "get", get);
  Nan::SetPrototypeMethod(lcons, "set", set);
  Nan::SetPrototypeMethod(lcons, "add", add);
  Nan::SetPrototypeMethod(lcons, "reverse", reverse);
  Nan::SetPrototypeMethod(lcons, "resize", resize);

  Nan::Set(target, Nan::New("LineStringPoints").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

LineStringPoints::LineStringPoints() : Nan::ObjectWrap() {
}

LineStringPoints::~LineStringPoints() {
}

/**
 * An encapsulation of a {@link LineString}'s points.
 *
 * @class LineStringPoints
 */
NAN_METHOD(LineStringPoints::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }
  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    LineStringPoints *geom = static_cast<LineStringPoints *>(ptr);
    geom->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create LineStringPoints directly");
    return;
  }
}

Local<Value> LineStringPoints::New(Local<Value> geom) {
  Nan::EscapableHandleScope scope;

  LineStringPoints *wrapped = new LineStringPoints();

  v8::Local<v8::Value> ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(LineStringPoints::constructor)).ToLocalChecked(), 1, &ext)
      .ToLocalChecked();
  Nan::SetPrivate(obj, Nan::New("parent_").ToLocalChecked(), geom);

  return scope.Escape(obj);
}

NAN_METHOD(LineStringPoints::toString) {
  info.GetReturnValue().Set(Nan::New("LineStringPoints").ToLocalChecked());
}

/**
 * Returns the number of points that are part of the line string.
 *
 * @method count
 * @instance
 * @memberof LineStringPoints
 * @return {number}
 */
NAN_METHOD(LineStringPoints::count) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  LineString *geom = Nan::ObjectWrap::Unwrap<LineString>(parent);

  info.GetReturnValue().Set(Nan::New<Integer>(geom->get()->getNumPoints()));
}

/**
 * Reverses the order of all the points.
 *
 * @method reverse
 * @instance
 * @memberof LineStringPoints
 */
NAN_METHOD(LineStringPoints::reverse) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  LineString *geom = Nan::ObjectWrap::Unwrap<LineString>(parent);

  geom->get()->reversePoints();

  return;
}

/**
 * Adjusts the number of points that make up the line string.
 *
 * @method resize
 * @instance
 * @memberof LineStringPoints
 * @param {number} count
 */
NAN_METHOD(LineStringPoints::resize) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  LineString *geom = Nan::ObjectWrap::Unwrap<LineString>(parent);

  int count;
  NODE_ARG_INT(0, "point count", count)
  geom->get()->setNumPoints(count);

  return;
}

/**
 * Returns the point at the specified index.
 *
 * @method get
 * @instance
 * @memberof LineStringPoints
 * @param {number} index 0-based index
 * @throws {Error}
 * @return {Point}
 */
NAN_METHOD(LineStringPoints::get) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  LineString *geom = Nan::ObjectWrap::Unwrap<LineString>(parent);

  OGRPoint pt;
  int i;

  NODE_ARG_INT(0, "index", i);
  if (i < 0 || i >= geom->get()->getNumPoints()) {
    Nan::ThrowError("Invalid point requested");
    return;
  }

  geom->get()->getPoint(i, &pt);

  // New will copy the point with GDAL clone()
  info.GetReturnValue().Set(Point::New(&pt, false));
}

/**
 * Sets the point at the specified index.
 *
 * @example
 *
 * lineString.points.set(0, new gdal.Point(1, 2));
 *
 * @method set
 * @instance
 * @memberof LineStringPoints
 * @throws {Error}
 * @param {number} index 0-based index
 * @param {Point|xyz} point
 */

/**
 * @method set
 * @instance
 * @memberof LineStringPoints
 * @throws {Error}
 * @param {number} index 0-based index
 * @param {number} x
 * @param {number} y
 * @param {number} [z]
 */
NAN_METHOD(LineStringPoints::set) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  LineString *geom = Nan::ObjectWrap::Unwrap<LineString>(parent);

  int i;
  NODE_ARG_INT(0, "index", i);
  if (i < 0 || i >= geom->get()->getNumPoints()) {
    Nan::ThrowError("Point index out of range");
    return;
  }

  int n = info.Length() - 1;

  if (n == 0) {
    Nan::ThrowError("Point must be given");
    return;
  } else if (n == 1) {
    if (!info[1]->IsObject()) {
      Nan::ThrowError("Point or object expected for second argument");
      return;
    }
    if (IS_WRAPPED(info[1], Point)) {
      // set from Point object
      Point *pt = Nan::ObjectWrap::Unwrap<Point>(info[1].As<Object>());
      geom->get()->setPoint(i, pt->get());
    } else {
      Local<Object> obj = info[1].As<Object>();
      // set from object {x: 0, y: 5}
      double x, y;
      NODE_DOUBLE_FROM_OBJ(obj, "x", x);
      NODE_DOUBLE_FROM_OBJ(obj, "y", y);

      Local<String> z_prop_name = Nan::New("z").ToLocalChecked();
      if (Nan::HasOwnProperty(obj, z_prop_name).FromMaybe(false)) {
        Local<Value> z_val = Nan::Get(obj, z_prop_name).ToLocalChecked();
        if (!z_val->IsNumber()) {
          Nan::ThrowError("z property must be number");
          return;
        }
        geom->get()->setPoint(i, x, y, Nan::To<double>(z_val).ToChecked());
      } else {
        geom->get()->setPoint(i, x, y);
      }
    }
  } else {
    // set x, y, z from numeric arguments
    if (!info[1]->IsNumber()) {
      Nan::ThrowError("Number expected for second argument");
      return;
    }
    if (!info[2]->IsNumber()) {
      Nan::ThrowError("Number expected for third argument");
      return;
    }
    if (n == 2) {
      geom->get()->setPoint(i, Nan::To<double>(info[1]).ToChecked(), Nan::To<double>(info[2]).ToChecked());
    } else {
      if (!info[3]->IsNumber()) {
        Nan::ThrowError("Number expected for fourth argument");
        return;
      }

      geom->get()->setPoint(
        i,
        Nan::To<double>(info[1]).ToChecked(),
        Nan::To<double>(info[2]).ToChecked(),
        Nan::To<double>(info[3]).ToChecked());
    }
  }

  return;
}

/**
 * Adds point(s) to the line string. Also accepts any object with an x and y
 * property.
 *
 * @example
 *
 * lineString.points.add(new gdal.Point(1, 2));
 * lineString.points.add([
 *     new gdal.Point(1, 2)
 *     new gdal.Point(3, 4)
 * ]);
 *
 * @method add
 * @instance
 * @memberof LineStringPoints
 * @throws {Error}
 * @param {Point|xyz|(Point|xyz)[]} points
 */

/**
 *
 * @method add
 * @instance
 * @memberof LineStringPoints
 * @throws {Error}
 * @param {number} x
 * @param {number} y
 * @param {number} [z]
 */
NAN_METHOD(LineStringPoints::add) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  LineString *geom = Nan::ObjectWrap::Unwrap<LineString>(parent);

  int n = info.Length();

  if (n == 0) {
    Nan::ThrowError("Point must be given");
    return;
  } else if (n == 1) {
    if (!info[0]->IsObject()) {
      Nan::ThrowError("Point, object, or array of points expected");
      return;
    }
    if (IS_WRAPPED(info[0], Point)) {
      // set from Point object
      Point *pt = Nan::ObjectWrap::Unwrap<Point>(info[0].As<Object>());
      geom->get()->addPoint(pt->get());
    } else if (info[0]->IsArray()) {
      // set from array of points
      Local<Array> array = info[0].As<Array>();
      int length = array->Length();
      for (int i = 0; i < length; i++) {
        Local<Value> element = Nan::Get(array, i).ToLocalChecked();
        if (!element->IsObject()) {
          Nan::ThrowError("All points must be Point objects or objects");
          return;
        }
        Local<Object> element_obj = element.As<Object>();
        if (IS_WRAPPED(element_obj, Point)) {
          // set from Point object
          Point *pt = Nan::ObjectWrap::Unwrap<Point>(element_obj);
          geom->get()->addPoint(pt->get());
        } else {
          // set from object {x: 0, y: 5}
          double x, y;
          NODE_DOUBLE_FROM_OBJ(element_obj, "x", x);
          NODE_DOUBLE_FROM_OBJ(element_obj, "y", y);

          Local<String> z_prop_name = Nan::New("z").ToLocalChecked();
          if (Nan::HasOwnProperty(element_obj, z_prop_name).FromMaybe(false)) {
            Local<Value> z_val = Nan::Get(element_obj, z_prop_name).ToLocalChecked();
            if (!z_val->IsNumber()) {
              Nan::ThrowError("z property must be number");
              return;
            }
            geom->get()->addPoint(x, y, Nan::To<double>(z_val).ToChecked());
          } else {
            geom->get()->addPoint(x, y);
          }
        }
      }
    } else {
      // set from object {x: 0, y: 5}
      Local<Object> obj = info[0].As<Object>();
      double x, y;
      NODE_DOUBLE_FROM_OBJ(obj, "x", x);
      NODE_DOUBLE_FROM_OBJ(obj, "y", y);

      Local<String> z_prop_name = Nan::New("z").ToLocalChecked();
      if (Nan::HasOwnProperty(obj, z_prop_name).FromMaybe(false)) {
        Local<Value> z_val = Nan::Get(obj, z_prop_name).ToLocalChecked();
        if (!z_val->IsNumber()) {
          Nan::ThrowError("z property must be number");
          return;
        }
        geom->get()->addPoint(x, y, Nan::To<double>(z_val).ToChecked());
      } else {
        geom->get()->addPoint(x, y);
      }
    }
  } else {
    // set x, y, z from numeric arguments
    if (!info[0]->IsNumber()) {
      Nan::ThrowError("Number expected for first argument");
      return;
    }
    if (!info[1]->IsNumber()) {
      Nan::ThrowError("Number expected for second argument");
      return;
    }
    if (n == 2) {
      geom->get()->addPoint(Nan::To<double>(info[0]).ToChecked(), Nan::To<double>(info[1]).ToChecked());
    } else {
      if (!info[2]->IsNumber()) {
        Nan::ThrowError("Number expected for third argument");
        return;
      }

      geom->get()->addPoint(
        Nan::To<double>(info[0]).ToChecked(),
        Nan::To<double>(info[1]).ToChecked(),
        Nan::To<double>(info[2]).ToChecked());
    }
  }

  return;
}

} // namespace node_gdal
