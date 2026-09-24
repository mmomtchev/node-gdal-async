#ifndef __NODE_OGR_GEOMETRYCOLLECTIONBASE_H__
#define __NODE_OGR_GEOMETRYCOLLECTIONBASE_H__

// ogr
#include <ogrsf_frmts.h>

#include "gdal_geometrybase.hpp"
#include "../collections/geometry_collection_children.hpp"

namespace node_gdal {

template <class T, class OGRT> class GeometryCollectionBase : public GeometryBase<T, OGRT> {

    public:
  GeometryCollectionBase(const Napi::CallbackInfo &info);
  using GeometryBase<T, OGRT>::New;
};

template <class T, class OGRT>
GeometryCollectionBase<T, OGRT>::GeometryCollectionBase(const Napi::CallbackInfo &info) : GeometryBase<T, OGRT>(info) {
  // The children collection is kept as a private (JS-invisible) property of the
  // geometry object
  Napi::Object this_obj = info.This().As<Napi::Object>();
  Napi::Value children = GeometryCollectionChildren::New(this_obj);
  GDAL_SET_PRIVATE(this_obj, "children_", children);
}

} // namespace node_gdal
#endif
