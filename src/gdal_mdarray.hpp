#ifndef __NODE_GDAL_MDARRAY_H__
#define __NODE_GDAL_MDARRAY_H__

// node

// nan
#include "gdal_common.hpp"

// gdal
#include <gdal_priv.h>

// ogr
#include <ogrsf_frmts.h>

#include "async.hpp"

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

namespace node_gdal {

class MDArray : public GDALObject<MDArray> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(std::shared_ptr<GDALMDArray> group, GDALDataset *parent_ds);
  GDAL_ASYNCABLE_DECLARE(read);
  static NAPI_METHOD(getView);
  static NAPI_METHOD(getMask);
  static NAPI_METHOD(asDataset);
  static NAPI_METHOD(toString);
  static NAPI_GETTER(typeGetter);
  static NAPI_GETTER(lengthGetter);
  static NAPI_GETTER(srsGetter);
  static NAPI_GETTER(unitTypeGetter);
  static NAPI_GETTER(scaleGetter);
  static NAPI_GETTER(offsetGetter);
  static NAPI_GETTER(noDataValueGetter);
  static NAPI_GETTER(dimensionsGetter);
  static NAPI_GETTER(descriptionGetter);
  static NAPI_GETTER(attributesGetter);
  static NAPI_GETTER(uidGetter);

  MDArray(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~MDArray();
  inline std::shared_ptr<GDALMDArray> get() {
    return this_;
  }

  void dispose();
  long uid;
  long parent_uid;
  size_t dimensions;

  inline bool isAlive() {
    return this_ && object_store.isAlive(uid);
  }

    private:
  std::shared_ptr<GDALMDArray> this_;
  GDALDataset *parent_ds;
};

} // namespace node_gdal
#endif
#endif
