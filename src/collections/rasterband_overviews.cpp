#include "rasterband_overviews.hpp"
#include "../gdal_common.hpp"
#include "../gdal_rasterband.hpp"

namespace node_gdal {

Napi::FunctionReference RasterBandOverviews::constructor;

void RasterBandOverviews::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(RasterBandOverviews);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "RasterBandOverviews",
    {
        METHOD(toString)
        METHOD_ASYNCABLE(count)
        METHOD_ASYNCABLE(get)
        METHOD_ASYNCABLE(getBySampleCount)
    });

  target.Set("RasterBandOverviews", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

RasterBandOverviews::RasterBandOverviews(const Napi::CallbackInfo &info) : GDALObject<RasterBandOverviews>(info) {
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::Error::New(info.Env(), "Cannot create RasterBandOverviews directly").ThrowAsJavaScriptException();
    return;
  }
  GDAL_SET_PRIVATE(info.This(), "parent_", info[0]);
}

RasterBandOverviews::~RasterBandOverviews() {
}

/**
 * An encapsulation of a {@link RasterBand} overview functionality.
 *
 * @class RasterBandOverviews
 */

Napi::Value RasterBandOverviews::New(Napi::Value band_obj) {

  std::vector<napi_value> args = {band_obj};
  Napi::Object obj = RasterBandOverviews::constructor.Value().New(args);

  return obj;
}

NAPI_METHOD(RasterBandOverviews::toString) {
  return Napi::String::New(node_gdal::napi_env(), "RasterBandOverviews");
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

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();

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
  return job.run(info, async, 1);
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

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
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
  return job.run(info, async, 1);
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

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  NODE_UNWRAP_CHECK(RasterBand, parent, band);

  GDALAsyncableJob<int> job(band->parent_uid);
  job.persist(parent);
  job.main = [band](const GDALExecutionProgress &) {
    int count = band->get()->GetOverviewCount();
    return count;
  };
  job.rval = [](int count, const GetFromPersistentFunc &) { return Napi::Number::New(node_gdal::napi_env(), count); };
  return job.run(info, async, 0);
}

} // namespace node_gdal
