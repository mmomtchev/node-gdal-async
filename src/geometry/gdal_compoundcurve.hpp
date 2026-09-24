#ifndef __NODE_OGR_COMPOUNDCURVE_H__
#define __NODE_OGR_COMPOUNDCURVE_H__

// node

// nan
#include "../gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

#include "gdal_curvebase.hpp"
#include "../collections/compound_curves.hpp"


namespace node_gdal {

class CompoundCurve : public CurveBase<CompoundCurve, OGRCompoundCurve, CompoundCurveCurves> {
  friend CurveBase;

    public:
  static Napi::FunctionReference constructor;
  using CurveBase<CompoundCurve, OGRCompoundCurve, CompoundCurveCurves>::CurveBase;

  static void Initialize(Napi::Object target);
  using CurveBase<CompoundCurve, OGRCompoundCurve, CompoundCurveCurves>::New;
  static NAPI_METHOD(toString);

  static NAPI_GETTER(curvesGetter);

    protected:
  static void SetPrivate(Napi::Object, Napi::Value);
};

} // namespace node_gdal
#endif
