#ifndef __NODE_GDAL_COLORTABLE_H__
#define __NODE_GDAL_COLORTABLE_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>

#include "../async.hpp"

namespace node_gdal {

class ColorTable : public GDALObject<ColorTable> {
    public:
  static Napi::FunctionReference constructor;

  static void Initialize(Napi::Object target);
  static Napi::Value New(GDALColorTable *raw, Napi::Value band);
  static Napi::Value New(GDALColorTable *raw);
  static NAPI_METHOD(toString);

  static NAPI_METHOD(isSame);
  static NAPI_METHOD(clone);
  static NAPI_METHOD(get);
  static NAPI_METHOD(count);
  static NAPI_METHOD(set);
  static NAPI_METHOD(ramp);

  static NAPI_GETTER(interpretationGetter);
  static NAPI_GETTER(bandGetter);

  ColorTable(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~ColorTable();
  inline GDALColorTable *get() {
    return this_;
  }

  void dispose();
  long uid;
  long parent_uid;

  inline bool isAlive() {
    return this_ && object_store.isAlive(uid) && (parent_uid == 0 || object_store.isAlive(parent_uid));
  }

    private:
  GDALColorTable *this_;
};

} // namespace node_gdal
#endif
