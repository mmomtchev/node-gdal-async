#ifndef __NODE_GDAL_DATASET_H__
#define __NODE_GDAL_DATASET_H__

// gdal
#include <gdal_priv.h>

// ogr
#include <ogrsf_frmts.h>

#include "async.hpp"
#include "gdal_common.hpp"

namespace node_gdal {

// > GDAL 2.0 : a wrapper for GDALDataset
// < GDAL 2.0 : a wrapper for either a GDALDataset or OGRDataSource that behaves
// like a 2.0 Dataset

class Dataset : public GDALObject<Dataset> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(GDALDataset *ds, GDALDataset *parent = nullptr, bool close = true);
  static NAPI_METHOD(toString);
  GDAL_ASYNCABLE_DECLARE(flush);
  GDAL_ASYNCABLE_DECLARE(getMetadata);
  GDAL_ASYNCABLE_DECLARE(setMetadata);
  static NAPI_METHOD(getFileList);
  static NAPI_METHOD(getGCPProjection);
  static NAPI_METHOD(getGCPs);
  static NAPI_METHOD(setGCPs);
  GDAL_ASYNCABLE_DECLARE(executeSQL);
  static NAPI_METHOD(testCapability);
  GDAL_ASYNCABLE_DECLARE(buildOverviews);
  static NAPI_METHOD(close);

  static NAPI_GETTER(bandsGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(rasterSizeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(srsGetter);
  static NAPI_GETTER(driverGetter);
  static NAPI_GETTER(threadSafeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(geoTransformGetter);
  static NAPI_GETTER(descriptionGetter);
  static NAPI_GETTER(layersGetter);
  static NAPI_GETTER(rootGetter);
  static NAPI_GETTER(uidGetter);

  static NAPI_SETTER(srsSetter);
  static NAPI_SETTER(geoTransformSetter);

  Dataset(const Napi::CallbackInfo &info);
  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~Dataset();

  inline GDALDataset *get() {
    return this_dataset;
  }

  void dispose(bool manual);
  long uid;
  long parent_uid;

  inline bool isAlive() {
    return this_dataset && object_store.isAlive(uid);
  }

    private:
  GDALDataset *this_dataset;
  GDALDataset *parent_ds;
};

} // namespace node_gdal
#endif
