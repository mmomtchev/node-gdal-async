#ifndef __NODE_OGR_CURVEBASE_H__
#define __NODE_OGR_CURVEBASE_H__

// ogr
#include <ogrsf_frmts.h>

#include "gdal_geometrybase.hpp"

namespace node_gdal {

template <class T, class OGRT, class COLLECTIONT> class CurveBase : public GeometryBase<T, OGRT> {
    public:
  CurveBase(const Napi::CallbackInfo &info);
  using GeometryBase<T, OGRT>::New;

    protected:
  static void SetPrivate(Napi::Object, Napi::Value);
};

template <class T, class OGRT, class COLLECTIONT>
CurveBase<T, OGRT, COLLECTIONT>::CurveBase(const Napi::CallbackInfo &info) : GeometryBase<T, OGRT>(info) {
  // The points/rings collection is kept as a private (JS-invisible) property of
  // the geometry object
  Napi::Object this_obj = info.This().As<Napi::Object>();
  Napi::Value points = COLLECTIONT::New(this_obj);
  // T:: is required: an unqualified call would find CurveBase's own SetPrivate
  // and store the collection under "points_", even for the subclasses (Polygon,
  // CompoundCurve) that override it to use a different key
  T::SetPrivate(this_obj, points);
}

template <class T, class OGRT, class COLLECTIONT>
void CurveBase<T, OGRT, COLLECTIONT>::SetPrivate(Napi::Object _this, Napi::Value value) {
  GDAL_SET_PRIVATE(_this, "points_", value);
};

} // namespace node_gdal
#endif
