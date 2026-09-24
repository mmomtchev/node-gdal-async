#include "gdal_algorithms.hpp"
#include "gdal_common.hpp"
#include "gdal_dataset.hpp"
#include "gdal_layer.hpp"
#include "gdal_rasterband.hpp"
#include "utils/number_list.hpp"
#include "utils/typed_array.hpp"

#include "node_gdal.h"

namespace node_gdal {

void Algorithms::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();

  GDAL_SetAsyncableMethod(env, target, "fillNodata", fillNodata);
  GDAL_SetAsyncableMethod(env, target, "contourGenerate", contourGenerate);
  GDAL_SetAsyncableMethod(env, target, "sieveFilter", sieveFilter);
  GDAL_SetAsyncableMethod(env, target, "checksumImage", checksumImage);
  GDAL_SetAsyncableMethod(env, target, "polygonize", polygonize);
  GDAL_SetMethod(env, target, "addPixelFunc", addPixelFunc);
  GDAL_SetMethod(env, target, "toPixelFunc", toPixelFunc);
  GDAL_SetAsyncableMethod(env, target, "_acquireLocks", _acquireLocks);
}

/**
 * @typedef {object} FillOptions
 * @property {RasterBand} src
 * @property {RasterBand} [mask]
 * @property {number} searchDist
 * @property {number} [smoothingIterations]
 */

/**
 * Fill raster regions by interpolation from edges.
 *
 * @throws {Error}
 * @method fillNodata
 * @static
 * @param {FillOptions} options
 * @param {RasterBand} options.src This band to be updated in-place.
 * @param {RasterBand} [options.mask] Mask band
 * @param {number} options.searchDist The maximum distance (in pixels) that the algorithm will search out for values to interpolate.
 * @param {number} [options.smoothingIterations=0] The number of 3x3 average filter smoothing iterations to run after the interpolation to dampen artifacts.
 */

/**
 * Fill raster regions by interpolation from edges.
 * @async
 *
 * @throws {Error}
 * @method fillNodataAsync
 * @static
 * @param {FillOptions} options
 * @param {RasterBand} options.src This band to be updated in-place.
 * @param {RasterBand} [options.mask] Mask band
 * @param {number} options.searchDist The maximum distance (in pixels) that the algorithm will search out for values to interpolate.
 * @param {number} [options.smoothingIterations=0] The number of 3x3 average filter smoothing iterations to run after the interpolation to dampen artifacts.
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 */

GDAL_ASYNCABLE_DEFINE(Algorithms::fillNodata) {
  Napi::Object obj;
  RasterBand *src;
  RasterBand *mask = NULL;
  double search_dist;
  int smooth_iterations = 0;

  NODE_ARG_OBJECT(0, "options", obj);

  NODE_WRAPPED_FROM_OBJ(obj, "src", RasterBand, src);
  NODE_WRAPPED_FROM_OBJ_OPT(obj, "mask", RasterBand, mask);
  NODE_DOUBLE_FROM_OBJ(obj, "searchDist", search_dist);
  NODE_INT_FROM_OBJ_OPT(obj, "smoothIterations", smooth_iterations);

  GDALRasterBand *gdal_src = src->get();
  GDALRasterBand *gdal_mask = mask ? mask->get() : nullptr;

  std::vector<long> ds_uids = {src->parent_uid};
  if (mask) ds_uids.push_back(mask->parent_uid);

  GDALAsyncableJob<CPLErr> job(ds_uids);
  job.persist(src->Value());
  if (mask) job.persist(mask->Value());
  job.main = [gdal_src, gdal_mask, search_dist, smooth_iterations](const GDALExecutionProgress &) {
    CPLErrorReset();
    CPLErr err = GDALFillNodata(gdal_src, gdal_mask, search_dist, 0, smooth_iterations, NULL, NULL, NULL);
    if (err) { throw CPLGetLastErrorMsg(); }
    return err;
  };
  job.rval = [](CPLErr r, const GetFromPersistentFunc &) { return node_gdal::napi_env().Undefined().As<Napi::Value>(); };
  return job.run(info, async, 1);
}

/**
 * @typedef {object} ContourOptions
 * @property {RasterBand} src
 * @property {Layer} dst
 * @property {number} [offset]
 * @property {number} [interval]
 * @property {number[]} [fixedLevels]
 * @property {number} [nodata]
 * @property {number} [idField]
 * @property {number} [elevField]
 * @property {ProgressCb} [progress_cb]
 */

/**
 * Create vector contours from raster DEM.
 *
 * This algorithm will generate contour vectors for the input raster band on the
 * requested set of contour levels. The vector contours are written to the
 * passed in vector layer. Also, a NODATA value may be specified to identify
 * pixels that should not be considered in contour line generation.
 *
 * @throws {Error}
 * @method contourGenerate
 * @static
 * @param {ContourOptions} options
 * @param {RasterBand} options.src
 * @param {Layer} options.dst
 * @param {number} [options.offset=0] The "offset" relative to which contour intervals are applied. This is normally zero, but could be different. To generate 10m contours at 5, 15, 25, ... the offset would be 5.
 * @param {number} [options.interval=100] The elevation interval between contours generated.
 * @param {number[]} [options.fixedLevels] A list of fixed contour levels at which contours should be generated. Overrides interval/base options if set.
 * @param {number} [options.nodata] The value to use as a "nodata" value. That is, a pixel value which should be ignored in generating contours as if the value of the pixel were not known.
 * @param {number} [options.idField] A field index to indicate where a unique id should be written for each feature (contour) written.
 * @param {number} [options.elevField] A field index to indicate where the elevation value of the contour should be written.
 * @param {ProgressCb} [options.progress_cb]
*/

/**
 * Create vector contours from raster DEM.
 * @async
 *
 * This algorithm will generate contour vectors for the input raster band on the
 * requested set of contour levels. The vector contours are written to the
 * passed in vector layer. Also, a NODATA value may be specified to identify
 * pixels that should not be considered in contour line generation.
 *
 * @throws {Error}
 * @method contourGenerateAsync
 * @static
 * @param {ContourOptions} options
 * @param {RasterBand} options.src
 * @param {Layer} options.dst
 * @param {number} [options.offset=0] The "offset" relative to which contour intervals are applied. This is normally zero, but could be different. To generate 10m contours at 5, 15, 25, ... the offset would be 5.
 * @param {number} [options.interval=100] The elevation interval between contours generated.
 * @param {number[]} [options.fixedLevels] A list of fixed contour levels at which contours should be generated. Overrides interval/base options if set.
 * @param {number} [options.nodata] The value to use as a "nodata" value. That is, a pixel value which should be ignored in generating contours as if the value of the pixel were not known.
 * @param {number} [options.idField] A field index to indicate where a unique id should be written for each feature (contour) written.
 * @param {number} [options.elevField] A field index to indicate where the elevation value of the contour should be written.
 * @param {ProgressCb} [options.progress_cb]
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 */
GDAL_ASYNCABLE_DEFINE(Algorithms::contourGenerate) {
  Napi::Object obj;
  Napi::Value prop;
  RasterBand *src;
  Layer *dst;
  double interval = 100, base = 0;
  double *fixed_levels = NULL;
  DoubleList fixed_level_array;
  int n_fixed_levels = 0;
  int use_nodata = 0;
  double nodata = 0;
  int id_field = -1, elev_field = -1;
  Napi::FunctionReference *progress_cb = nullptr;

  NODE_ARG_OBJECT(0, "options", obj);

  NODE_WRAPPED_FROM_OBJ(obj, "src", RasterBand, src);
  NODE_WRAPPED_FROM_OBJ(obj, "dst", Layer, dst);
  NODE_INT_FROM_OBJ_OPT(obj, "idField", id_field);
  NODE_INT_FROM_OBJ_OPT(obj, "elevField", elev_field);
  NODE_DOUBLE_FROM_OBJ_OPT(obj, "interval", interval);
  NODE_DOUBLE_FROM_OBJ_OPT(obj, "offset", base);
  NODE_CB_FROM_OBJ_OPT(obj, "progress_cb", progress_cb);
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "fixedLevels"))) {
    if (fixed_level_array.parse(obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "fixedLevels")))) {
      return node_gdal::napi_env().Undefined(); // error parsing double list
    } else {
      fixed_levels = fixed_level_array.get();
      n_fixed_levels = fixed_level_array.length();
    }
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "nodata"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "nodata"));
    if (prop.IsNumber()) {
      use_nodata = 1;
      nodata = prop.As<Napi::Number>().DoubleValue();
    } else if (!prop.IsNull() && !prop.IsUndefined()) {
      Napi::TypeError::New(node_gdal::napi_env(), "nodata property must be a number").ThrowAsJavaScriptException();
    }
  }

  GDALRasterBand *gdal_src = src->get();
  OGRLayer *gdal_dst = dst->get();

  long src_uid = src->parent_uid;
  long dst_uid = dst->parent_uid;

  GDALAsyncableJob<CPLErr> job({src_uid, dst_uid});
  job.progress = progress_cb;
  job.main = [gdal_src,
              interval,
              base,
              n_fixed_levels,
              fixed_levels,
              use_nodata,
              nodata,
              gdal_dst,
              id_field,
              elev_field,
              progress_cb](const GDALExecutionProgress &progress) {
    CPLErrorReset();
    CPLErr err = GDALContourGenerate(
      gdal_src,
      interval,
      base,
      n_fixed_levels,
      fixed_levels,
      use_nodata,
      nodata,
      gdal_dst,
      id_field,
      elev_field,
      progress_cb ? ProgressTrampoline : nullptr,
      progress_cb ? (void *)&progress : nullptr);
    if (err) { throw CPLGetLastErrorMsg(); }
    return err;
  };
  job.rval = [](CPLErr r, const GetFromPersistentFunc &) { return node_gdal::napi_env().Undefined().As<Napi::Value>(); };
  return job.run(info, async, 1);
}

/**
 * @typedef {object} SieveOptions
 * @property {RasterBand} src
 * @property {RasterBand} dst
 * @property {RasterBand} [mask]
 * @property {number} threshold
 * @property {number} [connectedness]
 * @property {ProgressCb} [progress_cb]
 */

/**
 * Removes small raster polygons.
 *
 * @throws {Error}
 * @method sieveFilter
 * @static
 * @param {SieveOptions} options
 * @param {RasterBand} options.src
 * @param {RasterBand} options.dst Output raster band. It may be the same as src band to update the source in place.
 * @param {RasterBand} [options.mask] All pixels in the mask band with a value other than zero will be considered suitable for inclusion in polygons.
 * @param {number} options.threshold Raster polygons with sizes smaller than this will be merged into their largest neighbour.
 * @param {number} [options.connectedness=4] Either 4 indicating that diagonal pixels are not considered directly adjacent for polygon membership purposes or 8 indicating they are.
 * @param {ProgressCb} [options.progress_cb]
 */

/**
 * Removes small raster polygons.
 * @async
 *
 * @throws {Error}
 * @method sieveFilterAsync
 * @static
 * @param {SieveOptions} options
 * @param {RasterBand} options.src
 * @param {RasterBand} options.dst Output raster band. It may be the same as src band to update the source in place.
 * @param {RasterBand} [options.mask] All pixels in the mask band with a value other than zero will be considered suitable for inclusion in polygons.
 * @param {number} options.threshold Raster polygons with sizes smaller than this will be merged into their largest neighbour.
 * @param {number} [options.connectedness=4] Either 4 indicating that diagonal pixels are not considered directly adjacent for polygon membership purposes or 8 indicating they are.
 * @param {ProgressCb} [options.progress_cb]
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 */
GDAL_ASYNCABLE_DEFINE(Algorithms::sieveFilter) {
  Napi::Object obj;
  RasterBand *src;
  RasterBand *dst;
  RasterBand *mask = NULL;
  int threshold;
  int connectedness = 4;
  Napi::FunctionReference *progress_cb = nullptr;

  NODE_ARG_OBJECT(0, "options", obj);

  NODE_WRAPPED_FROM_OBJ(obj, "src", RasterBand, src);
  NODE_WRAPPED_FROM_OBJ(obj, "dst", RasterBand, dst);
  NODE_WRAPPED_FROM_OBJ_OPT(obj, "mask", RasterBand, mask);
  NODE_INT_FROM_OBJ(obj, "threshold", threshold);
  NODE_INT_FROM_OBJ_OPT(obj, "connectedness", connectedness);
  NODE_CB_FROM_OBJ_OPT(obj, "progress_cb", progress_cb);

  if (connectedness != 4 && connectedness != 8) {
    Napi::Error::New(node_gdal::napi_env(), "connectedness option must be 4 or 8").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALRasterBand *gdal_src = src->get();
  GDALRasterBand *gdal_dst = dst->get();
  GDALRasterBand *gdal_mask = mask ? mask->get() : nullptr;

  std::vector<long> ds_uids = {src->parent_uid, dst->parent_uid};
  if (mask) ds_uids.push_back(mask->parent_uid);

  GDALAsyncableJob<CPLErr> job(ds_uids);
  job.progress = progress_cb;
  job.main =
    [gdal_src, gdal_dst, gdal_mask, threshold, connectedness, progress_cb](const GDALExecutionProgress &progress) {
      CPLErrorReset();
      CPLErr err = GDALSieveFilter(
        gdal_src,
        gdal_mask,
        gdal_dst,
        threshold,
        connectedness,
        NULL,
        progress_cb ? ProgressTrampoline : nullptr,
        progress_cb ? (void *)&progress : nullptr);
      if (err) { throw CPLGetLastErrorMsg(); }
      return err;
    };
  job.rval = [](CPLErr r, const GetFromPersistentFunc &) { return node_gdal::napi_env().Undefined().As<Napi::Value>(); };
  return job.run(info, async, 1);
}

/**
 * Compute checksum for image region.
 *
 * @throws {Error}
 * @method checksumImage
 * @static
 * @param {RasterBand} src
 * @param {number} [x=0]
 * @param {number} [y=0]
 * @param {number} [w=src.width]
 * @param {number} [h=src.height]
 * @return {number}
 */

/**
 * Compute checksum for image region.
 *
 * @throws {Error}
 * @method checksumImageAsync
 * @static
 * @param {RasterBand} src
 * @param {number} [x=0]
 * @param {number} [y=0]
 * @param {number} [w=src.width]
 * @param {number} [h=src.height]
 * @param {callback<number>} [callback=undefined]
 * @return {number}
 * @return {Promise<number>}
 */

GDAL_ASYNCABLE_DEFINE(Algorithms::checksumImage) {
  RasterBand *src;
  int x = 0, y = 0, w, h, bandw, bandh;

  NODE_ARG_WRAPPED(0, "src", RasterBand, src);
  GDAL_RAW_CHECK(GDALRasterBand *, src, gdal_src);

  w = bandw = src->get()->GetXSize();
  h = bandh = src->get()->GetYSize();

  NODE_ARG_INT_OPT(1, "x", x);
  NODE_ARG_INT_OPT(2, "y", y);
  NODE_ARG_INT_OPT(3, "xSize", w);
  NODE_ARG_INT_OPT(4, "ySize", h);

  if (x < 0 || y < 0 || x >= bandw || y >= bandh) {
    Napi::RangeError::New(node_gdal::napi_env(), "offset invalid for given band").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  if (w < 0 || h < 0 || w > bandw || h > bandh) {
    Napi::RangeError::New(node_gdal::napi_env(), "x and y size must be smaller than band dimensions and greater than 0").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  if (x + w - 1 >= bandw || y + h - 1 >= bandh) {
    Napi::RangeError::New(node_gdal::napi_env(), "given range is outside bounds of given band").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  long src_uid = src->parent_uid;

  GDALAsyncableJob<int> job(src_uid);
  job.persist(src->Value());
  job.main = [gdal_src, x, y, w, h](const GDALExecutionProgress &) {
    CPLErrorReset();
    int r = GDALChecksumImage(gdal_src, x, y, w, h);
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 6)
    if (r == -1) throw CPLGetLastErrorMsg();
#else
    if (CPLGetLastErrorType() != CE_None) throw CPLGetLastErrorMsg();
#endif
    return r;
  };
  job.rval = [](int r, const GetFromPersistentFunc &) { return Napi::Number::New(node_gdal::napi_env(), r); };
  return job.run(info, async, 5);
}

/**
 * @typedef {object} PolygonizeOptions
 * @property {RasterBand} src
 * @property {Layer} dst
 * @property {RasterBand} [mask]
 * @property {number} pixValField The attribute field index indicating the feature attribute into which the pixel value of the polygon should be written.
 * @property {number} [connectedness=4] Either 4 indicating that diagonal pixels are not considered directly adjacent for polygon membership purposes or 8 indicating they are.
 * @property {boolean} [useFloats=false] Use floating point buffers instead of int buffers.
 * @property {ProgressCb} [progress_cb]
 */

/**
 * Creates vector polygons for all connected regions of pixels in the raster
 * sharing a common pixel value. Each polygon is created with an attribute
 * indicating the pixel value of that polygon. A raster mask may also be
 * provided to determine which pixels are eligible for processing.
 *
 * @throws {Error}
 * @method polygonize
 * @static
 * @param {PolygonizeOptions} options
 * @param {RasterBand} options.src
 * @param {Layer} options.dst
 * @param {RasterBand} [options.mask]
 * @param {number} options.pixValField The attribute field index indicating the feature attribute into which the pixel value of the polygon should be written.
 * @param {number} [options.connectedness=4] Either 4 indicating that diagonal pixels are not considered directly adjacent for polygon membership purposes or 8 indicating they are.
 * @param {boolean} [options.useFloats=false] Use floating point buffers instead of int buffers.
 * @param {ProgressCb} [options.progress_cb]
 */

/**
 * Creates vector polygons for all connected regions of pixels in the raster
 * sharing a common pixel value. Each polygon is created with an attribute
 * indicating the pixel value of that polygon. A raster mask may also be
 * provided to determine which pixels are eligible for processing.
 * @async
 *
 * @throws {Error}
 * @method polygonizeAsync
 * @static
 * @param {PolygonizeOptions} options
 * @param {RasterBand} options.src
 * @param {Layer} options.dst
 * @param {RasterBand} [options.mask]
 * @param {number} options.pixValField The attribute field index indicating the feature attribute into which the pixel value of the polygon should be written.
 * @param {number} [options.connectedness=4] Either 4 indicating that diagonal pixels are not considered directly adjacent for polygon membership purposes or 8 indicating they are.
 * @param {boolean} [options.useFloats=false] Use floating point buffers instead of int buffers.
 * @param {ProgressCb} [options.progress_cb]
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 */
GDAL_ASYNCABLE_DEFINE(Algorithms::polygonize) {
  Napi::Object obj;
  RasterBand *src;
  RasterBand *mask = NULL;
  Layer *dst;
  int connectedness = 4;
  int pix_val_field = 0;
  char **papszOptions = NULL;
  Napi::FunctionReference *progress_cb = nullptr;

  NODE_ARG_OBJECT(0, "options", obj);

  NODE_WRAPPED_FROM_OBJ(obj, "src", RasterBand, src);
  NODE_WRAPPED_FROM_OBJ(obj, "dst", Layer, dst);
  NODE_WRAPPED_FROM_OBJ_OPT(obj, "mask", RasterBand, mask);
  NODE_INT_FROM_OBJ_OPT(obj, "connectedness", connectedness)
  NODE_INT_FROM_OBJ(obj, "pixValField", pix_val_field);
  NODE_CB_FROM_OBJ_OPT(obj, "progress_cb", progress_cb);

  if (connectedness == 8) {
    papszOptions = CSLSetNameValue(papszOptions, "8CONNECTED", "8");
  } else if (connectedness != 4) {
    Napi::Error::New(node_gdal::napi_env(), "connectedness must be 4 or 8").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALRasterBand *gdal_src = src->get();
  OGRLayer *gdal_dst = dst->get();
  GDALRasterBand *gdal_mask = mask ? mask->get() : nullptr;

  std::vector<long> ds_uids = {src->parent_uid, dst->parent_uid};
  if (mask) ds_uids.push_back(mask->parent_uid);

  GDALAsyncableJob<CPLErr> job(ds_uids);
  job.progress = progress_cb;

  if (
    obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "useFloats")) &&
    obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "useFloats")).As<Napi::Boolean>().Value()) {
    job.main =
      [gdal_src, gdal_mask, gdal_dst, pix_val_field, papszOptions, progress_cb](const GDALExecutionProgress &progress) {
        CPLErrorReset();
        CPLErr err = GDALFPolygonize(
          gdal_src,
          gdal_mask,
          reinterpret_cast<OGRLayerH>(gdal_dst),
          pix_val_field,
          papszOptions,
          progress_cb ? ProgressTrampoline : nullptr,
          progress_cb ? (void *)&progress : nullptr);
        if (papszOptions) CSLDestroy(papszOptions);
        if (err) throw CPLGetLastErrorMsg();
        return err;
      };
  } else {
    job.main =
      [gdal_src, gdal_mask, gdal_dst, pix_val_field, papszOptions, progress_cb](const GDALExecutionProgress &progress) {
        CPLErrorReset();
        CPLErr err = GDALPolygonize(
          gdal_src,
          gdal_mask,
          reinterpret_cast<OGRLayerH>(gdal_dst),
          pix_val_field,
          papszOptions,
          progress_cb ? ProgressTrampoline : nullptr,
          progress_cb ? (void *)&progress : nullptr);
        if (papszOptions) CSLDestroy(papszOptions);
        if (err) throw CPLGetLastErrorMsg();
        return err;
      };
  }
  job.rval = [](CPLErr r, const GetFromPersistentFunc &) { return node_gdal::napi_env().Undefined().As<Napi::Value>(); };
  return job.run(info, async, 1);
}

// This is used for stress-testing the locking mechanism
// it doesn't do anything but sollicit locks
GDAL_ASYNCABLE_DEFINE(Algorithms::_acquireLocks) {
  Dataset *ds1, *ds2, *ds3;

  NODE_ARG_WRAPPED(0, "ds1", Dataset, ds1);
  NODE_ARG_WRAPPED(1, "ds2", Dataset, ds2);
  NODE_ARG_WRAPPED(2, "ds3", Dataset, ds3);

  GDALAsyncableJob<int> job({ds1->uid, ds2->uid, ds3->uid});
  job.main = [](const GDALExecutionProgress &) {
    int i, sum = 0;
    // make sure the optimizer won't surprise us
    for (i = 0; i < 1e4; i++) sum += i;
    return sum;
  };
  job.rval = [](int, const GetFromPersistentFunc &) { return node_gdal::napi_env().Undefined().As<Napi::Value>(); };
  return job.run(info, async, 3);
}

/**
 * @typedef {Uint8Array} PixelFunction
 */

/**
 * Register a new compiled pixel function for derived virtual datasets.
 *
 * This method is not meant to be used directly by the end user,
 * it is an entry point for plugins implementing pixel functions.
 *
 * You can check the `gdal-exprtk` plugin for an implementation
 * which uses ExprTk expressions.
 *
 * As of GDAL 3.11, this method is obsolete, since GDAL now
 * has native support for ExprTk expressions.
 *
 * @throws {Error}
 * @method addPixelFunc
 * @static
 * @param {string} name
 * @param {PixelFunction} pixelFn
 */
NAPI_METHOD(Algorithms::addPixelFunc) {

  std::string name;
  NODE_ARG_STR(0, "name", name);

  Napi::Object arg;
  NODE_ARG_OBJECT(1, "pixelFn", arg);

  Napi::TypedArrayOf<uint64_t> magic = arg.As<Napi::TypedArrayOf<uint64_t>>();

  if (magic.ElementLength() < 1 || *magic.Data() != NODE_GDAL_CAPI_MAGIC) {
    Napi::TypeError::New(node_gdal::napi_env(), "pixelFn must be a native code pixel function").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  Napi::TypedArrayOf<uint8_t> data = arg.As<Napi::TypedArrayOf<uint8_t>>();

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
  pixel_func *desc = reinterpret_cast<pixel_func *>(data.Data());

  CPLErr err = GDALAddDerivedBandPixelFuncWithArgs(name.c_str(), desc->fn, desc->metadata);

  if (err != CE_None) {

    NODE_THROW_LAST_CPLERR;

  } else {

  }

#else
  Napi::Error::New(node_gdal::napi_env(), "Custom pixel functions require GDAL >= 3.5").ThrowAsJavaScriptException();
#endif
  return node_gdal::napi_env().Undefined();
}

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
// This is the arguments of one call of the pixel function
struct pixelFnCall {
  void **sources;
  size_t num;
  void *destination;
  int width, height;
  GDALDataType inType;
  GDALDataType outType;
  std::map<std::string, std::string> args;
  std::string *err;
};

// This is the pixel function descriptor
// The queue can be modified both by the main thread and the worker threads
// (as this structure is stored in a vector, it must support realloc)
struct pixelFn {
  Napi::FunctionReference *fn;
  pixelFnCall call;
  uv_mutex_t callJS;
  uv_sem_t returnJS;
  uv_async_t *async;
};

// Only the main thread can modify this and only by adding new elements
// No need to lock
std::vector<pixelFn> pixelFuncs;

#define PFN_ID_FIELD "node_gdal_pfn_id"
const char metadataTemplate[] =
  "<PixelFunctionArgumentsList>\n"
  "   <Argument name ='" PFN_ID_FIELD
  "' type='constant' value='%x' />\n"
  "</PixelFunctionArgumentsList>";

// This is the final step before calling the JS function
// This function is called by libuv on the main thread
// The async_send in the function below is what triggers this call
static void callJSpfn(uv_async_t *async) {
  // Here V8 is accessible, but a plain libuv callback does not come with a
  // handle scope the way a napi callback does
  Napi::HandleScope scope(node_gdal::napi_env());

  pixelFn *fn = reinterpret_cast<pixelFn *>(async->data);
  Napi::Array sources = Napi::Array::New(node_gdal::napi_env(), fn->call.num);
  size_t len = fn->call.width * fn->call.height;
  for (size_t i = 0; i < fn->call.num; i++) {
    sources.Set( i, TypedArray::New(fn->call.inType, fn->call.sources[i], len));
  }
  Napi::Value destination = TypedArray::New(fn->call.outType, fn->call.destination, len);
  Napi::Number width = Napi::Number::New(node_gdal::napi_env(), fn->call.width);
  Napi::Number height = Napi::Number::New(node_gdal::napi_env(), fn->call.height);

  Napi::Object pfArgs = Napi::Object::New(node_gdal::napi_env());
  if (fn->call.args.size() > 0) {
    for (auto const &el : fn->call.args) {
      char *end;
      double dval = std::strtod(el.second.c_str(), &end);
      if (*end == 0)
        pfArgs.Set( Napi::String::New(node_gdal::napi_env(), el.first), Napi::Number::New(node_gdal::napi_env(), dval));
      else
        pfArgs.Set( Napi::String::New(node_gdal::napi_env(), el.first), Napi::String::New(node_gdal::napi_env(), el.second));
    }
  }

  std::vector<napi_value> args = {sources, destination, pfArgs, width, height};

  fn->call.err = nullptr;
  try {
    // async_hooks do not make any sense for pixel functions
    fn->fn->Value().Call(args);
  } catch (const Napi::Error &e) {
    fn->call.err = new std::string(e.Message());
  }

  // unlock the worker thread (the function below)
  uv_sem_post(&fn->returnJS);
}

// This is the GDAL pixel function trampoline that calls the JS callback
// It is called on one of the libuv async worker threads
// It throws when called on the main thread to avoid deadlocking
static CPLErr pixelFunc(
  void **papoSources,
  int nSources,
  void *pData,
  int nBufXSize,
  int nBufYSize,
  GDALDataType eSrcType,
  GDALDataType eBufType,
  int nPixelSpace,
  int nLineSpace,
  CSLConstList papszFunctionArgs) {
  // Here V8 is (potentially) off-limits

  std::map<std::string, std::string> pfArgsMap;
  ParseCSLConstList(papszFunctionArgs, pfArgsMap);

  auto uid = pfArgsMap.find(PFN_ID_FIELD);
  if (uid == pfArgsMap.end()) {
    CPLError(CE_Failure, CPLE_AppDefined, "gdal-async Internal error, pixelFuncs inconsistency, id=NULL");
    return CE_Failure;
  }
  char *end;
  size_t id = std::strtoul(uid->second.c_str(), &end, 16);
  if (end == uid->second.c_str() || id >= pixelFuncs.size()) {
    CPLError(CE_Failure, CPLE_AppDefined, "gdal-async Internal error, pixelFuncs inconsistency");
    return CE_Failure;
  }
  pfArgsMap.erase(PFN_ID_FIELD);

  size_t size = GDALGetDataTypeSizeBytes(eBufType);
  if (size == 0) {
    CPLError(CE_Failure, CPLE_AppDefined, "Invalid GDAL data type");
    return CE_Failure;
  }
  if (
    size != static_cast<size_t>(nPixelSpace) ||
    size * static_cast<size_t>(nBufXSize) != static_cast<size_t>(nLineSpace)) {
    CPLError(CE_Failure, CPLE_AppDefined, "gdal-async still does not support irregular buffer strides");
    return CE_Failure;
  }

  // Only one dataset can use the JS function at a time
  uv_mutex_lock(&pixelFuncs[id].callJS);

  uv_async_t *async = pixelFuncs[id].async;
  async->data = &pixelFuncs[id];

  pixelFuncs[id].call = {
    papoSources,
    static_cast<size_t>(nSources),
    pData,
    nBufXSize,
    nBufYSize,
    eSrcType,
    eBufType,
    std::move(pfArgsMap),
    nullptr};
  if (std::this_thread::get_id() == mainV8ThreadId) {
    // Main thread = sync mode
    // Here we are abusing an uninitialized uv_async_t as a data holder
    callJSpfn(async);
  } else {
    // Worker thread = async mode
    int s = uv_async_send(async);
    if (s != 0) {
      CPLError(CE_Failure, CPLE_AppDefined, "Pixel function error: failed scheduling async");
      return CE_Failure;
    }

    uv_sem_wait(&pixelFuncs[id].returnJS);
  }
  uv_mutex_unlock(&pixelFuncs[id].callJS);

  if (pixelFuncs[id].call.err != nullptr) {
    CPLError(CE_Failure, CPLE_AppDefined, "Pixel function error: %s", pixelFuncs[id].call.err->c_str());
    delete pixelFuncs[id].call.err;
    pixelFuncs[id].call.err = nullptr;
    return CE_Failure;
  }

  return CE_None;
}
#endif

/**
 * Create a GDAL pixel function from a JS function.
 *
 * As V8, and JS in general, can only have a single active JS context per isolate,
 * even when using async I/O, the pixel function will be called on the main thread.
 * This can lead to increased latency when serving network requests.
 *
 * ExprTk pixel functions are usually a better alternative, as these can
 * be evaluated in background threads without soliciting the JS engine.
 *
 * As GDAL does not allow unregistering a previously registered pixel functions,
 * each call of this method will produce a permanently registered pixel function.
 *
 * The function signature should match the GDAL pixel function signature.
 * You can check `createPixelFunc` for a higher-level method that can be used
 * to construct a GDAL pixel function from a JavaScript expression for a single pixel.
 *
 * @example
 * // This example will register a new GDAL pixel function called sum2
 * // that requires a VRT dataset with 2 values per pixel
 * const sum2 = (sources: gdal.TypedArray[], buffer: gdal.TypedArray) => {
 *   for (let i = 0; i < buffer.length; i++) {
 *     buffer[i] = sources[0][i] + sources[1][i]
 *   }
 * };
 * gdal.addPixelFunc('sum2', gdal.toPixelFunc(sum2));
 *
 * @throws {Error}
 * @method toPixelFunc
 * @static
 * @param {(sources: TypedArray[], buffer: TypedArray, args: Record<string, string|number>, width: number, height: number) => void} pixelFn JavaScript pixel function
 * @returns {PixelFunction}
 */
NAPI_METHOD(Algorithms::toPixelFunc) {
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
  Napi::FunctionReference *pfn;
  NODE_ARG_CB(0, "pixelFn", pfn);

  size_t uid = pixelFuncs.size();
  pixelFuncs.push_back({pfn, {}, {}, {}, nullptr});
  int s = uv_mutex_init(&pixelFuncs[uid].callJS);
  if (s != 0) {
    Napi::Error::New(node_gdal::napi_env(), "Failed creating a mutex").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  s = uv_sem_init(&pixelFuncs[uid].returnJS, 0);
  if (s != 0) {
    Napi::Error::New(node_gdal::napi_env(), "Failed creating a semaphore").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  pixelFuncs[uid].async = new uv_async_t;
  s = uv_async_init(uv_default_loop(), pixelFuncs[uid].async, callJSpfn);
  if (s != 0) {
    Napi::Error::New(node_gdal::napi_env(), "Failed creating libuv async").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  uv_unref(reinterpret_cast<uv_handle_t *>(pixelFuncs[uid].async));

  // `reserve` leaves the string empty, so `c_str()` terminates it at [0]: the
  // length below and the copy would both be zero, while desc->metadata still
  // pointed at the uninitialized tail of the array (GDAL then parsed garbage as
  // XML). The size has to be real before anything reads the string.
  std::string metadata;
  metadata.resize(strlen(metadataTemplate) + 32);
  int metadata_len = snprintf(&metadata[0], metadata.size(), metadataTemplate, static_cast<unsigned>(uid));
  if (metadata_len < 0 || static_cast<size_t>(metadata_len) >= metadata.size()) {
    Napi::Error::New(node_gdal::napi_env(), "Failed formatting the pixel function metadata").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  metadata.resize(metadata_len);

  Napi::Value r = node_gdal::TypedArray::New(GDT_Byte, sizeof(node_gdal::pixel_func) + metadata.size() + 1);
  if (r.IsEmpty() || !r.IsObject()) {
    Napi::Error::New(node_gdal::napi_env(), "Failed creating TypedArray").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  Napi::TypedArrayOf<GByte> contents = r.As<Napi::TypedArrayOf<GByte>>();
  node_gdal::pixel_func *desc = reinterpret_cast<node_gdal::pixel_func *>(contents.Data());

  desc->magic = NODE_GDAL_CAPI_MAGIC;
  desc->fn = pixelFunc;
  char *md = reinterpret_cast<char *>(desc) + sizeof(node_gdal::pixel_func);
  memcpy(md, metadata.data(), metadata.size());
  desc->metadata = md;

  return r;
#else
  Napi::Error::New(node_gdal::napi_env(), "Custom pixel functions require GDAL >= 3.5").ThrowAsJavaScriptException();
#endif
}

} // namespace node_gdal
