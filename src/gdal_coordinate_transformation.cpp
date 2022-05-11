#include <string>
#include "gdal_coordinate_transformation.hpp"
#include "gdal_common.hpp"
#include "gdal_dataset.hpp"
#include "gdal_spatial_reference.hpp"
#ifdef BUNDLED_GDAL
#include "proj.h"
#endif

namespace node_gdal {

Nan::Persistent<FunctionTemplate> CoordinateTransformation::constructor;

void CoordinateTransformation::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(CoordinateTransformation::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("CoordinateTransformation").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan::SetPrototypeMethod(lcons, "transformPoint", transformPoint);

  Nan::Set(target, Nan::New("CoordinateTransformation").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

CoordinateTransformation::CoordinateTransformation(OGRCoordinateTransformation *transform)
  : Nan::ObjectWrap(), this_(transform) {
  LOG("Created CoordinateTransformation [%p]", transform);
}

CoordinateTransformation::CoordinateTransformation() : Nan::ObjectWrap(), this_(0) {
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
NAN_METHOD(CoordinateTransformation::New) {
  CoordinateTransformation *f;
  SpatialReference *source, *target;

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }

  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    f = static_cast<CoordinateTransformation *>(ptr);
  } else {
    if (info.Length() < 2) {
      Nan::ThrowError("Invalid number of arguments");
      return;
    }

    NODE_ARG_WRAPPED(0, "source", SpatialReference, source);

    if (!info[1]->IsObject() || info[1]->IsNull()) {
      Nan::ThrowTypeError("target must be a SpatialReference or Dataset object");
      return;
    }
    if (Nan::New(SpatialReference::constructor)->HasInstance(info[1])) {
      // srs -> srs
      NODE_ARG_WRAPPED(1, "target", SpatialReference, target);

      OGRCoordinateTransformation *transform = OGRCreateCoordinateTransformation(source->get(), target->get());
      if (!transform) {
        NODE_THROW_LAST_CPLERR;
        return;
      }
      f = new CoordinateTransformation(transform);
    } else if (Nan::New(Dataset::constructor)->HasInstance(info[1])) {
      // srs -> px/line
      // todo: allow additional options using StringList

      Dataset *ds;
      char **papszTO = NULL;
      char *src_wkt;

      ds = Nan::ObjectWrap::Unwrap<Dataset>(info[1].As<Object>());

      if (!ds->get()) {
        Nan::ThrowError("Dataset already closed");
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

      f = new CoordinateTransformation(transform);

      CPLFree(src_wkt);
      CSLDestroy(papszTO);
    } else {
      Nan::ThrowTypeError("target must be a SpatialReference or Dataset object");
      return;
    }
  }

  f->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

Local<Value> CoordinateTransformation::New(OGRCoordinateTransformation *transform) {
  Nan::EscapableHandleScope scope;

  if (!transform) { return scope.Escape(Nan::Null()); }

  CoordinateTransformation *wrapped = new CoordinateTransformation(transform);

  Local<Value> ext = Nan::New<External>(wrapped);
  Local<Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(CoordinateTransformation::constructor)).ToLocalChecked(), 1, &ext)
      .ToLocalChecked();

  return scope.Escape(obj);
}

NAN_METHOD(CoordinateTransformation::toString) {
  info.GetReturnValue().Set(Nan::New("CoordinateTransformation").ToLocalChecked());
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
NAN_METHOD(CoordinateTransformation::transformPoint) {
  CoordinateTransformation *transform = Nan::ObjectWrap::Unwrap<CoordinateTransformation>(info.This());

  double x, y, z = 0;

  if (info.Length() == 1 && info[0]->IsObject()) {
    Local<Object> obj = info[0].As<Object>();
    Local<Value> arg_x = Nan::Get(obj, Nan::New("x").ToLocalChecked()).ToLocalChecked();
    Local<Value> arg_y = Nan::Get(obj, Nan::New("y").ToLocalChecked()).ToLocalChecked();
    Local<Value> arg_z = Nan::Get(obj, Nan::New("z").ToLocalChecked()).ToLocalChecked();
    if (!arg_x->IsNumber() || !arg_y->IsNumber()) {
      Nan::ThrowError("point must contain numerical properties x and y");
      return;
    }
    x = static_cast<double>(Nan::To<double>(arg_x).ToChecked());
    y = static_cast<double>(Nan::To<double>(arg_y).ToChecked());
    if (arg_z->IsNumber()) { z = static_cast<double>(Nan::To<double>(arg_z).ToChecked()); }
  } else {
    NODE_ARG_DOUBLE(0, "x", x);
    NODE_ARG_DOUBLE(1, "y", y);
    NODE_ARG_DOUBLE_OPT(2, "z", z);
  }

#ifdef BUNDLED_GDAL
  int proj_error_code = 0;
  int r = transform->this_->TransformWithErrorCodes(1, &x, &y, &z, nullptr, &proj_error_code);
  if (!r || proj_error_code != 0) {
    Nan::ThrowError(
      ("Error transforming point: " + std::string(proj_context_errno_string(nullptr, proj_error_code))).c_str());
    return;
  }
#else
  if (!transform->this_->Transform(1, &x, &y, &z)) {
    Nan::ThrowError("Error transforming point");
    return;
  }
#endif

  Local<Object> result = Nan::New<Object>();
  Nan::Set(result, Nan::New("x").ToLocalChecked(), Nan::New<Number>(x));
  Nan::Set(result, Nan::New("y").ToLocalChecked(), Nan::New<Number>(y));
  Nan::Set(result, Nan::New("z").ToLocalChecked(), Nan::New<Number>(z));

  info.GetReturnValue().Set(result);
}

} // namespace node_gdal
