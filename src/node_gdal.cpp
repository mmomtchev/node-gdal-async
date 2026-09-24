// gdal-async
#include "gdal_common.hpp"

// gdal
#include <gdal.h>

// node-gdal
#include "gdal_algorithms.hpp"
#include "gdal_common.hpp"
#include "gdal_dataset.hpp"
#include "gdal_driver.hpp"
#include "gdal_rasterband.hpp"
#include "gdal_group.hpp"
#include "gdal_mdarray.hpp"
#include "gdal_dimension.hpp"
#include "gdal_attribute.hpp"
#include "gdal_warper.hpp"
#include "gdal_utils.hpp"
#include "gdal_algebra.hpp"

#include "gdal_coordinate_transformation.hpp"
#include "gdal_feature.hpp"
#include "gdal_feature_defn.hpp"
#include "gdal_field_defn.hpp"
#include "geometry/gdal_geometry.hpp"
#include "geometry/gdal_geometrycollection.hpp"
#include "gdal_layer.hpp"
#include "geometry/gdal_simplecurve.hpp"
#include "geometry/gdal_linearring.hpp"
#include "geometry/gdal_linestring.hpp"
#include "geometry/gdal_circularstring.hpp"
#include "geometry/gdal_compoundcurve.hpp"
#include "geometry/gdal_multilinestring.hpp"
#include "geometry/gdal_multicurve.hpp"
#include "geometry/gdal_multipoint.hpp"
#include "geometry/gdal_multipolygon.hpp"
#include "geometry/gdal_point.hpp"
#include "geometry/gdal_polygon.hpp"
#include "gdal_spatial_reference.hpp"
#include "gdal_memfile.hpp"
#include "gdal_fs.hpp"

#include "utils/field_types.hpp"

// collections
#include "collections/dataset_bands.hpp"
#include "collections/dataset_layers.hpp"
#include "collections/group_groups.hpp"
#include "collections/group_arrays.hpp"
#include "collections/group_dimensions.hpp"
#include "collections/group_attributes.hpp"
#include "collections/array_dimensions.hpp"
#include "collections/array_attributes.hpp"
#include "collections/feature_defn_fields.hpp"
#include "collections/feature_fields.hpp"
#include "collections/gdal_drivers.hpp"
#include "collections/geometry_collection_children.hpp"
#include "collections/layer_features.hpp"
#include "collections/layer_fields.hpp"
#include "collections/linestring_points.hpp"
#include "collections/polygon_rings.hpp"
#include "collections/compound_curves.hpp"
#include "collections/rasterband_overviews.hpp"
#include "collections/rasterband_pixels.hpp"
#include "collections/colortable.hpp"

// std
#include <sstream>
#include <string>
#include <vector>

namespace node_gdal {

FILE *log_file = NULL;
ObjectStore object_store;
bool eventLoopWarn = true;
::napi_env napi_env_storage = nullptr;

// Set by the env cleanup hook, see async.hpp. This definition has to stay outside
// the `extern "C"` block below - async.hpp declares it with C++ linkage, and
// `extern "C"` would give this definition C linkage instead.
bool gdalShuttingDown = false;

static NAPI_GETTER(LastErrorGetter) {

  int errtype = CPLGetLastErrorType();
  if (errtype == CE_None) return info.Env().Null();

  Napi::Object result = Napi::Object::New(info.Env());
  result.Set( Napi::String::New(info.Env(), "code"), Napi::Number::New(info.Env(), CPLGetLastErrorNo()));
  result.Set( Napi::String::New(info.Env(), "message"), Napi::String::New(info.Env(), CPLGetLastErrorMsg()));
  result.Set( Napi::String::New(info.Env(), "level"), Napi::Number::New(info.Env(), errtype));
  return result;
}

static NAPI_SETTER(LastErrorSetter) {

  if (value.IsNull()) {
    CPLErrorReset();
  } else {
    Napi::Error::New(info.Env(), "'lastError' only supports being set to null").ThrowAsJavaScriptException();
    return;
  }
}

static NAPI_GETTER(EventLoopWarningGetter) {
  return Napi::Boolean::New(info.Env(), eventLoopWarn);
}

static NAPI_SETTER(EventLoopWarningSetter) {
  if (!value.IsBoolean()) {
    Napi::Error::New(info.Env(), "'eventLoopWarning' must be a boolean value").ThrowAsJavaScriptException();
    return;
  }
  eventLoopWarn = value.As<Napi::Boolean>().Value();
}

extern "C" {

static NAPI_METHOD(QuietOutput) {
  CPLSetErrorHandler(CPLQuietErrorHandler);
  return info.Env().Undefined();
}

static NAPI_METHOD(VerboseOutput) {
  CPLSetErrorHandler(CPLDefaultErrorHandler);
  return info.Env().Undefined();
}

static NAPI_METHOD(StartLogging) {

#ifdef ENABLE_LOGGING
  std::string filename = "";
  NODE_ARG_STR(0, "filename", filename);
  if (filename.empty()) {
    Napi::Error::New(info.Env(), "Invalid filename").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  if (log_file) fclose(log_file);
  log_file = fopen(filename.c_str(), "w");
  if (!log_file) {
    Napi::Error::New(info.Env(), "Error creating log file").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

#else
  Napi::Error::New(info.Env(), "Logging requires node-gdal be compiled with --enable_logging=true").ThrowAsJavaScriptException();
#endif

  return info.Env().Undefined();
}

static NAPI_METHOD(StopLogging) {
#ifdef ENABLE_LOGGING
  if (log_file) {
    fclose(log_file);
    log_file = NULL;
  }
#endif

  return info.Env().Undefined();
}

static NAPI_METHOD(Log) {
  std::string msg;
  NODE_ARG_STR(0, "message", msg);
  msg = msg + "\n";

#ifdef ENABLE_LOGGING
  if (log_file) {
    fputs(msg.c_str(), log_file);
    fflush(log_file);
  }
#endif

  return info.Env().Undefined();
}

/*
 * Common code for sync and async opening.
 */
GDAL_ASYNCABLE_GLOBAL(gdal_open);
GDAL_ASYNCABLE_DEFINE(gdal_open) {

  std::string path;
  std::string mode = "r";

  NODE_ARG_STR(0, "path", path);
  NODE_ARG_OPT_STR(1, "mode", mode);

  unsigned int flags = 0;
  for (unsigned i = 0; i < mode.length(); i++) {
    if (mode[i] == 'r') {
      if (i < mode.length() - 1 && mode[i + 1] == '+') {
        flags |= GDAL_OF_UPDATE;
        i++;
      } else {
        flags |= GDAL_OF_READONLY;
      }
    } else if (mode[i] == 'm') {
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
      flags |= GDAL_OF_MULTIDIM_RASTER;
#else
      Napi::Error::New(info.Env(), "Multidimensional support requires GDAL 3.1").ThrowAsJavaScriptException();
#endif
    } else if (mode[i] == 't') {
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 10)
      flags |= GDAL_OF_THREAD_SAFE | GDAL_OF_RASTER;
#else
      Napi::Error::New(info.Env(), "Thread-safe read-only reading requires GDAL 3.10").ThrowAsJavaScriptException();
      return info.Env().Undefined();
#endif
    } else {
      Napi::Error::New(info.Env(), "Invalid open mode. Must contain only \"r\" or \"r+\" and \"m\" or \"t\" ").ThrowAsJavaScriptException();
      return info.Env().Undefined();
    }
  }
  flags |= GDAL_OF_VERBOSE_ERROR;

  GDALAsyncableJob<GDALDataset *> job(0);
  job.rval = [](GDALDataset *ds, const GetFromPersistentFunc &) { return Dataset::New(ds); };
  job.main = [path, flags](const GDALExecutionProgress &) {
    GDALDataset *ds = (GDALDataset *)GDALOpenEx(path.c_str(), flags, NULL, NULL, NULL);
    if (!ds) throw CPLGetLastErrorMsg();
    return ds;
  };
  return job.run(info, async, 2);
}

static NAPI_METHOD(setConfigOption) {

  std::string name;

  NODE_ARG_STR(0, "name", name);

  if (info.Length() < 2) {
    Napi::Error::New(info.Env(), "string or null value must be provided").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }
  if (info[1].IsString()) {
    std::string val = info[1].As<Napi::String>().Utf8Value();
    CPLSetConfigOption(name.c_str(), val.c_str());
  } else if (info[1].IsNull() || info[1].IsUndefined()) {
    CPLSetConfigOption(name.c_str(), NULL);
  } else {
    Napi::Error::New(info.Env(), "value must be a string or null").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  return info.Env().Undefined();
}

static NAPI_METHOD(getConfigOption) {

  std::string name;
  NODE_ARG_STR(0, "name", name);

  return SafeString::New(CPLGetConfigOption(name.c_str(), NULL));
}

/**
 * @typedef {string[]|Record<string, string|number|(string|number)[]>} StringOptions
 */

/**
 * Convert decimal degrees to degrees, minutes, and seconds string.
 *
 * @static
 * @method decToDMS
 * @param {number} angle
 * @param {string} axis `"lat"` or `"long"`
 * @param {number} [precision=2]
 * @return {string} A string nndnn'nn.nn'"L where n is a number and L is either
 * N or E
 */
static NAPI_METHOD(decToDMS) {

  double angle;
  std::string axis;
  int precision = 2;
  NODE_ARG_DOUBLE(0, "angle", angle);
  NODE_ARG_STR(1, "axis", axis);
  NODE_ARG_INT_OPT(2, "precision", precision);

  if (axis.length() > 0) { axis[0] = toupper(axis[0]); }
  if (axis != "Lat" && axis != "Long") {
    Napi::Error::New(info.Env(), "Axis must be 'lat' or 'long'").ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  return SafeString::New(GDALDecToDMS(angle, axis.c_str(), precision));
}

/**
 * Set paths where proj will search it data.
 *
 * @static
 * @method setPROJSearchPaths
 * @param {string} path `c:\ProjData`
 */
static NAPI_METHOD(setPROJSearchPath) {
  std::string path;

  NODE_ARG_STR(0, "path", path);

#if GDAL_VERSION_MAJOR >= 3
  const char *const paths[] = {path.c_str(), nullptr};
  OSRSetPROJSearchPaths(paths);
#endif

  return info.Env().Undefined();
}

static NAPI_METHOD(ThrowDummyCPLError) {
  CPLError(CE_Failure, CPLE_AppDefined, "Mock error");
  return info.Env().Undefined();
}

static NAPI_METHOD(isAlive) {

  long uid;
  NODE_ARG_INT(0, "uid", uid);

  return Napi::Boolean::New(info.Env(), object_store.isAlive(uid));
}

void Cleanup(void *) {
  gdalShuttingDown = true;
  object_store.cleanup();
}

Napi::Object Init(Napi::Env env, Napi::Object target) {
  // Note: the CJS and the ESM loader can both register the addon in the same
  // process. Each call gets its own empty exports object, so the registration
  // has to run every time - a guard here would hand the second one back empty.
  // everything that goes through the ambient node_gdal::napi_env() needs this
  napi_env_storage = env;
  mainV8ThreadId = std::this_thread::get_id();

  GDAL_SetAsyncableMethod(env, target, "open", gdal_open);
  GDAL_SetMethod(env, target, "setConfigOption", setConfigOption);
  GDAL_SetMethod(env, target, "getConfigOption", getConfigOption);
  GDAL_SetMethod(env, target, "decToDMS", decToDMS);
  GDAL_SetMethod(env, target, "setPROJSearchPath", setPROJSearchPath);
  GDAL_SetMethod(env, target, "_triggerCPLError", ThrowDummyCPLError); // for tests
  GDAL_SetMethod(env, target, "_isAlive", isAlive);                    // for tests

  Warper::Initialize(target);
  Algorithms::Initialize(target);
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 12)
  Algebra::Initialize(target);
#endif

  Driver::Initialize(target);
  Dataset::Initialize(target);
  RasterBand::Initialize(target);
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
  Group::Initialize(target);
  MDArray::Initialize(target);
  Dimension::Initialize(target);
  Attribute::Initialize(target);
#endif

  Layer::Initialize(target);
  Feature::Initialize(target);
  FeatureDefn::Initialize(target);
  FieldDefn::Initialize(target);
  Geometry::Initialize(target);
  Point::Initialize(target);
  SimpleCurve::Initialize(target);
  LineString::Initialize(target);
  LinearRing::Initialize(target);
  Polygon::Initialize(target);
  GeometryCollection::Initialize(target);
  MultiPoint::Initialize(target);
  MultiLineString::Initialize(target);
  MultiPolygon::Initialize(target);
  CircularString::Initialize(target);
  CompoundCurve::Initialize(target);
  MultiCurve::Initialize(target);

  SpatialReference::Initialize(target);
  CoordinateTransformation::Initialize(target);
  ColorTable::Initialize(target);

  DatasetBands::Initialize(target);
  DatasetLayers::Initialize(target);
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
  GroupGroups::Initialize(target);
  GroupArrays::Initialize(target);
  GroupDimensions::Initialize(target);
  GroupAttributes::Initialize(target);
  ArrayDimensions::Initialize(target);
  ArrayAttributes::Initialize(target);
#endif
  LayerFeatures::Initialize(target);
  FeatureFields::Initialize(target);
  LayerFields::Initialize(target);
  FeatureDefnFields::Initialize(target);
  GeometryCollectionChildren::Initialize(target);
  PolygonRings::Initialize(target);
  LineStringPoints::Initialize(target);
  CompoundCurveCurves::Initialize(target);
  RasterBandOverviews::Initialize(target);
  RasterBandPixels::Initialize(target);
  Memfile::Initialize(target);
  Utils::Initialize(target);
  VSI::Initialize(target);

  /**
   * The collection of all drivers registered with GDAL
   *
   * @readonly
   * @static
   * @constant
   * @name drivers
   * @type {GDALDrivers}
   */
  GDALDrivers::Initialize(target); // calls GDALRegisterAll()
  target.Set( Napi::String::New(env, "drivers"), GDALDrivers::New());

  /*
   * DMD Constants
   */

  /**
   * @final
   * @constant
   * @type {string}
   * @name DMD_LONGNAME
   */
  target.Set( Napi::String::New(env, "DMD_LONGNAME"), Napi::String::New(env, GDAL_DMD_LONGNAME));
  /**
   * @final
   * @constant
   * @name DMD_MIMETYPE
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DMD_MIMETYPE"), Napi::String::New(env, GDAL_DMD_MIMETYPE));
  /**
   * @final
   * @constant
   * @name DMD_HELPTOPIC
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DMD_HELPTOPIC"), Napi::String::New(env, GDAL_DMD_HELPTOPIC));
  /**
   * @final
   * @constant
   * @name DMD_EXTENSION
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DMD_EXTENSION"), Napi::String::New(env, GDAL_DMD_EXTENSION));
  /**
   * @final
   * @constant
   * @name DMD_CREATIONOPTIONLIST
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "DMD_CREATIONOPTIONLIST"),
    Napi::String::New(env, GDAL_DMD_CREATIONOPTIONLIST));
  /**
   * @final
   * @constant
   * @name DMD_CREATIONDATATYPES
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DMD_CREATIONDATATYPES"), Napi::String::New(env, GDAL_DMD_CREATIONDATATYPES));

  /*
   * CE Error levels
   */

  /**
   * Error level: (no error)
   *
   * @final
   * @constant
   * @name CE_None
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CE_None"), Napi::Number::New(env, CE_None));
  /**
   * Error level: Debug
   *
   * @final
   * @constant
   * @name CE_Debug
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CE_Debug"), Napi::Number::New(env, CE_Debug));
  /**
   * Error level: Warning
   *
   * @final
   * @constant
   * @name CE_Warning
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CE_Warning"), Napi::Number::New(env, CE_Warning));
  /**
   * Error level: Failure
   *
   * @final
   * @constant
   * @name CE_Failure
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CE_Failure"), Napi::Number::New(env, CE_Failure));
  /**
   * Error level: Fatal
   *
   * @final
   * @constant
   * @name CE_Fatal
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CE_Fatal"), Napi::Number::New(env, CE_Fatal));

  /*
   * CPL Error codes
   */

  /**
   * @final
   * @constant
   * @name CPLE_None
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CPLE_None"), Napi::Number::New(env, CPLE_None));
  /**
   * @final
   * @constant
   * @name CPLE_AppDefined
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CPLE_AppDefined"), Napi::Number::New(env, CPLE_AppDefined));
  /**
   * @final
   * @constant
   * @name CPLE_OutOfMemory
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CPLE_OutOfMemory"), Napi::Number::New(env, CPLE_OutOfMemory));
  /**
   * @final
   * @constant
   * @name CPLE_FileIO
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CPLE_FileIO"), Napi::Number::New(env, CPLE_FileIO));
  /**
   * @final
   * @constant
   * @name CPLE_OpenFailed
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CPLE_OpenFailed"), Napi::Number::New(env, CPLE_OpenFailed));
  /**
   * @final
   * @constant
   * @name CPLE_IllegalArg
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CPLE_IllegalArg"), Napi::Number::New(env, CPLE_IllegalArg));
  /**
   * @final
   * @constant
   * @name CPLE_NotSupported
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CPLE_NotSupported"), Napi::Number::New(env, CPLE_NotSupported));
  /**
   * @final
   * @constant
   * @name CPLE_AssertionFailed
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CPLE_AssertionFailed"), Napi::Number::New(env, CPLE_AssertionFailed));
  /**
   * @final
   * @constant
   * @name CPLE_NoWriteAccess
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CPLE_NoWriteAccess"), Napi::Number::New(env, CPLE_NoWriteAccess));
  /**
   * @final
   * @constant
   * @name CPLE_UserInterrupt
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CPLE_UserInterrupt"), Napi::Number::New(env, CPLE_UserInterrupt));
  /**
   * @final
   * @constant
   * @name CPLE_objectNull
   * @type {number}
   */
  target.Set( Napi::String::New(env, "CPLE_ObjectNull"), Napi::Number::New(env, CPLE_ObjectNull));

  /*
   * Driver Dataset creation constants
   */

  /**
   * @final
   * @constant
   * @name DCAP_CREATE
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DCAP_CREATE"), Napi::String::New(env, GDAL_DCAP_CREATE));
  /**
   * @final
   * @constant
   * @name DCAP_CREATECOPY
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DCAP_CREATECOPY"), Napi::String::New(env, GDAL_DCAP_CREATECOPY));
  /**
   * @final
   * @constant
   * @name DCAP_VIRTUALIO
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DCAP_VIRTUALIO"), Napi::String::New(env, GDAL_DCAP_VIRTUALIO));

  /*
   * OLC Constants
   */

  /**
   * @final
   * @constant
   * @name OLCRandomRead
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCRandomRead"), Napi::String::New(env, OLCRandomRead));
  /**
   * @final
   * @constant
   * @name OLCSequentialWrite
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCSequentialWrite"), Napi::String::New(env, OLCSequentialWrite));
  /**
   * @final
   * @constant
   * @name OLCRandomWrite
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCRandomWrite"), Napi::String::New(env, OLCRandomWrite));
  /**
   * @final
   * @constant
   * @name OLCFastSpatialFilter
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCFastSpatialFilter"), Napi::String::New(env, OLCFastSpatialFilter));
  /**
   * @final
   * @constant
   * @name OLCFastFeatureCount
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCFastFeatureCount"), Napi::String::New(env, OLCFastFeatureCount));
  /**
   * @final
   * @constant
   * @name OLCFastGetExtent
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCFastGetExtent"), Napi::String::New(env, OLCFastGetExtent));
  /**
   * @final
   * @constant
   * @name OLCCreateField
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCCreateField"), Napi::String::New(env, OLCCreateField));
  /**
   * @final
   * @constant
   * @name OLCDeleteField
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCDeleteField"), Napi::String::New(env, OLCDeleteField));
  /**
   * @final
   * @constant
   * @name OLCReorderFields
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCReorderFields"), Napi::String::New(env, OLCReorderFields));
  /**
   * @final
   * @constant
   * @name OLCAlterFieldDefn
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCAlterFieldDefn"), Napi::String::New(env, OLCAlterFieldDefn));
  /**
   * @final
   * @constant
   * @name OLCTransactions
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCTransactions"), Napi::String::New(env, OLCTransactions));
  /**
   * @final
   * @constant
   * @name OLCDeleteFeature
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCDeleteFeature"), Napi::String::New(env, OLCDeleteFeature));
  /**
   * @final
   * @constant
   * @name OLCFastSetNextByIndex
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCFastSetNextByIndex"), Napi::String::New(env, OLCFastSetNextByIndex));
  /**
   * @final
   * @constant
   * @name OLCStringsAsUTF8
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCStringsAsUTF8"), Napi::String::New(env, OLCStringsAsUTF8));
  /**
   * @final
   * @constant
   * @name OLCIgnoreFields
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCIgnoreFields"), Napi::String::New(env, OLCIgnoreFields));

#ifdef OLCCreateGeomField
  /**
   * @final
   * @constant
   * @name OLCCreateGeomField
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OLCCreateGeomField"), Napi::String::New(env, OLCCreateGeomField));
#endif
#ifdef ODsCCreateGeomFieldAfterCreateLayer

  /*
   * ODsC constants
   */

  /**
   * @final
   * @constant
   * @name ODsCCreateLayer
   * @type {string}
   */
  target.Set( Napi::String::New(env, "ODsCCreateLayer"), Napi::String::New(env, ODsCCreateLayer));
  /**
   * @final
   * @constant
   * @name ODsCDeleteLayer
   * @type {string}
   */
  target.Set( Napi::String::New(env, "ODsCDeleteLayer"), Napi::String::New(env, ODsCDeleteLayer));
  /**
   * @final
   * @constant
   * @name ODsCCreateGeomFieldAfterCreateLayer
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "ODsCCreateGeomFieldAfterCreateLayer"),
    Napi::String::New(env, ODsCCreateGeomFieldAfterCreateLayer));
#endif
  /**
   * @final
   * @constant
   * @name ODrCCreateDataSource
   * @type {string}
   */
  target.Set( Napi::String::New(env, "ODrCCreateDataSource"), Napi::String::New(env, ODrCCreateDataSource));
  /**
   * @final
   * @constant
   * @name ODrCDeleteDataSource
   * @type {string}
   */
  target.Set( Napi::String::New(env, "ODrCDeleteDataSource"), Napi::String::New(env, ODrCDeleteDataSource));

  /*
   * open flags
   */

  /**
   * @final
   * @constant
   * @name GA_Readonly
   * @type {number}
   */
  NODE_DEFINE_CONSTANT(target, GA_ReadOnly);

  /**
   * @final
   * @constant
   * @name GA_Update
   * @type {number}
   */
  NODE_DEFINE_CONSTANT(target, GA_Update);

  /*
   * RasterIO flags
   */

  /**
   * @final
   * @constant
   * @name GF_Read
   * @type {number}
   */
  NODE_DEFINE_CONSTANT(target, GF_Read);

  /**
   * @final
   * @constant
   * @name GF_Write
   * @type {number}
   */
  NODE_DEFINE_CONSTANT(target, GF_Write);

  /*
   * Pixel data types.
   */

  /**
   * Unknown or unspecified type
   * @final
   * @constant
   * @name GDT_Unknown
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_Unknown"), env.Undefined());
  /**
   * Eight bit unsigned integer
   * @final
   * @constant
   * @name GDT_Byte
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_Byte"), Napi::String::New(env, GDALGetDataTypeName(GDT_Byte)));
  target.Set( Napi::String::New(env, "GDT_UInt8"), Napi::String::New(env, GDALGetDataTypeName(GDT_Byte)));
  /**
   * Sixteen bit unsigned integer
   * @final
   * @constant
   * @name GDT_UInt16
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_UInt16"), Napi::String::New(env, GDALGetDataTypeName(GDT_UInt16)));
  /**
   * Sixteen bit signed integer
   * @final
   * @constant
   * @name GDT_Int16
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_Int16"), Napi::String::New(env, GDALGetDataTypeName(GDT_Int16)));
  /**
   * Thirty two bit unsigned integer
   * @final
   * @constant
   * @name GDT_UInt32
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_UInt32"), Napi::String::New(env, GDALGetDataTypeName(GDT_UInt32)));
  /**
   * Thirty two bit signed integer
   * @final
   * @constant
   * @name GDT_Int32
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_Int32"), Napi::String::New(env, GDALGetDataTypeName(GDT_Int32)));
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
  /**
   * Sixty four bit signed integer
   * @final
   * @constant
   * @name GDT_Int64
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_Int64"), Napi::String::New(env, GDALGetDataTypeName(GDT_Int64)));
  /**
   * Sixty four bit unsigned integer
   * @final
   * @constant
   * @name GDT_UInt64
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_UInt64"), Napi::String::New(env, GDALGetDataTypeName(GDT_UInt64)));
#endif
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
  /**
   * Sixteen bit floating point
   * @final
   * @constant
   * @name GDT_Float16
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_Float16"), Napi::String::New(env, GDALGetDataTypeName(GDT_Float16)));
#endif
  /**
   * Thirty two bit floating point
   * @final
   * @constant
   * @name GDT_Float32
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_Float32"), Napi::String::New(env, GDALGetDataTypeName(GDT_Float32)));
  /**
   * Sixty four bit floating point
   * @final
   * @constant
   * @name GDT_Float64
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_Float64"), Napi::String::New(env, GDALGetDataTypeName(GDT_Float64)));
  /**
   * Complex Int16
   * @final
   * @constant
   * @name GDT_CInt16
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_CInt16"), Napi::String::New(env, GDALGetDataTypeName(GDT_CInt16)));
  /**
   * Complex Int32
   * @final
   * @constant
   * @name GDT_CInt32
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_CInt32"), Napi::String::New(env, GDALGetDataTypeName(GDT_CInt32)));
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
  /**
   * Complex Float16
   * @final
   * @constant
   * @name GDT_CFloat16
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_CFloat16"), Napi::String::New(env, GDALGetDataTypeName(GDT_CFloat16)));
#endif
  /**
   * Complex Float32
   * @final
   * @constant
   * @name GDT_CFloat32
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_CFloat32"), Napi::String::New(env, GDALGetDataTypeName(GDT_CFloat32)));
  /**
   * Complex Float64
   * @final
   * @constant
   * @name GDT_CFloat64
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GDT_CFloat64"), Napi::String::New(env, GDALGetDataTypeName(GDT_CFloat64)));

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
  /**
   * String extended type for MDArrays (GDAL >= 3.1)
   * @final
   * @constant
   * @name GEDTC_String
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GEDTC_String"), Napi::String::New(env, "String"));

  /**
   * String extended type for MDArrays (GDAL >= 3.1)
   * @final
   * @constant
   * @name GEDTC_Compound
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GEDTC_Compound"), Napi::String::New(env, "Compound"));
#endif

  /*
   * Justification
   */

  /**
   * @final
   * @constant
   * @name OJUndefined
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OJUndefined"), env.Undefined());
  /**
   * @final
   * @constant
   * @name OJLeft
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OJLeft"), Napi::String::New(env, "Left"));
  /**
   * @final
   * @constant
   * @name OJRight
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OJRight"), Napi::String::New(env, "Right"));

  /*
   * Color interpretation constants
   */

  /**
   * @final
   * @constant
   * @name GCI_Undefined
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GCI_Undefined"), env.Undefined());
  /**
   * @final
   * @constant
   * @name GCI_GrayIndex
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_GrayIndex"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_GrayIndex)));
  /**
   * @final
   * @constant
   * @name GCI_PaletteIndex
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_PaletteIndex"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_PaletteIndex)));
  /**
   * @final
   * @constant
   * @name GCI_RedBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_RedBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_RedBand)));
  /**
   * @final
   * @constant
   * @name GCI_GreenBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_GreenBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_GreenBand)));
  /**
   * @final
   * @constant
   * @name GCI_BlueBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_BlueBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_BlueBand)));
  /**
   * @final
   * @constant
   * @name GCI_AlphaBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_AlphaBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_AlphaBand)));
  /**
   * @final
   * @constant
   * @name GCI_HueBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_HueBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_HueBand)));
  /**
   * @final
   * @constant
   * @name GCI_SaturationBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_SaturationBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_SaturationBand)));
  /**
   * @final
   * @constant
   * @name GCI_LightnessBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_LightnessBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_LightnessBand)));
  /**
   * @final
   * @constant
   * @name GCI_CyanBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_CyanBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_CyanBand)));
  /**
   * @final
   * @constant
   * @name GCI_MagentaBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_MagentaBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_MagentaBand)));
  /**
   * @final
   * @constant
   * @name GCI_YellowBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_YellowBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_YellowBand)));
  /**
   * @final
   * @constant
   * @name GCI_BlackBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_BlackBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_BlackBand)));
  /**
   * @final
   * @constant
   * @name GCI_YCbCr_YBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_YCbCr_YBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_YCbCr_YBand)));
  /**
   * @final
   * @constant
   * @name GCI_YCbCr_CbBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_YCbCr_CbBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_YCbCr_CbBand)));
  /**
   * @final
   * @constant
   * @name GCI_YCbCr_CrBand
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "GCI_YCbCr_CrBand"),
    Napi::String::New(env, GDALGetColorInterpretationName(GCI_YCbCr_CrBand)));

  /*
   * Palette types.
   */

  /**
   * Grayscale, only c1 defined
   * @final
   * @constant
   * @name GPI_Gray
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GPI_Gray"), Napi::String::New(env, "Gray"));

  /**
   * RGBA, alpha in c4
   * @final
   * @constant
   * @name GPI_RGB
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GPI_RGB"), Napi::String::New(env, "RGB"));

  /**
   * CMYK
   * @final
   * @constant
   * @name GPI_CMYK
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GPI_CMYK"), Napi::String::New(env, "CMYK"));

  /**
   * HLS, c4 is not defined
   * @final
   * @constant
   * @name GPI_HLS
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GPI_HLS"), Napi::String::New(env, "HLS"));

  /*
   * WKB Variants
   */

  /**
   * Old-style 99-402 extended dimension (Z) WKB types.
   * Synonymous with 'wkbVariantOldOgc' (gdal >= 2.0)
   *
   * @final
   * @constant
   * @name wkbVariantOgc
   * @type {string}
   */
  target.Set( Napi::String::New(env, "wkbVariantOgc"), Napi::String::New(env, "OGC"));

  /**
   * Old-style 99-402 extended dimension (Z) WKB types.
   * Synonymous with 'wkbVariantOgc' (gdal < 2.0)
   *
   * @final
   * @constant
   * @name wkbVariantOldOgc
   * @type {string}
   */
  target.Set( Napi::String::New(env, "wkbVariantOldOgc"), Napi::String::New(env, "OGC"));

  /**
   * SFSQL 1.2 and ISO SQL/MM Part 3 extended dimension (Z&M) WKB types.
   *
   * @final
   * @constant
   * @name wkbVariantIso
   * @type {string}
   */
  target.Set( Napi::String::New(env, "wkbVariantIso"), Napi::String::New(env, "ISO"));

  /*
   * WKB Byte Ordering
   */

  /**
   * @final
   * @constant
   * @name wkbXDR
   * @type {string}
   */
  target.Set( Napi::String::New(env, "wkbXDR"), Napi::String::New(env, "MSB"));
  /**
   * @final
   * @constant
   * @name wkbNDR
   * @type {string}
   */
  target.Set( Napi::String::New(env, "wkbNDR"), Napi::String::New(env, "LSB"));

  /*
   * WKB Geometry Types
   */

  /**
   * @final
   *
   * The `wkb25DBit` constant can be used to convert between 2D types to 2.5D
   * types
   *
   * @example
   *
   * // 2 -> 2.5D
   * wkbPoint25D = gdal.wkbPoint | gdal.wkb25DBit
   *
   * // 2.5D -> 2D (same as wkbFlatten())
   * wkbPoint = gdal.wkbPoint25D & (~gdal.wkb25DBit)
   *
   * @constant
   * @name wkb25DBit
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkb25DBit"), Napi::Number::New(env, wkb25DBit));

  int wkbLinearRing25D = wkbLinearRing | wkb25DBit;

  /**
   * @final
   * @constant
   * @name wkbUnknown
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbUnknown"), Napi::Number::New(env, wkbUnknown));
  /**
   * @final
   * @constant
   * @name wkbPoint
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbPoint"), Napi::Number::New(env, wkbPoint));
  /**
   * @final
   * @constant
   * @name wkbLineString
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbLineString"), Napi::Number::New(env, wkbLineString));
  /**
   * @final
   * @constant
   * @name wkbCircularString
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbCircularString"), Napi::Number::New(env, wkbCircularString));
  /**
   * @final
   * @constant
   * @name wkbCompoundCurve
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbCompoundCurve"), Napi::Number::New(env, wkbCompoundCurve));
  /**
   * @final
   * @constant
   * @name wkbMultiCurve
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbMultiCurve"), Napi::Number::New(env, wkbMultiCurve));
  /**
   * @final
   * @constant
   * @name wkbPolygon
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbPolygon"), Napi::Number::New(env, wkbPolygon));
  /**
   * @final
   * @constant
   * @name wkbMultiPoint
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbMultiPoint"), Napi::Number::New(env, wkbMultiPoint));
  /**
   * @final
   * @constant
   * @name wkbMultiLineString
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbMultiLineString"), Napi::Number::New(env, wkbMultiLineString));
  /**
   * @final
   * @constant
   * @name wkbMultiPolygon
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbMultiPolygon"), Napi::Number::New(env, wkbMultiPolygon));
  /**
   * @final
   * @constant
   * @name wkbGeometryCollection
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbGeometryCollection"), Napi::Number::New(env, wkbGeometryCollection));
  /**
   * @final
   * @constant
   * @name wkbNone
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbNone"), Napi::Number::New(env, wkbNone));
  /**
   * @final
   * @constant
   * @name wkbLinearRing
   * @type {string}
   */
  target.Set( Napi::String::New(env, "wkbLinearRing"), Napi::Number::New(env, wkbLinearRing));
  /**
   * @final
   * @constant
   * @name wkbPoint25D
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbPoint25D"), Napi::Number::New(env, wkbPoint25D));
  /**
   * @final
   * @constant
   * @name wkbLineString25D
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbLineString25D"), Napi::Number::New(env, wkbLineString25D));
  /**
   * @final
   * @constant
   * @name wkbPolygon25D
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbPolygon25D"), Napi::Number::New(env, wkbPolygon25D));
  /**
   * @final
   * @constant
   * @name wkbMultiPoint25D
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbMultiPoint25D"), Napi::Number::New(env, wkbMultiPoint25D));
  /**
   * @final
   * @constant
   * @name wkbMultiLineString25D
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbMultiLineString25D"), Napi::Number::New(env, wkbMultiLineString25D));
  /**
   * @final
   * @constant
   * @name wkbMultiPolygon25D
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbMultiPolygon25D"), Napi::Number::New(env, wkbMultiPolygon25D));
  /**
   * @final
   * @constant
   * @name wkbGeometryCollection25D
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbGeometryCollection25D"), Napi::Number::New(env, wkbGeometryCollection25D));
  /**
   * @final
   * @constant
   * @name wkbLinearRing25D
   * @type {number}
   */
  target.Set( Napi::String::New(env, "wkbLinearRing25D"), Napi::Number::New(env, wkbLinearRing25D));

  /*
   * Field types
   */

  /**
   * @final
   * @constant
   * @name OFTInteger
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OFTInteger"), Napi::String::New(env, getFieldTypeName(OFTInteger)));
  /**
   * @final
   * @constant
   * @name OFTIntegerList
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OFTIntegerList"), Napi::String::New(env, getFieldTypeName(OFTIntegerList)));

  /**
   * @final
   * @constant
   * @name OFTInteger64
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OFTInteger64"), Napi::String::New(env, getFieldTypeName(OFTInteger64)));
  /**
   * @final
   * @constant
   * @name OFTInteger64List
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "OFTInteger64List"),
    Napi::String::New(env, getFieldTypeName(OFTInteger64List)));
  /**
   * @final
   * @constant
   * @name OFTReal
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OFTReal"), Napi::String::New(env, getFieldTypeName(OFTReal)));
  /**
   * @final
   * @constant
   * @name OFTRealList
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OFTRealList"), Napi::String::New(env, getFieldTypeName(OFTRealList)));
  /**
   * @final
   * @constant
   * @name OFTString
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OFTString"), Napi::String::New(env, getFieldTypeName(OFTString)));
  /**
   * @final
   * @constant
   * @name OFTStringList
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OFTStringList"), Napi::String::New(env, getFieldTypeName(OFTStringList)));
  /**
   * @final
   * @constant
   * @name OFTWideString
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OFTWideString"), Napi::String::New(env, getFieldTypeName(OFTWideString)));
  /**
   * @final
   * @constant
   * @name OFTWideStringList
   * @type {string}
   */
  target.Set(
    Napi::String::New(env, "OFTWideStringList"),
    Napi::String::New(env, getFieldTypeName(OFTWideStringList)));
  /**
   * @final
   * @constant
   * @name OFTBinary
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OFTBinary"), Napi::String::New(env, getFieldTypeName(OFTBinary)));
  /**
   * @final
   * @constant
   * @name OFTDate
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OFTDate"), Napi::String::New(env, getFieldTypeName(OFTDate)));
  /**
   * @final
   * @constant
   * @name OFTTime
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OFTTime"), Napi::String::New(env, getFieldTypeName(OFTTime)));
  /**
   * @final
   * @constant
   * @name OFTDateTime
   * @type {string}
   */
  target.Set( Napi::String::New(env, "OFTDateTime"), Napi::String::New(env, getFieldTypeName(OFTDateTime)));

  /*
   * Resampling options that can be used with the gdal.reprojectImage() and gdal.RasterBandPixels.read methods.
   */

  /**
   * @final
   * @constant
   * @name GRA_NearestNeighbor
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GRA_NearestNeighbor"), Napi::String::New(env, "NearestNeighbor"));
  /**
   * @final
   * @constant
   * @name GRA_Bilinear
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GRA_Bilinear"), Napi::String::New(env, "Bilinear"));
  /**
   * @final
   * @constant
   * @name GRA_Cubic
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GRA_Cubic"), Napi::String::New(env, "Cubic"));
  /**
   * @final
   * @constant
   * @name GRA_CubicSpline
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GRA_CubicSpline"), Napi::String::New(env, "CubicSpline"));
  /**
   * @final
   * @constant
   * @name GRA_Lanczos
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GRA_Lanczos"), Napi::String::New(env, "Lanczos"));
  /**
   * @final
   * @constant
   * @name GRA_Average
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GRA_Average"), Napi::String::New(env, "Average"));
  /**
   * @final
   * @constant
   * @name GRA_Mode
   * @type {string}
   */
  target.Set( Napi::String::New(env, "GRA_Mode"), Napi::String::New(env, "Mode"));

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
  /*
   * Dimension types for gdal.Dimension (GDAL >= 3.3)
   */

  /**
   * @final
   * @constant
   * @name DIM_HORIZONTAL_X
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIM_HORIZONTAL_X"), Napi::String::New(env, GDAL_DIM_TYPE_HORIZONTAL_X));

  /**
   * @final
   * @constant
   * @name DIM_HORIZONTAL_Y
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIM_HORIZONTAL_Y"), Napi::String::New(env, GDAL_DIM_TYPE_HORIZONTAL_Y));

  /**
   * @final
   * @constant
   * @name DIM_VERTICAL
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIM_VERTICAL"), Napi::String::New(env, GDAL_DIM_TYPE_VERTICAL));

  /**
   * @final
   * @constant
   * @name DIM_TEMPORAL
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIM_TEMPORAL"), Napi::String::New(env, GDAL_DIM_TYPE_TEMPORAL));

  /**
   * @final
   * @constant
   * @name DIM_PARAMETRIC
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIM_PARAMETRIC"), Napi::String::New(env, GDAL_DIM_TYPE_PARAMETRIC));
#endif

  /*
   * Direction types for gdal.Dimension (GDAL >= 3.3)
   */

  /**
   * @final
   * @constant
   * @name DIR_EAST
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIR_EAST"), Napi::String::New(env, "EAST"));

  /**
   * @final
   * @constant
   * @name DIR_WEST
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIR_WEST"), Napi::String::New(env, "WEST"));

  /**
   * @final
   * @constant
   * @name DIR_SOUTH
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIR_SOUTH"), Napi::String::New(env, "SOUTH"));

  /**
   * @final
   * @constant
   * @name DIR_NORTH
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIR_NORTH"), Napi::String::New(env, "NORTH"));

  /**
   * @final
   * @constant
   * @name DIR_UP
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIR_UP"), Napi::String::New(env, "UP"));

  /**
   * @final
   * @constant
   * @name DIR_DOWN
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIR_DOWN"), Napi::String::New(env, "DOWN"));

  /**
   * @final
   * @constant
   * @name DIR_FUTURE
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIR_FUTURE"), Napi::String::New(env, "FUTURE"));

  /**
   * @final
   * @constant
   * @name DIR_PAST
   * @type {string}
   */
  target.Set( Napi::String::New(env, "DIR_PAST"), Napi::String::New(env, "PAST"));

  /**
   * GDAL version (not the binding version)
   *
   * @final
   * @constant {string} version
   */
  target.Set( Napi::String::New(env, "version"), Napi::String::New(env, GDAL_RELEASE_NAME));

  /**
   * GDAL library - system library (false) or bundled (true)
   *
   * @final
   * @constant {boolean} bundled
   */
#ifdef BUNDLED_GDAL
  target.Set( Napi::String::New(env, "bundled"), Napi::Boolean::New(env, true));
#else
  target.Set( Napi::String::New(env, "bundled"), Napi::Boolean::New(env, false));
#endif

  /**
   * Details about the last error that occurred. The property
   * will be null or an object containing three properties: "number",
   * "message", and "type".
   *
   * @var {object} lastError
   */
  GDAL_DEFINE_ACCESSOR(target, "lastError", LastErrorGetter, LastErrorSetter);

  /**
   * Should a warning be emitted to stderr when a synchronous operation
   * is blocking the event loop, can be safely disabled unless
   * the user application needs to remain responsive at all times
   * Use `(gdal as any).eventLoopWarning = false` to set the value from TypeScript
   *
   * @var {boolean} eventLoopWarning
   */
  GDAL_DEFINE_ACCESSOR(target, "eventLoopWarning", EventLoopWarningGetter, EventLoopWarningSetter);

  // Napi::Object versions = Napi::Object::New(env);
  // versions.Set( Napi::String::New(env, "node"),
  // Napi::Number::New(env, NODE_VERSION+1)); versions.Set(
  // Napi::String::New(env, "v8"), Napi::Number::New(env, V8::GetVersion()));
  // target.Set( Napi::String::New(env, "versions"), versions);

  /**
   * Disables all output.
   *
   * @static
   * @method quiet
   */
  GDAL_SetMethod(env, target, "quiet", QuietOutput);

  /**
   * Displays extra debugging information from GDAL.
   *
   * @static
   * @method verbose
   */
  GDAL_SetMethod(env, target, "verbose", VerboseOutput);

  GDAL_SetMethod(env, target, "startLogging", StartLogging);
  GDAL_SetMethod(env, target, "stopLogging", StopLogging);
  GDAL_SetMethod(env, target, "log", Log);

  Napi::Object supports = Napi::Object::New(env);
  target.Set( Napi::String::New(env, "supports"), supports);

  target.Set(Napi::String::New(env, "CPLE_OpenFailed"), Napi::Number::New(env, CPLE_OpenFailed));
  target.Set(Napi::String::New(env, "CPLE_IllegalArg"), Napi::Number::New(env, CPLE_IllegalArg));
  target.Set(Napi::String::New(env, "CPLE_NotSupported"), Napi::Number::New(env, CPLE_NotSupported));
  target.Set(Napi::String::New(env, "CPLE_AssertionFailed"), Napi::Number::New(env, CPLE_AssertionFailed));
  target.Set(Napi::String::New(env, "CPLE_NoWriteAccess"), Napi::Number::New(env, CPLE_NoWriteAccess));
  target.Set(Napi::String::New(env, "CPLE_UserInterrupt"), Napi::Number::New(env, CPLE_UserInterrupt));
  napi_add_env_cleanup_hook(env, Cleanup, nullptr);

  return target;
}
}

} // namespace node_gdal

// NODE_API_MODULE concatenates the registration function into an identifier
// (__napi_##regfunc), so it has to be a plain name - not node_gdal::Init
static Napi::Object GDALInit(Napi::Env env, Napi::Object target) {
  try {
    node_gdal::Init(env, target);
  } catch (const Napi::Error &) {
    // Init completes its work; a failing N-API call follows it. Swallowing it
    // here is what keeps the module loadable - see the note in the header.
  }
  return target;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, GDALInit)
