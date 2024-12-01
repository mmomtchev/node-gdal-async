#ifndef __NODE_GDAL_DATASET_H__
#define __NODE_GDAL_DATASET_H__

// node
#include <node.h>
#include <node_object_wrap.h>

// nan
#include "nan-wrapper.h"

// gdal
#include <gdal_priv.h>

// ogr
#include <ogrsf_frmts.h>

#include "async.hpp"

using namespace v8;
using namespace node;

// > GDAL 2.0 : a wrapper for GDALDataset
// < GDAL 2.0 : a wrapper for either a GDALDataset or OGRDataSource that behaves
// like a 2.0 Dataset

namespace node_gdal {

class Dataset : public Nan::ObjectWrap {
    public:
  static Nan::Persistent<FunctionTemplate> constructor;
  static void Initialize(Local<Object> target);
  static NAN_METHOD(New);
  static Local<Value> New(GDALDataset *ds, GDALDataset *parent = nullptr);
  static NAN_METHOD(toString);
  GDAL_ASYNCABLE_DECLARE(flush);
  GDAL_ASYNCABLE_DECLARE(getMetadata);
  GDAL_ASYNCABLE_DECLARE(setMetadata);
  static NAN_METHOD(getFileList);
  static NAN_METHOD(getGCPProjection);
  static NAN_METHOD(getGCPs);
  static NAN_METHOD(setGCPs);
  GDAL_ASYNCABLE_DECLARE(executeSQL);
  static NAN_METHOD(testCapability);
  GDAL_ASYNCABLE_DECLARE(buildOverviews);
  static NAN_METHOD(close);

  static NAN_GETTER(bandsGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(rasterSizeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(srsGetter);
  static NAN_GETTER(driverGetter);
  static NAN_GETTER(threadSafeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(geoTransformGetter);
  static NAN_GETTER(descriptionGetter);
  static NAN_GETTER(layersGetter);
  static NAN_GETTER(rootGetter);
  static NAN_GETTER(uidGetter);

  static NAN_SETTER(srsSetter);
  static NAN_SETTER(geoTransformSetter);

  Dataset(GDALDataset *ds);
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
  ~Dataset();
  GDALDataset *this_dataset;
  GDALDataset *parent_ds;
};

} // namespace node_gdal
#endif
