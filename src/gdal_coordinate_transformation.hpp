#ifndef __NODE_OGR_COORDINATETRANSFORMATION_H__
#define __NODE_OGR_COORDINATETRANSFORMATION_H__

// node
#include <node.h>
#include <node_object_wrap.h>

// nan
#include "nan-wrapper.h"

// ogr
#include <ogrsf_frmts.h>

// gdal
#include <gdalwarper.h>

using namespace v8;
using namespace node;

namespace node_gdal {

class GeoTransformTransformer;

class CoordinateTransformation : public Nan::ObjectWrap {
    public:
  static Nan::Persistent<FunctionTemplate> constructor;
  static void Initialize(Local<Object> target);
  static NAN_METHOD(New);
  static Local<Value> New(OGRCoordinateTransformation *transform);
  static NAN_METHOD(toString);
  static NAN_METHOD(transformPoint);

  CoordinateTransformation();
  CoordinateTransformation(OGRCoordinateTransformation *srs);
  inline OGRCoordinateTransformation *get() {
    return this_;
  }
  inline bool isAlive() {
    return this_;
  }

    private:
  ~CoordinateTransformation();
  OGRCoordinateTransformation *this_;
};

// adapted from gdalwarp source

class GeoTransformTransformer : public OGRCoordinateTransformation {
    public:
  void *hSrcImageTransformer = nullptr;

  virtual OGRSpatialReference *GetSourceCS() override {
    return nullptr;
  }
  virtual OGRSpatialReference *GetTargetCS() override {
    return nullptr;
  }

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
  virtual int Transform(int nCount, double *x, double *y, double *z, double * /* t */, int *pabSuccess) override {
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
