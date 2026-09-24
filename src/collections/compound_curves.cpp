#include "compound_curves.hpp"
#include "../gdal_common.hpp"
#include "../geometry/gdal_geometry.hpp"
#include "../geometry/gdal_linearring.hpp"
#include "../geometry/gdal_simplecurve.hpp"
#include "../geometry/gdal_compoundcurve.hpp"

namespace node_gdal {

Napi::FunctionReference CompoundCurveCurves::constructor;

void CompoundCurveCurves::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(CompoundCurveCurves);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "CompoundCurveCurves",
    {
        METHOD(toString)
        METHOD(count)
        METHOD(get)
        METHOD(add)
    });

  target.Set("CompoundCurveCurves", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

CompoundCurveCurves::CompoundCurveCurves(const Napi::CallbackInfo &info) : GDALObject<CompoundCurveCurves>(info) {
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::Error::New(info.Env(), "Cannot create CompoundCurveCurves directly").ThrowAsJavaScriptException();
    return;
  }
  GDAL_SET_PRIVATE(info.This(), "parent_", info[0]);
}

CompoundCurveCurves::~CompoundCurveCurves() {
}

/**
 * A collection of connected curves, used by {@link CompoundCurve}
 *
 * @class CompoundCurveCurves
 */

Napi::Value CompoundCurveCurves::New(Napi::Value geom) {

  std::vector<napi_value> args = {geom};
  Napi::Object obj = CompoundCurveCurves::constructor.Value().New(args);

  return obj;
}

NAPI_METHOD(CompoundCurveCurves::toString) {
  return Napi::String::New(node_gdal::napi_env(), "CompoundCurveCurves");
}

/**
 * Returns the number of curves that exist in the collection.
 *
 * @method count
 * @instance
 * @memberof CompoundCurveCurves
 * @return {number}
 */
NAPI_METHOD(CompoundCurveCurves::count) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  CompoundCurve *geom = node_gdal::UnwrapWrapped<CompoundCurve>(parent);

  return Napi::Number::New(node_gdal::napi_env(), geom->get()->getNumCurves());
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
NAPI_METHOD(CompoundCurveCurves::get) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  CompoundCurve *geom = node_gdal::UnwrapWrapped<CompoundCurve>(parent);

  int i;
  NODE_ARG_INT(0, "index", i);

  if (i >= 0 && i < geom->get()->getNumCurves())
    return Geometry::New(geom->get()->getCurve(i), false);
  Napi::RangeError::New(node_gdal::napi_env(), "Invalid curve requested").ThrowAsJavaScriptException();
  return node_gdal::napi_env().Undefined();
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
NAPI_METHOD(CompoundCurveCurves::add) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  CompoundCurve *geom = node_gdal::UnwrapWrapped<CompoundCurve>(parent);

  SimpleCurve *ring;

  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env(), "curve(s) must be given").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  if (info[0].IsArray()) {
    // set from array of geometry objects
    Napi::Array array = info[0].As<Napi::Array>();
    int length = array.Length();
    for (int i = 0; i < length; i++) {
      Napi::Value element = array.As<Napi::Object>().Get(i);
      if (IS_WRAPPED(element, SimpleCurve)) {
        ring = node_gdal::UnwrapWrapped<SimpleCurve>(element.As<Napi::Object>());
        OGRErr err = geom->get()->addCurve(ring->get());
        if (err) {
          NODE_THROW_OGRERR(err);
          return node_gdal::napi_env().Undefined();
        }
      } else {
        Napi::Error::New(node_gdal::napi_env(), "All array elements must be SimpleCurves").ThrowAsJavaScriptException();
        return node_gdal::napi_env().Undefined();
      }
    }
  } else if (IS_WRAPPED(info[0], SimpleCurve)) {
    ring = node_gdal::UnwrapWrapped<SimpleCurve>(info[0].As<Napi::Object>());
    OGRErr err = geom->get()->addCurve(ring->get());
    if (err) {
      NODE_THROW_OGRERR(err);
      return node_gdal::napi_env().Undefined();
    }
  } else {
    Napi::Error::New(node_gdal::napi_env(), "curve(s) must be a SimpleCurve or array of SimpleCurves").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  return node_gdal::napi_env().Undefined();
}

} // namespace node_gdal
