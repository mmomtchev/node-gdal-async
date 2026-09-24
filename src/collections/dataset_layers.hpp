#ifndef __NODE_GDAL_LAYER_COLLECTION_H__
#define __NODE_GDAL_LAYER_COLLECTION_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

#include "../async.hpp"

namespace node_gdal {

class DatasetLayers : public GDALObject<DatasetLayers> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(Napi::Value ds_obj);
  static NAPI_METHOD(toString);

  GDAL_ASYNCABLE_DECLARE(get);
  GDAL_ASYNCABLE_DECLARE(count);
  GDAL_ASYNCABLE_DECLARE(create);
  GDAL_ASYNCABLE_DECLARE(copy);
  GDAL_ASYNCABLE_DECLARE(remove);

  static NAPI_GETTER(dsGetter);

  DatasetLayers(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~DatasetLayers();
    private:
};

} // namespace node_gdal
#endif
