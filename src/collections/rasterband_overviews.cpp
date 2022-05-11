#include "rasterband_overviews.hpp"
#include "../gdal_common.hpp"
#include "../gdal_rasterband.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> RasterBandOverviews::constructor;

void RasterBandOverviews::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(RasterBandOverviews::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("RasterBandOverviews").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan__SetPrototypeAsyncableMethod(lcons, "count", count);
  Nan__SetPrototypeAsyncableMethod(lcons, "get", get);
  Nan__SetPrototypeAsyncableMethod(lcons, "getBySampleCount", getBySampleCount);

  Nan::Set(target, Nan::New("RasterBandOverviews").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

RasterBandOverviews::RasterBandOverviews() : Nan::ObjectWrap() {
}

RasterBandOverviews::~RasterBandOverviews() {
}

/**
 * An encapsulation of a {@link RasterBand} overview functionality.
 *
 * @class RasterBandOverviews
 */
NAN_METHOD(RasterBandOverviews::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }
  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    RasterBandOverviews *f = static_cast<RasterBandOverviews *>(ptr);
    f->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create RasterBandOverviews directly");
    return;
  }
}

Local<Value> RasterBandOverviews::New(Local<Value> band_obj) {
  Nan::EscapableHandleScope scope;

  RasterBandOverviews *wrapped = new RasterBandOverviews();

  v8::Local<v8::Value> ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(RasterBandOverviews::constructor)).ToLocalChecked(), 1, &ext)
      .ToLocalChecked();
  Nan::SetPrivate(obj, Nan::New("parent_").ToLocalChecked(), band_obj);

  return scope.Escape(obj);
}

NAN_METHOD(RasterBandOverviews::toString) {
  info.GetReturnValue().Set(Nan::New("RasterBandOverviews").ToLocalChecked());
}

/**
 * Fetches the overview at the provided index.
 *
 * @method get
 * @instance
 * @memberof RasterBandOverviews
 * @throws {Error}
 * @param {number} index 0-based index
 * @return {RasterBand}
 */

/**
 * Fetches the overview at the provided index.
 * @async
 *
 * @method getAsync
 * @instance
 * @memberof RasterBandOverviews
 * @throws {Error}
 * @param {number} index 0-based index
 * @param {callback<RasterBand>} [callback=undefined]
 * @return {Promise<RasterBand>}
 */
GDAL_ASYNCABLE_DEFINE(RasterBandOverviews::get) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();

  NODE_UNWRAP_CHECK(RasterBand, parent, band);

  int id;
  NODE_ARG_INT(0, "id", id);

  GDALAsyncableJob<GDALRasterBand *> job(band->parent_uid);
  job.persist(parent);
  job.main = [band, id](const GDALExecutionProgress &) {
    CPLErrorReset();
    GDALRasterBand *result = band->get()->GetOverview(id);
    if (result == nullptr) { throw "Specified overview not found"; }
    return result;
  };
  job.rval = [band](GDALRasterBand *result, const GetFromPersistentFunc &) {
    return RasterBand::New(result, band->getParent());
  };
  job.run(info, async, 1);
}

/**
 * Fetch best sampling overview.
 *
 * Returns the most reduced overview of the given band that still satisfies the
 * desired number of samples. This function can be used with zero as the number
 * of desired samples to fetch the most reduced overview. The same band as was
 * passed in will be returned if it has not overviews, or if none of the
 * overviews have enough samples.
 *
 * @method getBySampleCount
 * @instance
 * @memberof RasterBandOverviews
 * @param {number} samples
 * @return {RasterBand}
 */

/**
 * Fetch best sampling overview.
 * @async
 *
 * Returns the most reduced overview of the given band that still satisfies the
 * desired number of samples. This function can be used with zero as the number
 * of desired samples to fetch the most reduced overview. The same band as was
 * passed in will be returned if it has not overviews, or if none of the
 * overviews have enough samples.
 *
 * @method getBySampleCountAsync
 * @instance
 * @memberof RasterBandOverviews
 * @param {number} samples
 * @param {callback<RasterBand>} [callback=undefined]
 * @return {Promise<RasterBand>}
 */
GDAL_ASYNCABLE_DEFINE(RasterBandOverviews::getBySampleCount) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  NODE_UNWRAP_CHECK(RasterBand, parent, band);

  int n_samples;
  NODE_ARG_INT(0, "minimum number of samples", n_samples);

  GDALAsyncableJob<GDALRasterBand *> job(band->parent_uid);
  job.persist(parent);
  job.main = [band, n_samples](const GDALExecutionProgress &) {
    CPLErrorReset();
    GDALRasterBand *result = band->get()->GetRasterSampleOverview(n_samples);
    if (result == nullptr) { throw "Specified overview not found"; }
    return result;
  };
  job.rval = [band](GDALRasterBand *result, const GetFromPersistentFunc &) {
    return RasterBand::New(result, band->getParent());
  };
  job.run(info, async, 1);
}

/**
 * Returns the number of overviews.
 *
 * @method count
 * @instance
 * @memberof RasterBandOverviews
 * @return {number}
 */

/**
 * Returns the number of overviews.
 * @async
 *
 * @method countAsync
 * @instance
 * @memberof RasterBandOverviews
 * @param {callback<number>} [callback=undefined]
 * @return {Promise<number>}
 */
GDAL_ASYNCABLE_DEFINE(RasterBandOverviews::count) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  NODE_UNWRAP_CHECK(RasterBand, parent, band);

  GDALAsyncableJob<int> job(band->parent_uid);
  job.persist(parent);
  job.main = [band](const GDALExecutionProgress &) {
    int count = band->get()->GetOverviewCount();
    return count;
  };
  job.rval = [](int count, const GetFromPersistentFunc &) { return Nan::New<Integer>(count); };
  job.run(info, async, 0);
}

} // namespace node_gdal
