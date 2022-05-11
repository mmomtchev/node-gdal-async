#include "gdal_drivers.hpp"
#include "../gdal_common.hpp"
#include "../gdal_driver.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> GDALDrivers::constructor;

void GDALDrivers::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(GDALDrivers::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("GDALDrivers").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan::SetPrototypeMethod(lcons, "count", count);
  Nan::SetPrototypeMethod(lcons, "get", get);
  Nan::SetPrototypeMethod(lcons, "getNames", getNames);

  GDALAllRegister();

  Nan::Set(target, Nan::New("GDALDrivers").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

GDALDrivers::GDALDrivers() : Nan::ObjectWrap() {
}

GDALDrivers::~GDALDrivers() {
}

/**
 * An collection of all {@link Driver}
 * registered with GDAL.
 *
 * @class GDALDrivers
 */
NAN_METHOD(GDALDrivers::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }
  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    GDALDrivers *f = static_cast<GDALDrivers *>(ptr);
    f->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create GDALDrivers directly");
    return;
  }
}

Local<Value> GDALDrivers::New() {
  Nan::EscapableHandleScope scope;

  GDALDrivers *wrapped = new GDALDrivers();

  v8::Local<v8::Value> ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(GDALDrivers::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();

  return scope.Escape(obj);
}

NAN_METHOD(GDALDrivers::toString) {
  info.GetReturnValue().Set(Nan::New("GDALDrivers").ToLocalChecked());
}

/**
 * Returns a driver with the specified name.
 *
 * Note: Prior to GDAL2.x there is a separate driver for vector VRTs and raster
 * VRTs. Use `"VRT:vector"` to fetch the vector VRT driver and `"VRT:raster"` to
 * fetch the raster VRT driver.
 *
 * @method get
 * @instance
 * @memberof GDALDrivers
 * @param {number|string} index 0-based index or driver name
 * @throws {Error}
 * @return {Driver}
 */
NAN_METHOD(GDALDrivers::get) {

  GDALDriver *gdal_driver;

  if (info.Length() == 0) {
    Nan::ThrowError("Either driver name or index must be provided");
    return;
  }

  if (info[0]->IsString()) {
    // try getting OGR driver first, and then GDAL driver if it fails
    // A driver named "VRT" exists for both GDAL and OGR, so if building
    // with <2.0 require user to specify which driver to pick
    std::string name = *Nan::Utf8String(info[0]);

    if (name == "VRT:vector") { name = "VRT"; }

    if (name == "VRT:raster") { name = "VRT"; }
    gdal_driver = GetGDALDriverManager()->GetDriverByName(name.c_str());
    if (gdal_driver) {
      info.GetReturnValue().Set(Driver::New(gdal_driver));
      return;
    }

  } else if (info[0]->IsNumber()) {
    int i = static_cast<int>(Nan::To<int64_t>(info[0]).ToChecked());

    gdal_driver = GetGDALDriverManager()->GetDriver(i);
    if (gdal_driver) {
      info.GetReturnValue().Set(Driver::New(gdal_driver));
      return;
    }

  } else {
    Nan::ThrowError("Argument must be string or integer");
    return;
  }

  NODE_THROW_LAST_CPLERR;
}

/**
 * Returns an array with the names of all the drivers registered with GDAL.
 *
 * @method getNames
 * @instance
 * @memberof GDALDrivers
 * @return {string[]}
 */
NAN_METHOD(GDALDrivers::getNames) {
  int gdal_count = GetGDALDriverManager()->GetDriverCount();
  int i, ogr_count = 0;
  std::string name;

  int n = gdal_count + ogr_count;

  Local<Array> driver_names = Nan::New<Array>(n);

  for (i = 0; i < gdal_count; ++i) {
    GDALDriver *driver = GetGDALDriverManager()->GetDriver(i);
    name = driver->GetDescription();
    Nan::Set(driver_names, i, SafeString::New(name.c_str()));
  }

  info.GetReturnValue().Set(driver_names);
}

/**
 * Returns the number of drivers registered with GDAL.
 *
 * @method count
 * @instance
 * @memberof GDALDrivers
 * @return {number}
 */
NAN_METHOD(GDALDrivers::count) {

  int count = GetGDALDriverManager()->GetDriverCount();

  info.GetReturnValue().Set(Nan::New<Integer>(count));
}

} // namespace node_gdal
