#ifndef __NODE_GDAL_RASTERBAND_COLLECTION_H__
#define __NODE_GDAL_RASTERBAND_COLLECTION_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

#include "../async.hpp"

namespace node_gdal {

class DatasetBands : public GDALObject<DatasetBands> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(Napi::Value ds_obj);
  static NAPI_METHOD(toString);

  GDAL_ASYNCABLE_DECLARE(get);
  GDAL_ASYNCABLE_DECLARE(count);
  GDAL_ASYNCABLE_DECLARE(create);

  static NAPI_GETTER(dsGetter);

  DatasetBands(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~DatasetBands();
    private:
};

} // namespace node_gdal
#endif
