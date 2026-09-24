
#include "gdal_common.hpp"

#include "collections/rasterband_overviews.hpp"
#include "collections/rasterband_pixels.hpp"
#include "collections/colortable.hpp"
#include "gdal_dataset.hpp"
#include "gdal_mdarray.hpp"
#include "gdal_majorobject.hpp"
#include "gdal_rasterband.hpp"
#include "utils/string_list.hpp"

#include <cpl_port.h>
#include <limits>
#include <mutex>

namespace node_gdal {

Napi::FunctionReference RasterBand::constructor;

void RasterBand::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(RasterBand);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "RasterBand",
    {
        METHOD(toString)
        METHOD_ASYNCABLE(flush)
        METHOD_ASYNCABLE(fill)
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
        METHOD(asMDArray)
#endif
        METHOD(getStatistics)
        METHOD(setStatistics)
        METHOD_ASYNCABLE(computeStatistics)
        METHOD(getMaskBand)
        METHOD(getMaskFlags)
        METHOD(createMaskBand)
        METHOD_ASYNCABLE(getMetadata)
        METHOD_ASYNCABLE(setMetadata)
        ATTR_DONT_ENUM(lcons, "ds", dsGetter, READ_ONLY_SETTER)
        ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER)
        ATTR_ASYNCABLE(lcons, "id", idGetter, READ_ONLY_SETTER)
        ATTR_ASYNCABLE(lcons, "description", descriptionGetter, READ_ONLY_SETTER)
        ATTR_ASYNCABLE(lcons, "size", sizeGetter, READ_ONLY_SETTER)
        ATTR(lcons, "overviews", overviewsGetter, READ_ONLY_SETTER)
        ATTR(lcons, "pixels", pixelsGetter, READ_ONLY_SETTER)
        ATTR_ASYNCABLE(lcons, "blockSize", blockSizeGetter, READ_ONLY_SETTER)
        ATTR_ASYNCABLE(lcons, "minimum", minimumGetter, READ_ONLY_SETTER)
        ATTR_ASYNCABLE(lcons, "maximum", maximumGetter, READ_ONLY_SETTER)
        ATTR_ASYNCABLE(lcons, "readOnly", readOnlyGetter, READ_ONLY_SETTER)
        ATTR_ASYNCABLE(lcons, "dataType", dataTypeGetter, READ_ONLY_SETTER)
        ATTR_ASYNCABLE(lcons, "hasArbitraryOverviews", hasArbitraryOverviewsGetter, READ_ONLY_SETTER)
        ATTR_ASYNCABLE(lcons, "unitType", unitTypeGetter, unitTypeSetter)
        ATTR_ASYNCABLE(lcons, "scale", scaleGetter, scaleSetter)
        ATTR_ASYNCABLE(lcons, "offset", offsetGetter, offsetSetter)
        ATTR_ASYNCABLE(lcons, "noDataValue", noDataValueGetter, noDataValueSetter)
        ATTR_ASYNCABLE(lcons, "categoryNames", categoryNamesGetter, categoryNamesSetter)
        ATTR_ASYNCABLE(lcons, "colorInterpretation", colorInterpretationGetter, colorInterpretationSetter)
        ATTR_ASYNCABLE(lcons, "colorTable", colorTableGetter, colorTableSetter)
    });

  target.Set("RasterBand", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}


RasterBand::RasterBand(const Napi::CallbackInfo &info) : GDALObject<RasterBand>(info), uid(0), this_(nullptr), parent_ds(0) {
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = static_cast<GDALRasterBand *>(info[0].As<Napi::External<void>>().Data());
    LOG("Created band [%p] (dataset = %p)", this_, this_->GetDataset());
  } else {
    Napi::Error::New(info.Env(), "Cannot create RasterBand directly").ThrowAsJavaScriptException();
    return;
  }

  GDAL_SET_PRIVATE(info.This(), "overviews_", RasterBandOverviews::New(info.This()));
  GDAL_SET_PRIVATE(info.This(), "pixels_", RasterBandPixels::New(info.This()));
}

RasterBand::~RasterBand() {
  dispose();
}

void RasterBand::dispose() {
  if (this_) {
    LOG("Disposing band [%p]", this_);

    object_store.dispose(uid);

    LOG("Disposed band [%p]", this_);

    this_ = NULL;
  }
}

/**
 * A single raster band (or channel).
 *
 * @class RasterBand
 */
Napi::Value RasterBand::New(GDALRasterBand *raw, GDALDataset *raw_parent) {

  if (!raw) { return node_gdal::napi_env().Null(); }
  if (object_store.has(raw)) { return object_store.get(raw); }

  std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env(), raw)};
  Napi::Object obj = RasterBand::constructor.Value().New(args);
  RasterBand *wrapped = node_gdal::UnwrapWrapped<RasterBand>(obj);

  LOG("Adding band to cache[%p] (parent=%p)", raw, raw_parent);

  // add reference to dataset so dataset doesnt get GC'ed while band is alive
  // DONT USE GDALRasterBand.GetDataset() ... it will return a "fake" dataset
  // for overview bands
  // https://github.com/naturalatlas/node-gdal/blob/master/deps/libgdal/gdal/frmts/gtiff/geotiff.cpp#L84

  Napi::Object ds;
  if (!object_store.has(raw_parent)) {
    LOG("Band's parent dataset disappeared from cache (band = %p, dataset = %p)", raw, raw_parent);
    Napi::Error::New(node_gdal::napi_env(), "Band's parent dataset disappeared from cache").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
    // ds = Dataset::New(raw_parent); //this should never happen
  }

  ds = object_store.get(raw_parent);
  Dataset *parent = node_gdal::UnwrapWrapped<Dataset>(ds);
  long parent_uid = parent->uid;
  wrapped->uid = object_store.add(raw, *wrapped, parent_uid);
  wrapped->parent_ds = raw_parent;
  wrapped->parent_uid = parent_uid;

  // the band reads it back in dsGetter, and it also keeps the dataset alive
  GDAL_SET_PRIVATE(obj, "ds_", ds);

  return obj;
}

NAPI_METHOD(RasterBand::toString) {
  return Napi::String::New(node_gdal::napi_env(), "RasterBand");
}

/**
 * Saves changes to disk.
 * @method flush
 * @instance
 * @memberof RasterBand
 */

/**
 * Saves changes to disk.
 * @async
 *
 * @method flushAsync
 * @instance
 * @memberof RasterBand
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 *
 */
NODE_WRAPPED_ASYNC_METHOD_WITH_OGRERR_RESULT_LOCKED(RasterBand, flush, FlushCache);

/**
 * Return the status flags of the mask band associated with the band.
 *
 * The result will be a bitwise OR-ed set of status flags with the following
 * available definitions that may be extended in the future:
 *
 *   - `GMF_ALL_VALID` (`0x01`): There are no invalid pixels, all mask values
 * will be 255. When used this will normally be the only flag set.
 *   - `GMF_PER_DATASET` (`0x02`): The mask band is shared between all bands on
 * the dataset.
 *   - `GMF_ALPHA` (`0x04`): The mask band is actually an alpha band and may
 * have values other than 0 and 255.
 *   - `GMF_NODATA` (`0x08`): Indicates the mask is actually being generated
 * from nodata values. (mutually exclusive of `GMF_ALPHA`)
 *
 * @method getMaskFlags
 * @instance
 * @memberof RasterBand
 * @return {number} Mask flags
 */
NODE_WRAPPED_METHOD_WITH_RESULT_LOCKED(RasterBand, getMaskFlags, Integer, GetMaskFlags);
// TODO: expose GMF constants in API
// ({@link GMF|see flags})

/**
 * Adds a mask band to the current band.
 *
 * @throws {Error}
 * @method createMaskBand
 * @instance
 * @memberof RasterBand

 * @param {number} flags Mask flags
 */
NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_INTEGER_PARAM_LOCKED(RasterBand, createMaskBand, CreateMaskBand, "mask flags");
// TODO: expose GMF constants in API
// ({@link GMF|see flags})

/**
 * Return the mask band associated with the band.
 *
 * @method getMaskBand
 * @instance
 * @memberof RasterBand
 * @return {RasterBand|null}
 */
NAPI_METHOD(RasterBand::getMaskBand) {
  NODE_UNWRAP_CHECK(RasterBand, info.This(), band);
  GDAL_LOCK_PARENT(band);
  GDALRasterBand *mask_band = band->this_->GetMaskBand();

  if (!mask_band) {
    return node_gdal::napi_env().Null();
    return node_gdal::napi_env().Undefined();
  }

  return RasterBand::New(mask_band, band->getParent());
}

/**
 * Fill this band with a constant value.
 *
 * @throws {Error}
 * @method fill
 * @instance
 * @memberof RasterBand
 * @throws {Error}
 * @param {number} real_value
 * @param {number} [imaginary_value]
 */

/**
 * Fill this band with a constant value.
 * @async
 *
 * @throws {Error}
 * @method fillAsync
 * @instance
 * @memberof RasterBand
 * @throws {Error}
 * @param {number} real_value
 * @param {number} [imaginary_value]
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 */
GDAL_ASYNCABLE_DEFINE(RasterBand::fill) {
  double real, imaginary = 0;
  NODE_ARG_DOUBLE(0, "real value", real);
  NODE_ARG_DOUBLE_OPT(1, "imaginary value", real);

  NODE_UNWRAP_CHECK(RasterBand, info.This(), band);

  GDALAsyncableJob<CPLErr> job(band->parent_uid);
  GDALRasterBand *gdal_obj = band->this_;

  job.main = [gdal_obj, real, imaginary](const GDALExecutionProgress &) {
    CPLErrorReset();
    CPLErr err = gdal_obj->Fill(real, imaginary);
    if (err) { throw CPLGetLastErrorMsg(); }
    return err;
  };
  job.rval = [](CPLErr, const GetFromPersistentFunc &) { return node_gdal::napi_env().Undefined().As<Napi::Value>(); };

  return job.run(info, async, 2);
}

// --- Custom error handling to handle VRT errors ---
// see: https://github.com/mapbox/mapnik-omnivore/issues/10

std::mutex stats_lock;
std::string stats_file_err = "";
CPLErrorHandler last_err_handler;
void CPL_STDCALL statisticsErrorHandler(CPLErr eErrClass, int err_no, const char *msg) {
  if (err_no == CPLE_OpenFailed) { stats_file_err = msg; }
  if (last_err_handler) { last_err_handler(eErrClass, err_no, msg); }
}
void pushStatsErrorHandler() {
  last_err_handler = CPLSetErrorHandler(statisticsErrorHandler);
}
void popStatsErrorHandler() {
  if (!last_err_handler) return;
  CPLSetErrorHandler(last_err_handler);
}

/**
 * Return a view of this raster band as a 2D multidimensional GDALMDArray.
 *
 * The band must be linked to a GDALDataset.
 *
 * If the dataset has a geotransform attached, the X and Y dimensions of the returned array will have an associated indexing variable.
 *
 * Requires GDAL>=3.3 with MDArray support, won't be defined otherwise
 *
 * @throws {Error}
 * @method asMDArray
 * @instance
 * @memberof RasterBand
 * @return {MDArray}
 */
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
NAPI_METHOD(RasterBand::asMDArray) {
  NODE_UNWRAP_CHECK(RasterBand, info.This(), band);
  GDAL_RAW_CHECK(GDALRasterBand *, band, raw);
  GDAL_LOCK_PARENT(band);
  CPLErrorReset();
  std::shared_ptr<GDALMDArray> mdarray = raw->AsMDArray();
  if (mdarray == nullptr) {
    Napi::Error::New(node_gdal::napi_env(), CPLGetLastErrorMsg()).ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  Napi::Value obj = MDArray::New(mdarray, band->parent_ds);
  return obj;
}
#endif

/**
 * Fetch image statistics.
 *
 * Returns the minimum, maximum, mean and standard deviation of all pixel values
 * in this band. If approximate statistics are sufficient, the
 * `allow_approximation` argument can be set to `true` in which case overviews,
 * or a subset of image tiles may be used in computing the statistics.
 *
 * @throws {Error}
 * @method getStatistics
 * @instance
 * @memberof RasterBand
 * @param {boolean} allow_approximation If `true` statistics may be computed
 * based on overviews or a subset of all tiles.
 * @param {boolean} force If `false` statistics will only be returned if it can
 * be done without rescanning the image.
 * @return {object} Statistics containing `"min"`, `"max"`, `"mean"`,
 * `"std_dev"` properties.
 */
NAPI_METHOD(RasterBand::getStatistics) {
  double min, max, mean, std_dev;
  int approx, force;
  NODE_ARG_BOOL(0, "allow approximation", approx);
  NODE_ARG_BOOL(1, "force", force);
  NODE_UNWRAP_CHECK(RasterBand, info.This(), band);
  GDAL_LOCK_PARENT(band);
  pushStatsErrorHandler();
  CPLErr err = band->this_->GetStatistics(approx, force, &min, &max, &mean, &std_dev);
  popStatsErrorHandler();
  if (!stats_file_err.empty()) {
    Napi::Error::New(node_gdal::napi_env(), stats_file_err.c_str()).ThrowAsJavaScriptException();
  } else if (err) {
    if (!force && err == CE_Warning) {
      Napi::Error::New(node_gdal::napi_env(), "Statistics cannot be efficiently computed without scanning raster").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }
    NODE_THROW_LAST_CPLERR;
    return node_gdal::napi_env().Undefined();
  }

  Napi::Object result = Napi::Object::New(node_gdal::napi_env());
  result.Set( Napi::String::New(node_gdal::napi_env(), "min"), Napi::Number::New(node_gdal::napi_env(), min));
  result.Set( Napi::String::New(node_gdal::napi_env(), "max"), Napi::Number::New(node_gdal::napi_env(), max));
  result.Set( Napi::String::New(node_gdal::napi_env(), "mean"), Napi::Number::New(node_gdal::napi_env(), mean));
  result.Set( Napi::String::New(node_gdal::napi_env(), "std_dev"), Napi::Number::New(node_gdal::napi_env(), std_dev));

  return result;
}

/**
 * @typedef {object} stats
 * @property {number} min
 * @property {number} max
 * @property {number} mean
 * @property {number} std_dev
 */

/**
 * Computes image statistics.
 *
 * Returns the minimum, maximum, mean and standard deviation of all pixel values
 * in this band. If approximate statistics are sufficient, the
 * `allow_approximation` argument can be set to `true` in which case overviews,
 * or a subset of image tiles may be used in computing the statistics.
 *
 * @throws {Error}
 * @method computeStatistics
 * @instance
 * @memberof RasterBand

 * @param {boolean} allow_approximation If `true` statistics may be computed
 * based on overviews or a subset of all tiles.
 * @return {stats} Statistics containing `"min"`, `"max"`, `"mean"`,
 * `"std_dev"` properties.
 */

/**
 * Computes image statistics.
 * {{async}}
 *
 * Returns the minimum, maximum, mean and standard deviation of all pixel values
 * in this band. If approximate statistics are sufficient, the
 * `allow_approximation` argument can be set to `true` in which case overviews,
 * or a subset of image tiles may be used in computing the statistics.
 *
 * @throws {Error}
 * @method computeStatisticsAsync
 * @instance
 * @memberof RasterBand
 * @param {boolean} allow_approximation If `true` statistics may be computed
 * based on overviews or a subset of all tiles.
 * @param {callback<stats>} [callback=undefined]
 * @return {Promise<stats>} Statistics containing `"min"`, `"max"`, `"mean"`,
 * `"std_dev"` properties.
 */
GDAL_ASYNCABLE_DEFINE(RasterBand::computeStatistics) {
  struct stats_t {
    double min, max, mean, std_dev;
  };
  int approx;

  NODE_ARG_BOOL(0, "allow approximation", approx);
  NODE_UNWRAP_CHECK(RasterBand, info.This(), band);

  GDALAsyncableJob<stats_t> job(band->parent_uid);
  GDALRasterBand *gdal_obj = band->this_;

  job.main = [gdal_obj, approx](const GDALExecutionProgress &) {
    struct stats_t stats;
    std::lock_guard<std::mutex> guard(stats_lock);

    CPLErrorReset();
    pushStatsErrorHandler();
    CPLErr err = gdal_obj->ComputeStatistics(approx, &stats.min, &stats.max, &stats.mean, &stats.std_dev, NULL, NULL);
    popStatsErrorHandler();
    if (!stats_file_err.empty()) {
      throw stats_file_err.c_str();
    } else if (err != CPLE_None) {
      throw CPLGetLastErrorMsg();
    }

    return stats;
  };

  job.rval = [](stats_t r, const GetFromPersistentFunc &) {
    Napi::Object result = Napi::Object::New(node_gdal::napi_env());
    result.Set( Napi::String::New(node_gdal::napi_env(), "min"), Napi::Number::New(node_gdal::napi_env(), r.min));
    result.Set( Napi::String::New(node_gdal::napi_env(), "max"), Napi::Number::New(node_gdal::napi_env(), r.max));
    result.Set( Napi::String::New(node_gdal::napi_env(), "mean"), Napi::Number::New(node_gdal::napi_env(), r.mean));
    result.Set( Napi::String::New(node_gdal::napi_env(), "std_dev"), Napi::Number::New(node_gdal::napi_env(), r.std_dev));
    return result;
  };

  return job.run(info, async, 1);
}

/**
 * Set statistics on the band. This method can be used to store
 * min/max/mean/standard deviation statistics.
 *
 * @throws {Error}
 * @method setStatistics
 * @instance
 * @memberof RasterBand
 * @param {number} min
 * @param {number} max
 * @param {number} mean
 * @param {number} std_dev
 */
NAPI_METHOD(RasterBand::setStatistics) {
  double min, max, mean, std_dev;

  NODE_ARG_DOUBLE(0, "min", min);
  NODE_ARG_DOUBLE(1, "max", max);
  NODE_ARG_DOUBLE(2, "mean", mean);
  NODE_ARG_DOUBLE(3, "standard deviation", std_dev);
  NODE_UNWRAP_CHECK(RasterBand, info.This(), band);
  GDAL_LOCK_PARENT(band);
  CPLErr err = band->this_->SetStatistics(min, max, mean, std_dev);

  if (err) {
    NODE_THROW_LAST_CPLERR;
    return node_gdal::napi_env().Undefined();
  }
  return node_gdal::napi_env().Undefined();
}

/**
 * Returns band metadata.
 *
 * @method getMetadata
 * @instance
 * @memberof RasterBand
 * @param {string} [domain]
 * @return {any}
 */

/**
 * Returns band metadata.
 * @async
 *
 * @method getMetadataAsync
 * @instance
 * @memberof RasterBand
 * @param {string} [domain]
 * @param {callback<any>} [callback=undefined]
 * @return {Promise<any>}
 */
GDAL_ASYNCABLE_DEFINE(RasterBand::getMetadata) {

  std::string domain("");
  NODE_ARG_OPT_STR(0, "domain", domain);
  NODE_UNWRAP_CHECK(RasterBand, info.This(), band);
  GDAL_RAW_CHECK(GDALRasterBand *, band, raw);

  GDALAsyncableJob<CSLConstList> job(band->parent_uid);
  job.main = [raw, domain](const GDALExecutionProgress &) {
    return raw->GetMetadata(domain.empty() ? nullptr : domain.c_str());
  };
  job.rval = [](CSLConstList md, const GetFromPersistentFunc &) { return MajorObject::getMetadata(md); };
  return job.run(info, async, 1);
}

/**
 * Set metadata. Can return a warning (false) for formats not supporting persistent metadata.
 *
 * @method setMetadata
 * @instance
 * @memberof RasterBand
 * @param {object|string[]} metadata
 * @param {string} [domain]
 * @return {boolean}
 */

/**
 * Set metadata. Can return a warning (false) for formats not supporting persistent metadata.
 * @async
 *
 * @method setMetadataAsync
 * @instance
 * @memberof RasterBand
 * @param {object|string[]} metadata
 * @param {string} [domain]
 * @param {callback<boolean>} [callback=undefined]
 * @return {Promise<boolean>}
 */
GDAL_ASYNCABLE_DEFINE(RasterBand::setMetadata) {
  NODE_UNWRAP_CHECK(RasterBand, info.This(), band);
  GDAL_RAW_CHECK(GDALRasterBand *, band, raw);

  auto options = make_shared<StringList>();
  if (info.Length() == 0 || options->parse(info[0])) {
    node_gdal::ThrowOverPending(node_gdal::napi_env(), "Failed parsing metadata");
    return node_gdal::napi_env().Undefined();
  }

  std::string domain("");
  NODE_ARG_OPT_STR(1, "domain", domain);

  GDALAsyncableJob<CPLErr> job(band->parent_uid);
  job.main = [raw, options, domain](const GDALExecutionProgress &) {
    CPLErr r = raw->SetMetadata(options->get(), domain.empty() ? nullptr : domain.c_str());
    if (r == CE_Failure) throw CPLGetLastErrorMsg();
    return r;
  };
  job.rval = [](CPLErr r, const GetFromPersistentFunc &) { return Napi::Boolean::New(node_gdal::napi_env(), r == CE_None); };
  return job.run(info, async, 2);
}

/**
 * @readonly
 * @kind member
 * @name ds
 * @instance
 * @memberof RasterBand
 * @type {Dataset}
 */
NAPI_GETTER(RasterBand::dsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "ds_");
}

/**
 * @readonly
 * @kind member
 * @name overviews
 * @instance
 * @memberof RasterBand
 * @type {RasterBandOverviews}
 */
NAPI_GETTER(RasterBand::overviewsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "overviews_");
}

/**
 * @readonly
 * @kind member
 * @name pixels
 * @instance
 * @memberof RasterBand
 * @type {RasterBandPixels}
 */
NAPI_GETTER(RasterBand::pixelsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "pixels_");
}

/**
 * @readonly
 * @kind member
 * @name id
 * @instance
 * @memberof RasterBand
 * @type {number|null}
 */

/**
 * @readonly
 * @kind member
 * @name idAsync
 * @instance
 * @memberof RasterBand
 * @type {Promise<number|null>}
 * @asyncGetter
 */

GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::idGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<int> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    CPLErrorReset();
    return raw->GetBand();
  };
  job.rval = [](int id, const GetFromPersistentFunc &) {
    if (id == 0) return node_gdal::napi_env().Null().As<Napi::Value>();
    return Napi::Number::New(node_gdal::napi_env(), id).As<Napi::Value>();
  };
  return job.run(info, async);
}

/**
 * Name of of band.
 *
 * @readonly
 * @kind member
 * @name description
 * @instance
 * @memberof RasterBand
 * @type {string}
 */

/**
 * Name of of band.
 * @asyncGetter
 *
 * @readonly
 * @kind member
 * @name descriptionAsync
 * @instance
 * @memberof RasterBand
 * @type {Promise<string>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::descriptionGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<const char *> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) { return raw->GetDescription(); };
  job.rval = [](const char *desc, const GetFromPersistentFunc &) { return SafeString::New(desc); };
  return job.run(info, async);
}

/**
 * Size object containing `"x"` and `"y"` properties.
 *
 * @readonly
 * @kind member
 * @name size
 * @instance
 * @memberof RasterBand
 * @type {xyz}
 */

/**
 * Size object containing `"x"` and `"y"` properties.
 * @asyncGetter
 *
 * @readonly
 * @kind member
 * @name sizeAsync
 * @instance
 * @memberof RasterBand
 * @type {Promise<xyz>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::sizeGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  struct xy {
    int x, y;
  };
  GDALAsyncableJob<xy> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    xy r;
    r.x = raw->GetXSize();
    r.y = raw->GetYSize();
    return r;
  };
  job.rval = [](xy r, const GetFromPersistentFunc &) {
    Napi::Object result = Napi::Object::New(node_gdal::napi_env());
    result.Set( Napi::String::New(node_gdal::napi_env(), "x"), Napi::Number::New(node_gdal::napi_env(), r.x));
    result.Set( Napi::String::New(node_gdal::napi_env(), "y"), Napi::Number::New(node_gdal::napi_env(), r.y));
    return result;
  };
  return job.run(info, async);
}

/**
 * Size object containing `"x"` and `"y"` properties.
 *
 * @readonly
 * @kind member
 * @name blockSize
 * @instance
 * @memberof RasterBand
 * @type {xyz}
 */

/**
 * Size object containing `"x"` and `"y"` properties.
 * @asyncGetter
 *
 * @readonly
 * @kind member
 * @name blockSizeAsync
 * @instance
 * @memberof RasterBand
 * @type {Promise<xyz>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::blockSizeGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  struct xy {
    int x, y;
  };
  GDALAsyncableJob<xy> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    xy r;
    raw->GetBlockSize(&r.x, &r.y);
    return r;
  };
  job.rval = [](xy r, const GetFromPersistentFunc &) {
    Napi::Object result = Napi::Object::New(node_gdal::napi_env());
    result.Set( Napi::String::New(node_gdal::napi_env(), "x"), Napi::Number::New(node_gdal::napi_env(), r.x));
    result.Set( Napi::String::New(node_gdal::napi_env(), "y"), Napi::Number::New(node_gdal::napi_env(), r.y));
    return result;
  };
  return job.run(info, async);
}

template <typename T> struct MaybeResult {
  T value;
  int success;
};

/**
 * Minimum value for this band.
 *
 * @readonly
 * @kind member
 * @name minimum
 * @instance
 * @memberof RasterBand
 * @type {number|null}
 */

/**
 * Minimum value for this band.
 * @asyncGetter
 *
 * @readonly
 * @kind member
 * @name minimumAsync
 * @instance
 * @memberof RasterBand
 * @type {Promise<number|null>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::minimumGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<MaybeResult<double>> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    MaybeResult<double> r;
    CPLErrorReset();
    r.value = raw->GetMinimum(&r.success);
    return r;
  };
  job.rval = [](MaybeResult<double> r, const GetFromPersistentFunc &) {
    if (r.success) return Napi::Number::New(node_gdal::napi_env(), r.value).As<Napi::Value>();
    return node_gdal::napi_env().Null().As<Napi::Value>();
  };
  return job.run(info, async);
}

/**
 * Maximum value for this band.
 *
 * @readonly
 * @kind member
 * @name maximum
 * @instance
 * @memberof RasterBand
 * @type {number|null}
 */

/**
 * Maximum value for this band.
 * @asyncGetter
 *
 * @readonly
 * @kind member
 * @name maximumAsync
 * @instance
 * @memberof RasterBand
 * @type {Promise<number|null>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::maximumGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<MaybeResult<double>> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    MaybeResult<double> r;
    CPLErrorReset();
    r.value = raw->GetMaximum(&r.success);
    return r;
  };
  job.rval = [](MaybeResult<double> r, const GetFromPersistentFunc &) {
    if (r.success) return Napi::Number::New(node_gdal::napi_env(), r.value).As<Napi::Value>();
    return node_gdal::napi_env().Null().As<Napi::Value>();
  };
  return job.run(info, async);
}

/**
 * Raster value offset.
 *
 * @kind member
 * @name offset
 * @instance
 * @memberof RasterBand
 * @type {number|null}
 */

/**
 * Raster value offset.
 * @asyncGetter
 *
 * @kind member
 * @name offsetAsync
 * @instance
 * @memberof RasterBand
 * @readonly
 * @type {Promise<number|null>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::offsetGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<MaybeResult<double>> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    MaybeResult<double> r;
    CPLErrorReset();
    r.value = raw->GetOffset(&r.success);
    return r;
  };
  job.rval = [](MaybeResult<double> r, const GetFromPersistentFunc &) {
    if (r.success) return Napi::Number::New(node_gdal::napi_env(), r.value).As<Napi::Value>();
    return node_gdal::napi_env().Null().As<Napi::Value>();
  };
  return job.run(info, async);
}

/**
 * Raster value scale.
 *
 * @kind member
 * @name scale
 * @instance
 * @memberof RasterBand
 * @type {number|null}
 */

/**
 * Raster value scale.
 * @asyncGetter
 *
 * @kind member
 * @name scaleAsync
 * @instance
 * @memberof RasterBand
 * @readonly
 * @type {Promise<number|null>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::scaleGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<MaybeResult<double>> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    MaybeResult<double> r;
    CPLErrorReset();
    r.value = raw->GetScale(&r.success);
    return r;
  };
  job.rval = [](MaybeResult<double> r, const GetFromPersistentFunc &) {
    if (r.success) return Napi::Number::New(node_gdal::napi_env(), r.value).As<Napi::Value>();
    return node_gdal::napi_env().Null().As<Napi::Value>();
  };
  return job.run(info, async);
}

/**
 * No data value for this band.
 *
 * @kind member
 * @name noDataValue
 * @instance
 * @memberof RasterBand
 * @type {number|null}
 */

/**
 * No data value for this band.
 * @asyncGetter
 *
 * @kind member
 * @name noDataValueAsync
 * @instance
 * @memberof RasterBand
 * @readonly
 * @type {Promise<number|null>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::noDataValueGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<MaybeResult<double>> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    MaybeResult<double> r;
    CPLErrorReset();
    r.value = raw->GetNoDataValue(&r.success);
    return r;
  };
  job.rval = [](MaybeResult<double> r, const GetFromPersistentFunc &) {
    if (r.success)
      return Napi::Number::New(node_gdal::napi_env(), r.value).As<Napi::Value>();
    else
      return node_gdal::napi_env().Null().As<Napi::Value>();
  };
  return job.run(info, async);
}

/**
 * Raster unit type (name for the units of this raster's values).
 * For instance, it might be `"m"` for an elevation model in meters,
 * or `"ft"` for feet. If no units are available, a value of `""`
 * will be returned.
 *
 * @kind member
 * @name unitType
 * @instance
 * @memberof RasterBand
 * @type {string|null}
 */

/**
 * Raster unit type (name for the units of this raster's values).
 * For instance, it might be `"m"` for an elevation model in meters,
 * or `"ft"` for feet. If no units are available, a value of `""`
 * will be returned.
 * @asyncGetter
 *
 * @kind member
 * @name unitTypeAsync
 * @instance
 * @memberof RasterBand
 * @readonly
 * @type {Promise<string|null>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::unitTypeGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<const char *> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    CPLErrorReset();
    return raw->GetUnitType();
  };
  job.rval = [](const char *r, const GetFromPersistentFunc &) { return SafeString::New(r); };
  return job.run(info, async);
}

/**
 *
 * Pixel data type ({@link GDT|see GDT constants} used for this band.
 *
 * @readonly
 * @kind member
 * @name dataType
 * @instance
 * @memberof RasterBand
 * @type {string|null}
 */

/**
 *
 * Pixel data type ({@link GDT|see GDT constants} used for this band.
 * @asyncGetter
 *
 * @readonly
 * @kind member
 * @name dataTypeAsync
 * @instance
 * @memberof RasterBand
 * @type {Promise<string|null>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::dataTypeGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<GDALDataType> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    CPLErrorReset();
    return raw->GetRasterDataType();
  };
  job.rval = [](GDALDataType type, const GetFromPersistentFunc &) {
    if (type == GDT_Unknown) return node_gdal::napi_env().Null().As<Napi::Value>();
    return SafeString::New(GDALGetDataTypeName(type));
  };
  return job.run(info, async);
}

/**
 * Indicates if the band is read-only.
 *
 * @readonly
 * @kind member
 * @name readOnly
 * @instance
 * @memberof RasterBand
 * @type {boolean}
 */

/**
 * Indicates if the band is read-only.
 * @asyncGetter
 *
 * @readonly
 * @kind member
 * @name readOnlyAsync
 * @instance
 * @memberof RasterBand
 * @type {Promise<boolean>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::readOnlyGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<GDALAccess> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    CPLErrorReset();
    return raw->GetAccess();
  };
  job.rval = [](GDALAccess r, const GetFromPersistentFunc &) { return (r == GA_Update ? Napi::Boolean::New(node_gdal::napi_env(), false) : Napi::Boolean::New(node_gdal::napi_env(), true)); };
  return job.run(info, async);
}

/**
 * An indicator if the underlying datastore can compute arbitrary overviews
 * efficiently, such as is the case with OGDI over a network. Datastores with
 * arbitrary overviews don't generally have any fixed overviews, but GDAL's
 * `RasterIO()` method can be used in downsampling mode to get overview
 * data efficiently.
 *
 * @readonly
 * @kind member
 * @name hasArbitraryOverviews
 * @instance
 * @memberof RasterBand
 * @type {boolean}
 */

/**
 * An indicator if the underlying datastore can compute arbitrary overviews
 * efficiently, such as is the case with OGDI over a network. Datastores with
 * arbitrary overviews don't generally have any fixed overviews, but GDAL's
 * `RasterIO()` method can be used in downsampling mode to get overview
 * data efficiently.
 * @asyncGetter
 *
 * @readonly
 * @kind member
 * @name hasArbitraryOverviewsAsync
 * @instance
 * @memberof RasterBand
 * @type {Promise<boolean>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::hasArbitraryOverviewsGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<bool> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    CPLErrorReset();
    return static_cast<bool>(raw->HasArbitraryOverviews());
  };
  job.rval = [](bool r, const GetFromPersistentFunc &) { return Napi::Boolean::New(node_gdal::napi_env(), r); };
  return job.run(info, async);
}

/**
 * List of list of category names for this raster.
 *
 * @kind member
 * @name categoryNames
 * @instance
 * @memberof RasterBand
 * @type {string[]}
 */

/**
 * List of list of category names for this raster.
 * @asyncGetter
 *
 * @kind member
 * @name categoryNamesAsync
 * @instance
 * @memberof RasterBand
 * @readonly
 * @type {Promise<string[]>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::categoryNamesGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<std::shared_ptr<std::vector<std::string>>> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) {
    auto names = std::make_shared<std::vector<std::string>>();
    CPLErrorReset();
    // Here we copy twice what we could have copied only once
    // In practice, GetCategoryNames always returns the same buffer
    // But in theory, a clever driver could implement some crazy lazy method
    // So we do everything "The Right Way"
    // This is not a performance-critical function anyway
    char **raw_names = raw->GetCategoryNames();
    int i = 0;

    while (raw_names != nullptr && raw_names[i] != nullptr) {
      names->push_back(raw_names[i]);
      i++;
    }
    return names;
  };
  job.rval = [](std::shared_ptr<std::vector<std::string>> names, const GetFromPersistentFunc &) {
    Napi::Array results = Napi::Array::New(node_gdal::napi_env());
    for (std::size_t i = 0; i < names->size(); ++i) results.Set( i, SafeString::New((*names.get())[i].c_str()));
    return results;
  };
  return job.run(info, async);
}

/**
 * Color interpretation mode ({@link GCI|see GCI constants})
 *
 * @kind member
 * @name colorInterpretation
 * @instance
 * @memberof RasterBand
 * @type {string|null}
 */

/**
 * Color interpretation mode ({@link GCI|see GCI constants})
 * @asyncGetter
 *
 * @kind member
 * @name colorInterpretationAsync
 * @instance
 * @memberof RasterBand
 * @readonly
 * @type {Promise<string|null>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::colorInterpretationGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);
  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<GDALColorInterp> job(band->parent_uid);
  job.persist("this", info.This());
  job.main = [raw](const GDALExecutionProgress &) { return raw->GetColorInterpretation(); };
  job.rval = [](GDALColorInterp ci, const GetFromPersistentFunc &) {
    if (ci == GCI_Undefined)
      return node_gdal::napi_env().Undefined().As<Napi::Value>();
    else
      return SafeString::New(GDALGetColorInterpretationName(ci));
  };

  return job.run(info, async);
}

NAPI_SETTER(RasterBand::unitTypeSetter) {
  NODE_UNWRAP_CHECK_VOID(RasterBand, info.This(), band);
  if (!value.IsString()) {
    Napi::Error::New(node_gdal::napi_env(), "Unit type must be a string").ThrowAsJavaScriptException();
    return;
  }
  std::string input = value.As<Napi::String>().Utf8Value();
  GDAL_LOCK_PARENT_VOID(band);
  CPLErr err = band->this_->SetUnitType(input.c_str());
  if (err) { NODE_THROW_LAST_CPLERR; }
}

NAPI_SETTER(RasterBand::noDataValueSetter) {
  NODE_UNWRAP_CHECK_VOID(RasterBand, info.This(), band);

  CPLErr err;
  GDAL_LOCK_PARENT_VOID(band);
  CPLErrorReset();
  if (value.IsNull() || value.IsUndefined()) {
    err = band->this_->DeleteNoDataValue();
  } else if (value.IsNumber()) {
    err = band->this_->SetNoDataValue(value.As<Napi::Number>().DoubleValue());
  } else {
    Napi::Error::New(node_gdal::napi_env(), "No data value must be a number").ThrowAsJavaScriptException();
    return;
  }

  if (err != CE_None) { NODE_THROW_LAST_CPLERR; }
}

NAPI_SETTER(RasterBand::scaleSetter) {
  NODE_UNWRAP_CHECK_VOID(RasterBand, info.This(), band);

  if (!value.IsNumber()) {
    Napi::Error::New(node_gdal::napi_env(), "Scale must be a number").ThrowAsJavaScriptException();
    return;
  }
  double input = value.As<Napi::Number>().DoubleValue();
  GDAL_LOCK_PARENT_VOID(band);
  CPLErr err = band->this_->SetScale(input);
  if (err) { NODE_THROW_LAST_CPLERR; }
}

NAPI_SETTER(RasterBand::offsetSetter) {
  NODE_UNWRAP_CHECK_VOID(RasterBand, info.This(), band);

  if (!value.IsNumber()) {
    Napi::Error::New(node_gdal::napi_env(), "Offset must be a number").ThrowAsJavaScriptException();
    return;
  }
  double input = value.As<Napi::Number>().DoubleValue();
  GDAL_LOCK_PARENT_VOID(band);
  CPLErr err = band->this_->SetOffset(input);
  if (err) { NODE_THROW_LAST_CPLERR; }
}

NAPI_SETTER(RasterBand::categoryNamesSetter) {
  NODE_UNWRAP_CHECK_VOID(RasterBand, info.This(), band);

  if (!value.IsArray()) {
    Napi::Error::New(node_gdal::napi_env(), "Category names must be an array").ThrowAsJavaScriptException();
    return;
  }
  Napi::Array names = value.As<Napi::Array>();

  char **list = NULL;
  std::shared_ptr<std::string[]> strlist;

  if (names.Length() > 0) {
    list = new char *[names.Length() + 1];
    strlist = std::shared_ptr<std::string[]>(new std::string[names.Length()]);
    unsigned int i;
    for (i = 0; i < names.Length(); i++) {
      strlist.get()[i] = names.As<Napi::Object>().Get(i).As<Napi::String>().Utf8Value();
      list[i] = (char *)strlist.get()[i].c_str();
    }
    list[i] = NULL;
  }

  GDAL_LOCK_PARENT_VOID(band);
  int err = band->this_->SetCategoryNames(list);

  if (list) { delete[] list; }

  if (err) { NODE_THROW_LAST_CPLERR; }
}

NAPI_SETTER(RasterBand::colorInterpretationSetter) {
  NODE_UNWRAP_CHECK_VOID(RasterBand, info.This(), band);

  GDALColorInterp ci = GCI_Undefined;

  if (value.IsString()) {
    std::string name = value.As<Napi::String>().Utf8Value();
    ci = GDALGetColorInterpretationByName(name.c_str());
  } else if (!value.IsNull() && !value.IsUndefined()) {
    Napi::Error::New(node_gdal::napi_env(), "color interpretation must be a string or undefined").ThrowAsJavaScriptException();
    return;
  }

  GDAL_LOCK_PARENT_VOID(band);
  CPLErr err = band->this_->SetColorInterpretation(ci);
  if (err) { NODE_THROW_LAST_CPLERR; }
}

/**
 * Color table ({@link ColorTable|see ColorTable})
 *
 * @kind member
 * @name colorTable
 * @instance
 * @memberof RasterBand
 * @type {ColorTable|null}
 */

/**
 * Color table ({@link ColorTable|see ColorTable})
 * @asyncGetter
 *
 * @kind member
 * @name colorTableAsync
 * @instance
 * @memberof RasterBand
 * @readonly
 * @type {Promise<ColorTable|null>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(RasterBand::colorTableGetter) {
  NODE_UNWRAP_CHECK_ASYNC(RasterBand, info.This(), band);

  GDAL_RAW_CHECK_ASYNC(GDALRasterBand *, band, raw);

  GDALAsyncableJob<GDALColorTable *> job(band->parent_uid);
  job.main = [raw](const GDALExecutionProgress &) { return raw->GetColorTable(); };
  job.rval = [](GDALColorTable *ct, const GetFromPersistentFunc &getter) {
    if (ct != nullptr) return ColorTable::New(ct, getter("this"));
    return node_gdal::napi_env().Undefined().As<Napi::Value>();
  };

  return job.run(info, async);
}

NAPI_SETTER(RasterBand::colorTableSetter) {

  NODE_UNWRAP_CHECK_VOID(RasterBand, info.This(), band);

  GDALColorTable *raw;
  if (value.IsNull() || value.IsUndefined()) {
    raw = nullptr;
  } else if (IS_WRAPPED(value, ColorTable)) {
    NODE_UNWRAP_CHECK_VOID(ColorTable, value.As<Napi::Object>(), ct);
    GDAL_RAW_CHECK_VOID(GDALColorTable *, ct, _raw);
    raw = _raw;
  } else {
    Napi::TypeError::New(node_gdal::napi_env(), "color table must be a gdal.ColorTable object or null").ThrowAsJavaScriptException();
    return;
  }

  GDAL_LOCK_PARENT_VOID(band);
  CPLErr err = band->this_->SetColorTable(raw);
  if (err != CE_None) { NODE_THROW_LAST_CPLERR; }
}

NAPI_GETTER(RasterBand::uidGetter) {
  RasterBand *band = node_gdal::UnwrapWrapped<RasterBand>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env(), (int)band->uid);
}

} // namespace node_gdal
