#ifndef __NODE_GDAL_COMPOUND_CURVES_H__
#define __NODE_GDAL_COMPOUND_CURVES_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

// CompoundCurve.curves

namespace node_gdal {

class CompoundCurveCurves : public GDALObject<CompoundCurveCurves> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(Napi::Value geom);
  static NAPI_METHOD(toString);

  static NAPI_METHOD(get);
  static NAPI_METHOD(count);
  static NAPI_METHOD(add);

  CompoundCurveCurves(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~CompoundCurveCurves();
    private:
};

} // namespace node_gdal
#endif
