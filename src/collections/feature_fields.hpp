#ifndef __NODE_GDAL_FIELD_COLLECTION_H__
#define __NODE_GDAL_FIELD_COLLECTION_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

namespace node_gdal {

class FeatureFields : public GDALObject<FeatureFields> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(Napi::Value layer_obj);
  static NAPI_METHOD(toString);
  static NAPI_METHOD(toArray);
  static NAPI_METHOD(toObject);

  static NAPI_METHOD(get);
  static NAPI_METHOD(getNames);
  static NAPI_METHOD(set);
  static NAPI_METHOD(reset);
  static NAPI_METHOD(count);
  static NAPI_METHOD(indexOf);

  static Napi::Value get(OGRFeature *f, int field_index);
  static Napi::Value getFieldAsIntegerList(OGRFeature *feature, int field_index);
  static Napi::Value getFieldAsInteger64List(OGRFeature *feature, int field_index);
  static Napi::Value getFieldAsDoubleList(OGRFeature *feature, int field_index);
  static Napi::Value getFieldAsStringList(OGRFeature *feature, int field_index);
  static Napi::Value getFieldAsBinary(OGRFeature *feature, int field_index);
  static Napi::Value getFieldAsDateTime(OGRFeature *feature, int field_index);

  static NAPI_GETTER(featureGetter);

  FeatureFields(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~FeatureFields();
    private:
};

} // namespace node_gdal
#endif
