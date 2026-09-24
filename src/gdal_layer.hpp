#ifndef __NODE_OGR_LAYER_H__
#define __NODE_OGR_LAYER_H__

// node

// nan
#include "gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_dataset.hpp"

namespace node_gdal {

class Layer : public GDALObject<Layer> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(OGRLayer *raw, GDALDataset *raw_parent);
  static Napi::Value New(OGRLayer *raw, GDALDataset *raw_parent, bool result_set);
  static NAPI_METHOD(toString);
  static NAPI_METHOD(getExtent);
  static NAPI_METHOD(setAttributeFilter);
  static NAPI_METHOD(setSpatialFilter);
  static NAPI_METHOD(getSpatialFilter);
  static NAPI_METHOD(testCapability);
  GDAL_ASYNCABLE_DECLARE(syncToDisk);

  static NAPI_SETTER(dsSetter);
  static NAPI_GETTER(dsGetter);
  static NAPI_GETTER(srsGetter);
  static NAPI_GETTER(featuresGetter);
  static NAPI_GETTER(fieldsGetter);
  static NAPI_GETTER(nameGetter);
  static NAPI_GETTER(fidColumnGetter);
  static NAPI_GETTER(geomColumnGetter);
  static NAPI_GETTER(geomTypeGetter);
  static NAPI_GETTER(uidGetter);

  Layer(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~Layer();
  inline OGRLayer *get() {
    return this_;
  }
  inline bool isAlive() {
    return this_ && object_store.isAlive(uid);
  }
  inline GDALDataset *getParent() {
    return parent_ds;
  }
  void dispose();
  long uid;
  long parent_uid;

    private:
  OGRLayer *this_;
  GDALDataset *parent_ds;
};

} // namespace node_gdal
#endif
