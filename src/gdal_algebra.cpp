#include "node_gdal.h"
#include "gdal_dataset.hpp"
#include "gdal_rasterband.hpp"
#include "gdal_algebra.hpp"
#include <string>
#include <cmath>

namespace node_gdal {

/**
 * GDAL RasterBand Algebra operations
 *
 * @namespace algebra
 */

namespace Algebra {

void Initialize(Local<Object> target) {
  Local<Object> algebra = Nan::New<Object>();
  Nan::Set(target, Nan::New("algebra").ToLocalChecked(), algebra);
  Nan__SetAsyncableMethod(algebra, "add", add);
}

#define NODE_ALGEBRA_ARG(num, var)                                                                                     \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError("Two arguments must be given");                                                                    \
    return;                                                                                                            \
  }                                                                                                                    \
  if (info[num]->IsNumber()) {                                                                                         \
    var##Number = Nan::To<double>(info[num]).ToChecked();                                                              \
    var##Band = nullptr;                                                                                               \
  } else if (info[num]->IsObject() && Nan::New(RasterBand::constructor)->HasInstance(info[num])) {                     \
    var##Number = NAN;                                                                                                 \
    var##Band = Nan::ObjectWrap::Unwrap<RasterBand>(info[num].As<Object>());                                           \
  } else {                                                                                                             \
    Nan::ThrowError("Argument must be either a number or a RasterBand");                                               \
    return;                                                                                                            \
  }

/**
 * Fill raster regions by interpolation from edges.
 *
 * @throws {Error}
 * @method add
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Fill raster regions by interpolation from edges.
 * @async
 *
 * @throws {Error}
 * @method addAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */

GDAL_ASYNCABLE_DEFINE(add) {
  RasterBand *arg1Band, *arg2Band;
  double arg1Number, arg2Number;

  NODE_ALGEBRA_ARG(0, arg1);
  NODE_ALGEBRA_ARG(1, arg2);

  std::vector<long> ds_uids = {};
  if (arg1Band) ds_uids.push_back(arg1Band->parent_uid);
  if (arg2Band) ds_uids.push_back(arg2Band->parent_uid);
  GDALAsyncableJob<GDALRasterBand *> job(ds_uids);
  if (arg1Band) job.persist(arg1Band->handle());
  if (arg2Band) job.persist(arg2Band->handle());

  if (arg1Band && arg2Band) {
    GDALRasterBand *arg1 = arg1Band->get();
    GDALRasterBand *arg2 = arg2Band->get();
    job.main = [arg1, arg2](const GDALExecutionProgress &) { return new GDALComputedRasterBand(*arg1 + *arg2); };
  } else if (arg1Band) {
    GDALRasterBand *arg1 = arg1Band->get();
    double arg2 = arg2Number;
    job.main = [arg1, arg2](const GDALExecutionProgress &) { return new GDALComputedRasterBand(*arg1 + arg2); };
  } else if (arg2Band) {
    double arg1 = arg1Number;
    GDALRasterBand *arg2 = arg2Band->get();
    job.main = [arg1, arg2](const GDALExecutionProgress &) { return new GDALComputedRasterBand(arg1 + *arg2); };
  } else {
    Nan::ThrowError("At least one RasterBand must be given");
    return;
  }

  job.rval = [](GDALRasterBand *r, const GetFromPersistentFunc &) {
    // Create an anonymous object cache entry, it will be immediately
    // referenced as RasterBand.ds. This is a GDALComputedDataset that
    // should not be closed.
    Dataset::New(r->GetDataset(), nullptr, false);
    return RasterBand::New(r, r->GetDataset());
  };
  job.run(info, async, 2);
}

} // namespace Algebra
} // namespace node_gdal
