#include <string>
#include "gdal_coordinate_transformation.hpp"
#include "gdal_common.hpp"
#include "gdal_dataset.hpp"
#include "gdal_spatial_reference.hpp"
#ifdef BUNDLED_GDAL
#include "proj.h"
#endif

namespace node_gdal {

Napi::FunctionReference CoordinateTransformation::constructor;

void CoordinateTransformation::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(CoordinateTransformation);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "CoordinateTransformation",
    {
        METHOD(toString)
        METHOD(transformPoint)
    });

  target.Set("CoordinateTransformation", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

CoordinateTransformation::CoordinateTransformation(const Napi::CallbackInfo &info)
  : GDALObject<CoordinateTransformation>(info), this_(nullptr) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = static_cast<OGRCoordinateTransformation *>(info[0].As<Napi::External<void>>().Data());
    LOG("Created CoordinateTransformation [%p]", this_);
    return;
  }

  // Constructed from JS: CoordinateTransformation(source, target)
  // The NODE_ARG_* macros cannot be used here - they return a value on error
  if (info.Length() < 2) {
    Napi::Error::New(info.Env(), "Invalid number of arguments").ThrowAsJavaScriptException();
    return;
  }

  if (!node_gdal::IsInstanceOf<SpatialReference>(info[0])) {
    Napi::TypeError::New(info.Env(), "source must be an instance of SpatialReference").ThrowAsJavaScriptException();
    return;
  }
  SpatialReference *source = node_gdal::UnwrapWrapped<SpatialReference>(info[0].As<Napi::Object>());
  if (!source->isAlive()) {
    Napi::Error::New(info.Env(), "SpatialReference parameter already destroyed").ThrowAsJavaScriptException();
    return;
  }

  if (!info[1].IsObject() || info[1].IsNull()) {
    Napi::TypeError::New(info.Env(), "target must be a SpatialReference or Dataset object").ThrowAsJavaScriptException();
    return;
  }

  if (info[1].As<Napi::Object>().InstanceOf(SpatialReference::constructor.Value())) {
    // srs -> srs
    if (!node_gdal::IsInstanceOf<SpatialReference>(info[1])) {
      Napi::TypeError::New(info.Env(), "target must be an instance of SpatialReference").ThrowAsJavaScriptException();
      return;
    }
    SpatialReference *target = node_gdal::UnwrapWrapped<SpatialReference>(info[1].As<Napi::Object>());
    if (!target->isAlive()) {
      Napi::Error::New(info.Env(), "SpatialReference parameter already destroyed").ThrowAsJavaScriptException();
      return;
    }

    OGRCoordinateTransformation *transform = OGRCreateCoordinateTransformation(source->get(), target->get());
    if (!transform) {
      NODE_THROW_LAST_CPLERR;
      return;
    }
    this_ = transform;
  } else if (info[1].As<Napi::Object>().InstanceOf(Dataset::constructor.Value())) {
    // srs -> px/line
    // todo: allow additional options using StringList

    Dataset *ds;
    char **papszTO = NULL;
    char *src_wkt;

    ds = node_gdal::UnwrapWrapped<Dataset>(info[1].As<Napi::Object>());

    if (!ds->get()) {
      Napi::Error::New(info.Env(), "Dataset already closed").ThrowAsJavaScriptException();
      return;
    }

    OGRErr err = source->get()->exportToWkt(&src_wkt);
    if (err) {
      NODE_THROW_OGRERR(err);
      return;
    }

    papszTO = CSLSetNameValue(papszTO, "DST_SRS", src_wkt);
    papszTO = CSLSetNameValue(papszTO, "INSERT_CENTER_LONG", "FALSE");

    GeoTransformTransformer *transform = new GeoTransformTransformer();
    transform->hSrcImageTransformer = GDALCreateGenImgProjTransformer2(ds->get(), NULL, papszTO);
    if (!transform->hSrcImageTransformer) {
      NODE_THROW_LAST_CPLERR;
      return;
    }

    this_ = transform;

    CPLFree(src_wkt);
    CSLDestroy(papszTO);
  } else {
    Napi::TypeError::New(info.Env(), "target must be a SpatialReference or Dataset object").ThrowAsJavaScriptException();
    return;
  }

  LOG("Created CoordinateTransformation [%p]", this_);
}

CoordinateTransformation::~CoordinateTransformation() {
  if (this_) {
    LOG("Disposing CoordinateTransformation [%p]", this_);
    OGRCoordinateTransformation::DestroyCT(this_);
    LOG("Disposed CoordinateTransformation [%p]", this_);
    this_ = NULL;
  }
}

/**
 * Object for transforming between coordinate systems.
 *
 * @throws {Error}
 * @constructor
 * @class CoordinateTransformation
 * @param {SpatialReference} source
 * @param {SpatialReference|Dataset} target If a raster Dataset, the
 * conversion will represent a conversion to pixel coordinates.
 */

Napi::Value CoordinateTransformation::New(OGRCoordinateTransformation *transform) {

  if (!transform) { return node_gdal::napi_env().Null(); }

  std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env(), transform)};
  Napi::Object obj = CoordinateTransformation::constructor.Value().New(args);
  CoordinateTransformation *wrapped = node_gdal::UnwrapWrapped<CoordinateTransformation>(obj);

  return obj;
}

NAPI_METHOD(CoordinateTransformation::toString) {
  return Napi::String::New(node_gdal::napi_env(), "CoordinateTransformation");
}

/**
 * Transform point from source to destination space.
 *
 * @example
 *
 * pt = transform.transformPoint(0, 0, 0);
 *
 * @method transformPoint
 * @instance
 * @memberof CoordinateTransformation
 * @param {number} x
 * @param {number} y
 * @param {number} [z]
 * @return {xyz} A regular object containing `x`, `y`, `z` properties.
 */

/**
 * Transform point from source to destination space.
 *
 * @example
 *
 * pt = transform.transformPoint({x: 0, y: 0, z: 0});
 *
 * @method transformPoint
 * @instance
 * @memberof CoordinateTransformation
 * @param {xyz} point
 * @return {xyz} A regular object containing `x`, `y`, `z` properties.
 */
NAPI_METHOD(CoordinateTransformation::transformPoint) {
  CoordinateTransformation *transform = node_gdal::UnwrapWrapped<CoordinateTransformation>(info.This().As<Napi::Object>());

  double x, y, z = 0;

  if (info.Length() == 1 && info[0].IsObject()) {
    Napi::Object obj = info[0].As<Napi::Object>();
    Napi::Value arg_x = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "x"));
    Napi::Value arg_y = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "y"));
    Napi::Value arg_z = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "z"));
    if (!arg_x.IsNumber() || !arg_y.IsNumber()) {
      Napi::Error::New(node_gdal::napi_env(), "point must contain numerical properties x and y").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }
    x = static_cast<double>(arg_x.As<Napi::Number>().DoubleValue());
    y = static_cast<double>(arg_y.As<Napi::Number>().DoubleValue());
    if (arg_z.IsNumber()) { z = static_cast<double>(arg_z.As<Napi::Number>().DoubleValue()); }
  } else {
    NODE_ARG_DOUBLE(0, "x", x);
    NODE_ARG_DOUBLE(1, "y", y);
    NODE_ARG_DOUBLE_OPT(2, "z", z);
  }

#ifdef BUNDLED_GDAL
  int proj_error_code = 0;
  int r = transform->this_->TransformWithErrorCodes(1, &x, &y, &z, nullptr, &proj_error_code);
  if (!r || proj_error_code != 0) {
    Napi::Error::New(
      node_gdal::napi_env(),
      ("Error transforming point: " + std::string(proj_context_errno_string(nullptr, proj_error_code))).c_str())
      .ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
#else
  if (!transform->this_->Transform(1, &x, &y, &z)) {
    Napi::Error::New(node_gdal::napi_env(), "Error transforming point").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
#endif

  Napi::Object result = Napi::Object::New(node_gdal::napi_env());
  result.Set( Napi::String::New(node_gdal::napi_env(), "x"), Napi::Number::New(node_gdal::napi_env(), x));
  result.Set( Napi::String::New(node_gdal::napi_env(), "y"), Napi::Number::New(node_gdal::napi_env(), y));
  result.Set( Napi::String::New(node_gdal::napi_env(), "z"), Napi::Number::New(node_gdal::napi_env(), z));

  return result;
}

} // namespace node_gdal
