#ifndef __NODE_OGR_FEATURE_H__
#define __NODE_OGR_FEATURE_H__

// node

// nan
#include "gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include <ogr_featurestyle.h>

namespace node_gdal {

class Feature : public GDALObject<Feature> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(OGRFeature *feature);
  static Napi::Value New(OGRFeature *feature, bool owned);
  static NAPI_METHOD(toString);
  static NAPI_METHOD(getGeometry);
  //	static NAPI_METHOD(setGeometryDirectly);
  static NAPI_METHOD(setGeometry);
  //  static NAPI_METHOD(stealGeometry);
  static NAPI_METHOD(clone);
  static NAPI_METHOD(equals);
  static NAPI_METHOD(getFieldDefn);
  static NAPI_METHOD(setFrom);
  static NAPI_METHOD(destroy);
  static NAPI_METHOD(getStyleString);
  static NAPI_METHOD(setStyleString);

  static NAPI_GETTER(fieldsGetter);
  static NAPI_GETTER(fidGetter);
  static NAPI_GETTER(defnGetter);

  static NAPI_SETTER(fidSetter);

  Feature(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~Feature();
  inline OGRFeature *get() {
    return this_;
  }
  inline bool isAlive() {
    return this_;
  }
  void dispose();

    private:
  OGRFeature *this_;
  bool owned_;
  // int size_;
};

} // namespace node_gdal
#endif
