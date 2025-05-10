#include "gdal_utils.hpp"
#include "gdal_common.hpp"
#include "gdal_dataset.hpp"
#include "gdal_spatial_reference.hpp"

#if GDAL_VERSION_MAJOR > 2 || (GDAL_VERSION_MAJOR == 2 && GDAL_VERSION_MINOR >= 3)
#define GDALDatasetToHandle(x) GDALDataset::ToHandle(x)
#define GDALDatasetFromHandle(x) GDALDataset::FromHandle(x)
#else
#define GDALDatasetToHandle(x) static_cast<GDALDatasetH>(x)
#define GDALDatasetFromHandle(x) static_cast<GDALDataset *>(x)
#endif

namespace node_gdal {

void Utils::Initialize(Local<Object> target) {
  Nan__SetAsyncableMethod(target, "info", info);
  Nan__SetAsyncableMethod(target, "translate", translate);
  Nan__SetAsyncableMethod(target, "vectorTranslate", vectorTranslate);
  Nan__SetAsyncableMethod(target, "warp", warp);
  Nan__SetAsyncableMethod(target, "buildVRT", buildvrt);
  Nan__SetAsyncableMethod(target, "rasterize", rasterize);
  Nan__SetAsyncableMethod(target, "dem", dem);
}

/**
 * @typedef {object} UtilOptions
 * @property {ProgressCb} [progress_cb]
 */

/**
 * Library version of gdal_translate.
 *
 * @example
 * const ds = gdal.open('input.tif')
 * const out = gdal.translate('/vsimem/temp.tif', ds, [ '-b', '1' ])
 *
 * @throws {Error}
 * @method translate
 * @static
 * @param {string} destination destination filename
 * @param {Dataset} source source dataset
 * @param {string[]} [args] array of CLI options for gdal_translate
 * @param {UtilOptions} [options] additional options
 * @param {ProgressCb} [options.progress_cb]
 * @return {Dataset}
 */

/**
 * Library version of gdal_translate.
 * Remember that all library version tools return a live opened
 * dataset. If you need to use the file immediately following an
 * operation, you should flush/close it.
 * @async
 *
 * @example
 * const ds = gdal.open('input.tif')
 * const out = gdal.translate('/vsimem/temp.tif', ds, [ '-b', '1' ])
 * @throws {Error}
 *
 * @method translateAsync
 * @static
 * @param {string} destination destination filename
 * @param {Dataset} source source dataset
 * @param {string[]} [args] array of CLI options for gdal_translate
 * @param {UtilOptions} [options] additional options
 * @param {ProgressCb} [options.progress_cb]
 * @param {callback<Dataset>} [callback=undefined]
 * @return {Promise<Dataset>}
 */
GDAL_ASYNCABLE_DEFINE(Utils::translate) {
  auto aosOptions = std::make_shared<CPLStringList>();

  std::string dst;
  NODE_ARG_STR(0, "dst", dst);

  Local<Object> src;
  NODE_ARG_OBJECT(1, "src", src);
  NODE_UNWRAP_CHECK(Dataset, src, ds);
  GDAL_RAW_CHECK(GDALDataset *, ds, raw);

  Local<Array> args;
  NODE_ARG_ARRAY_OPT(2, "args", args);
  if (!args.IsEmpty())
    for (unsigned i = 0; i < args->Length(); ++i) {
      aosOptions->AddString(*Nan::Utf8String(Nan::Get(args, i).ToLocalChecked()));
    }

  Local<Object> options;
  Nan::Callback *progress_cb = nullptr;
  NODE_ARG_OBJECT_OPT(3, "options", options);
  if (!options.IsEmpty()) NODE_CB_FROM_OBJ_OPT(options, "progress_cb", progress_cb);

  GDALAsyncableJob<GDALDataset *> job(ds->uid);
  job.progress = progress_cb;
  job.main = [raw, dst, aosOptions, progress_cb](const GDALExecutionProgress &progress) {
    CPLErrorReset();
    auto b = aosOptions;
    auto psOptions = GDALTranslateOptionsNew(aosOptions->List(), nullptr);
    if (psOptions == nullptr) throw CPLGetLastErrorMsg();
    if (progress_cb) GDALTranslateOptionsSetProgress(psOptions, ProgressTrampoline, (void *)&progress);
    GDALDataset *r = GDALDatasetFromHandle(GDALTranslate(dst.c_str(), GDALDatasetToHandle(raw), psOptions, nullptr));
    GDALTranslateOptionsFree(psOptions);
    if (r == nullptr) throw CPLGetLastErrorMsg();
    return r;
  };
  job.rval = [](GDALDataset *ds, const GetFromPersistentFunc &) { return Dataset::New(ds); };

  job.run(info, async, 4);
}

/**
 * Library version of ogr2ogr.
 *
 * @example
 * const ds = gdal.open('input.geojson')
 * const out = gdal.vectorTranslate('/vsimem/temp.gpkg', ds, [ '-of', 'GPKG' ])
 *
 * @throws {Error}
 * @method vectorTranslate
 * @static
 * @param {string|Dataset} destination destination
 * @param {Dataset} source source dataset
 * @param {string[]} [args] array of CLI options for ogr2ogr
 * @param {UtilOptions} [options] additional options
 * @param {ProgressCb} [options.progress_cb]
 * @return {Dataset}
 */

/**
 * Library version of ogr2ogr.
 * @async
 *
 * @example
 * const ds = gdal.open('input.geojson')
 * const out = gdal.vectorTranslate('/vsimem/temp.gpkg', ds, [ '-of', 'GPKG' ])
 * @throws {Error}
 *
 * @method vectorTranslateAsync
 * @static
 * @param {string|Dataset} destination destination
 * @param {Dataset} source source dataset
 * @param {string[]} [args] array of CLI options for ogr2ogr
 * @param {UtilOptions} [options] additional options
 * @param {ProgressCb} [options.progress_cb]
 * @param {callback<Dataset>} [callback=undefined]
 * @return {Promise<Dataset>}
 */
GDAL_ASYNCABLE_DEFINE(Utils::vectorTranslate) {
  auto aosOptions = std::make_shared<CPLStringList>();

  std::string dst_filename("");
  Dataset *dst_ds = nullptr;
  if (info.Length() < 1) {
    Nan::ThrowError("\"dst\" must be given");
    return;
  }

  GDALDataset *dst_raw = nullptr;
  if (info[0]->IsString()) {
    NODE_ARG_STR(0, "dst", dst_filename);
  } else if (info[0]->IsObject()) {
    NODE_ARG_WRAPPED(0, "dst", Dataset, dst_ds);
    GDAL_RAW_CHECK(GDALDataset *, dst_ds, _dst_raw);
    dst_raw = _dst_raw;
  } else {
    Nan::ThrowError("\"dst\" must be an object or a gdal.Dataset");
    return;
  }

  Local<Object> src;
  NODE_ARG_OBJECT(1, "src", src);
  NODE_UNWRAP_CHECK(Dataset, src, ds);
  GDAL_RAW_CHECK(GDALDataset *, ds, src_raw);

  Local<Array> args;
  NODE_ARG_ARRAY_OPT(2, "args", args);
  if (!args.IsEmpty())
    for (unsigned i = 0; i < args->Length(); ++i) {
      aosOptions->AddString(*Nan::Utf8String(Nan::Get(args, i).ToLocalChecked()));
    }

  Local<Object> options;
  Nan::Callback *progress_cb = nullptr;
  NODE_ARG_OBJECT_OPT(3, "options", options);
  if (!options.IsEmpty()) NODE_CB_FROM_OBJ_OPT(options, "progress_cb", progress_cb);

  std::vector<long> uids = {ds->uid};
  if (dst_ds != nullptr) uids.push_back(dst_ds->uid);
  GDALAsyncableJob<GDALDataset *> job(uids);
  job.progress = progress_cb;

  job.main = [src_raw, dst_filename, dst_raw, aosOptions, progress_cb](const GDALExecutionProgress &progress) {
    CPLErrorReset();
    if (progress_cb) aosOptions->AddString("-progress");
    auto psOptions = GDALVectorTranslateOptionsNew(aosOptions->List(), nullptr);
    if (psOptions == nullptr) throw CPLGetLastErrorMsg();

    if (progress_cb) GDALVectorTranslateOptionsSetProgress(psOptions, ProgressTrampoline, (void *)&progress);

    auto srcH = GDALDatasetToHandle(src_raw);
    GDALDataset *r = GDALDatasetFromHandle(
      GDALVectorTranslate(dst_filename.c_str(), GDALDatasetToHandle(dst_raw), 1, &srcH, psOptions, nullptr));

    GDALVectorTranslateOptionsFree(psOptions);
    if (r == nullptr) throw CPLGetLastErrorMsg();
    return r;
  };
  job.rval = [](GDALDataset *ds, const GetFromPersistentFunc &) { return Dataset::New(ds); };

  job.run(info, async, 4);
}

/**
 * Library version of gdalinfo.
 *
 * @example
 * const output = gdal.info(gdal.open('input.tif'))
 * const output = gdal.info(gdal.open('/vsimem/temp.tif'), ['-json'])
 *
 * @throws {Error}
 * @method info
 * @static
 * @param {Dataset} dataset
 * @param {string[]} [args] array of CLI options for gdalinfo
 * @return {string}
 */

/**
 * Library version of gdalinfo.
 * @async
 *
 * @example
 * const ds = gdal.open('input.tif')
 * const output = gdal.info('/vsimem/temp.tif')
 * @throws {Error}
 *
 * @method infoAsync
 * @static
 * @param {Dataset} dataset
 * @param {string[]} [args] array of CLI options for gdalinfo
 * @param {callback<string>} [callback=undefined]
 * @return {Promise<string>}
 */
GDAL_ASYNCABLE_DEFINE(Utils::info) {
  auto aosOptions = std::make_shared<CPLStringList>();

  Local<Object> src;
  NODE_ARG_OBJECT(0, "src", src);
  NODE_UNWRAP_CHECK(Dataset, src, ds);
  GDAL_RAW_CHECK(GDALDataset *, ds, raw);

  Local<Array> args;
  NODE_ARG_ARRAY_OPT(1, "args", args);
  if (!args.IsEmpty())
    for (unsigned i = 0; i < args->Length(); ++i) {
      aosOptions->AddString(*Nan::Utf8String(Nan::Get(args, i).ToLocalChecked()));
    }

  GDALAsyncableJob<std::string> job(ds->uid);
  job.main = [raw, aosOptions](const GDALExecutionProgress &) {
    CPLErrorReset();
    auto psOptions = GDALInfoOptionsNew(aosOptions->List(), nullptr);
    if (psOptions == nullptr) throw CPLGetLastErrorMsg();
    char *r = GDALInfo(GDALDatasetToHandle(raw), psOptions);
    GDALInfoOptionsFree(psOptions);
    if (r == nullptr) throw CPLGetLastErrorMsg();
    std::string s(r);
    CPLFree(r);
    return s;
  };
  job.rval = [](std::string s, const GetFromPersistentFunc &) { return SafeString::New(s.c_str()); };

  job.run(info, async, 2);
}

/**
 * Library version of gdalwarp.
 *
 * @example
 * const ds = gdal.open('input.tif')
 * const output = gdal.warp('/vsimem/output.tiff', null, [ ds ], [ '-t_srs', 'epsg:3587' ])
 *
 * @throws {Error}
 * @method warp
 * @static
 * @param {string|null} dst_path destination path, null for an in-memory operation
 * @param {Dataset|null} dst_ds destination dataset, null for a new dataset
 * @param {Dataset[]} src_ds array of source datasets
 * @param {string[]} [args] array of CLI options for gdalwarp
 * @param {UtilOptions} [options] additional options
 * @param {ProgressCb} [options.progress_cb]
 * @return {Dataset}
 */

/**
 * Library version of gdalwarp.
 * @async
 *
 * @example
 * const ds = await gdal.openAsync('input.tif')
 * const output = await gdal.warpAsync('/vsimem/output.tiff', null, [ ds ], [ '-t_srs', 'epsg:3587' ])
 * @throws {Error}
 *
 * @method warpAsync
 * @static
 * @param {string|null} dst_path destination path, null for an in-memory operation
 * @param {Dataset|null} dst_ds destination dataset, null for a new dataset
 * @param {Dataset[]} src_ds array of source datasets
 * @param {string[]} [args] array of CLI options for gdalwarp
 * @param {UtilOptions} [options] additional options
 * @param {ProgressCb} [options.progress_cb]
 * @param {callback<Dataset>} [callback=undefined]
 * @return {Promise<Dataset>}
 */
GDAL_ASYNCABLE_DEFINE(Utils::warp) {
  auto aosOptions = std::make_shared<CPLStringList>();
  std::vector<long> uids;

  std::string dst_path("");
  Local<Object> dst_ds;

  NODE_ARG_OPT_STR(0, "dst_path", dst_path);

  NODE_ARG_OBJECT_OPT(1, "dst_ds", dst_ds);
  GDALDatasetH gdal_dst_ds = nullptr;
  if (!dst_ds.IsEmpty()) {
    NODE_UNWRAP_CHECK(Dataset, dst_ds, ds);
    GDAL_RAW_CHECK(GDALDataset *, ds, raw);
    gdal_dst_ds = GDALDatasetToHandle(raw);
    uids.push_back(ds->uid);
  }

  if (dst_path.length() == 0 && gdal_dst_ds == nullptr) {
    Nan::ThrowError("Either \"dst_path\" or \"dst_ds\" must be given");
    return;
  }

  Local<Array> src_ds;
  NODE_ARG_ARRAY(2, "src_ds", src_ds);
  if (src_ds->Length() < 1) {
    Nan::ThrowError("\"src_ds\" must contain at least one element");
    return;
  }
  auto gdal_src_ds = std::shared_ptr<GDALDatasetH>(new GDALDatasetH[src_ds->Length()], array_deleter<GDALDatasetH>());
  for (unsigned i = 0; i < src_ds->Length(); ++i) {
    NODE_UNWRAP_CHECK(Dataset, Nan::Get(src_ds, i).ToLocalChecked().As<Object>(), ds);
    GDAL_RAW_CHECK(GDALDataset *, ds, raw);
    gdal_src_ds.get()[i] = GDALDatasetToHandle(raw);
    uids.push_back(ds->uid);
  }

  Local<Array> args;
  NODE_ARG_ARRAY_OPT(3, "args", args);
  if (!args.IsEmpty())
    for (unsigned i = 0; i < args->Length(); ++i) {
      aosOptions->AddString(*Nan::Utf8String(Nan::Get(args, i).ToLocalChecked()));
    }

  Local<Object> options;
  Nan::Callback *progress_cb = nullptr;
  NODE_ARG_OBJECT_OPT(4, "options", options);
  if (!options.IsEmpty()) NODE_CB_FROM_OBJ_OPT(options, "progress_cb", progress_cb);

  GDALAsyncableJob<GDALDataset *> job(uids);
  int src_count = src_ds->Length();
  job.progress = progress_cb;
  job.main =
    [dst_path, gdal_dst_ds, src_count, gdal_src_ds, aosOptions, progress_cb](const GDALExecutionProgress &progress) {
      CPLErrorReset();
      auto psOptions = GDALWarpAppOptionsNew(aosOptions->List(), nullptr);
      if (psOptions == nullptr) throw CPLGetLastErrorMsg();
      if (progress_cb) GDALWarpAppOptionsSetProgress(psOptions, ProgressTrampoline, (void *)&progress);
      GDALDatasetH r = GDALWarp(
        dst_path.length() > 0 ? dst_path.c_str() : nullptr,
        gdal_dst_ds,
        src_count,
        gdal_src_ds.get(),
        psOptions,
        nullptr);
      GDALWarpAppOptionsFree(psOptions);
      if (r == nullptr) throw CPLGetLastErrorMsg();
      return GDALDatasetFromHandle(r);
    };
  job.rval = [](GDALDataset *ds, const GetFromPersistentFunc &) { return Dataset::New(ds); };

  job.run(info, async, 5);
}

/**
 * Library version of gdalbuildvrt.
 *
 * @example
 * const ds1 = gdal.buildVRT('/vsimem/target.tiff',
 *    [ 'input1.tif', 'input2.tif' ],
 *    [ '-resolution', 'highest' ] );
 *
 * const ds2 = gdal.buildVRT('/vsimem/target.tiff',
 *    [ gdal.open('input1.tif'), gdal.open('input2.tif') ],
 *    [ '-resolution', 'highest' ] );
 *
 * @throws {Error}
 * @method buildVRT
 * @static
 * @param {string|null} dst_path destination path, null for an in-memory operation
 * @param {Dataset[]|string[]} src_ds array of source datasets
 * @param {string[]} [args] array of CLI options for gdalbuildvrt
 * @param {UtilOptions} [options] additional options
 * @param {ProgressCb} [options.progress_cb]
 * @return {Dataset}
 */

/**
 * Library version of gdalbuildvrt.
 * @async
 *
 * @example
 * const ds1 = await gdal.buildVRTAsync('/vsimem/target.tiff',
 *    [ 'input1.tif', 'input2.tif' ],
 *    [ '-resolution', 'highest' ] );
 *
 * const ds2 = gdal.buildVRT('/vsimem/target.tiff',
 *    [ await gdal.openAsync('input1.tif'), await gdal.openAsync('input2.tif') ],
 *    [ '-resolution', 'highest' ] );
 *
 * @throws {Error}
 *
 * @method buildVRTAsync
 * @static
 * @param {string|null} dst_path destination path, null for an in-memory operation
 * @param {Dataset[]|string[]} src_ds array of source datasets
 * @param {string[]} [args] array of CLI options for gdalbuildvrt
 * @param {UtilOptions} [options] additional options
 * @param {ProgressCb} [options.progress_cb]
 * @param {callback<Dataset>} [callback=undefined]
 * @return {Promise<Dataset>}
 */
GDAL_ASYNCABLE_DEFINE(Utils::buildvrt) {
  auto aosOptions = std::make_shared<CPLStringList>();
  std::vector<long> uids;

  std::string dst_path("");

  NODE_ARG_STR(0, "dst_path", dst_path);

  Local<Array> src_ds;
  NODE_ARG_ARRAY(1, "src_ds", src_ds);
  if (src_ds->Length() < 1) {
    Nan::ThrowError("\"src_ds\" must contain at least one element");
    return;
  }

  std::shared_ptr<CPLStringList> aosSrcDs = nullptr;
  std::shared_ptr<GDALDatasetH> gdalSrcDs = nullptr;
  if (Nan::Get(src_ds, 0).ToLocalChecked()->IsString()) {
    aosSrcDs = std::make_shared<CPLStringList>();
    for (unsigned i = 0; i < src_ds->Length(); ++i) {
      if (!Nan::Get(src_ds, i).ToLocalChecked()->IsString()) {
        Nan::ThrowError("All \"src_ds\" elements must have the same type");
        return;
      }
      aosSrcDs->AddString(*Nan::Utf8String(Nan::Get(src_ds, i).ToLocalChecked()));
    }
    uids.push_back(0);
  } else {
    gdalSrcDs = std::shared_ptr<GDALDatasetH>(new GDALDatasetH[src_ds->Length()], array_deleter<GDALDatasetH>());
    for (unsigned i = 0; i < src_ds->Length(); ++i) {
      Local<Value> v = Nan::Get(src_ds, i).ToLocalChecked();
      NODE_UNWRAP_CHECK(Dataset, v, ds);
      GDAL_RAW_CHECK(GDALDataset *, ds, raw);
      gdalSrcDs.get()[i] = GDALDatasetToHandle(raw);
      uids.push_back(ds->uid);
    }
  }

  Local<Array> args;
  NODE_ARG_ARRAY_OPT(2, "args", args);
  if (!args.IsEmpty())
    for (unsigned i = 0; i < args->Length(); ++i) {
      aosOptions->AddString(*Nan::Utf8String(Nan::Get(args, i).ToLocalChecked()));
    }

  Local<Object> options;
  Nan::Callback *progress_cb = nullptr;
  NODE_ARG_OBJECT_OPT(3, "options", options);
  if (!options.IsEmpty()) NODE_CB_FROM_OBJ_OPT(options, "progress_cb", progress_cb);

  GDALAsyncableJob<GDALDataset *> job(uids);
  int src_count = src_ds->Length();
  job.progress = progress_cb;
  job.main =
    [dst_path, src_count, gdalSrcDs, aosSrcDs, aosOptions, progress_cb](const GDALExecutionProgress &progress) {
      CPLErrorReset();
      auto psOptions = GDALBuildVRTOptionsNew(aosOptions->List(), nullptr);
      if (psOptions == nullptr) throw CPLGetLastErrorMsg();
      if (progress_cb) GDALBuildVRTOptionsSetProgress(psOptions, ProgressTrampoline, (void *)&progress);

      GDALDatasetH r = GDALBuildVRT(
        dst_path.c_str(),
        src_count,
        gdalSrcDs.get(),
        aosSrcDs.get() != nullptr ? aosSrcDs->List() : nullptr,
        psOptions,
        nullptr);

      GDALBuildVRTOptionsFree(psOptions);
      if (r == nullptr) throw CPLGetLastErrorMsg();
      return GDALDatasetFromHandle(r);
    };
  job.rval = [](GDALDataset *ds, const GetFromPersistentFunc &) { return Dataset::New(ds); };

  job.run(info, async, 4);
}

/**
 * Library version of gdal_rasterize.
 *
 * @example
 * const ds1 = gdal.rasterize('/vsimem/target.tiff',
 *    src_ds,
 *    [ '-b', '1' ] );
 *
 * const ds2 = gdal.rasterize(dst_ds,
 *    src_ds,
 *    [ '-b', '1' ] );
 *
 * @throws {Error}
 * @method rasterize
 * @static
 * @param {string|Dataset} destination
 * @param {Dataset} source dataset
 * @param {string[]} [args] array of CLI options for gdal_rasterize
 * @param {UtilOptions} [options] additional options
 * @param {ProgressCb} [options.progress_cb]
 * @return {Dataset}
 */

/**
 * Library version of gdal_rasterize.
 * @async
 *
 * @example
 * const ds1 = await gdal.rasterizeAsync('/vsimem/target.tiff',
 *    src_ds,
 *    [ '-b', '1' ] );
 *
 * const ds2 = await gdal.rasterizeAsync(dst_ds,
 *    src_ds,
 *    [ '-b', '1' ] );
 *
 * @throws {Error}
 * @method rasterizeAsync
 * @static
 * @param {string|Dataset} destination
 * @param {Dataset} source dataset
 * @param {string[]} [args] array of CLI options for gdal_rasterize
 * @param {UtilOptions} [options] additional options
 * @param {ProgressCb} [options.progress_cb]
 * @param {callback<Dataset>} [callback=undefined]
 * @return {Promise<Dataset>}
 */
GDAL_ASYNCABLE_DEFINE(Utils::rasterize) {
  auto aosOptions = std::make_shared<CPLStringList>();

  std::string dst_path("");
  Local<Object> dst_ds;

  GDALDataset *dst_raw = nullptr;
  if (info.Length() > 1 && info[0]->IsString()) {
    NODE_ARG_STR(0, "dst", dst_path);
  } else if (info.Length() > 1 && info[0]->IsObject()) {
    NODE_ARG_OBJECT(0, "dst", dst_ds);
    NODE_UNWRAP_CHECK(Dataset, dst_ds, dst_obj);
    GDAL_RAW_CHECK(GDALDataset *, dst_obj, _dst_raw);
    dst_raw = _dst_raw;
  } else {
    Nan::ThrowError("dst must be given");
    return;
  }

  Local<Object> src;
  NODE_ARG_OBJECT(1, "src", src);
  NODE_UNWRAP_CHECK(Dataset, src, ds);
  GDAL_RAW_CHECK(GDALDataset *, ds, src_raw);

  Local<Array> args;
  NODE_ARG_ARRAY_OPT(2, "args", args);
  if (!args.IsEmpty())
    for (unsigned i = 0; i < args->Length(); ++i) {
      aosOptions->AddString(*Nan::Utf8String(Nan::Get(args, i).ToLocalChecked()));
    }

  Local<Object> options;
  Nan::Callback *progress_cb = nullptr;
  NODE_ARG_OBJECT_OPT(3, "options", options);
  if (!options.IsEmpty()) NODE_CB_FROM_OBJ_OPT(options, "progress_cb", progress_cb);

  GDALAsyncableJob<GDALDataset *> job(ds->uid);
  job.progress = progress_cb;
  job.main = [dst_path, dst_raw, src_raw, aosOptions, progress_cb](const GDALExecutionProgress &progress) {
    CPLErrorReset();
    auto psOptions = GDALRasterizeOptionsNew(aosOptions->List(), nullptr);
    if (psOptions == nullptr) throw CPLGetLastErrorMsg();
    if (progress_cb) GDALRasterizeOptionsSetProgress(psOptions, ProgressTrampoline, (void *)&progress);

    GDALDatasetH r = GDALRasterize(
      dst_path.length() > 0 ? dst_path.c_str() : nullptr,
      dst_raw != nullptr ? dst_raw : nullptr,
      src_raw,
      psOptions,
      nullptr);

    GDALRasterizeOptionsFree(psOptions);
    if (r == nullptr) throw CPLGetLastErrorMsg();
    return GDALDatasetFromHandle(r);
  };
  job.rval = [](GDALDataset *ds, const GetFromPersistentFunc &) { return Dataset::New(ds); };

  job.run(info, async, 4);
}

/**
 * Library version of gdaldem.
 *
 * @example
 * const ds = gdal.open('input.tif')
 * const output = gdal.dem('/vsimem/output.tiff', ds, 'hillshade',  [ '-z', '2' ])
 *
 * @throws {Error}
 * @method dem
 * @static
 * @param {string} dst_path destination path
 * @param {Dataset} src_ds source dataset
 * @param {'hillshade'|'slope'|'aspect'|'color-relief'|'TRI'|'TPI'|'Roughness'} mode processing mode
 * @param {string[]} [args] array of CLI options for gdaldem
 * @param {string} [colorFile] optional color filename, see https://gdal.org/programs/gdaldem.html for more details
 * @param {UtilOptions} [options] additional options
 * @param {ProgressCb} [options.progress_cb]
 * @return {Dataset}
 */

/**
 * Library version of gdaldem.
 * @async
 *
 * @example
 * const ds = await gdal.openAsync('input.tif')
 * const output = await gdal.demAsync('/vsimem/output.tiff', ds, 'hillshade', [ '-z', '2' ])
 *
 * @throws {Error}
 * @method demAsync
 * @static
 * @param {string} dst_path destination path
 * @param {Dataset} src_ds source dataset
 * @param {'hillshade'|'slope'|'aspect'|'color-relief'|'TRI'|'TPI'|'Roughness'} mode processing mode
 * @param {string[]} [args] array of CLI options for gdaldem
 * @param {string} [colorFile] optional color filename, see https://gdal.org/programs/gdaldem.html for more details
 * @param {UtilOptions} [options] additional options
 * @param {ProgressCb} [options.progress_cb]
 * @param {callback<Dataset>} [callback=undefined]
 * @return {Promise<Dataset>}
 */
GDAL_ASYNCABLE_DEFINE(Utils::dem) {
  auto aosOptions = std::make_shared<CPLStringList>();

  std::string dst_path;
  NODE_ARG_STR(0, "dst_path", dst_path);

  Local<Object> src;
  NODE_ARG_OBJECT(1, "src", src);
  NODE_UNWRAP_CHECK(Dataset, src, ds);
  GDAL_RAW_CHECK(GDALDataset *, ds, raw);

  std::string mode;
  NODE_ARG_STR(2, "mode", mode);

  Local<Array> args;
  NODE_ARG_ARRAY_OPT(3, "args", args);
  if (!args.IsEmpty()) {
    for (unsigned i = 0; i < args->Length(); ++i) {
      aosOptions->AddString(*Nan::Utf8String(Nan::Get(args, i).ToLocalChecked()));
    }
  }

  std::string colorFilename = "";
  NODE_ARG_OPT_STR(4, "colorFilename", colorFilename);

  Local<Object> options;
  Nan::Callback *progress_cb = nullptr;
  NODE_ARG_OBJECT_OPT(5, "options", options);
  if (!options.IsEmpty()) NODE_CB_FROM_OBJ_OPT(options, "progress_cb", progress_cb);

  GDALAsyncableJob<GDALDataset *> job(ds->uid);
  job.progress = progress_cb;
  job.main = [dst_path, mode, raw, colorFilename, aosOptions, progress_cb](const GDALExecutionProgress &progress) {
    CPLErrorReset();
    auto psOptions = GDALDEMProcessingOptionsNew(aosOptions->List(), nullptr);
    if (psOptions == nullptr) throw CPLGetLastErrorMsg();
    if (progress_cb) GDALDEMProcessingOptionsSetProgress(psOptions, ProgressTrampoline, (void *)&progress);
    GDALDataset *r = GDALDatasetFromHandle(GDALDEMProcessing(
      dst_path.c_str(),
      GDALDatasetToHandle(raw),
      mode.c_str(),
      colorFilename.size() > 0 ? colorFilename.c_str() : nullptr,
      psOptions,
      nullptr));

    GDALDEMProcessingOptionsFree(psOptions);
    if (r == nullptr) throw CPLGetLastErrorMsg();
    return GDALDatasetFromHandle(r);
  };
  job.rval = [](GDALDataset *ds, const GetFromPersistentFunc &) { return Dataset::New(ds); };

  job.run(info, async, 6);
}

} // namespace node_gdal
