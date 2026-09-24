#include "gdal_dataset.hpp"
#include "gdal_group.hpp"
#include "collections/dataset_bands.hpp"
#include "collections/dataset_layers.hpp"
#include "gdal_common.hpp"
#include "gdal_driver.hpp"
#include "geometry/gdal_geometry.hpp"
#include "gdal_layer.hpp"
#include "gdal_majorobject.hpp"
#include "gdal_rasterband.hpp"
#include "gdal_spatial_reference.hpp"
#include "utils/string_list.hpp"

namespace node_gdal {

Napi::FunctionReference Dataset::constructor;

void Dataset::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(Dataset);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "Dataset",
    {
        METHOD(toString)
        METHOD(setGCPs)
        METHOD(getGCPs)
        METHOD(getGCPProjection)
        METHOD(getFileList)
        METHOD_ASYNCABLE(flush)
        METHOD(close)
        METHOD_ASYNCABLE(getMetadata)
        METHOD_ASYNCABLE(setMetadata)
        METHOD(testCapability)
        METHOD_ASYNCABLE(executeSQL)
        METHOD_ASYNCABLE(buildOverviews)
        ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER)
        ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER)
        ATTR(lcons, "bands", bandsGetter, READ_ONLY_SETTER)
        ATTR(lcons, "layers", layersGetter, READ_ONLY_SETTER)
        ATTR_ASYNCABLE(lcons, "rasterSize", rasterSizeGetter, READ_ONLY_SETTER)
        ATTR(lcons, "driver", driverGetter, READ_ONLY_SETTER)
        ATTR(lcons, "threadSafe", threadSafeGetter, READ_ONLY_SETTER)
        ATTR(lcons, "root", rootGetter, READ_ONLY_SETTER)
        ATTR_ASYNCABLE(lcons, "srs", srsGetter, srsSetter)
        ATTR_ASYNCABLE(lcons, "geoTransform", geoTransformGetter, geoTransformSetter)
    });

  target.Set("Dataset", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}


Dataset::Dataset(const Napi::CallbackInfo &info) : GDALObject<Dataset>(info), uid(0), parent_uid(0), this_dataset(nullptr), parent_ds(nullptr)  {
  LOG("Created Dataset [%p]", this_dataset);
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_dataset = static_cast<GDALDataset *>(info[0].As<Napi::External<void>>().Data());
  } else {
    Napi::Error::New(node_gdal::napi_env(), "Cannot create dataset directly").ThrowAsJavaScriptException();
    return;
  }

  // Every Dataset carries its collections and, when it is a dependent dataset,
  // a reference on its parent so that the GC cannot collect it
  GDAL_SET_PRIVATE(info.This(), "layers_", DatasetLayers::New(info.This()));

  Napi::Value bandsObj;
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
  GDALDataset *gdal_ds = get();
  std::shared_ptr<GDALGroup> root = gdal_ds->GetRootGroup();
  if (root == nullptr) {
#endif
    bandsObj = DatasetBands::New(info.This());
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
  } else {
    bandsObj = info.Env().Null();
  }
#endif
  GDAL_SET_PRIVATE(info.This(), "bands_", bandsObj);
  if (parent_ds) GDAL_SET_PRIVATE(info.This(), "parent_", object_store.get(parent_ds));
}
Dataset::~Dataset() {
  // Destroy at garbage collection time if not already explicitly destroyed
  dispose(false);
}

void Dataset::dispose(bool manual) {
  if (this_dataset) {
    LOG("Disposing Dataset [%p]", this_dataset);

    object_store.dispose(uid, manual);

    LOG("Disposed Dataset [%p]", this_dataset);

    this_dataset = NULL;
  }
}

/**
 * A set of associated raster bands and/or vector layers, usually from one file.
 *
 * @example
 * // raster dataset:
 * dataset = gdal.open('file.tif');
 * bands = dataset.bands;
 *
 * // vector dataset:
 * dataset = gdal.open('file.shp');
 * layers = dataset.layers;
 *
 * @class Dataset
 */

Napi::Value Dataset::New(GDALDataset *raw, GDALDataset *parent, bool close) {

  if (!raw) { return node_gdal::napi_env().Null(); }
  if (object_store.has(raw)) { return object_store.get(raw); }

  long parent_uid = 0;
  if (parent != nullptr) {
    /* A dependent Dataset shares the lock of its parent
     */
    Dataset *parent_ds = node_gdal::UnwrapWrapped<Dataset>(object_store.get(parent));
    parent_uid = parent_ds->uid;
  }

  std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env(), raw)};
  Napi::Object obj = Dataset::constructor.Value().New(args);
  Dataset *wrapped = node_gdal::UnwrapWrapped<Dataset>(obj);

  wrapped->uid = object_store.add(raw, *wrapped, parent_uid, close);

  return obj;
}

NAPI_METHOD(Dataset::toString) {
  return Napi::String::New(node_gdal::napi_env(), "Dataset");
}

/**
 * Fetch metadata.
 *
 * @method getMetadata
 * @instance
 * @memberof Dataset
 * @param {string} [domain]
 * @return {any}
 */

/**
 * Fetch metadata.
 * @async
 *
 * @method getMetadataAsync
 * @instance
 * @memberof Dataset
 * @param {string} [domain]
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<any>}
 */
GDAL_ASYNCABLE_DEFINE(Dataset::getMetadata) {
  NODE_UNWRAP_CHECK(Dataset, info.This(), ds);
  GDAL_RAW_CHECK(GDALDataset *, ds, raw);

  std::string domain("");
  NODE_ARG_OPT_STR(0, "domain", domain);

  GDALAsyncableJob<CSLConstList> job(ds->uid);
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
 * @memberof Dataset
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
 * @memberof Dataset
 * @param {object|string[]} metadata
 * @param {string} [domain]
 * @param {callback<boolean>} [callback=undefined]
 * @return {Promise<boolean>}
 */
GDAL_ASYNCABLE_DEFINE(Dataset::setMetadata) {
  NODE_UNWRAP_CHECK(Dataset, info.This(), ds);
  GDAL_RAW_CHECK(GDALDataset *, ds, raw);

  auto options = make_shared<StringList>();
  if (info.Length() == 0 || options->parse(info[0])) {
    node_gdal::ThrowOverPending(node_gdal::napi_env(), "Failed parsing metadata");
    return node_gdal::napi_env().Undefined();
  }

  std::string domain("");
  NODE_ARG_OPT_STR(1, "domain", domain);

  GDALAsyncableJob<CPLErr> job(ds->uid);
  job.main = [raw, options, domain](const GDALExecutionProgress &) {
    CPLErr r = raw->SetMetadata(options->get(), domain.empty() ? nullptr : domain.c_str());
    if (r == CE_Failure) throw CPLGetLastErrorMsg();
    return r;
  };
  job.rval = [](CPLErr r, const GetFromPersistentFunc &) { return Napi::Boolean::New(node_gdal::napi_env(), r == CE_None); };
  return job.run(info, async, 2);
}

/**
 * Determines if the dataset supports the indicated operation.
 *
 * @method testCapability
 * @instance
 * @memberof Dataset
 * @param {string} capability {@link ODsC|capability list}
 * @return {boolean}
 */
NAPI_METHOD(Dataset::testCapability) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();

  std::string capability("");
  NODE_ARG_STR(0, "capability", capability);

  AsyncGuard lock({ds->uid}, eventLoopWarn);
  return Napi::Boolean::New(node_gdal::napi_env(), raw->TestCapability(capability.c_str()));
}

/**
 * Get output projection for GCPs.
 *
 * @method getGCPProjection
 * @instance
 * @memberof Dataset
 * @return {string}
 */
NAPI_METHOD(Dataset::getGCPProjection) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();
  AsyncGuard lock({ds->uid}, eventLoopWarn);
  return SafeString::New(raw->GetGCPProjection());
}

/**
 * Closes the dataset to further operations. It releases all memory and ressources held
 * by the dataset.
 *
 * This is normally an instantenous atomic operation that won't block the event loop
 * except if there is an operation running on this dataset in asynchronous context - in this case
 * this call will block until that operation finishes.
 *
 * If this could potentially be the case and blocking the event loop is not possible (server code),
 * then the best option is to simply dereference it (ds = null) and leave
 * the garbage collector to expire it.
 *
 * Implementing an asynchronous delete is difficult since all V8 object creation/deletion
 * must take place on the main thread.
 *
 * flush()/flushAsync() ensure that, when writing, all data has been written.
 *
 * @method close
 * @instance
 * @memberof Dataset
 */
NAPI_METHOD(Dataset::close) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  ds->dispose(true);

  return node_gdal::napi_env().Undefined();
}

/**
 * Flushes all changes to disk.
 *
 * @throws {Error}
 * @method flush
 * @instance
 * @memberof Dataset
 */

/**
 * Flushes all changes to disk.
 * @async
 *
 * @method flushAsync
 * @instance
 * @memberof Dataset
 * @throws {Error}
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 */
GDAL_ASYNCABLE_DEFINE(Dataset::flush) {
  NODE_UNWRAP_CHECK(Dataset, info.This(), ds);
  GDAL_RAW_CHECK(GDALDataset *, ds, raw);
  GDALAsyncableJob<int> job(ds->uid);
  job.main = [raw](const GDALExecutionProgress &) {
    raw->FlushCache();
    return 0;
  };
  job.rval = [](int, const GetFromPersistentFunc &) { return node_gdal::napi_env().Undefined().As<Napi::Value>(); };
  return job.run(info, async, 0);

  return node_gdal::napi_env().Undefined();
}

/**
 * Execute an SQL statement against the data store.
 *
 * @throws {Error}
 * @method executeSQL
 * @instance
 * @memberof Dataset
 * @param {string} statement SQL statement to execute.
 * @param {Geometry} [spatial_filter=null] Geometry which represents a
 * spatial filter.
 * @param {string} [dialect=null] Allows control of the statement dialect. If
 * set to `null`, the OGR SQL engine will be used, except for RDBMS drivers that
 * will use their dedicated SQL engine, unless `"OGRSQL"` is explicitely passed
 * as the dialect. Starting with OGR 1.10, the `"SQLITE"` dialect can also be
 * used.
 * @return {Layer}
 */

/**
 * Execute an SQL statement against the data store.
 * @async
 *
 * @throws {Error}
 * @method executeSQLAsync
 * @instance
 * @memberof Dataset
 * @param {string} statement SQL statement to execute.
 * @param {Geometry} [spatial_filter=null] Geometry which represents a
 * spatial filter.
 * @param {string} [dialect=null] Allows control of the statement dialect. If
 * set to `null`, the OGR SQL engine will be used, except for RDBMS drivers that
 * will use their dedicated SQL engine, unless `"OGRSQL"` is explicitely passed
 * as the dialect. Starting with OGR 1.10, the `"SQLITE"` dialect can also be
 * used.
 * @param {callback<Layer>} [callback=undefined]
 * @return {Promise<Layer>}
 */
GDAL_ASYNCABLE_DEFINE(Dataset::executeSQL) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();

  std::string sql;
  std::string sql_dialect;
  Geometry *spatial_filter = NULL;

  NODE_ARG_STR(0, "sql text", sql);
  NODE_ARG_WRAPPED_OPT(1, "spatial filter geometry", Geometry, spatial_filter);
  NODE_ARG_OPT_STR(2, "sql dialect", sql_dialect);

  GDALAsyncableJob<OGRLayer *> job(ds->uid);
  OGRGeometry *geom_filter = spatial_filter ? spatial_filter->get() : NULL;
  job.main = [raw, sql, sql_dialect, geom_filter](const GDALExecutionProgress &) {
    CPLErrorReset();
    OGRLayer *layer = raw->ExecuteSQL(sql.c_str(), geom_filter, sql_dialect.empty() ? NULL : sql_dialect.c_str());
    if (layer == nullptr) throw CPLGetLastErrorMsg();
    return layer;
  };
  job.rval = [raw](OGRLayer *layer, const GetFromPersistentFunc &) { return Layer::New(layer, raw, true); };

  return job.run(info, async, 3);
}

/**
 * Fetch files forming dataset.
 *
 * Returns a list of files believed to be part of this dataset. If it returns an
 * empty list of files it means there is believed to be no local file system
 * files associated with the dataset (for instance a virtual dataset).
 *
 * Returns an empty array for vector datasets if GDAL version is below 2.0
 *
 * @method getFileList
 * @instance
 * @memberof Dataset
 * @return {string[]}
 */
NAPI_METHOD(Dataset::getFileList) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  Napi::Array results = Napi::Array::New(node_gdal::napi_env(), 0);

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();
  if (!raw) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  AsyncGuard lock({ds->uid}, eventLoopWarn);
  char **list = raw->GetFileList();
  if (!list) {
    return results;
    return node_gdal::napi_env().Undefined();
  }

  int i = 0;
  while (list[i]) {
    results.Set( i, SafeString::New(list[i]));
    i++;
  }

  CSLDestroy(list);

  return results;
}

/**
 * Fetches GCPs.
 *
 * @method getGCPs
 * @instance
 * @memberof Dataset
 * @return {any[]}
 */
NAPI_METHOD(Dataset::getGCPs) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  Napi::Array results = Napi::Array::New(node_gdal::napi_env(), 0);

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();
  if (!raw) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  AsyncGuard lock({ds->uid}, eventLoopWarn);
  int n = raw->GetGCPCount();
  const GDAL_GCP *gcps = raw->GetGCPs();

  if (!gcps) {
    return results;
    return node_gdal::napi_env().Undefined();
  }

  for (int i = 0; i < n; i++) {
    GDAL_GCP gcp = gcps[i];
    Napi::Object obj = Napi::Object::New(node_gdal::napi_env());
    obj.Set( Napi::String::New(node_gdal::napi_env(), "pszId"), Napi::String::New(node_gdal::napi_env(), gcp.pszId));
    obj.Set( Napi::String::New(node_gdal::napi_env(), "pszInfo"), Napi::String::New(node_gdal::napi_env(), gcp.pszInfo));
    obj.Set( Napi::String::New(node_gdal::napi_env(), "dfGCPPixel"), Napi::Number::New(node_gdal::napi_env(), gcp.dfGCPPixel));
    obj.Set( Napi::String::New(node_gdal::napi_env(), "dfGCPLine"), Napi::Number::New(node_gdal::napi_env(), gcp.dfGCPLine));
    obj.Set( Napi::String::New(node_gdal::napi_env(), "dfGCPX"), Napi::Number::New(node_gdal::napi_env(), gcp.dfGCPX));
    obj.Set( Napi::String::New(node_gdal::napi_env(), "dfGCPY"), Napi::Number::New(node_gdal::napi_env(), gcp.dfGCPY));
    obj.Set( Napi::String::New(node_gdal::napi_env(), "dfGCPZ"), Napi::Number::New(node_gdal::napi_env(), gcp.dfGCPZ));
    results.Set( i, obj);
  }

  return results;
}

/**
 * Sets GCPs.
 *
 * @throws {Error}
 * @method setGCPs
 * @instance
 * @memberof Dataset
 * @param {object[]} gcps
 * @param {string} [projection]
 */
NAPI_METHOD(Dataset::setGCPs) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();
  if (!raw) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  Napi::Array gcps;
  std::string projection("");
  NODE_ARG_ARRAY(0, "gcps", gcps);
  NODE_ARG_OPT_STR(1, "projection", projection);

  std::shared_ptr<GDAL_GCP[]> list(new GDAL_GCP[gcps.Length()]);
  std::shared_ptr<std::string[]> pszId_list(new std::string[gcps.Length()]);
  std::shared_ptr<std::string[]> pszInfo_list(new std::string[gcps.Length()]);
  GDAL_GCP *gcp = list.get();
  for (unsigned int i = 0; i < gcps.Length(); ++i) {
    Napi::Value val = gcps.As<Napi::Object>().Get(i);
    if (!val.IsObject()) {
      Napi::Error::New(node_gdal::napi_env(), "GCP array must only include objects").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }
    Napi::Object obj = val.As<Napi::Object>();

    NODE_DOUBLE_FROM_OBJ(obj, "dfGCPPixel", gcp->dfGCPPixel);
    NODE_DOUBLE_FROM_OBJ(obj, "dfGCPLine", gcp->dfGCPLine);
    NODE_DOUBLE_FROM_OBJ(obj, "dfGCPX", gcp->dfGCPX);
    NODE_DOUBLE_FROM_OBJ(obj, "dfGCPY", gcp->dfGCPY);
    NODE_DOUBLE_FROM_OBJ_OPT(obj, "dfGCPZ", gcp->dfGCPZ);
    NODE_STR_FROM_OBJ_OPT(obj, "pszId", pszId_list.get()[i]);
    NODE_STR_FROM_OBJ_OPT(obj, "pszInfo", pszInfo_list.get()[i]);

    gcp->pszId = (char *)pszId_list.get()[i].c_str();
    gcp->pszInfo = (char *)pszInfo_list.get()[i].c_str();

    gcp++;
  }

  AsyncGuard lock({ds->uid}, eventLoopWarn);
  CPLErr err = raw->SetGCPs(gcps.Length(), list.get(), projection.c_str());

  if (err) {
    NODE_THROW_LAST_CPLERR;
    return node_gdal::napi_env().Undefined();
  }

  return node_gdal::napi_env().Undefined();
}

/**
 * Builds dataset overviews.
 *
 * @throws {Error}
 * @method buildOverviews
 * @instance
 * @memberof Dataset
 * @param {string} resampling `"NEAREST"`, `"GAUSS"`, `"CUBIC"`, `"AVERAGE"`,
 * `"MODE"`, `"AVERAGE_MAGPHASE"` or `"NONE"`
 * @param {number[]} overviews
 * @param {number[]} [bands] Note: Generation of overviews in external TIFF currently only supported when operating on all bands.
 * @param {ProgressOptions} [options] options
 * @param {ProgressCb} [options.progress_cb]
 */

/**
 * Builds dataset overviews.
 * @async
 *
 * @throws {Error}
 * @method buildOverviewsAsync
 * @instance
 * @memberof Dataset
 * @param {string} resampling `"NEAREST"`, `"GAUSS"`, `"CUBIC"`, `"AVERAGE"`,
 * `"MODE"`, `"AVERAGE_MAGPHASE"` or `"NONE"`
 * @param {number[]} overviews
 * @param {number[]} [bands] Note: Generation of overviews in external TIFF currently only supported when operating on all bands.
 * @param {ProgressOptions} [options] options
 * @param {ProgressCb} [options.progress_cb]
 * @param {callback<void>} [callback=undefined]
 * @return {Promise<void>}
 */
GDAL_ASYNCABLE_DEFINE(Dataset::buildOverviews) {

  NODE_UNWRAP_CHECK(Dataset, info.This(), ds);
  GDAL_RAW_CHECK(GDALDataset *, ds, raw);

  std::string resampling = "";
  Napi::Array overviews;
  Napi::Array bands;

  NODE_ARG_STR(0, "resampling", resampling);
  NODE_ARG_ARRAY(1, "overviews", overviews);
  NODE_ARG_ARRAY_OPT(2, "bands", bands);

  int n_overviews = overviews.Length();
  int i, n_bands = 0;

  std::shared_ptr<int[]> o(new int[n_overviews]);
  std::shared_ptr<int[]> b;
  for (i = 0; i < n_overviews; i++) {
    Napi::Value val = overviews.As<Napi::Object>().Get(i);
    if (!val.IsNumber()) {
      Napi::Error::New(node_gdal::napi_env(), "overviews array must only contain numbers").ThrowAsJavaScriptException();
      return node_gdal::napi_env().Undefined();
    }
    o.get()[i] = val.As<Napi::Number>().Int32Value();
  }

  if (!bands.IsEmpty()) {
    n_bands = bands.Length();
    b = std::shared_ptr<int[]>(new int[n_bands]);
    for (i = 0; i < n_bands; i++) {
      Napi::Value val = bands.As<Napi::Object>().Get(i);
      if (!val.IsNumber()) {
        Napi::Error::New(node_gdal::napi_env(), "band array must only contain numbers").ThrowAsJavaScriptException();
        return node_gdal::napi_env().Undefined();
      }
      b.get()[i] = val.As<Napi::Number>().Int32Value();
    }
  }

  GDALAsyncableJob<CPLErr> job(ds->uid);

  Napi::FunctionReference *progress_cb;
  NODE_PROGRESS_CB_OPT(3, progress_cb, job);
  job.progress = progress_cb;
  // Alas one cannot capture-move a unique_ptr and assign the lambda to a variable
  // because the lambda becomes non-copyable
  // But we can use a shared_ptr because the lifetime of the lambda is limited by the lifetime
  // of the async worker
  job.main = [raw, resampling, n_overviews, o, n_bands, b, progress_cb](const GDALExecutionProgress &progress) {
    if (b != nullptr) {
      for (int i = 0; i < n_bands; i++) {
        if (b.get()[i] > raw->GetRasterCount() || b.get()[i] < 1) { throw "invalid band id"; }
      }
    }
    CPLErrorReset();
    CPLErr err = raw->BuildOverviews(
      resampling.c_str(),
      n_overviews,
      o.get(),
      n_bands,
      b.get(),
      progress_cb ? ProgressTrampoline : nullptr,
      progress_cb ? (void *)&progress : nullptr);
    if (err != CE_None) { throw CPLGetLastErrorMsg(); }
    return err;
  };
  job.rval = [](CPLErr, const GetFromPersistentFunc &) { return node_gdal::napi_env().Undefined().As<Napi::Value>(); };

  return job.run(info, async, 4);
}

/**
 * @readonly
 * @kind member
 * @name description
 * @instance
 * @memberof Dataset
 * @type {string}
 */
NAPI_GETTER(Dataset::descriptionGetter) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();
  if (!raw) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  AsyncGuard lock({ds->uid}, eventLoopWarn);
  return SafeString::New(raw->GetDescription());
}

/**
 * Raster dimensions. An object containing `x` and `y` properties.
 *
 * @readonly
 * @kind member
 * @name rasterSize
 * @instance
 * @memberof Dataset
 * @type {xyz}
 */

/**
 * Raster dimensions. An object containing `x` and `y` properties.
 * @asyncGetter
 *
 * @readonly
 * @kind member
 * @name rasterSizeAsync
 * @instance
 * @memberof Dataset
 * @type {Promise<xyz>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(Dataset::rasterSizeGetter) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());
  struct xy {
    int x, y;
    bool null;
  };

  if (!ds->isAlive()) {
    THROW_OR_REJECT("Dataset object has already been destroyed")
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();

  GDALAsyncableJob<xy> job(ds->uid);

  job.main = [raw](const GDALExecutionProgress &) {
    xy result;
    // GDAL 2.x will return 512x512 for vector datasets... which doesn't really make
    // sense in JS where we can return null instead of a number
    // https://github.com/OSGeo/gdal/blob/beef45c130cc2778dcc56d85aed1104a9b31f7e6/gdal/gcore/gdaldataset.cpp#L173-L174
    if (raw->GetDriver() == nullptr || !raw->GetDriver()->GetMetadataItem(GDAL_DCAP_RASTER)) {
      result.null = true;
      return result;
    }
    result.x = raw->GetRasterXSize();
    result.y = raw->GetRasterYSize();
    result.null = false;
    return result;
  };

  job.rval = [](xy xy, const GetFromPersistentFunc &) {
    if (xy.null) return node_gdal::napi_env().Null().As<Napi::Value>();
    Napi::Object result = Napi::Object::New(node_gdal::napi_env());
    result.Set( Napi::String::New(node_gdal::napi_env(), "x"), Napi::Number::New(node_gdal::napi_env(), xy.x));
    result.Set( Napi::String::New(node_gdal::napi_env(), "y"), Napi::Number::New(node_gdal::napi_env(), xy.y));
    return result.As<Napi::Value>();
  };

  return job.run(info, async);
}

/**
 * Spatial reference associated with raster dataset.
 *
 * @throws {Error}
 * @kind member
 * @name srs
 * @instance
 * @memberof Dataset
 * @type {SpatialReference|null}
 */

/**
 * Spatial reference associated with raster dataset.
 * @asyncGetter
 *
 * @throws {Error}
 * @kind member
 * @name srsAsync
 * @instance
 * @memberof Dataset
 * @readonly
 * @type {Promise<SpatialReference|null>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(Dataset::srsGetter) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  if (!ds->isAlive()) {
    THROW_OR_REJECT("Dataset object has already been destroyed");
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();

  GDALAsyncableJob<OGRSpatialReference *> job(ds->uid);

  job.main = [raw](const GDALExecutionProgress &) -> OGRSpatialReference * {
    // The pointer returned references an ephemere object
    // We immediately copy it in order to avoid storing
    // it in the Object Store.
    // Alternatively, this could have been stored as
    // children in the Object Store Dataset object, but
    // an SRS is a standalone object and does not have a parent
    // (unlike a RasterBand).
    auto *srs = raw->GetSpatialRef();
    if (srs == nullptr) return nullptr;
    return srs->Clone();
  };

  job.rval = [](OGRSpatialReference *srs, const GetFromPersistentFunc &) {
    if (srs != nullptr)
      return SpatialReference::New(srs, true);
    else
      return node_gdal::napi_env().Null().As<Napi::Value>();
  };
  return job.run(info, async);
}

/**
 * An affine transform which maps pixel/line coordinates into georeferenced
 * space using the following relationship:
 *
 * @example
 *
 * var GT = dataset.geoTransform;
 * var Xgeo = GT[0] + Xpixel*GT[1] + Yline*GT[2];
 * var Ygeo = GT[3] + Xpixel*GT[4] + Yline*GT[5];
 *
 * @kind member
 * @name geoTransform
 * @instance
 * @memberof Dataset
 * @type {number[]|null}
 */

/**
 * An affine transform which maps pixel/line coordinates into georeferenced
 * space using the following relationship:
 *
 * @example
 *
 * var GT = dataset.geoTransform;
 * var Xgeo = GT[0] + Xpixel*GT[1] + Yline*GT[2];
 * var Ygeo = GT[3] + Xpixel*GT[4] + Yline*GT[5];
 *
 * @asyncGetter
 * @readonly
 * @kind member
 * @name geoTransformAsync
 * @instance
 * @memberof Dataset
 * @type {Promise<number[]|null>}
 */
GDAL_ASYNCABLE_GETTER_DEFINE(Dataset::geoTransformGetter) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  if (!ds->isAlive()) {
    THROW_OR_REJECT("Dataset object has already been destroyed");
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();

  GDALAsyncableJob<std::shared_ptr<double[]>> job(ds->uid);

  job.main = [raw](const GDALExecutionProgress &) {
    auto transform = std::shared_ptr<double[]>(new double[6]);
    CPLErr err = raw->GetGeoTransform(transform.get());
    // This is mostly (always?) a sign that it has not been set
    if (err) { return std::shared_ptr<double[]>(nullptr); }
    return transform;
  };

  job.rval = [](std::shared_ptr<double[]> transform, const GetFromPersistentFunc &) -> Napi::Value {
    if (transform == nullptr) return node_gdal::napi_env().Null();
    Napi::Array result = Napi::Array::New(node_gdal::napi_env(), 6);
    result.Set(static_cast<uint32_t>(0), Napi::Number::New(node_gdal::napi_env(), transform.get()[0]));
    result.Set(static_cast<uint32_t>(1), Napi::Number::New(node_gdal::napi_env(), transform.get()[1]));
    result.Set(static_cast<uint32_t>(2), Napi::Number::New(node_gdal::napi_env(), transform.get()[2]));
    result.Set(static_cast<uint32_t>(3), Napi::Number::New(node_gdal::napi_env(), transform.get()[3]));
    result.Set(static_cast<uint32_t>(4), Napi::Number::New(node_gdal::napi_env(), transform.get()[4]));
    result.Set(static_cast<uint32_t>(5), Napi::Number::New(node_gdal::napi_env(), transform.get()[5]));

    return result;
  };

  return job.run(info, async);
}

/**
 * @readonly
 * @kind member
 * @name driver
 * @instance
 * @memberof Dataset
 * @type {Driver}
 */
NAPI_GETTER(Dataset::driverGetter) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();
  if (raw->GetDriver() != nullptr) { return Driver::New(raw->GetDriver()); }
  return node_gdal::napi_env().Undefined();
}

/**
 * @readonly
 * @kind member
 * @name threadSafe
 * @instance
 * @memberof Dataset
 * @type {boolean}
 */
NAPI_GETTER(Dataset::threadSafeGetter) {
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 10)
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();
  if (raw->GetDriver() != nullptr) { return Napi::Boolean::New(node_gdal::napi_env(), raw->IsThreadSafe(GDAL_OF_RASTER)); }
  return node_gdal::napi_env().Undefined();
#else
  return Napi::Boolean::New(node_gdal::napi_env(), false);
#endif
}

NAPI_SETTER(Dataset::srsSetter) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return;
  }

  GDALDataset *raw = ds->get();
  std::string wkt("");
  if (IS_WRAPPED(value, SpatialReference)) {

    SpatialReference *srs_obj = node_gdal::UnwrapWrapped<SpatialReference>(value.As<Napi::Object>());
    OGRSpatialReference *srs = srs_obj->get();
    // Get wkt from OGRSpatialReference
    char *str;
    if (srs->exportToWkt(&str)) {
      Napi::Error::New(node_gdal::napi_env(), "Error exporting srs to wkt").ThrowAsJavaScriptException();
      return;
    }
    wkt = str; // copy string
    CPLFree(str);

  } else if (!value.IsNull() && !value.IsUndefined()) {
    Napi::Error::New(node_gdal::napi_env(), "srs must be SpatialReference object").ThrowAsJavaScriptException();
    return;
  }

  AsyncGuard lock({ds->uid}, eventLoopWarn);
  CPLErr err = raw->SetProjection(wkt.c_str());

  if (err) { NODE_THROW_LAST_CPLERR; }
}

NAPI_SETTER(Dataset::geoTransformSetter) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return;
  }

  GDALDataset *raw = ds->get();

  if (!value.IsArray()) {
    Napi::Error::New(node_gdal::napi_env(), "Transform must be an array").ThrowAsJavaScriptException();
    return;
  }
  Napi::Array transform = value.As<Napi::Array>();

  if (transform.Length() != 6) {
    Napi::Error::New(node_gdal::napi_env(), "Transform array must have 6 elements").ThrowAsJavaScriptException();
    return;
  }

  double buffer[6];
  for (int i = 0; i < 6; i++) {
    Napi::Value val = transform.As<Napi::Object>().Get(i);
    if (!val.IsNumber()) {
      Napi::Error::New(node_gdal::napi_env(), "Transform array must only contain numbers").ThrowAsJavaScriptException();
      return;
    }
    buffer[i] = val.As<Napi::Number>().DoubleValue();
  }

  AsyncGuard lock({ds->uid}, eventLoopWarn);
  CPLErr err = raw->SetGeoTransform(buffer);

  if (err) { NODE_THROW_LAST_CPLERR; }
}

/**
 * @readonly
 * @kind member
 * @name bands
 * @instance
 * @memberof Dataset
 * @type {DatasetBands}
 */
NAPI_GETTER(Dataset::bandsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "bands_");
}

/**
 * @readonly
 * @kind member
 * @name layers
 * @instance
 * @memberof Dataset
 * @type {DatasetLayers}
 */
NAPI_GETTER(Dataset::layersGetter) {
  return GDAL_GET_PRIVATE(info.This(), "layers_");
}

/**
 * @readonly
 * @kind member
 * @name root
 * @instance
 * @memberof Dataset
 * @type {Group}
 */
NAPI_GETTER(Dataset::rootGetter) {
  Napi::Value rootObj = GDAL_GET_PRIVATE(info.This(), "root_");
  if (rootObj.IsUndefined()) {
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
    NODE_UNWRAP_CHECK(Dataset, info.This(), ds);
    GDAL_RAW_CHECK(GDALDataset *, ds, gdal_ds);
    AsyncGuard lock({ds->uid}, eventLoopWarn);
    std::shared_ptr<GDALGroup> root = gdal_ds->GetRootGroup();
    if (root == nullptr) {
#endif
      rootObj = node_gdal::napi_env().Null();
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
    } else {
      rootObj = Group::New(root, info.This().As<Napi::Object>());
    }
#endif
    GDAL_SET_PRIVATE(info.This(), "root_", rootObj);
  }
  return rootObj;
}

NAPI_GETTER(Dataset::uidGetter) {
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(info.This().As<Napi::Object>());
  return Napi::Number::New(node_gdal::napi_env(), (int)ds->uid);
}

} // namespace node_gdal
