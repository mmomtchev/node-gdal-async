#ifndef __NODE_GDAL_RASTERBAND_H__
#define __NODE_GDAL_RASTERBAND_H__

// node

// nan
#include "gdal_common.hpp"

// gdal
#include <gdal_priv.h>

#include "gdal_dataset.hpp"

namespace node_gdal {

class RasterBand : public GDALObject<RasterBand> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(GDALRasterBand *band, GDALDataset *parent);
  static NAPI_METHOD(toString);
  GDAL_ASYNCABLE_DECLARE(flush);
  GDAL_ASYNCABLE_DECLARE(fill);
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
  static NAPI_METHOD(asMDArray);
#endif
  static NAPI_METHOD(getStatistics);
  GDAL_ASYNCABLE_DECLARE(computeStatistics);
  static NAPI_METHOD(setStatistics);
  static NAPI_METHOD(getMaskBand);
  static NAPI_METHOD(getMaskFlags);
  static NAPI_METHOD(createMaskBand);
  GDAL_ASYNCABLE_DECLARE(getMetadata);
  GDAL_ASYNCABLE_DECLARE(setMetadata);
  static NAPI_GETTER(dsGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(sizeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(idGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(descriptionGetter);
  static NAPI_GETTER(overviewsGetter);
  static NAPI_GETTER(pixelsGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(blockSizeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(minimumGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(maximumGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(readOnlyGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(dataTypeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(hasArbitraryOverviewsGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(unitTypeGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(scaleGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(offsetGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(noDataValueGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(categoryNamesGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(colorInterpretationGetter);
  GDAL_ASYNCABLE_GETTER_DECLARE(colorTableGetter);
  static NAPI_GETTER(uidGetter);

  static NAPI_SETTER(unitTypeSetter);
  static NAPI_SETTER(scaleSetter);
  static NAPI_SETTER(offsetSetter);
  static NAPI_SETTER(noDataValueSetter);
  static NAPI_SETTER(categoryNamesSetter);
  static NAPI_SETTER(colorInterpretationSetter);
  static NAPI_SETTER(colorTableSetter);

  RasterBand(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~RasterBand();
  inline bool isAlive() {
    return this_ && object_store.isAlive(uid);
  }
  inline GDALRasterBand *get() {
    return this_;
  }
  inline GDALDataset *getParent() {
    return parent_ds;
  }
  void dispose();
  long uid;
  // Dataset that will be locked
  long parent_uid;

    private:
  GDALRasterBand *this_;
  GDALDataset *parent_ds;
};

} // namespace node_gdal
#endif
