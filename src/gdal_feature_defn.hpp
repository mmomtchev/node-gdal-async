#ifndef __NODE_OGR_FEATURE_DEFN_H__
#define __NODE_OGR_FEATURE_DEFN_H__

// node

// nan
#include "gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

namespace node_gdal {

class FeatureDefn : public GDALObject<FeatureDefn> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(const OGRFeatureDefn *def);
  static Napi::Value New(OGRFeatureDefn *def, bool owned);
  static NAPI_METHOD(toString);
  static NAPI_METHOD(clone);

  static NAPI_GETTER(fieldsGetter);
  static NAPI_GETTER(nameGetter);
  static NAPI_GETTER(geomTypeGetter);
  static NAPI_GETTER(geomIgnoredGetter);
  static NAPI_GETTER(styleIgnoredGetter);

  static NAPI_SETTER(geomTypeSetter);
  static NAPI_SETTER(geomIgnoredSetter);
  static NAPI_SETTER(styleIgnoredSetter);

  FeatureDefn(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~FeatureDefn();
  inline OGRFeatureDefn *get() {
    return this_;
  }
  inline bool isAlive() {
    return this_;
  }

    private:
  OGRFeatureDefn *this_;
  bool owned_;
};

} // namespace node_gdal
#endif
