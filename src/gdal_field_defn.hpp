#ifndef __NODE_OGR_FIELD_DEFN_H__
#define __NODE_OGR_FIELD_DEFN_H__

// node

// nan
#include "gdal_common.hpp"

// ogr
#include <ogrsf_frmts.h>

namespace node_gdal {

class FieldDefn : public GDALObject<FieldDefn> {
    public:
  static Napi::FunctionReference constructor;
  static void Initialize(Napi::Object target);
  static Napi::Value New(const OGRFieldDefn *def);
  static Napi::Value New(OGRFieldDefn *def, bool owned);
  static NAPI_METHOD(toString);

  static NAPI_GETTER(nameGetter);
  static NAPI_GETTER(typeGetter);
  static NAPI_GETTER(justificationGetter);
  static NAPI_GETTER(precisionGetter);
  static NAPI_GETTER(widthGetter);
  static NAPI_GETTER(ignoredGetter);

  static NAPI_SETTER(nameSetter);
  static NAPI_SETTER(typeSetter);
  static NAPI_SETTER(justificationSetter);
  static NAPI_SETTER(precisionSetter);
  static NAPI_SETTER(widthSetter);
  static NAPI_SETTER(ignoredSetter);

  FieldDefn(const Napi::CallbackInfo &info);

  // Must be accessible: ObjectWrap's finalizer deletes the instance itself
  ~FieldDefn();
  inline OGRFieldDefn *get() {
    return this_;
  }
  inline bool isAlive() {
    return this_;
  }

    private:
  OGRFieldDefn *this_;
  bool owned_;
};

} // namespace node_gdal
#endif
