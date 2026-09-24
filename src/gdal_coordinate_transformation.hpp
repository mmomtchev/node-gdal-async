#ifndef __NODE_OGR_COORDINATETRANSFORMATION_H__
#define __NODE_OGR_COORDINATETRANSFORMATION_H__

// node

// nan
#include "gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

// gdal
#include <gdalwarper.h>

namespace node_gdal {

class GeoTransformTransformer;

class CoordinateTransformation : public GDALObject<CoordinateTransformation> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(OGRCoordinateTransformation *transform);
  static NAPI_METHOD(toString);
  static NAPI_METHOD(transformPoint);

  CoordinateTransformation(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~CoordinateTransformation();
  inline OGRCoordinateTransformation *get() {
    return this_;
  }
  inline bool isAlive() {
    return this_;
  }

    private:
  OGRCoordinateTransformation *this_;
};

// adapted from gdalwarp source

class GeoTransformTransformer : public OGRCoordinateTransformation {
    public:
  void *hSrcImageTransformer = nullptr;

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 7)
  virtual const OGRSpatialReference *GetSourceCS() const override {
    return nullptr;
  }
  virtual const OGRSpatialReference *GetTargetCS() const override {
    return nullptr;
  }
#else
  virtual OGRSpatialReference *GetSourceCS() override {
    return nullptr;
  }
  virtual OGRSpatialReference *GetTargetCS() override {
    return nullptr;
  }
#endif

#if GDAL_VERSION_MAJOR < 3
  virtual int TransformEx(int nCount, double *x, double *y, double *z = NULL, int *pabSuccess = NULL) {
    return GDALGenImgProjTransform(hSrcImageTransformer, TRUE, nCount, x, y, z, pabSuccess);
  }
#endif

  virtual int Transform(int nCount, double *x, double *y, double *z = NULL) {
    int nResult;

    int *pabSuccess = (int *)CPLCalloc(sizeof(int), nCount);
    nResult = Transform(nCount, x, y, z, pabSuccess);
    CPLFree(pabSuccess);

    return nResult;
  }

  int Transform(int nCount, double *x, double *y, double *z, int *pabSuccess) {
    return GDALGenImgProjTransform(hSrcImageTransformer, TRUE, nCount, x, y, z, pabSuccess);
  }

#if GDAL_VERSION_MAJOR >= 3
  virtual int Transform(
#if GDAL_VERSION_MINOR >= 9
    size_t
#else
    int
#endif
      nCount,
    double *x,
    double *y,
    double *z,
    double * /* t */,
    int *pabSuccess) override {
    return GDALGenImgProjTransform(hSrcImageTransformer, TRUE, nCount, x, y, z, pabSuccess);
  }
#endif

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 3)
  virtual OGRCoordinateTransformation *GetInverse() const override {
    return nullptr;
  }
#endif

  virtual OGRCoordinateTransformation *Clone() const
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
    override
#endif
  {
    return new GeoTransformTransformer(*this);
  }

  ~GeoTransformTransformer() {
    if (hSrcImageTransformer) {
      GDALDestroyGenImgProjTransformer(hSrcImageTransformer);
      hSrcImageTransformer = nullptr;
    }
  }
};
} // namespace node_gdal
#endif
