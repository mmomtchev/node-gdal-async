#include <memory>
#include "dataset_bands.hpp"
#include "../gdal_common.hpp"
#include "../gdal_dataset.hpp"
#include "../gdal_rasterband.hpp"
#include "../utils/string_list.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> DatasetBands::constructor;

void DatasetBands::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(DatasetBands::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("DatasetBands").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan__SetPrototypeAsyncableMethod(lcons, "count", count);
  Nan__SetPrototypeAsyncableMethod(lcons, "create", create);
  Nan__SetPrototypeAsyncableMethod(lcons, "get", get);

  ATTR_DONT_ENUM(lcons, "ds", dsGetter, READ_ONLY_SETTER);

  Nan::Set(target, Nan::New("DatasetBands").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

DatasetBands::DatasetBands() : Nan::ObjectWrap() {
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
NAN_METHOD(DatasetBands::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }
  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    DatasetBands *f = static_cast<DatasetBands *>(ptr);
    f->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create DatasetBands directly");
    return;
  }
}

Local<Value> DatasetBands::New(Local<Value> ds_obj) {
  Nan::EscapableHandleScope scope;

  DatasetBands *wrapped = new DatasetBands();

  v8::Local<v8::Value> ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(DatasetBands::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();
  Nan::SetPrivate(obj, Nan::New("parent_").ToLocalChecked(), ds_obj);

  return scope.Escape(obj);
}

NAN_METHOD(DatasetBands::toString) {
  info.GetReturnValue().Set(Nan::New("DatasetBands").ToLocalChecked());
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

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(parent);

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
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
  job.run(info, async, 1);
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

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(parent);

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();
  GDALDataType type;
  std::shared_ptr<StringList> options(new StringList);

  // NODE_ARG_ENUM(0, "data type", GDALDataType, type);
  if (info.Length() < 1) {
    Nan::ThrowError("data type argument needed");
    return;
  }
  if (info[0]->IsString()) {
    std::string type_name = *Nan::Utf8String(info[0]);
    type = GDALGetDataTypeByName(type_name.c_str());
  } else if (info[0]->IsNull() || info[0]->IsUndefined()) {
    type = GDT_Unknown;
  } else {
    Nan::ThrowError("data type must be string or undefined");
    return;
  }

  if (info.Length() > 1 && options->parse(info[1])) {
    return; // error parsing creation options, options->parse does the throwing
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
  job.run(info, async, 2);
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

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(parent);

  if (!ds->isAlive()) {
    Nan::ThrowError("Dataset object has already been destroyed");
    return;
  }

  GDALDataset *raw = ds->get();
  GDALAsyncableJob<int> job(ds->uid);
  job.persist(parent);
  job.main = [raw](const GDALExecutionProgress &) {
    int count = raw->GetRasterCount();
    return count;
  };
  job.rval = [](int count, const GetFromPersistentFunc &) { return Nan::New<Integer>(count); };
  job.run(info, async, 0);
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
NAN_GETTER(DatasetBands::dsGetter) {
  info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked());
}

} // namespace node_gdal
