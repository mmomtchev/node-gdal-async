#ifndef __NODE_GDAL_GEOM_COLLECTION_CHILDREN_H__
#define __NODE_GDAL_GEOM_COLLECTION_CHILDREN_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

// GeometryCollection.children

namespace node_gdal {

class GeometryCollectionChildren : public GDALObject<GeometryCollectionChildren> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(Napi::Value geom);
  static NAPI_METHOD(toString);

  static NAPI_METHOD(get);
  static NAPI_METHOD(count);
  static NAPI_METHOD(add);
  static NAPI_METHOD(remove);

  GeometryCollectionChildren(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~GeometryCollectionChildren();
    private:
};

} // namespace node_gdal
#endif
