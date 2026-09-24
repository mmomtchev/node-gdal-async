#include "gdal_drivers.hpp"
#include "../gdal_common.hpp"
#include "../gdal_driver.hpp"

namespace node_gdal {

Napi::FunctionReference GDALDrivers::constructor;

void GDALDrivers::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(GDALDrivers);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "GDALDrivers",
    {
        METHOD(toString)
        METHOD(count)
        METHOD(get)
        METHOD(getNames)
    });

  GDALAllRegister();

  target.Set("GDALDrivers", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

GDALDrivers::GDALDrivers(const Napi::CallbackInfo &info) : GDALObject<GDALDrivers>(info) {
  // `gdal.drivers` is created by the module; the class is not constructible
  // from JS, which the test suite asserts
  if (info.Length() > 0 && info[0].IsExternal()) return;
  Napi::Error::New(info.Env(), "Cannot create GDALDrivers directly").ThrowAsJavaScriptException();
}

GDALDrivers::~GDALDrivers() {
}

/**
 * An collection of all {@link Driver}
 * registered with GDAL.
 *
 * @class GDALDrivers
 */

Napi::Value GDALDrivers::New() {

  std::vector<napi_value> args = {Napi::External<void>::New(node_gdal::napi_env(), nullptr)};
  Napi::Object obj = GDALDrivers::constructor.Value().New(args);

  return obj;
}

NAPI_METHOD(GDALDrivers::toString) {
  return Napi::String::New(node_gdal::napi_env(), "GDALDrivers");
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
NAPI_METHOD(GDALDrivers::get) {

  GDALDriver *gdal_driver;

  if (info.Length() == 0) {
    Napi::Error::New(node_gdal::napi_env(), "Either driver name or index must be provided").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (info[0].IsString()) {
    // try getting OGR driver first, and then GDAL driver if it fails
    // A driver named "VRT" exists for both GDAL and OGR, so if building
    // with <2.0 require user to specify which driver to pick
    std::string name = info[0].As<Napi::String>().Utf8Value();

    if (name == "VRT:vector") { name = "VRT"; }

    if (name == "VRT:raster") { name = "VRT"; }
    gdal_driver = GetGDALDriverManager()->GetDriverByName(name.c_str());
    if (gdal_driver) {
      return Driver::New(gdal_driver);
      return node_gdal::napi_env().Undefined();
    }

  } else if (info[0].IsNumber()) {
    int i = static_cast<int>(info[0].As<Napi::Number>().Int64Value());

    gdal_driver = GetGDALDriverManager()->GetDriver(i);
    if (gdal_driver) {
      return Driver::New(gdal_driver);
      return node_gdal::napi_env().Undefined();
    }

  } else {
    Napi::Error::New(node_gdal::napi_env(), "Argument must be string or integer").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  NODE_THROW_LAST_CPLERR;
  return node_gdal::napi_env().Undefined();
}

/**
 * Returns an array with the names of all the drivers registered with GDAL.
 *
 * @method getNames
 * @instance
 * @memberof GDALDrivers
 * @return {string[]}
 */
NAPI_METHOD(GDALDrivers::getNames) {
  int gdal_count = GetGDALDriverManager()->GetDriverCount();
  int i, ogr_count = 0;
  std::string name;

  int n = gdal_count + ogr_count;

  Napi::Array driver_names = Napi::Array::New(node_gdal::napi_env(), n);

  for (i = 0; i < gdal_count; ++i) {
    GDALDriver *driver = GetGDALDriverManager()->GetDriver(i);
    name = driver->GetDescription();
    driver_names.Set( i, SafeString::New(name.c_str()));
  }

  return driver_names;
}

/**
 * Returns the number of drivers registered with GDAL.
 *
 * @method count
 * @instance
 * @memberof GDALDrivers
 * @return {number}
 */
NAPI_METHOD(GDALDrivers::count) {

  int count = GetGDALDriverManager()->GetDriverCount();

  return Napi::Number::New(node_gdal::napi_env(), count);
}

} // namespace node_gdal
