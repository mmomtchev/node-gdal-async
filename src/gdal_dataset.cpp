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

Nan::Persistent<FunctionTemplate> Dataset::constructor;

void Dataset::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(Dataset::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("Dataset").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan::SetPrototypeMethod(lcons, "setGCPs", setGCPs);
  Nan::SetPrototypeMethod(lcons, "getGCPs", getGCPs);
  Nan::SetPrototypeMethod(lcons, "getGCPProjection", getGCPProjection);
  Nan::SetPrototypeMethod(lcons, "getFileList", getFileList);
  Nan__SetPrototypeAsyncableMethod(lcons, "flush", flush);
  Nan::SetPrototypeMethod(lcons, "close", close);
  Nan__SetPrototypeAsyncableMethod(lcons, "getMetadata", getMetadata);
  Nan__SetPrototypeAsyncableMethod(lcons, "setMetadata", setMetadata);
  Nan::SetPrototypeMethod(lcons, "testCapability", testCapability);
  Nan__SetPrototypeAsyncableMethod(lcons, "executeSQL", executeSQL);
  Nan__SetPrototypeAsyncableMethod(lcons, "buildOverviews", buildOverviews);

  ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER);
  ATTR(lcons, "description", descriptionGetter, READ_ONLY_SETTER);
  ATTR(lcons, "bands", bandsGetter, READ_ONLY_SETTER);
  ATTR(lcons, "layers", layersGetter, READ_ONLY_SETTER);
  ATTR_ASYNCABLE(lcons, "rasterSize", rasterSizeGetter, READ_ONLY_SETTER);
  ATTR(lcons, "driver", driverGetter, READ_ONLY_SETTER);
  ATTR(lcons, "threadSafe", threadSafeGetter, READ_ONLY_SETTER);
  ATTR(lcons, "root", rootGetter, READ_ONLY_SETTER);
  ATTR_ASYNCABLE(lcons, "srs", srsGetter, srsSetter);
  ATTR_ASYNCABLE(lcons, "geoTransform", geoTransformGetter, geoTransformSetter);

  Nan::Set(target, Nan::New("Dataset").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

Dataset::Dataset(GDALDataset *ds) : Nan::ObjectWrap(), uid(0), parent_uid(0), this_dataset(ds), parent_ds(nullptr) {
  LOG("Created Dataset [%p]", ds);
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
NAN_METHOD(Dataset::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }
  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    Dataset *f = static_cast<Dataset *>(ptr);
    f->Wrap(info.This());

    Local<Value> layers = DatasetLayers::New(info.This());
    Nan::SetPrivate(info.This(), Nan::New("layers_").ToLocalChecked(), layers);

    Local<Value> bandsObj;
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
    GDALDataset *gdal_ds = f->get();
    std::shared_ptr<GDALGroup> root = gdal_ds->GetRootGroup();
    if (root == nullptr) {
#endif
      bandsObj = DatasetBands::New(info.This());
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
    } else {
      bandsObj = Nan::Null();
    }
#endif
    Nan::SetPrivate(info.This(), Nan::New("bands_").ToLocalChecked(), bandsObj);
    if (f->parent_ds)
      // For dependent Datasets, keep a reference on the parent to protect it from the GC
      Nan::SetPrivate(info.This(), Nan::New("parent_").ToLocalChecked(), object_store.get(f->parent_ds));

    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create dataset directly");
    return;
  }
}

Local<Value> Dataset::New(GDALDataset *raw, GDALDataset *parent) {
  Nan::EscapableHandleScope scope;

  if (!raw) { return scope.Escape(Nan::Null()); }
  if (object_store.has(raw)) { return scope.Escape(object_store.get(raw)); }

  Dataset *wrapped = new Dataset(raw);

  long parent_uid = 0;
  if (parent != nullptr) {
    /* A dependent Dataset shares the lock of its parent
     */
    Dataset *parent_ds = Nan::ObjectWrap::Unwrap<Dataset>(object_store.get(parent));
    parent_uid = parent_ds->uid;
  }

  Local<Value> ext = Nan::New<External>(wrapped);
  Local<Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(Dataset::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();

  wrapped->uid = object_store.add(raw, wrapped->persistent(), parent_uid);

  return scope.Escape(obj);
}

NAN_METHOD(Dataset::toString) {
  info.GetReturnValue().Set(Nan::New("Dataset").ToLocalChecked());
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

  GDALAsyncableJob<char **> job(ds->uid);
  job.main = [raw, domain](const GDALExecutionProgress &) {
    return raw->GetMetadata(domain.empty() ? nullptr : domain.c_str());
  };
  job.rval = [](char **md, const GetFromPersistentFunc &) { return MajorObject::getMetadata(md); };
  job.run(info, async, 1);
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
    Nan::ThrowError("Failed parsing metadata");
    return;
  }

  std::string domain("");
  NODE_ARG_OPT_STR(1, "domain", domain);

  GDALAsyncableJob<CPLErr> job(ds->uid);
  job.main = [raw, options, domain](const GDALExecutionProgress &) {
    CPLErr r = raw->SetMetadata(options->get(), domain.empty() ? nullptr : domain.c_str());
    if (r == CE_Failure) throw CPLGetLastErrorMsg();
    return r;
  };
  job.rval = [](CPLErr r, const GetFromPersistentFunc &) { return Nan::New<Boolean>(r == CE_None); };
  job.run(info, async, 2);
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
NAN_METHOD(Dataset::testCapability) {
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();

  std::string capability("");
  NODE_ARG_STR(0, "capability", capability);

  AsyncGuard lock({ds->uid}, eventLoopWarn);
  info.GetReturnValue().Set(Nan::New<Boolean>(raw->TestCapability(capability.c_str())));
}

/**
 * Get output projection for GCPs.
 *
 * @method getGCPProjection
 * @instance
 * @memberof Dataset
 * @return {string}
 */
NAN_METHOD(Dataset::getGCPProjection) {
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();
  AsyncGuard lock({ds->uid}, eventLoopWarn);
  info.GetReturnValue().Set(SafeString::New(raw->GetGCPProjection()));
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
NAN_METHOD(Dataset::close) {
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  ds->dispose(true);

  return;
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
  job.rval = [](int, const GetFromPersistentFunc &) { return Nan::Undefined().As<Value>(); };
  job.run(info, async, 0);

  return;
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
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
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

  job.run(info, async, 3);
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
NAN_METHOD(Dataset::getFileList) {
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  Local<Array> results = Nan::New<Array>(0);

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();
  if (!raw) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  AsyncGuard lock({ds->uid}, eventLoopWarn);
  char **list = raw->GetFileList();
  if (!list) {
    info.GetReturnValue().Set(results);
    return;
  }

  int i = 0;
  while (list[i]) {
    Nan::Set(results, i, SafeString::New(list[i]));
    i++;
  }

  CSLDestroy(list);

  info.GetReturnValue().Set(results);
}

/**
 * Fetches GCPs.
 *
 * @method getGCPs
 * @instance
 * @memberof Dataset
 * @return {any[]}
 */
NAN_METHOD(Dataset::getGCPs) {
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  Local<Array> results = Nan::New<Array>(0);

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();
  if (!raw) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  AsyncGuard lock({ds->uid}, eventLoopWarn);
  int n = raw->GetGCPCount();
  const GDAL_GCP *gcps = raw->GetGCPs();

  if (!gcps) {
    info.GetReturnValue().Set(results);
    return;
  }

  for (int i = 0; i < n; i++) {
    GDAL_GCP gcp = gcps[i];
    Local<Object> obj = Nan::New<Object>();
    Nan::Set(obj, Nan::New("pszId").ToLocalChecked(), Nan::New(gcp.pszId).ToLocalChecked());
    Nan::Set(obj, Nan::New("pszInfo").ToLocalChecked(), Nan::New(gcp.pszInfo).ToLocalChecked());
    Nan::Set(obj, Nan::New("dfGCPPixel").ToLocalChecked(), Nan::New<Number>(gcp.dfGCPPixel));
    Nan::Set(obj, Nan::New("dfGCPLine").ToLocalChecked(), Nan::New<Number>(gcp.dfGCPLine));
    Nan::Set(obj, Nan::New("dfGCPX").ToLocalChecked(), Nan::New<Number>(gcp.dfGCPX));
    Nan::Set(obj, Nan::New("dfGCPY").ToLocalChecked(), Nan::New<Number>(gcp.dfGCPY));
    Nan::Set(obj, Nan::New("dfGCPZ").ToLocalChecked(), Nan::New<Number>(gcp.dfGCPZ));
    Nan::Set(results, i, obj);
  }

  info.GetReturnValue().Set(results);
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
NAN_METHOD(Dataset::setGCPs) {
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();
  if (!raw) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  Local<Array> gcps;
  std::string projection("");
  NODE_ARG_ARRAY(0, "gcps", gcps);
  NODE_ARG_OPT_STR(1, "projection", projection);

  std::shared_ptr<GDAL_GCP> list(new GDAL_GCP[gcps->Length()], array_deleter<GDAL_GCP>());
  std::shared_ptr<std::string> pszId_list(new std::string[gcps->Length()], array_deleter<std::string>());
  std::shared_ptr<std::string> pszInfo_list(new std::string[gcps->Length()], array_deleter<std::string>());
  GDAL_GCP *gcp = list.get();
  for (unsigned int i = 0; i < gcps->Length(); ++i) {
    Local<Value> val = Nan::Get(gcps, i).ToLocalChecked();
    if (!val->IsObject()) {
      Nan::ThrowError("GCP array must only include objects");
      return;
    }
    Local<Object> obj = val.As<Object>();

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
  CPLErr err = raw->SetGCPs(gcps->Length(), list.get(), projection.c_str());

  if (err) {
    NODE_THROW_LAST_CPLERR;
    return;
  }

  return;
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
  Local<Array> overviews;
  Local<Array> bands;

  NODE_ARG_STR(0, "resampling", resampling);
  NODE_ARG_ARRAY(1, "overviews", overviews);
  NODE_ARG_ARRAY_OPT(2, "bands", bands);

  int n_overviews = overviews->Length();
  int i, n_bands = 0;

  // shared_ptr to array is C++17 :(
  std::shared_ptr<int> o(new int[n_overviews], array_deleter<int>());
  std::shared_ptr<int> b;
  for (i = 0; i < n_overviews; i++) {
    Local<Value> val = Nan::Get(overviews, i).ToLocalChecked();
    if (!val->IsNumber()) {
      Nan::ThrowError("overviews array must only contain numbers");
      return;
    }
    o.get()[i] = Nan::To<int32_t>(val).ToChecked();
  }

  if (!bands.IsEmpty()) {
    n_bands = bands->Length();
    b = std::shared_ptr<int>(new int[n_bands], array_deleter<int>());
    for (i = 0; i < n_bands; i++) {
      Local<Value> val = Nan::Get(bands, i).ToLocalChecked();
      if (!val->IsNumber()) {
        Nan::ThrowError("band array must only contain numbers");
        return;
      }
      b.get()[i] = Nan::To<int32_t>(val).ToChecked();
    }
  }

  GDALAsyncableJob<CPLErr> job(ds->uid);

  Nan::Callback *progress_cb;
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
  job.rval = [](CPLErr, const GetFromPersistentFunc &) { return Nan::Undefined().As<Value>(); };

  job.run(info, async, 4);
}

/**
 * @readonly
 * @kind member
 * @name description
 * @instance
 * @memberof Dataset
 * @type {string}
 */
NAN_GETTER(Dataset::descriptionGetter) {
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();
  if (!raw) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }
  AsyncGuard lock({ds->uid}, eventLoopWarn);
  info.GetReturnValue().Set(SafeString::New(raw->GetDescription()));
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
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());
  struct xy {
    int x, y;
    bool null;
  };

  if (!ds->isAlive()) {
    THROW_OR_REJECT("Dataset object has already been destroyed")
    return;
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
    Nan::EscapableHandleScope scope;
    if (xy.null) return Nan::Null().As<Value>();
    Local<Object> result = Nan::New<Object>();
    Nan::Set(result, Nan::New("x").ToLocalChecked(), Nan::New<Integer>(xy.x));
    Nan::Set(result, Nan::New("y").ToLocalChecked(), Nan::New<Integer>(xy.y));
    return scope.Escape(result.As<Value>());
  };

  job.run(info, async);
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
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  if (!ds->isAlive()) {
    THROW_OR_REJECT("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();

  GDALAsyncableJob<OGRSpatialReference *> job(ds->uid);

  job.main = [raw](const GDALExecutionProgress &) {
    // get projection wkt and return null if not set
    OGRChar *wkt = (OGRChar *)raw->GetProjectionRef();
    if (*wkt == '\0') {
      // getProjectionRef returns string of length 0 if no srs set
      return (OGRSpatialReference *)nullptr;
    }
    // otherwise construct and return SpatialReference from wkt
    OGRSpatialReference *srs = new OGRSpatialReference();
    int err = srs->importFromWkt(&wkt);
    if (err) throw getOGRErrMsg(err);
    return srs;
  };

  job.rval = [](OGRSpatialReference *srs, const GetFromPersistentFunc &) {
    if (srs != nullptr)
      return SpatialReference::New(srs, true);
    else
      return Nan::Null().As<Value>();
  };
  job.run(info, async);
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
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  if (!ds->isAlive()) {
    THROW_OR_REJECT("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();

  GDALAsyncableJob<std::shared_ptr<double>> job(ds->uid);

  job.main = [raw](const GDALExecutionProgress &) {
    auto transform = std::shared_ptr<double>(new double[6], array_deleter<double>());
    CPLErr err = raw->GetGeoTransform(transform.get());
    // This is mostly (always?) a sign that it has not been set
    if (err) { return std::shared_ptr<double>(nullptr); }
    return transform;
  };

  job.rval = [](std::shared_ptr<double> transform, const GetFromPersistentFunc &) {
    if (transform == nullptr) return Nan::Null().As<v8::Value>();
    Local<Array> result = Nan::New<Array>(6);
    Nan::Set(result, 0, Nan::New<Number>(transform.get()[0]));
    Nan::Set(result, 1, Nan::New<Number>(transform.get()[1]));
    Nan::Set(result, 2, Nan::New<Number>(transform.get()[2]));
    Nan::Set(result, 3, Nan::New<Number>(transform.get()[3]));
    Nan::Set(result, 4, Nan::New<Number>(transform.get()[4]));
    Nan::Set(result, 5, Nan::New<Number>(transform.get()[5]));

    return result.As<v8::Value>();
  };

  job.run(info, async);
}

/**
 * @readonly
 * @kind member
 * @name driver
 * @instance
 * @memberof Dataset
 * @type {Driver}
 */
NAN_GETTER(Dataset::driverGetter) {
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();
  if (raw->GetDriver() != nullptr) { info.GetReturnValue().Set(Driver::New(raw->GetDriver())); }
}

/**
 * @readonly
 * @kind member
 * @name threadSafe
 * @instance
 * @memberof Dataset
 * @type {boolean}
 */
NAN_GETTER(Dataset::threadSafeGetter) {
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 10)
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();
  if (raw->GetDriver() != nullptr) { info.GetReturnValue().Set(Nan::New<Boolean>(raw->IsThreadSafe(GDAL_OF_RASTER))); }
#else
  info.GetReturnValue().Set(Nan::New<Boolean>(false));
#endif
}

NAN_SETTER(Dataset::srsSetter) {
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();
  std::string wkt("");
  if (IS_WRAPPED(value, SpatialReference)) {

    SpatialReference *srs_obj = Nan::ObjectWrap::Unwrap<SpatialReference>(value.As<Object>());
    OGRSpatialReference *srs = srs_obj->get();
    // Get wkt from OGRSpatialReference
    char *str;
    if (srs->exportToWkt(&str)) {
      Nan::ThrowError("Error exporting srs to wkt");
      return;
    }
    wkt = str; // copy string
    CPLFree(str);

  } else if (!value->IsNull() && !value->IsUndefined()) {
    Nan::ThrowError("srs must be SpatialReference object");
    return;
  }

  AsyncGuard lock({ds->uid}, eventLoopWarn);
  CPLErr err = raw->SetProjection(wkt.c_str());

  if (err) { NODE_THROW_LAST_CPLERR; }
}

NAN_SETTER(Dataset::geoTransformSetter) {
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();

  if (!value->IsArray()) {
    Nan::ThrowError("Transform must be an array");
    return;
  }
  Local<Array> transform = value.As<Array>();

  if (transform->Length() != 6) {
    Nan::ThrowError("Transform array must have 6 elements");
    return;
  }

  double buffer[6];
  for (int i = 0; i < 6; i++) {
    Local<Value> val = Nan::Get(transform, i).ToLocalChecked();
    if (!val->IsNumber()) {
      Nan::ThrowError("Transform array must only contain numbers");
      return;
    }
    buffer[i] = Nan::To<double>(val).ToChecked();
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
NAN_GETTER(Dataset::bandsGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("bands_").ToLocalChecked()).ToLocalChecked());
}

/**
 * @readonly
 * @kind member
 * @name layers
 * @instance
 * @memberof Dataset
 * @type {DatasetLayers}
 */
NAN_GETTER(Dataset::layersGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("layers_").ToLocalChecked()).ToLocalChecked());
}

/**
 * @readonly
 * @kind member
 * @name root
 * @instance
 * @memberof Dataset
 * @type {Group}
 */
NAN_GETTER(Dataset::rootGetter) {
  Local<Value> rootObj = Nan::GetPrivate(info.This(), Nan::New("root_").ToLocalChecked()).ToLocalChecked();
  if (rootObj->IsUndefined()) {
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
    NODE_UNWRAP_CHECK(Dataset, info.This(), ds);
    GDAL_RAW_CHECK(GDALDataset *, ds, gdal_ds);
    AsyncGuard lock({ds->uid}, eventLoopWarn);
    std::shared_ptr<GDALGroup> root = gdal_ds->GetRootGroup();
    if (root == nullptr) {
#endif
      rootObj = Nan::Null();
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
    } else {
      rootObj = Group::New(root, info.This());
    }
#endif
    Nan::SetPrivate(info.This(), Nan::New("root_").ToLocalChecked(), rootObj);
  }
  info.GetReturnValue().Set(rootObj);
}

NAN_GETTER(Dataset::uidGetter) {
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(info.This());
  info.GetReturnValue().Set(Nan::New((int)ds->uid));
}

} // namespace node_gdal
