#ifndef __NODE_OGR_SPATIALREFERENCE_H__
#define __NODE_OGR_SPATIALREFERENCE_H__

// node
#include <node.h>
#include <node_object_wrap.h>

// nan
#include "nan-wrapper.h"

// ogr
#include <ogrsf_frmts.h>

#include "async.hpp"

using namespace v8;
using namespace node;

namespace node_gdal {

#if GDAL_VERSION_MAJOR == 2 && GDAL_VERSION_MINOR < 3
typedef char OGRChar;
#else
typedef const char OGRChar;
#endif

class SpatialReference : public Nan::ObjectWrap {
    public:
  static Nan::Persistent<FunctionTemplate> constructor;
  static void Initialize(Local<Object> target);

  static NAN_METHOD(New);
  static Local<Value> New(const OGRSpatialReference *srs);
  static Local<Value> New(OGRSpatialReference *srs, bool owned);
  static NAN_METHOD(toString);
  static NAN_METHOD(clone);
  static NAN_METHOD(cloneGeogCS);
  static NAN_METHOD(exportToWKT);
  static NAN_METHOD(exportToPrettyWKT);
  static NAN_METHOD(exportToProj4);
  static NAN_METHOD(exportToXML);
  static NAN_METHOD(setWellKnownGeogCS);
  static NAN_METHOD(morphToESRI);
  static NAN_METHOD(morphFromESRI);
  static NAN_METHOD(EPSGTreatsAsLatLong);
  static NAN_METHOD(EPSGTreatsAsNorthingEasting);
  static NAN_METHOD(getLinearUnits);
  static NAN_METHOD(getAngularUnits);
  static NAN_METHOD(isGeocentric);
  static NAN_METHOD(isGeographic);
  static NAN_METHOD(isProjected);
  static NAN_METHOD(isLocal);
  static NAN_METHOD(isVertical);
  static NAN_METHOD(isCompound);
  static NAN_METHOD(isSameGeogCS);
  static NAN_METHOD(isSameVertCS);
  static NAN_METHOD(isSame);
  static NAN_METHOD(autoIdentifyEPSG);
  static NAN_METHOD(getAuthorityCode);
  static NAN_METHOD(getAuthorityName);
  static NAN_METHOD(getAttrValue);
  static NAN_METHOD(validate);

  GDAL_ASYNCABLE_DECLARE(fromUserInput);
  static NAN_METHOD(fromWKT);
  static NAN_METHOD(fromProj4);
  static NAN_METHOD(fromEPSG);
  static NAN_METHOD(fromEPSGA);
  static NAN_METHOD(fromESRI);
  static NAN_METHOD(fromWMSAUTO);
  static NAN_METHOD(fromXML);
  static NAN_METHOD(fromURN);
  GDAL_ASYNCABLE_DECLARE(fromCRSURL);
  GDAL_ASYNCABLE_DECLARE(fromURL);
  static NAN_METHOD(fromMICoordSys);

  SpatialReference();
  SpatialReference(OGRSpatialReference *srs);
  inline OGRSpatialReference *get() {
    return this_;
  }
  inline bool isAlive() {
    return this_;
  }
  void dispose();
  long uid;

    private:
  ~SpatialReference();
  OGRSpatialReference *this_;
  bool owned_;
};

} // namespace node_gdal
#endif
