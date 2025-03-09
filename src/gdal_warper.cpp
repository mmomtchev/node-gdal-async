#include "gdal_warper.hpp"
#include "gdal_common.hpp"
#include "gdal_dataset.hpp"
#include "gdal_spatial_reference.hpp"
#include "utils/warp_options.hpp"

namespace node_gdal {

void Warper::Initialize(Local<Object> target) {
  Nan__SetAsyncableMethod(target, "reprojectImage", reprojectImage);
  Nan__SetAsyncableMethod(target, "suggestedWarpOutput", suggestedWarpOutput);
}

/**
 * @typedef {object} ReprojectOptions
 * @property {Dataset} src
 * @property {Dataset} dst
 * @property {SpatialReference} s_srs
 * @property {SpatialReference} t_srs
 * @property {string} [resampling]
 * @property {Geometry} [cutline]
 * @property {number[]} [srcBands]
 * @property {number[]} [dstBands]
 * @property {number} [srcAlphaBand]
 * @property {number} [dstAlphaBand]
 * @property {number} [srcNodata]
 * @property {number} [dstNodata]
 * @property {number} [blend]
 * @property {number} [memoryLimit]
 * @property {number} [maxError]
 * @property {boolean} [multi]
 * @property {object} [options]
 * @property {ProgressCb} [progress_cb]
 */

/*
 * GDALReprojectImage() method with a ChunkAndWarpImage replaced with
 * ChunkAndWarpMulti.
 */
CPLErr GDALReprojectImageMulti(
  GDALDatasetH hSrcDS,
  const char *pszSrcWKT,
  GDALDatasetH hDstDS,
  const char *pszDstWKT,
  GDALResampleAlg eResampleAlg,
  double dfWarpMemoryLimit,
  double dfMaxError,
  GDALProgressFunc pfnProgress,
  void *pProgressArg,
  GDALWarpOptions *psOptions)

{
  GDALWarpOptions *psWOptions;

  /* -------------------------------------------------------------------- */
  /*      Setup a reprojection based transformer.                         */
  /* -------------------------------------------------------------------- */
  void *hTransformArg;

  hTransformArg = GDALCreateGenImgProjTransformer(hSrcDS, pszSrcWKT, hDstDS, pszDstWKT, TRUE, 1000.0, 0);

  if (hTransformArg == NULL) return CE_Failure;

  /* -------------------------------------------------------------------- */
  /*      Create a copy of the user provided options, or a defaulted      */
  /*      options structure.                                              */
  /* -------------------------------------------------------------------- */
  if (psOptions == NULL)
    psWOptions = GDALCreateWarpOptions();
  else
    psWOptions = GDALCloneWarpOptions(psOptions);

  psWOptions->eResampleAlg = eResampleAlg;

  /* -------------------------------------------------------------------- */
  /*      Set transform.                                                  */
  /* -------------------------------------------------------------------- */
  if (dfMaxError > 0.0) {
    psWOptions->pTransformerArg = GDALCreateApproxTransformer(GDALGenImgProjTransform, hTransformArg, dfMaxError);

    psWOptions->pfnTransformer = GDALApproxTransform;
  } else {
    psWOptions->pfnTransformer = GDALGenImgProjTransform;
    psWOptions->pTransformerArg = hTransformArg;
  }

  /* -------------------------------------------------------------------- */
  /*      Set file and band mapping.                                      */
  /* -------------------------------------------------------------------- */
  int iBand;

  psWOptions->hSrcDS = hSrcDS;
  psWOptions->hDstDS = hDstDS;

  if (psWOptions->nBandCount == 0) {
    psWOptions->nBandCount = MIN(GDALGetRasterCount(hSrcDS), GDALGetRasterCount(hDstDS));

    psWOptions->panSrcBands = (int *)CPLMalloc(sizeof(int) * psWOptions->nBandCount);
    psWOptions->panDstBands = (int *)CPLMalloc(sizeof(int) * psWOptions->nBandCount);

    for (iBand = 0; iBand < psWOptions->nBandCount; iBand++) {
      psWOptions->panSrcBands[iBand] = iBand + 1;
      psWOptions->panDstBands[iBand] = iBand + 1;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Set source nodata values if the source dataset seems to have    */
  /*      any. Same for target nodata values                              */
  /* -------------------------------------------------------------------- */
  for (iBand = 0; iBand < psWOptions->nBandCount; iBand++) {
    GDALRasterBandH hBand = GDALGetRasterBand(hSrcDS, iBand + 1);
    int bGotNoData = FALSE;
    double dfNoDataValue;

    if (GDALGetRasterColorInterpretation(hBand) == GCI_AlphaBand) { psWOptions->nSrcAlphaBand = iBand + 1; }

    dfNoDataValue = GDALGetRasterNoDataValue(hBand, &bGotNoData);
    if (bGotNoData) {
      if (psWOptions->padfSrcNoDataReal == NULL) {
        int ii;

        psWOptions->padfSrcNoDataReal = (double *)CPLMalloc(sizeof(double) * psWOptions->nBandCount);
        psWOptions->padfSrcNoDataImag = (double *)CPLMalloc(sizeof(double) * psWOptions->nBandCount);

        for (ii = 0; ii < psWOptions->nBandCount; ii++) {
          psWOptions->padfSrcNoDataReal[ii] = -1.1e20;
          psWOptions->padfSrcNoDataImag[ii] = 0.0;
        }
      }

      psWOptions->padfSrcNoDataReal[iBand] = dfNoDataValue;
    }

    // Deal with target band
    hBand = GDALGetRasterBand(hDstDS, iBand + 1);
    if (hBand && GDALGetRasterColorInterpretation(hBand) == GCI_AlphaBand) { psWOptions->nDstAlphaBand = iBand + 1; }

    dfNoDataValue = GDALGetRasterNoDataValue(hBand, &bGotNoData);
    if (bGotNoData) {
      if (psWOptions->padfDstNoDataReal == NULL) {
        int ii;

        psWOptions->padfDstNoDataReal = (double *)CPLMalloc(sizeof(double) * psWOptions->nBandCount);
        psWOptions->padfDstNoDataImag = (double *)CPLMalloc(sizeof(double) * psWOptions->nBandCount);

        for (ii = 0; ii < psWOptions->nBandCount; ii++) {
          psWOptions->padfDstNoDataReal[ii] = -1.1e20;
          psWOptions->padfDstNoDataImag[ii] = 0.0;
        }
      }

      psWOptions->padfDstNoDataReal[iBand] = dfNoDataValue;
    }
  }

  /* -------------------------------------------------------------------- */
  /*      Set the progress function.                                      */
  /* -------------------------------------------------------------------- */
  if (pfnProgress != nullptr) {
    psWOptions->pfnProgress = pfnProgress;
    psWOptions->pProgressArg = pProgressArg;
  }

  /* -------------------------------------------------------------------- */
  /*      Create a warp options based on the options.                     */
  /* -------------------------------------------------------------------- */
  GDALWarpOperation oWarper;
  CPLErr eErr;

  eErr = oWarper.Initialize(psWOptions);

  if (eErr == CE_None) eErr = oWarper.ChunkAndWarpMulti(0, 0, GDALGetRasterXSize(hDstDS), GDALGetRasterYSize(hDstDS));

  /* -------------------------------------------------------------------- */
  /*      Cleanup.                                                        */
  /* -------------------------------------------------------------------- */
  GDALDestroyGenImgProjTransformer(hTransformArg);

  if (dfMaxError > 0.0) GDALDestroyApproxTransformer(psWOptions->pTransformerArg);

  GDALDestroyWarpOptions(psWOptions);

  return eErr;
}

/**
 * Reprojects a dataset.
 *
 * @throws {Error}
 * @method reprojectImage
 * @static
 * @param {ReprojectOptions} options
 * @param {Dataset} options.src
 * @param {Dataset} options.dst
 * @param {SpatialReference} options.s_srs
 * @param {SpatialReference} options.t_srs
 * @param {string} [options.resampling] Resampling algorithm ({@link GRA|available options})
 * @param {Geometry} [options.cutline] Must be in src dataset pixel coordinates. Use CoordinateTransformation to convert between georeferenced coordinates and pixel coordinates
 * @param {number[]} [options.srcBands]
 * @param {number[]} [options.dstBands]
 * @param {number} [options.srcAlphaBand]
 * @param {number} [options.dstAlphaBand]
 * @param {number} [options.srcNodata]
 * @param {number} [options.dstNodata]
 * @param {number} [options.memoryLimit]
 * @param {number} [options.maxError]
 * @param {boolean} [options.multi]
 * @param {string[]|object} [options.options] Warp options (see: [reference](https://gdal.org/doxygen/structGDALWarpOptions.html))
 * @param {ProgressCb} [options.progress_cb]
 */

/**
 * Reprojects a dataset.
 * @async
 *
 * @throws {Error}
 * @method reprojectImageAsync
 * @static
 * @param {ReprojectOptions} options
 * @param {Dataset} options.src
 * @param {Dataset} options.dst
 * @param {SpatialReference} options.s_srs
 * @param {SpatialReference} options.t_srs
 * @param {string} [options.resampling] Resampling algorithm ({@link GRA|available options})
 * @param {Geometry} [options.cutline] Must be in src dataset pixel coordinates. Use CoordinateTransformation to convert between georeferenced coordinates and pixel coordinates
 * @param {number[]} [options.srcBands]
 * @param {number[]} [options.dstBands]
 * @param {number} [options.srcAlphaBand]
 * @param {number} [options.dstAlphaBand]
 * @param {number} [options.srcNodata]
 * @param {number} [options.dstNodata]
 * @param {number} [options.memoryLimit]
 * @param {number} [options.maxError]
 * @param {boolean} [options.multi]
 * @param {string[]|object} [options.options] Warp options (see:[reference](https://gdal.org/doxygen/structGDALWarpOptions.html)
 * @param {ProgressCb} [options.progress_cb]
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 */
GDAL_ASYNCABLE_DEFINE(Warper::reprojectImage) {

  Local<Object> obj;

  auto options = std::make_shared<WarpOptions>();
  GDALWarpOptions *opts;
  std::string s_srs_str;
  std::string t_srs_str;
  SpatialReference *s_srs;
  SpatialReference *t_srs;
  double maxError = 0;
  Nan::Callback *progress_cb = nullptr;

  NODE_ARG_OBJECT(0, "Warp options", obj);

  if (options->parse(obj)) {
    return; // error parsing options object
  } else {
    opts = options->get();
  }
  if (!opts->hDstDS) {
    Nan::ThrowTypeError("dst Dataset must be provided");
    return;
  }

  NODE_WRAPPED_FROM_OBJ(obj, "s_srs", SpatialReference, s_srs);
  NODE_WRAPPED_FROM_OBJ(obj, "t_srs", SpatialReference, t_srs);
  NODE_DOUBLE_FROM_OBJ_OPT(obj, "maxError", maxError);
  NODE_CB_FROM_OBJ_OPT(obj, "progress_cb", progress_cb);

  char *s_srs_wkt, *t_srs_wkt;
  if (s_srs->get()->exportToWkt(&s_srs_wkt)) {
    Nan::ThrowError("Error converting s_srs to WKT");
    return;
  }
  s_srs_str = std::string(s_srs_wkt);
  CPLFree(s_srs_wkt);
  if (t_srs->get()->exportToWkt(&t_srs_wkt)) {
    Nan::ThrowError("Error converting t_srs to WKT");
    return;
  }
  t_srs_str = std::string(t_srs_wkt);
  CPLFree(t_srs_wkt);

  std::vector<long> uids = options->datasetUids();
  GDALAsyncableJob<CPLErr> job(uids);

  job.progress = progress_cb;

  // opts is a pointer inside options memory space
  // the lifetime of the options shared_ptr is limited by the lifetime of the lambda
  if (options->useMultithreading()) {
    job.main = [options, opts, s_srs_str, t_srs_str, maxError, progress_cb](const GDALExecutionProgress &progress) {
      CPLErrorReset();
      CPLErr err = GDALReprojectImageMulti(
        opts->hSrcDS,
        s_srs_str.c_str(),
        opts->hDstDS,
        t_srs_str.c_str(),
        opts->eResampleAlg,
        opts->dfWarpMemoryLimit,
        maxError,
        progress_cb ? ProgressTrampoline : nullptr,
        progress_cb ? (void *)&progress : nullptr,
        opts);
      if (err) { throw CPLGetLastErrorMsg(); }
      return err;
    };
  } else {
    job.main = [options, opts, s_srs_str, t_srs_str, maxError, progress_cb](const GDALExecutionProgress &progress) {
      CPLErrorReset();
      CPLErr err = GDALReprojectImage(
        opts->hSrcDS,
        s_srs_str.c_str(),
        opts->hDstDS,
        t_srs_str.c_str(),
        opts->eResampleAlg,
        opts->dfWarpMemoryLimit,
        maxError,
        progress_cb ? ProgressTrampoline : nullptr,
        progress_cb ? (void *)&progress : nullptr,
        opts);
      if (err) { throw CPLGetLastErrorMsg(); }
      return err;
    };
  }

  job.rval = [](CPLErr r, const GetFromPersistentFunc &) { return Nan::Undefined(); };
  job.run(info, async, 1);
}

/**
 * @typedef {object} WarpOptions
 * @property {Dataset} src
 * @property {SpatialReference} s_srs
 * @property {SpatialReference} t_srs
 * @property {number} [maxError]
 */

/**
 * @typedef {object} WarpOutput
 * @property {xyz} rasterSize
 * @property {number[]} geoTransform
 */

/**
 * Used to determine the bounds and resolution of the output virtual file which
 * should be large enough to include all the input image.
 *
 * @throws {Error}
 * @method suggestedWarpOutput
 * @static
 * @param {WarpOptions} options Warp options
 * @param {Dataset} options.src
 * @param {SpatialReference} options.s_srs
 * @param {SpatialReference} options.t_srs
 * @param {number} [options.maxError=0]
 * @return {WarpOutput} An object containing `"rasterSize"` and `"geoTransform"`
 * properties.
 */

/**
 * Used to determine the bounds and resolution of the output virtual file which
 * should be large enough to include all the input image.
 * @async
 *
 * @throws {Error}
 * @method suggestedWarpOutputAsync
 * @static
 * @param {WarpOptions} options Warp options
 * @param {Dataset} options.src
 * @param {SpatialReference} options.s_srs
 * @param {SpatialReference} options.t_srs
 * @param {number} [options.maxError=0]
 * @param {callback<WarpOutput>} [callback=undefined]
 * @return {Promise<WarpOutput>}
 */
GDAL_ASYNCABLE_DEFINE(Warper::suggestedWarpOutput) {

  Local<Object> obj;
  Local<Value> prop;

  Dataset *ds;
  SpatialReference *s_srs;
  SpatialReference *t_srs;
  double maxError = 0;

  NODE_ARG_OBJECT(0, "Warp options", obj);

  if (Nan::HasOwnProperty(obj, Nan::New("src").ToLocalChecked()).FromMaybe(false)) {
    prop = Nan::Get(obj, Nan::New("src").ToLocalChecked()).ToLocalChecked();
    if (prop->IsObject() && !prop->IsNull() && Nan::New(Dataset::constructor)->HasInstance(prop)) {
      ds = Nan::ObjectWrap::Unwrap<Dataset>(prop.As<Object>());
      if (!ds->get()) {
        Nan::ThrowError("src dataset already closed");
        return;
      }
    } else {
      Nan::ThrowTypeError("src property must be a Dataset object");
      return;
    }
  } else {
    Nan::ThrowError("src dataset must be provided");
    return;
  }

  NODE_WRAPPED_FROM_OBJ(obj, "s_srs", SpatialReference, s_srs);
  NODE_WRAPPED_FROM_OBJ(obj, "t_srs", SpatialReference, t_srs);
  NODE_DOUBLE_FROM_OBJ_OPT(obj, "maxError", maxError);

  char *s_srs_wkt, *t_srs_wkt;
  if (s_srs->get()->exportToWkt(&s_srs_wkt)) {
    Nan::ThrowError("Error converting s_srs to WKT");
    return;
  }
  std::string s_srs_str = std::string(s_srs_wkt);
  CPLFree(s_srs_wkt);
  if (t_srs->get()->exportToWkt(&t_srs_wkt)) {
    Nan::ThrowError("Error converting t_srs to WKT");
    return;
  }
  std::string t_srs_str = std::string(t_srs_wkt);
  CPLFree(t_srs_wkt);

  struct warpOutputResult {
    double geotransform[6];
    int w, h;
  };

#if GDAL_VERSION_MAJOR == 2 && GDAL_VERSION_MINOR < 3
  GDALDatasetH gdal_ds = static_cast<GDALDatasetH>(ds->get());
#else
  GDALDatasetH gdal_ds = GDALDataset::ToHandle(ds->get());
#endif
  GDALAsyncableJob<warpOutputResult> job(ds->uid);

  job.main = [gdal_ds, s_srs_str, t_srs_str, maxError](const GDALExecutionProgress &) {
    struct warpOutputResult r;
    CPLErrorReset();

    void *hTransformArg;
    void *hGenTransformArg =
      GDALCreateGenImgProjTransformer(gdal_ds, s_srs_str.c_str(), NULL, t_srs_str.c_str(), TRUE, 1000.0, 0);

    if (!hGenTransformArg) { throw CPLGetLastErrorMsg(); }

    GDALTransformerFunc pfnTransformer;

    if (maxError > 0.0) {
      hTransformArg = GDALCreateApproxTransformer(GDALGenImgProjTransform, hGenTransformArg, maxError);
      pfnTransformer = GDALApproxTransform;

      if (!hTransformArg) {
        GDALDestroyGenImgProjTransformer(hGenTransformArg);
        throw CPLGetLastErrorMsg();
      }
    } else {
      hTransformArg = hGenTransformArg;
      pfnTransformer = GDALGenImgProjTransform;
    }

    CPLErr err = GDALSuggestedWarpOutput(gdal_ds, pfnTransformer, hTransformArg, r.geotransform, &r.w, &r.h);

    GDALDestroyGenImgProjTransformer(hGenTransformArg);
    if (maxError > 0.0) { GDALDestroyApproxTransformer(hTransformArg); }

    if (err) { throw CPLGetLastErrorMsg(); }
    return r;
  };

  job.rval = [](warpOutputResult r, const GetFromPersistentFunc &) {
    Nan::EscapableHandleScope scope;
    Local<Array> result_geotransform = Nan::New<Array>();
    Nan::Set(result_geotransform, 0, Nan::New<Number>(r.geotransform[0]));
    Nan::Set(result_geotransform, 1, Nan::New<Number>(r.geotransform[1]));
    Nan::Set(result_geotransform, 2, Nan::New<Number>(r.geotransform[2]));
    Nan::Set(result_geotransform, 3, Nan::New<Number>(r.geotransform[3]));
    Nan::Set(result_geotransform, 4, Nan::New<Number>(r.geotransform[4]));
    Nan::Set(result_geotransform, 5, Nan::New<Number>(r.geotransform[5]));

    Local<Object> result_size = Nan::New<Object>();
    Nan::Set(result_size, Nan::New("x").ToLocalChecked(), Nan::New<Integer>(r.w));
    Nan::Set(result_size, Nan::New("y").ToLocalChecked(), Nan::New<Integer>(r.h));

    Local<Object> result = Nan::New<Object>();
    Nan::Set(result, Nan::New("rasterSize").ToLocalChecked(), result_size);
    Nan::Set(result, Nan::New("geoTransform").ToLocalChecked(), result_geotransform);

    return scope.Escape(result);
  };

  job.run(info, async, 1);
}

} // namespace node_gdal
