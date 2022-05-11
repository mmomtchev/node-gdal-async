#include "compound_curves.hpp"
#include "../gdal_common.hpp"
#include "../geometry/gdal_geometry.hpp"
#include "../geometry/gdal_linearring.hpp"
#include "../geometry/gdal_simplecurve.hpp"
#include "../geometry/gdal_compoundcurve.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> CompoundCurveCurves::constructor;

void CompoundCurveCurves::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(CompoundCurveCurves::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("CompoundCurveCurves").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan::SetPrototypeMethod(lcons, "count", count);
  Nan::SetPrototypeMethod(lcons, "get", get);
  Nan::SetPrototypeMethod(lcons, "add", add);

  Nan::Set(target, Nan::New("CompoundCurveCurves").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

CompoundCurveCurves::CompoundCurveCurves() : Nan::ObjectWrap() {
}

CompoundCurveCurves::~CompoundCurveCurves() {
}

/**
 * A collection of connected curves, used by {@link CompoundCurve}
 *
 * @class CompoundCurveCurves
 */
NAN_METHOD(CompoundCurveCurves::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }
  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    CompoundCurveCurves *geom = static_cast<CompoundCurveCurves *>(ptr);
    geom->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create CompoundCurveCurves directly");
    return;
  }
}

Local<Value> CompoundCurveCurves::New(Local<Value> geom) {
  Nan::EscapableHandleScope scope;

  CompoundCurveCurves *wrapped = new CompoundCurveCurves();

  v8::Local<v8::Value> ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(CompoundCurveCurves::constructor)).ToLocalChecked(), 1, &ext)
      .ToLocalChecked();
  Nan::SetPrivate(obj, Nan::New("parent_").ToLocalChecked(), geom);

  return scope.Escape(obj);
}

NAN_METHOD(CompoundCurveCurves::toString) {
  info.GetReturnValue().Set(Nan::New("CompoundCurveCurves").ToLocalChecked());
}

/**
 * Returns the number of curves that exist in the collection.
 *
 * @method count
 * @instance
 * @memberof CompoundCurveCurves
 * @return {number}
 */
NAN_METHOD(CompoundCurveCurves::count) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  CompoundCurve *geom = Nan::ObjectWrap::Unwrap<CompoundCurve>(parent);

  info.GetReturnValue().Set(Nan::New<Integer>(geom->get()->getNumCurves()));
}

/**
 * Returns the curve at the specified index.
 *
 * @example
 *
 * var curve0 = compound.curves.get(0);
 * var curve1 = compound.curves.get(1);
 *
 * @method get
 * @instance
 * @memberof CompoundCurveCurves
 * @param {number} index
 * @throws {Error}
 * @return {CompoundCurve|SimpleCurve}
 */
NAN_METHOD(CompoundCurveCurves::get) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  CompoundCurve *geom = Nan::ObjectWrap::Unwrap<CompoundCurve>(parent);

  int i;
  NODE_ARG_INT(0, "index", i);

  if (i >= 0 && i < geom->get()->getNumCurves())
    info.GetReturnValue().Set(Geometry::New(geom->get()->getCurve(i), false));
  else
    Nan::ThrowRangeError("Invalid curve requested");
}

/**
 * Adds a curve to the collection.
 *
 * @example
 *
 * var ring1 = new gdal.CircularString();
 * ring1.points.add(0,0);
 * ring1.points.add(1,0);
 * ring1.points.add(1,1);
 * ring1.points.add(0,1);
 * ring1.points.add(0,0);
 *
 * // one at a time:
 * compound.curves.add(ring1);
 *
 * // many at once:
 * compound.curves.add([ring1, ...]);
 *
 * @method add
 * @instance
 * @memberof CompoundCurveCurves
 * @param {SimpleCurve|SimpleCurve[]} curves
 */
NAN_METHOD(CompoundCurveCurves::add) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  CompoundCurve *geom = Nan::ObjectWrap::Unwrap<CompoundCurve>(parent);

  SimpleCurve *ring;

  if (info.Length() < 1) {
    Nan::ThrowError("curve(s) must be given");
    return;
  }
  if (info[0]->IsArray()) {
    // set from array of geometry objects
    Local<Array> array = info[0].As<Array>();
    int length = array->Length();
    for (int i = 0; i < length; i++) {
      Local<Value> element = Nan::Get(array, i).ToLocalChecked();
      if (IS_WRAPPED(element, SimpleCurve)) {
        ring = Nan::ObjectWrap::Unwrap<SimpleCurve>(element.As<Object>());
        OGRErr err = geom->get()->addCurve(ring->get());
        if (err) {
          NODE_THROW_OGRERR(err);
          return;
        }
      } else {
        Nan::ThrowError("All array elements must be SimpleCurves");
        return;
      }
    }
  } else if (IS_WRAPPED(info[0], SimpleCurve)) {
    ring = Nan::ObjectWrap::Unwrap<SimpleCurve>(info[0].As<Object>());
    OGRErr err = geom->get()->addCurve(ring->get());
    if (err) {
      NODE_THROW_OGRERR(err);
      return;
    }
  } else {
    Nan::ThrowError("curve(s) must be a SimpleCurve or array of SimpleCurves");
    return;
  }

  return;
}

} // namespace node_gdal
