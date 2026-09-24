#include <memory>
#include "dataset_bands.hpp"
#include "../gdal_common.hpp"
#include "../gdal_dataset.hpp"
#include "../gdal_rasterband.hpp"
#include "../utils/string_list.hpp"

namespace node_gdal {

Napi::FunctionReference DatasetBands::constructor;

void DatasetBands::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(DatasetBands);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "DatasetBands",
    {
        METHOD(toString)
        METHOD_ASYNCABLE(count)
        METHOD_ASYNCABLE(create)
        METHOD_ASYNCABLE(get)
        ATTR_DONT_ENUM(lcons, "ds", dsGetter, READ_ONLY_SETTER)
    });

  target.Set("DatasetBands", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

DatasetBands::DatasetBands(const Napi::CallbackInfo &info) : GDALObject<DatasetBands>(info) {
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::Error::New(info.Env(), "Cannot create DatasetBands directly").ThrowAsJavaScriptException();
    return;
  }
  GDAL_SET_PRIVATE(info.This(), "parent_", info[0]);
}

DatasetBands::~DatasetBands() {
}

/**
 * An encapsulation of a {@link Dataset}
 * raster bands.
 *
 * @example
 * var bands = dataset.bands;
 *
 * @class DatasetBands
 */

Napi::Value DatasetBands::New(Napi::Value ds_obj) {

  std::vector<napi_value> args = {ds_obj};
  Napi::Object obj = DatasetBands::constructor.Value().New(args);

  return obj;
}

NAPI_METHOD(DatasetBands::toString) {
  return Napi::String::New(node_gdal::napi_env(), "DatasetBands");
}

/**
 * Returns the band with the given ID.
 *
 * @method get
 * @instance
 * @memberof DatasetBands
 * @param {number} id
 * @throws {Error}
 * @return {RasterBand}
 */

/**
 * Returns the band with the given ID.
 * @async
 *
 * @method getAsync
 * @instance
 * @memberof DatasetBands
 *
 * @param {number} id
 * @param {callback<RasterBand>} [callback=undefined]
 * @throws {Error}
 * @return {Promise<RasterBand>}
 */
GDAL_ASYNCABLE_DEFINE(DatasetBands::get) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(parent);

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();
  int band_id;
  NODE_ARG_INT(0, "band id", band_id);

  GDALAsyncableJob<GDALRasterBand *> job(ds->uid);
  job.persist(parent);
  job.main = [raw, band_id](const GDALExecutionProgress &) {
    CPLErrorReset();
    GDALRasterBand *band = raw->GetRasterBand(band_id);
    if (band == nullptr) { throw CPLGetLastErrorMsg(); }
    return band;
  };
  job.rval = [raw](GDALRasterBand *band, const GetFromPersistentFunc &) { return RasterBand::New(band, raw); };
  return job.run(info, async, 1);
}

/**
 * Adds a new band.
 *
 * @method create
 * @instance
 * @memberof DatasetBands
 * @throws {Error}
 * @param {string} dataType Type of band ({@link GDT|see GDT constants})
 * @param {object|string[]} [options] Creation options
 * @return {RasterBand}
 */

/**
 * Adds a new band.
 * @async
 *
 * @method createAsync
 * @instance
 * @memberof DatasetBands
 * @throws {Error}
 * @param {string} dataType Type of band ({@link GDT|see GDT constants})
 * @param {object|string[]} [options] Creation options
 * @param {callback<RasterBand>} [callback=undefined]
 * @return {Promise<RasterBand>}
 */

GDAL_ASYNCABLE_DEFINE(DatasetBands::create) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(parent);

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();
  GDALDataType type;
  std::shared_ptr<StringList> options(new StringList);

  // NODE_ARG_ENUM(0, "data type", GDALDataType, type);
  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env(), "data type argument needed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }
  if (info[0].IsString()) {
    std::string type_name = info[0].As<Napi::String>().Utf8Value();
    type = GDALGetDataTypeByName(type_name.c_str());
  } else if (info[0].IsNull() || info[0].IsUndefined()) {
    type = GDT_Unknown;
  } else {
    Napi::Error::New(node_gdal::napi_env(), "data type must be string or undefined").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (info.Length() > 1 && options->parse(info[1])) {
    return node_gdal::napi_env().Undefined(); // error parsing creation options, options->parse does the throwing
  }

  GDALAsyncableJob<GDALRasterBand *> job(ds->uid);
  job.persist(parent);
  job.main = [raw, type, options](const GDALExecutionProgress &) {
    CPLErrorReset();
    CPLErr err = raw->AddBand(type, options->get());
    if (err != CE_None) { throw CPLGetLastErrorMsg(); }
    return raw->GetRasterBand(raw->GetRasterCount());
  };
  job.rval = [raw](GDALRasterBand *r, const GetFromPersistentFunc &) { return RasterBand::New(r, raw); };
  return job.run(info, async, 2);
}

/**
 * Returns the number of bands.
 *
 * @method count
 * @instance
 * @memberof DatasetBands
 * @return {number}
 */

/**
 *
 * Returns the number of bands.
 * @async
 *
 * @method countAsync
 * @instance
 * @memberof DatasetBands
 *
 * @param {callback<number>} [callback=undefined]
 * @return {Promise<number>}
 */
GDAL_ASYNCABLE_DEFINE(DatasetBands::count) {

  Napi::Object parent =
    GDAL_GET_PRIVATE(info.This(), "parent_").As<Napi::Object>();
  Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(parent);

  if (!ds->isAlive()) {
    Napi::Error::New(node_gdal::napi_env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALDataset *raw = ds->get();
  GDALAsyncableJob<int> job(ds->uid);
  job.persist(parent);
  job.main = [raw](const GDALExecutionProgress &) {
    int count = raw->GetRasterCount();
    return count;
  };
  job.rval = [](int count, const GetFromPersistentFunc &) { return Napi::Number::New(node_gdal::napi_env(), count); };
  return job.run(info, async, 0);
}

/**
 * Returns the parent dataset.
 *
 * @readonly
 * @kind member
 * @name ds
 * @instance
 * @memberof DatasetBands
 * @type {Dataset}
 */
NAPI_GETTER(DatasetBands::dsGetter) {
  return GDAL_GET_PRIVATE(info.This(), "parent_");
}

} // namespace node_gdal
