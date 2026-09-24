#ifndef __NODE_GDAL_FEATURE_COLLECTION_H__
#define __NODE_GDAL_FEATURE_COLLECTION_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

#include "../async.hpp"

namespace node_gdal {

class LayerFeatures : public GDALObject<LayerFeatures> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(Napi::Value layer_obj);
  static NAPI_METHOD(toString);

  GDAL_ASYNCABLE_DECLARE(get);
  GDAL_ASYNCABLE_DECLARE(first);
  GDAL_ASYNCABLE_DECLARE(next);
  GDAL_ASYNCABLE_DECLARE(count);
  GDAL_ASYNCABLE_DECLARE(add);
  GDAL_ASYNCABLE_DECLARE(set);
  GDAL_ASYNCABLE_DECLARE(remove);

  static NAPI_GETTER(layerGetter);

  LayerFeatures(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~LayerFeatures();
    private:
};

} // namespace node_gdal
#endif
