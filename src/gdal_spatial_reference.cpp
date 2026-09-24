
#include "gdal_spatial_reference.hpp"
#include "gdal_common.hpp"
#include "utils/string_list.hpp"
#include "async.hpp"

namespace node_gdal {

Napi::FunctionReference SpatialReference::constructor;

void SpatialReference::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(SpatialReference);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "SpatialReference",
    {
        METHOD(toString)
        METHOD_AS("toWKT", exportToWKT)
        METHOD_AS("toPrettyWKT", exportToPrettyWKT)
        METHOD_AS("toProj4", exportToProj4)
        METHOD_AS("toXML", exportToXML)
        METHOD(clone)
        METHOD(cloneGeogCS)
        METHOD(setWellKnownGeogCS)
        METHOD(morphToESRI)
        METHOD(morphFromESRI)
        METHOD(EPSGTreatsAsLatLong)
        METHOD(EPSGTreatsAsNorthingEasting)
        METHOD(getLinearUnits)
        METHOD(getAngularUnits)
        METHOD(isGeographic)
        METHOD(isGeocentric)
        METHOD(isProjected)
        METHOD(isLocal)
        METHOD_AS("isVectical", isVertical)
        METHOD(isVertical)
        METHOD(isCompound)
        METHOD(isSameGeogCS)
        METHOD(isSameVertCS)
        METHOD(isSame)
        METHOD(getAuthorityName)
        METHOD(getAuthorityCode)
        METHOD(getAttrValue)
        METHOD(autoIdentifyEPSG)
        METHOD(validate)
    });

  GDAL_SetAsyncableMethod(env, lcons, "fromUserInput", fromUserInput);
  GDAL_SetMethod(env, lcons, "fromWKT", fromWKT);
  GDAL_SetMethod(env, lcons, "fromProj4", fromProj4);
  GDAL_SetMethod(env, lcons, "fromEPSG", fromEPSG);
  GDAL_SetMethod(env, lcons, "fromEPSGA", fromEPSGA);
  GDAL_SetMethod(env, lcons, "fromESRI", fromESRI);
  GDAL_SetMethod(env, lcons, "fromWMSAUTO", fromWMSAUTO);
  GDAL_SetMethod(env, lcons, "fromXML", fromXML);
  GDAL_SetMethod(env, lcons, "fromURN", fromURN);
  GDAL_SetAsyncableMethod(env, lcons, "fromCRSURL", fromCRSURL);
  GDAL_SetAsyncableMethod(env, lcons, "fromURL", fromURL);
  GDAL_SetMethod(env, lcons, "fromMICoordSys", fromMICoordSys);

  target.Set("SpatialReference", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

SpatialReference::~SpatialReference() {
  dispose();
}

void SpatialReference::dispose() {
  if (this_) {
    LOG("Disposing SpatialReference [%p] (%s)", this_, owned_ ? "owned" : "unowned");
    object_store.dispose(uid);
    if (owned_) {
      // Decrements the reference count by one, and destroy if zero.
      this_->Release();
    }
    LOG("Disposed SpatialReference [%p]", this_);
    this_ = NULL;
  }
}

/**
 * This class respresents a OpenGIS Spatial Reference System, and contains
 * methods for converting between this object organization and well known text
 * (WKT) format.
 *
 * @constructor
 * @class SpatialReference
 * @param {string} [wkt]
 */
SpatialReference::SpatialReference(const Napi::CallbackInfo &info)
  : GDALObject<SpatialReference>(info), this_(nullptr), owned_(false), uid(0) {
  // Internal construction: the factory passes the OGR object through an External
  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRSpatialReference>>().Data();
    LOG("Created SpatialReference [%p]", this_);
    return;
  }

  // Constructed from JS: SpatialReference([wkt])
  // (the NODE_ARG_* macros cannot be used here: they return a value on error,
  // which a constructor cannot do)
  std::string wkt("");
  if (info.Length() > 0 && !info[0].IsNull() && !info[0].IsUndefined()) {
    if (!info[0].IsString()) {
      Napi::TypeError::New(info.Env(), "wkt must be a string").ThrowAsJavaScriptException();
      return;
    }
    wkt = info[0].As<Napi::String>().Utf8Value();
  }

  // sets reference count to one
  OGRSpatialReference *srs = new OGRSpatialReference(wkt.empty() ? 0 : wkt.c_str());
  if (!wkt.empty()) {
    OGRChar *wkt_c = (OGRChar *)wkt.c_str();
    int err = srs->importFromWkt(&wkt_c);
    if (err) {
      delete srs;
      NODE_THROW_OGRERR(err);
      return;
    }
  }

  this_ = srs;
  owned_ = true;
  uid = object_store.add(srs, *this, 0);
  LOG("Created SpatialReference [%p]", this_);
}

// Currently read-only spatial references are copied.
// Modifying a spatial reference that should have been
// read-only modifies a shadow copy without any real effect.
// TODO: Implement proper read-only objects that throw
// Fixing this for srs obtained from a Layer is trivial
// But fixing it for srs obtained from a Feature requires moving the Features to the ObjectStore

Napi::Value SpatialReference::New(const OGRSpatialReference *srs) {
  // This const_cast is not a problem since the very first operation
  // below is copying the SpatialReference
  return SpatialReference::New(const_cast<OGRSpatialReference *>(srs), false);
}

Napi::Value SpatialReference::New(OGRSpatialReference *raw, bool owned) {
  Napi::Env env = node_gdal::napi_env();

  if (!raw) { return env.Null(); }
  if (object_store.has(raw)) { return object_store.get(raw); }

  OGRSpatialReference *cloned_srs = raw;
  if (!owned) cloned_srs = raw->Clone();

  std::vector<napi_value> args = {Napi::External<OGRSpatialReference>::New(env, cloned_srs)};
  Napi::Object obj = constructor.Value().New(args);
  SpatialReference *wrapped = node_gdal::UnwrapWrapped<SpatialReference>(obj);
  wrapped->owned_ = true;

  wrapped->uid = object_store.add(raw, *wrapped, 0);

  return obj;
}

NAPI_METHOD(SpatialReference::toString) {
  return Napi::String::New(node_gdal::napi_env(), "SpatialReference");
}

/**
 * Set a GeogCS based on well known name.
 *
 * @method setWellKnownGeogCS
 * @instance
 * @memberof SpatialReference
 * @param {string} name
 */
NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_STRING_PARAM(
  SpatialReference, setWellKnownGeogCS, SetWellKnownGeogCS, "input");

/**
 * Convert in place to ESRI WKT format.
 *
 * @throws {Error}
 * @method morphToESRI
 * @instance
 * @memberof SpatialReference
 */
NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT(SpatialReference, morphToESRI, morphToESRI);

/**
 * Convert in place from ESRI WKT format.
 *
 * @throws {Error}
 * @method morphFromESRI
 * @instance
 * @memberof SpatialReference
 */
NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT(SpatialReference, morphFromESRI, morphFromESRI);

/**
 * This method returns `true` if EPSG feels this geographic coordinate system
 * should be treated as having lat/long coordinate ordering.
 *
 * Currently this returns `true` for all geographic coordinate systems with an
 * EPSG code set, and AXIS values set defining it as lat, long. Note that
 * coordinate systems with an EPSG code and no axis settings will be assumed
 * to not be lat/long.
 *
 * `false` will be returned for all coordinate systems that are not geographic,
 * or that do not have an EPSG code set.
 *
 * @method EPSGTreatsAsLatLong
 * @instance
 * @memberof SpatialReference
 * @return {boolean}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(SpatialReference, EPSGTreatsAsLatLong, Boolean, EPSGTreatsAsLatLong);

/**
 * This method returns `true` if EPSG feels this projected coordinate system
 * should be treated as having northing/easting coordinate ordering.
 *
 * @method EPSGTreatsAsNorthingEasting
 * @instance
 * @memberof SpatialReference
 * @return {boolean}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(SpatialReference, EPSGTreatsAsNorthingEasting, Boolean, EPSGTreatsAsNorthingEasting);

/**
 * Check if geocentric coordinate system.
 *
 * @method isGeocentric
 * @instance
 * @memberof SpatialReference
 * @return {boolean}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(SpatialReference, isGeocentric, Boolean, IsGeocentric);

/**
 * Check if geographic coordinate system.
 *
 * @method isGeographic
 * @instance
 * @memberof SpatialReference
 * @return {boolean}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(SpatialReference, isGeographic, Boolean, IsGeographic);

/**
 * Check if projected coordinate system.
 *
 * @method isProjected
 * @instance
 * @memberof SpatialReference
 * @return {boolean}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(SpatialReference, isProjected, Boolean, IsProjected);

/**
 * Check if local coordinate system.
 *
 * @method isLocal
 * @instance
 * @memberof SpatialReference
 * @return {boolean}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(SpatialReference, isLocal, Boolean, IsLocal);

/**
 * Check if vertical coordinate system.
 *
 * @method isVertical
 * @instance
 * @memberof SpatialReference
 * @return {boolean}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(SpatialReference, isVertical, Boolean, IsVertical);

/**
 * Check if compound coordinate system.
 *
 * @method isCompound
 * @instance
 * @memberof SpatialReference
 * @return {boolean}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(SpatialReference, isCompound, Boolean, IsCompound);

/**
 * Do the GeogCS'es match?
 *
 * @method isSameGeogCS
 * @instance
 * @memberof SpatialReference
 * @param {SpatialReference} srs
 * @return {boolean}
 */
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(
  SpatialReference, isSameGeogCS, Boolean, IsSameGeogCS, SpatialReference, "srs");

/**
 * Do the VertCS'es match?
 *
 * @method isSameVertCS
 * @instance
 * @memberof SpatialReference
 * @param {SpatialReference} srs
 * @return {boolean}
 */
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(
  SpatialReference, isSameVertCS, Boolean, IsSameVertCS, SpatialReference, "srs");

/**
 * Do these two spatial references describe the same system?
 *
 * @method isSame
 * @instance
 * @memberof SpatialReference
 * @param {SpatialReference} srs
 * @return {boolean}
 */
NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(SpatialReference, isSame, Boolean, IsSame, SpatialReference, "srs");

/**
 * Set EPSG authority info if possible.
 *
 * @throws {Error}
 * @method autoIdentifyEPSG
 * @instance
 * @memberof SpatialReference
 */
NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT(SpatialReference, autoIdentifyEPSG, AutoIdentifyEPSG);

/**
 * Clones the spatial reference.
 *
 * @method clone
 * @instance
 * @memberof SpatialReference
 * @return {SpatialReference}
 */
NAPI_METHOD(SpatialReference::clone) {
  SpatialReference *srs = node_gdal::UnwrapWrapped<SpatialReference>(info.This().As<Napi::Object>());
  return SpatialReference::New(srs->this_->Clone(), true);
}

/**
 * Make a duplicate of the GEOGCS node of this OGRSpatialReference object.
 *
 * @method cloneGeogCS
 * @instance
 * @memberof SpatialReference
 * @return {SpatialReference}
 */
NAPI_METHOD(SpatialReference::cloneGeogCS) {
  SpatialReference *srs = node_gdal::UnwrapWrapped<SpatialReference>(info.This().As<Napi::Object>());
  return SpatialReference::New(srs->this_->CloneGeogCS(), true);
}

/**
 * Get the authority name for a node. The most common authority is "EPSG".
 *
 * @method getAuthorityName
 * @instance
 * @memberof SpatialReference
 * @param {string|null} [target_key] The partial or complete path to the node to get an authority from. ie. `"PROJCS"`, `"GEOGCS"`, "`GEOGCS|UNIT"` or `null` to search for an authority node on the root element.
 * @return {string}
 */
NAPI_METHOD(SpatialReference::getAuthorityName) {

  std::string key = "";
  NODE_ARG_OPT_STR(0, "target key", key);

  SpatialReference *srs = node_gdal::UnwrapWrapped<SpatialReference>(info.This().As<Napi::Object>());

  return SafeString::New(srs->this_->GetAuthorityName(key.length() ? key.c_str() : NULL));
}

/**
 * Get the authority code for a node.
 *
 * @method getAuthorityCode
 * @instance
 * @memberof SpatialReference
 * @param {string|null} [target_key] The partial or complete path to the node to get an authority from. ie. `"PROJCS"`, `"GEOGCS"`, "`GEOGCS|UNIT"` or `null` to search for an authority node on the root element.
 * @return {string}
 */
NAPI_METHOD(SpatialReference::getAuthorityCode) {

  std::string key = "";
  NODE_ARG_OPT_STR(0, "target key", key);

  SpatialReference *srs = node_gdal::UnwrapWrapped<SpatialReference>(info.This().As<Napi::Object>());

  return SafeString::New(srs->this_->GetAuthorityCode(key.length() ? key.c_str() : NULL));
}

/**
 * Convert this SRS into WKT format.
 *
 * @throws {Error}
 * @method toWKT
 * @instance
 * @memberof SpatialReference
 * @return {string}
 */
NAPI_METHOD(SpatialReference::exportToWKT) {

  SpatialReference *srs = node_gdal::UnwrapWrapped<SpatialReference>(info.This().As<Napi::Object>());
  char *str;
  Napi::Value result;

  int err = srs->this_->exportToWkt(&str);
  if (err) {
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }
  result = SafeString::New(str);
  CPLFree(str);

  return result;
}

/**
 * Convert this SRS into a a nicely formatted WKT string for display to a
 * person.
 *
 * @throws {Error}
 * @method toPrettyWKT
 * @instance
 * @memberof SpatialReference
 * @param {boolean} [simplify=false]
 * @return {string}
 */
NAPI_METHOD(SpatialReference::exportToPrettyWKT) {

  int simplify = 0;
  NODE_ARG_BOOL_OPT(0, "simplify", simplify);

  SpatialReference *srs = node_gdal::UnwrapWrapped<SpatialReference>(info.This().As<Napi::Object>());
  char *str;
  Napi::Value result;

  int err = srs->this_->exportToPrettyWkt(&str, simplify);
  if (err) {
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }
  result = SafeString::New(str);
  CPLFree(str);

  return result;
}

/**
 * Export coordinate system in PROJ.4 format.
 *
 * @throws {Error}
 * @method toProj4
 * @instance
 * @memberof SpatialReference
 * @return {string}
 */
NAPI_METHOD(SpatialReference::exportToProj4) {

  SpatialReference *srs = node_gdal::UnwrapWrapped<SpatialReference>(info.This().As<Napi::Object>());
  char *str;
  Napi::Value result;

  int err = srs->this_->exportToProj4(&str);
  if (err) {
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }

  if (str) {
    result = Napi::String::New(node_gdal::napi_env(), CPLString(str).Trim().c_str());
  } else {
    result = node_gdal::napi_env().Null();
  }
  CPLFree(str);

  return result;
}

/**
 * Export coordinate system in XML format.
 *
 * @throws {Error}
 * @method toXML
 * @instance
 * @memberof SpatialReference
 * @return {string}
 */
NAPI_METHOD(SpatialReference::exportToXML) {

  SpatialReference *srs = node_gdal::UnwrapWrapped<SpatialReference>(info.This().As<Napi::Object>());
  char *str;
  Napi::Value result;

  int err = srs->this_->exportToXML(&str);
  if (err) {
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }
  result = SafeString::New(str);
  CPLFree(str);

  return result;
}

/**
 * Fetch indicated attribute of named node.
 *
 * @method getAttrValue
 * @instance
 * @memberof SpatialReference
 * @param {string} node_name
 * @param {number} [attr_index=0]
 * @return {string}
 */
NAPI_METHOD(SpatialReference::getAttrValue) {

  SpatialReference *srs = node_gdal::UnwrapWrapped<SpatialReference>(info.This().As<Napi::Object>());
  std::string node_name("");
  int child = 0;
  NODE_ARG_STR(0, "node name", node_name);
  NODE_ARG_INT_OPT(1, "child", child);
  return SafeString::New(srs->this_->GetAttrValue(node_name.c_str(), child));
}

/**
 * Creates a spatial reference from a WKT string.
 *
 * @static
 * @throws {Error}
 * @method fromWKT
 * @instance
 * @memberof SpatialReference
 * @param {string} wkt
 * @return {SpatialReference}
 */
NAPI_METHOD(SpatialReference::fromWKT) {

  std::string wkt("");
  NODE_ARG_STR(0, "wkt", wkt);
  OGRChar *str = (OGRChar *)wkt.c_str();

  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromWkt(&str);
  if (err) {
    delete srs;
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }

  return SpatialReference::New(srs, true);
}

/**
 * Creates a spatial reference from a Proj.4 string.
 *
 * @static
 * @throws {Error}
 * @method fromProj4
 * @instance
 * @memberof SpatialReference
 * @param {string} input
 * @return {SpatialReference}
 */
NAPI_METHOD(SpatialReference::fromProj4) {

  std::string input("");
  NODE_ARG_STR(0, "input", input);

  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromProj4(input.c_str());
  if (err) {
    delete srs;
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }

  return SpatialReference::New(srs, true);
}

/**
 * Creates a spatial reference from a WMSAUTO string.
 *
 * Note that the WMS 1.3 specification does not include the units code, while
 * apparently earlier specs do. GDAL tries to guess around this.
 *
 * @example
 *
 * var wms = 'AUTO:42001,99,8888';
 * var ref = gdal.SpatialReference.fromWMSAUTO(wms);
 *
 * @static
 * @throws {Error}
 * @method fromWMSAUTO
 * @instance
 * @memberof SpatialReference
 * @param {string} input
 * @return {SpatialReference}
 */
NAPI_METHOD(SpatialReference::fromWMSAUTO) {

  std::string input("");
  NODE_ARG_STR(0, "input", input);

  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromWMSAUTO(input.c_str());
  if (err) {
    delete srs;
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }

  return SpatialReference::New(srs, true);
}

/**
 * Import coordinate system from XML format (GML only currently).
 *
 * @static
 * @throws {Error}
 * @method fromXML
 * @instance
 * @memberof SpatialReference
 * @param {string} input
 * @return {SpatialReference}
 */
NAPI_METHOD(SpatialReference::fromXML) {

  std::string input("");
  NODE_ARG_STR(0, "xml", input);

  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromXML(input.c_str());
  if (err) {
    delete srs;
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }

  return SpatialReference::New(srs, true);
}

/**
 * Initialize from OGC URN.
 *
 * The OGC URN should be prefixed with "urn:ogc:def:crs:" per recommendation
 * paper 06-023r1. Currently EPSG and OGC authority values are supported,
 * including OGC auto codes, but not including CRS1 or CRS88 (NAVD88).
 *
 * @static
 * @throws {Error}
 * @method fromURN
 * @instance
 * @memberof SpatialReference
 * @param {string} input
 * @return {SpatialReference}
 */
NAPI_METHOD(SpatialReference::fromURN) {

  std::string input("");
  NODE_ARG_STR(0, "input", input);

  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromURN(input.c_str());
  if (err) {
    delete srs;
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }

  return SpatialReference::New(srs, true);
}

/**
 * Initialize from OGC URL.
 *
 * The OGC URL should be prefixed with "http://opengis.net/def/crs" per best
 * practice paper 11-135. Currently EPSG and OGC authority values are supported,
 * including OGC auto codes, but not including CRS1 or CRS88 (NAVD88).
 *
 * @static
 * @throws {Error}
 * @method fromCRSURL
 * @instance
 * @memberof SpatialReference
 * @param {string} input
 * @return {SpatialReference}
 */

/**
 * Initialize from OGC URL.
 * @async
 *
 * The OGC URL should be prefixed with "http://opengis.net/def/crs" per best
 * practice paper 11-135. Currently EPSG and OGC authority values are supported,
 * including OGC auto codes, but not including CRS1 or CRS88 (NAVD88).
 *
 * @static
 * @throws {Error}
 * @method fromCRSURLAsync
 * @instance
 * @memberof SpatialReference
 * @param {string} input
 * @param {callback<SpatialReference>} [callback=undefined]
 * @return {Promise<SpatialReference>}
 */
GDAL_ASYNCABLE_DEFINE(SpatialReference::fromCRSURL) {

  std::string input("");
  NODE_ARG_STR(0, "url", input);

  GDALAsyncableJob<OGRSpatialReference *> job(0);

  job.main = [input](const GDALExecutionProgress &progress) {
    OGRSpatialReference *srs = new OGRSpatialReference();
    int err = srs->importFromCRSURL(input.c_str());
    if (err) {
      delete srs;
      throw getOGRErrMsg(err);
    }
    return srs;
  };
  job.rval = [](OGRSpatialReference *srs, const GetFromPersistentFunc &) { return SpatialReference::New(srs, true); };
  return job.run(info, async, 1);
}

/**
 * Initialize spatial reference from a URL.
 *
 * This method will download the spatial reference from the given URL.
 *
 * @static
 * @throws {Error}
 * @method fromURL
 * @instance
 * @memberof SpatialReference
 * @param {string} url
 * @return {SpatialReference}
 */

/**
 * Initialize spatial reference from a URL.
 * {{async}}
 *
 * This method will download the spatial reference from the given URL.
 *
 * @static
 * @throws {Error}
 * @method fromURLAsync
 * @instance
 * @memberof SpatialReference
 * @param {string} url
 * @param {callback<SpatialReference>} [callback=undefined]
 * @return {Promise<SpatialReference>}
 */

GDAL_ASYNCABLE_DEFINE(SpatialReference::fromURL) {

  std::string input("");
  NODE_ARG_STR(0, "url", input);

  GDALAsyncableJob<OGRSpatialReference *> job(0);

  job.main = [input](const GDALExecutionProgress &progress) {
    OGRSpatialReference *srs = new OGRSpatialReference();
    OGRErr err = srs->importFromUrl(input.c_str());
    if (err) {
      delete srs;
      throw getOGRErrMsg(err);
    }
    return srs;
  };
  job.rval = [](OGRSpatialReference *srs, const GetFromPersistentFunc &) { return SpatialReference::New(srs, true); };
  return job.run(info, async, 1);
}

/**
 * Initialize from a Mapinfo style CoordSys definition.
 *
 * @static
 * @throws {Error}
 * @method fromMICoordSys
 * @instance
 * @memberof SpatialReference
 * @param {string} input
 * @return {SpatialReference}
 */
NAPI_METHOD(SpatialReference::fromMICoordSys) {

  std::string input("");
  NODE_ARG_STR(0, "input", input);

  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromMICoordSys(input.c_str());
  if (err) {
    delete srs;
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }

  return SpatialReference::New(srs, true);
}

/**
 * Initialize from an arbitrary spatial reference string.
 *
 * This method will examine the provided input, and try to deduce the format,
 * and then use it to initialize the spatial reference system.
 *
 * @static
 * @throws {Error}
 * @method fromUserInput
 * @instance
 * @memberof SpatialReference
 * @param {string} input
 * @return {SpatialReference}
 */

/**
 * Initialize from an arbitrary spatial reference string.
 *
 * This method will examine the provided input, and try to deduce the format,
 * and then use it to initialize the spatial reference system.
 * @async
 *
 * @static
 * @throws {Error}
 * @method fromUserInputAsync
 * @instance
 * @memberof SpatialReference
 * @param {string} input
 * @param {callback<SpatialReference>} [callback=undefined]
 * @return {Promise<SpatialReference>}
 */
GDAL_ASYNCABLE_DEFINE(SpatialReference::fromUserInput) {

  std::string input("");
  NODE_ARG_STR(0, "url", input);

  GDALAsyncableJob<OGRSpatialReference *> job(0);

  job.main = [input](const GDALExecutionProgress &progress) {
    OGRSpatialReference *srs = new OGRSpatialReference();
    int err = srs->SetFromUserInput(input.c_str());
    if (err) {
      delete srs;
      throw getOGRErrMsg(err);
    }
    return srs;
  };
  job.rval = [](OGRSpatialReference *srs, const GetFromPersistentFunc &) { return SpatialReference::New(srs, true); };
  return job.run(info, async, 1);
}

/**
 * Initialize from EPSG GCS or PCS code.
 *
 * @example
 *
 * var ref = gdal.SpatialReference.fromEPSGA(4326);
 *
 * @static
 * @throws {Error}
 * @method fromEPSG
 * @instance
 * @memberof SpatialReference
 * @param {number} input
 * @return {SpatialReference}
 */
NAPI_METHOD(SpatialReference::fromEPSG) {

  int epsg;
  NODE_ARG_INT(0, "epsg", epsg);

  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromEPSG(epsg);
  if (err) {
    delete srs;
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }

  return SpatialReference::New(srs, true);
}

/**
 * Initialize from EPSG GCS or PCS code.
 *
 * This method is similar to `fromEPSG()` except that EPSG preferred axis
 * ordering *will* be applied for geographic and projected coordinate systems.
 * EPSG normally defines geographic coordinate systems to use lat/long, and also
 * there are also a few projected coordinate systems that use northing/easting
 * order contrary to typical GIS use).
 *
 * @example
 *
 * var ref = gdal.SpatialReference.fromEPSGA(26910);
 *
 * @static
 * @throws {Error}
 * @method fromEPSGA
 * @instance
 * @memberof SpatialReference
 * @param {number} input
 * @return {SpatialReference}
 */
NAPI_METHOD(SpatialReference::fromEPSGA) {

  int epsg;
  NODE_ARG_INT(0, "epsg", epsg);

  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromEPSGA(epsg);
  if (err) {
    delete srs;
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }

  return SpatialReference::New(srs, true);
}

/**
 * Import coordinate system from ESRI .prj format(s).
 *
 * This function will read the text loaded from an ESRI .prj file, and translate
 * it into an OGRSpatialReference definition. This should support many (but by
 * no means all) old style (Arc/Info 7.x) .prj files, as well as the newer
 * pseudo-OGC WKT .prj files. Note that new style .prj files are in OGC WKT
 * format, but require some manipulation to correct datum names, and units on
 * some projection parameters. This is addressed within importFromESRI() by an
 * automatical call to morphFromESRI().
 *
 * Currently only GEOGRAPHIC, UTM, STATEPLANE, GREATBRITIAN_GRID, ALBERS,
 * EQUIDISTANT_CONIC, TRANSVERSE (mercator), POLAR, MERCATOR and POLYCONIC
 * projections are supported from old style files.
 *
 * @static
 * @throws {Error}
 * @method fromESRI
 * @instance
 * @memberof SpatialReference
 * @param {object|string[]} input
 * @return {SpatialReference}
 */
NAPI_METHOD(SpatialReference::fromESRI) {

  StringList list;

  if (info.Length() < 1) {
    Napi::Error::New(node_gdal::napi_env(), "input string list must be provided").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  if (list.parse(info[0])) {
    return node_gdal::napi_env().Undefined(); // error parsing string list
  }

  OGRSpatialReference *srs = new OGRSpatialReference();
  int err = srs->importFromESRI(list.get());
  if (err) {
    delete srs;
    NODE_THROW_OGRERR(err);
    return node_gdal::napi_env().Undefined();
  }

  return SpatialReference::New(srs, true);
}

/**
 * @typedef {object} units
 * @property {string} units
 * @property {number} value
 */

/**
 * Fetch linear geographic coordinate system units.
 *
 * @method getLinearUnits
 * @instance
 * @memberof SpatialReference
 * @return {units} An object containing `value` and `unit` properties.
 */
NAPI_METHOD(SpatialReference::getLinearUnits) {

  SpatialReference *srs = node_gdal::UnwrapWrapped<SpatialReference>(info.This().As<Napi::Object>());

  OGRChar *unit_name;
  double units = srs->this_->GetLinearUnits(&unit_name);

  Napi::Object result = Napi::Object::New(node_gdal::napi_env());
  result.Set( Napi::String::New(node_gdal::napi_env(), "value"), Napi::Number::New(node_gdal::napi_env(), units));
  result.Set( Napi::String::New(node_gdal::napi_env(), "units"), SafeString::New(unit_name));

  return result;
}

/**
 * Fetch angular geographic coordinate system units.
 *
 * @method getAngularUnits
 * @instance
 * @memberof SpatialReference
 * @return {units} An object containing `value` and `unit` properties.
 */
NAPI_METHOD(SpatialReference::getAngularUnits) {

  SpatialReference *srs = node_gdal::UnwrapWrapped<SpatialReference>(info.This().As<Napi::Object>());

  OGRChar *unit_name;
  double units = srs->this_->GetAngularUnits(&unit_name);

  Napi::Object result = Napi::Object::New(node_gdal::napi_env());
  result.Set( Napi::String::New(node_gdal::napi_env(), "value"), Napi::Number::New(node_gdal::napi_env(), units));
  result.Set( Napi::String::New(node_gdal::napi_env(), "units"), SafeString::New(unit_name));

  return result;
}

/**
 * Validate SRS tokens.
 *
 * This method attempts to verify that the spatial reference system is well
 * formed, and consists of known tokens. The validation is not comprehensive.
 *
 * @method validate
 * @instance
 * @memberof SpatialReference
 * @return {string|null} `"corrupt"`, '"unsupported"', `null` (if fine)
 */
NAPI_METHOD(SpatialReference::validate) {

  SpatialReference *srs = node_gdal::UnwrapWrapped<SpatialReference>(info.This().As<Napi::Object>());

  OGRErr err = srs->this_->Validate();

  if (err == OGRERR_NONE) {
    return node_gdal::napi_env().Null();
    return node_gdal::napi_env().Undefined();
  }
  if (err == OGRERR_CORRUPT_DATA) {
    return Napi::String::New(node_gdal::napi_env(), "corrupt");
    return node_gdal::napi_env().Undefined();
  }
  if (err == OGRERR_UNSUPPORTED_SRS) {
    return Napi::String::New(node_gdal::napi_env(), "unsupported");
    return node_gdal::napi_env().Undefined();
  }

  NODE_THROW_OGRERR(err);
  return node_gdal::napi_env().Undefined();
}

} // namespace node_gdal
