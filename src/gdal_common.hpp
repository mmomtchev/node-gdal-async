#ifndef __GDAL_COMMON_H__
#define __GDAL_COMMON_H__

#include <cpl_error.h>
#include <gdal_version.h>
#include <thread>
#include <stdio.h>

// nan
#include "nan-wrapper.h"

#include "utils/ptr_manager.hpp"

#if GDAL_VERSION_MAJOR < 2 || (GDAL_VERSION_MAJOR == 2 && GDAL_VERSION_MINOR < 2)
#error gdal-async now requires GDAL >= 2.2, downgrade to gdal-async@3.6.x for earlier versions
#endif

namespace node_gdal {
extern FILE *log_file;
extern ObjectStore object_store;
extern bool eventLoopWarn;
} // namespace node_gdal

#ifdef ENABLE_LOGGING
#define LOG(fmt, ...)                                                                                                  \
  if (node_gdal::log_file) {                                                                                           \
    fprintf(node_gdal::log_file, fmt "\n", __VA_ARGS__);                                                               \
    fflush(node_gdal::log_file);                                                                                       \
  }
#else
#define LOG(fmt, ...)
#endif

// Nan::New(null) -> seg fault
class SafeString {
    public:
  static v8::Local<v8::Value> New(const char *data) {
    Nan::EscapableHandleScope scope;
    if (!data) {
      return scope.Escape(Nan::Null());
    } else {
      return scope.Escape(Nan::New<v8::String>(data).ToLocalChecked());
    }
  }
};

inline const char *getOGRErrMsg(int err) {
  if (err == 6) {
    // get more descriptive error
    // TODO: test if all OGRErr failures report an error msg
    return CPLGetLastErrorMsg();
  }
  switch (err) {
    case 0: return "No error";
    case 1: return "Not enough data";
    case 2: return "Not enough memory";
    case 3: return "Unsupported geometry type";
    case 4: return "Unsupported operation";
    case 5: return "Corrupt Data";
    case 6: return "Failure";
    case 7: return "Unsupported SRS";
    default: return "Invalid Error";
  }
};

template <typename INPUT, typename RETURN>
std::shared_ptr<RETURN[]> NumberArrayToSharedPtr(Local<Array> array, size_t count = 0) {
  if (array.IsEmpty()) return nullptr;
  if (count != 0 && array->Length() != count) throw "Array size must match the number of dimensions";
  std::shared_ptr<RETURN[]> ptr(new RETURN[array->Length()]);
  for (unsigned i = 0; i < array->Length(); i++) {
    Local<Value> val = Nan::Get(array, i).ToLocalChecked();
    if (!val->IsNumber()) throw "Array must contain only numbers";
    ptr.get()[i] = static_cast<RETURN>(Nan::To<INPUT>(val).ToChecked());
  }

  return ptr;
}

#define NODE_THROW_LAST_CPLERR Nan::ThrowError(CPLGetLastErrorMsg())

#define NODE_THROW_OGRERR(err) Nan::ThrowError(getOGRErrMsg(err))

#define ATTR(t, name, get, set) Nan::SetAccessor(t->InstanceTemplate(), Nan::New(name).ToLocalChecked(), get, set);

#define ATTR_ASYNCABLE(t, name, get, set)                                                                              \
  Nan::SetAccessor(t->InstanceTemplate(), Nan::New(name).ToLocalChecked(), get, set);                                  \
  Nan::SetAccessor(                                                                                                    \
    t->InstanceTemplate(),                                                                                             \
    Nan::New(name "Async").ToLocalChecked(),                                                                           \
    get##Async,                                                                                                        \
    READ_ONLY_SETTER,                                                                                                  \
    Local<Value>(),                                                                                                    \
    DEFAULT,                                                                                                           \
    DontEnum);

#define ATTR_DONT_ENUM(t, name, get, set)                                                                              \
  Nan::SetAccessor(t->InstanceTemplate(), Nan::New(name).ToLocalChecked(), get, set, Local<Value>(), DEFAULT, DontEnum);

NAN_SETTER(READ_ONLY_SETTER);

#define IS_WRAPPED(obj, type) Nan::New(type::constructor)->HasInstance(obj)

// ----- async method definition shortcuts ------
#define Nan__SetAsyncableMethod(lcons, name, method)                                                                   \
  Nan::SetMethod(lcons, name, method);                                                                                 \
  Nan::SetMethod(lcons, name "Async", method##Async)

#define Nan__SetPrototypeAsyncableMethod(lcons, name, method)                                                          \
  Nan::SetPrototypeMethod(lcons, name, method);                                                                        \
  Nan::SetPrototypeMethod(lcons, name "Async", method##Async)

#define NODE_UNWRAP_CHECK(type, obj, var)                                                                              \
  if (!obj->IsObject() || obj->IsNull() || !Nan::New(type::constructor)->HasInstance(obj)) {                           \
    Nan::ThrowTypeError("Object must be a " #type " object");                                                          \
    return;                                                                                                            \
  }                                                                                                                    \
  type *var = Nan::ObjectWrap::Unwrap<type>(obj.As<Object>());                                                         \
  if (!var->isAlive()) {                                                                                               \
    Nan::ThrowError(#type " object has already been destroyed");                                                       \
    return;                                                                                                            \
  }

#define GDAL_RAW_CHECK(type, obj, var)                                                                                 \
  type var = obj->get();                                                                                               \
  if (!obj) {                                                                                                          \
    Nan::ThrowError(#type " object has already been destroyed");                                                       \
    return;                                                                                                            \
  }

// Defines to be used in async getters (returning a Promise)
// DO NOT USE IN A NORMAL ASYNC METHOD
// It will return a rejected Promise on error
#define NODE_UNWRAP_CHECK_ASYNC(type, obj, var)                                                                        \
  if (!obj->IsObject() || obj->IsNull() || !Nan::New(type::constructor)->HasInstance(obj)) {                           \
    THROW_OR_REJECT("Object must be a " #type " object");                                                              \
    return;                                                                                                            \
  }                                                                                                                    \
  type *var = Nan::ObjectWrap::Unwrap<type>(obj.As<Object>());                                                         \
  if (!var->isAlive()) {                                                                                               \
    THROW_OR_REJECT(#type " object has already been destroyed");                                                       \
    return;                                                                                                            \
  }

#define GDAL_RAW_CHECK_ASYNC(type, obj, var)                                                                           \
  type var = obj->get();                                                                                               \
  if (!obj) { THROW_OR_REJECT(#type " object has already been destroyed"); }

// ----- object property conversion -------

#define NODE_DOUBLE_FROM_OBJ(obj, key, var)                                                                            \
  {                                                                                                                    \
    Local<String> sym = Nan::New(key).ToLocalChecked();                                                                \
    if (!Nan::HasOwnProperty(obj, sym).FromMaybe(false)) {                                                             \
      Nan::ThrowError("Object must contain property \"" key "\"");                                                     \
      return;                                                                                                          \
    }                                                                                                                  \
    Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();                                                            \
    if (!val->IsNumber()) {                                                                                            \
      Nan::ThrowTypeError("Property \"" key "\" must be a number");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    var = Nan::To<double>(val).ToChecked();                                                                            \
  }

#define NODE_INT_FROM_OBJ(obj, key, var)                                                                               \
  {                                                                                                                    \
    Local<String> sym = Nan::New(key).ToLocalChecked();                                                                \
    if (!Nan::HasOwnProperty(obj, sym).FromMaybe(false)) {                                                             \
      Nan::ThrowError("Object must contain property \"" key "\"");                                                     \
      return;                                                                                                          \
    }                                                                                                                  \
    Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();                                                            \
    if (!val->IsNumber()) {                                                                                            \
      Nan::ThrowTypeError("Property \"" key "\" must be a number");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    var = Nan::To<int32_t>(val).ToChecked();                                                                           \
  }

#define NODE_STR_FROM_OBJ(obj, key, var)                                                                               \
  {                                                                                                                    \
    Local<String> sym = Nan::New(key).ToLocalChecked();                                                                \
    if (!Nan::HasOwnProperty(obj, sym).FromMaybe(false)) {                                                             \
      Nan::ThrowError("Object must contain property \"" key "\"");                                                     \
      return;                                                                                                          \
    }                                                                                                                  \
    Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();                                                            \
    if (!val->IsString()) {                                                                                            \
      Nan::ThrowTypeError("Property \"" key "\" must be a string");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    var = *Nan::Utf8String(val);                                                                                       \
  }

#define NODE_ARRAY_FROM_OBJ(obj, key, var)                                                                             \
  {                                                                                                                    \
    Local<String> sym = Nan::New(key).ToLocalChecked();                                                                \
    if (!Nan::HasOwnProperty(obj, sym).FromMaybe(false)) {                                                             \
      Nan::ThrowError("Object must contain property \"" key "\"");                                                     \
      return;                                                                                                          \
    }                                                                                                                  \
    Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();                                                            \
    if (!val->IsArray()) {                                                                                             \
      Nan::ThrowTypeError("Property \"" key "\" must be an array");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    var = val.As<Array>();                                                                                             \
  }

#define NODE_ARRAY_FROM_OBJ_OPT(obj, key, var)                                                                         \
  {                                                                                                                    \
    Local<String> sym = Nan::New(key).ToLocalChecked();                                                                \
    if (Nan::HasOwnProperty(obj, sym).FromMaybe(false)) {                                                              \
      Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();                                                          \
      if (!val->IsUndefined() && !val->IsNull()) {                                                                     \
        if (!val->IsArray()) {                                                                                         \
          Nan::ThrowTypeError("Property \"" key "\" must be an array");                                                \
          return;                                                                                                      \
        }                                                                                                              \
        var = val.As<Array>();                                                                                         \
      }                                                                                                                \
    }                                                                                                                  \
  }

#define NODE_WRAPPED_FROM_OBJ(obj, key, type, var)                                                                     \
  {                                                                                                                    \
    Local<String> sym = Nan::New(key).ToLocalChecked();                                                                \
    if (!Nan::HasOwnProperty(obj, sym).FromMaybe(false)) {                                                             \
      Nan::ThrowError("Object must contain property \"" key "\"");                                                     \
      return;                                                                                                          \
    }                                                                                                                  \
    Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();                                                            \
    if (!val->IsObject() || val->IsNull() || !Nan::New(type::constructor)->HasInstance(val)) {                         \
      Nan::ThrowTypeError("Property \"" key "\" must be a " #type " object");                                          \
      return;                                                                                                          \
    }                                                                                                                  \
    var = Nan::ObjectWrap::Unwrap<type>(val.As<Object>());                                                             \
    if (!var->isAlive()) {                                                                                             \
      Nan::ThrowError(key ": " #type " object has already been destroyed");                                            \
      return;                                                                                                          \
    }                                                                                                                  \
  }

#define NODE_WRAPPED_FROM_OBJ_OPT(obj, key, type, var)                                                                 \
  {                                                                                                                    \
    Local<String> sym = Nan::New(key).ToLocalChecked();                                                                \
    if (Nan::HasOwnProperty(obj, sym).FromMaybe(false)) {                                                              \
      Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();                                                          \
      if (val->IsObject() && Nan::New(type::constructor)->HasInstance(val)) {                                          \
        var = Nan::ObjectWrap::Unwrap<type>(val.As<Object>());                                                         \
        if (!var->isAlive()) {                                                                                         \
          Nan::ThrowError(key ": " #type " object has already been destroyed");                                        \
          return;                                                                                                      \
        }                                                                                                              \
      } else if (!val->IsNull() && !val->IsUndefined()) {                                                              \
        Nan::ThrowTypeError(key "property must be a " #type " object");                                                \
        return;                                                                                                        \
      }                                                                                                                \
    }                                                                                                                  \
  }

#define NODE_DOUBLE_FROM_OBJ_OPT(obj, key, var)                                                                        \
  {                                                                                                                    \
    Local<String> sym = Nan::New(key).ToLocalChecked();                                                                \
    if (Nan::HasOwnProperty(obj, sym).FromMaybe(false)) {                                                              \
      Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();                                                          \
      if (!val->IsNumber()) {                                                                                          \
        Nan::ThrowTypeError("Property \"" key "\" must be a number");                                                  \
        return;                                                                                                        \
      }                                                                                                                \
      var = Nan::To<double>(val).ToChecked();                                                                          \
    }                                                                                                                  \
  }

#define NODE_INT_FROM_OBJ_OPT(obj, key, var)                                                                           \
  {                                                                                                                    \
    Local<String> sym = Nan::New(key).ToLocalChecked();                                                                \
    if (Nan::HasOwnProperty(obj, sym).FromMaybe(false)) {                                                              \
      Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();                                                          \
      if (!val->IsNumber()) {                                                                                          \
        Nan::ThrowTypeError("Property \"" key "\" must be a number");                                                  \
        return;                                                                                                        \
      }                                                                                                                \
      var = Nan::To<int32_t>(val).ToChecked();                                                                         \
    }                                                                                                                  \
  }

#define NODE_INT64_FROM_OBJ_OPT(obj, key, var)                                                                         \
  {                                                                                                                    \
    Local<String> sym = Nan::New(key).ToLocalChecked();                                                                \
    if (Nan::HasOwnProperty(obj, sym).FromMaybe(false)) {                                                              \
      Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();                                                          \
      if (!val->IsNumber()) {                                                                                          \
        Nan::ThrowTypeError("Property \"" key "\" must be a number");                                                  \
        return;                                                                                                        \
      }                                                                                                                \
      var = Nan::To<int64_t>(val).ToChecked();                                                                         \
    }                                                                                                                  \
  }

#define NODE_STR_FROM_OBJ_OPT(obj, key, var)                                                                           \
  {                                                                                                                    \
    Local<String> sym = Nan::New(key).ToLocalChecked();                                                                \
    if (Nan::HasOwnProperty(obj, sym).FromMaybe(false)) {                                                              \
      Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();                                                          \
      if (!val->IsString()) {                                                                                          \
        Nan::ThrowTypeError("Property \"" key "\" must be a string");                                                  \
        return;                                                                                                        \
      }                                                                                                                \
      var = *Nan::Utf8String(val);                                                                                     \
    }                                                                                                                  \
  }

#define NODE_CB_FROM_OBJ_OPT(obj, key, var)                                                                            \
  {                                                                                                                    \
    var = nullptr;                                                                                                     \
    Local<String> sym = Nan::New(key).ToLocalChecked();                                                                \
    if (Nan::HasOwnProperty(obj, sym).FromMaybe(false)) {                                                              \
      Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();                                                          \
      if (!val->IsFunction()) {                                                                                        \
        Nan::ThrowTypeError("Property \"" key "\" must be a function");                                                \
        return;                                                                                                        \
      } else {                                                                                                         \
        var = new Nan::Callback(val.As<Function>());                                                                   \
      }                                                                                                                \
    }                                                                                                                  \
  }

// ----- argument conversion -------

// determine field index based on string/numeric js argument
// f -> OGRFeature* or OGRFeatureDefn*

#define ARG_FIELD_ID(num, f, var)                                                                                      \
  {                                                                                                                    \
    if (info[num]->IsString()) {                                                                                       \
      std::string field_name = *Nan::Utf8String(info[num]);                                                            \
      var = f->GetFieldIndex(field_name.c_str());                                                                      \
      if (field_index == -1) {                                                                                         \
        Nan::ThrowError("Specified field name does not exist");                                                        \
        return;                                                                                                        \
      }                                                                                                                \
    } else if (info[num]->IsInt32()) {                                                                                 \
      var = Nan::To<int32_t>(info[num]).ToChecked();                                                                   \
      if (var < 0 || var >= f->GetFieldCount()) {                                                                      \
        Nan::ThrowRangeError("Invalid field index");                                                                   \
        return;                                                                                                        \
      }                                                                                                                \
    } else {                                                                                                           \
      Nan::ThrowTypeError("Field index must be integer or string");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
  }

#define NODE_ARG_INT(num, name, var)                                                                                   \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError(name " must be given");                                                                            \
    return;                                                                                                            \
  }                                                                                                                    \
  if (!info[num]->IsNumber()) {                                                                                        \
    Nan::ThrowTypeError(name " must be an integer");                                                                   \
    return;                                                                                                            \
  }                                                                                                                    \
  var = static_cast<int>(Nan::To<int64_t>(info[num]).ToChecked());

#define NODE_ARG_ENUM(num, name, enum_type, var)                                                                       \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError(name " must be given");                                                                            \
    return;                                                                                                            \
  }                                                                                                                    \
  if (!info[num]->IsInt32() && !info[num]->IsUint32()) {                                                               \
    Nan::ThrowTypeError(name " must be of type " #enum_type);                                                          \
    return;                                                                                                            \
  }                                                                                                                    \
  var = enum_type(Nan::To<uint32_t>(info[num]).ToChecked());

#define NODE_ARG_BOOL(num, name, var)                                                                                  \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError(name " must be given");                                                                            \
    return;                                                                                                            \
  }                                                                                                                    \
  if (!info[num]->IsBoolean()) {                                                                                       \
    Nan::ThrowTypeError(name " must be an boolean");                                                                   \
    return;                                                                                                            \
  }                                                                                                                    \
  var = Nan::To<bool>(info[num]).ToChecked();

#define NODE_ARG_DOUBLE(num, name, var)                                                                                \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError(name " must be given");                                                                            \
    return;                                                                                                            \
  }                                                                                                                    \
  if (!info[num]->IsNumber()) {                                                                                        \
    Nan::ThrowTypeError(name " must be a number");                                                                     \
    return;                                                                                                            \
  }                                                                                                                    \
  var = Nan::To<double>(info[num]).ToChecked();

#define NODE_ARG_ARRAY(num, name, var)                                                                                 \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError(name " must be given");                                                                            \
    return;                                                                                                            \
  }                                                                                                                    \
  if (!info[num]->IsArray()) { return Nan::ThrowTypeError((std::string(name) + " must be an array").c_str()); }        \
  var = info[num].As<Array>();

#define NODE_ARG_OBJECT(num, name, var)                                                                                \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError(name " must be given");                                                                            \
    return;                                                                                                            \
  }                                                                                                                    \
  if (!info[num]->IsObject()) { return Nan::ThrowTypeError((std::string(name) + " must be an object").c_str()); }      \
  var = info[num].As<Object>();

#define NODE_ARG_WRAPPED(num, name, type, var)                                                                         \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError(name " must be given");                                                                            \
    return;                                                                                                            \
  }                                                                                                                    \
  if (info[num]->IsNull() || info[num]->IsUndefined() || !Nan::New(type::constructor)->HasInstance(info[num])) {       \
    Nan::ThrowTypeError(name " must be an instance of " #type);                                                        \
    return;                                                                                                            \
  }                                                                                                                    \
  var = Nan::ObjectWrap::Unwrap<type>(info[num].As<Object>());                                                         \
  if (!var->isAlive()) {                                                                                               \
    Nan::ThrowError(#type " parameter already destroyed");                                                             \
    return;                                                                                                            \
  }

#define NODE_ARG_STR(num, name, var)                                                                                   \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError(name " must be given");                                                                            \
    return;                                                                                                            \
  }                                                                                                                    \
  if (!info[num]->IsString()) {                                                                                        \
    Nan::ThrowTypeError(name " must be a string");                                                                     \
    return;                                                                                                            \
  }                                                                                                                    \
  var = (*Nan::Utf8String(info[num]))

#define NODE_ARG_STR_INT(num, name, varString, varInt, isString)                                                       \
  bool isString = false;                                                                                               \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError(name " must be given");                                                                            \
    return;                                                                                                            \
  }                                                                                                                    \
  if (info[num]->IsString()) {                                                                                         \
    varString = (*Nan::Utf8String(info[num]));                                                                         \
    isString = true;                                                                                                   \
  } else if (info[num]->IsNumber()) {                                                                                  \
    varInt = static_cast<int>(Nan::To<int64_t>(info[num]).ToChecked());                                                \
    isString = false;                                                                                                  \
  } else {                                                                                                             \
    Nan::ThrowTypeError(name " must be a string or a number");                                                         \
    return;                                                                                                            \
  }

#define NODE_ARG_BUFFER(num, name, var)                                                                                \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError(name " must be given");                                                                            \
    return;                                                                                                            \
  }                                                                                                                    \
  if (!info[num]->IsArrayBuffer()) {                                                                                   \
    Nan::ThrowTypeError(name " must be a buffer");                                                                     \
    return;                                                                                                            \
  }                                                                                                                    \
  var = info[num].As<Object>();

// delete callback is in AsyncWorker::~AsyncWorker
#define NODE_ARG_CB(num, name, var)                                                                                    \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError(name " must be given");                                                                            \
    return;                                                                                                            \
  }                                                                                                                    \
  if (!info[num]->IsFunction()) {                                                                                      \
    Nan::ThrowTypeError(name " must be a function");                                                                   \
    return;                                                                                                            \
  }                                                                                                                    \
  var = new Nan::Callback(info[num].As<Function>())

// ----- optional argument conversion -------

#define NODE_ARG_INT_OPT(num, name, var)                                                                               \
  if (info.Length() > num) {                                                                                           \
    if (info[num]->IsInt32()) {                                                                                        \
      var = static_cast<int>(Nan::To<int64_t>(info[num]).ToChecked());                                                 \
    } else if (!info[num]->IsNull() && !info[num]->IsUndefined()) {                                                    \
      Nan::ThrowTypeError(name " must be an integer");                                                                 \
      return;                                                                                                          \
    }                                                                                                                  \
  }

#define NODE_ARG_ENUM_OPT(num, name, enum_type, var)                                                                   \
  if (info.Length() > num) {                                                                                           \
    if (info[num]->IsInt32() || info[num]->IsUint32()) {                                                               \
      var = static_cast<enum_type>(Nan::To<uint32_t>(info[num]).ToChecked());                                          \
    } else if (!info[num]->IsNull() && !info[num]->IsUndefined()) {                                                    \
      Nan::ThrowTypeError(name " must be an integer");                                                                 \
      return;                                                                                                          \
    }                                                                                                                  \
  }

#define NODE_ARG_BOOL_OPT(num, name, var)                                                                              \
  if (info.Length() > num) {                                                                                           \
    if (info[num]->IsBoolean()) {                                                                                      \
      var = Nan::To<bool>(info[num]).ToChecked();                                                                      \
    } else if (!info[num]->IsNull() && !info[num]->IsUndefined()) {                                                    \
      Nan::ThrowTypeError(name " must be an boolean");                                                                 \
      return;                                                                                                          \
    }                                                                                                                  \
  }

#define NODE_ARG_OPT_STR(num, name, var)                                                                               \
  if (info.Length() > num) {                                                                                           \
    if (info[num]->IsString()) {                                                                                       \
      var = *Nan::Utf8String(info[num]);                                                                               \
    } else if (!info[num]->IsNull() && !info[num]->IsUndefined()) {                                                    \
      Nan::ThrowTypeError(name " must be a string");                                                                   \
      return;                                                                                                          \
    }                                                                                                                  \
  }

#define NODE_ARG_DOUBLE_OPT(num, name, var)                                                                            \
  if (info.Length() > num) {                                                                                           \
    if (info[num]->IsNumber()) {                                                                                       \
      var = Nan::To<double>(info[num]).ToChecked();                                                                    \
    } else if (!info[num]->IsNull() && !info[num]->IsUndefined()) {                                                    \
      Nan::ThrowTypeError(name " must be a number");                                                                   \
      return;                                                                                                          \
    }                                                                                                                  \
  }

#define NODE_ARG_WRAPPED_OPT(num, name, type, var)                                                                     \
  if (info.Length() > num && !info[num]->IsNull() && !info[num]->IsUndefined()) {                                      \
    if (!Nan::New(type::constructor)->HasInstance(info[num])) {                                                        \
      Nan::ThrowTypeError(name " must be an instance of " #type);                                                      \
      return;                                                                                                          \
    }                                                                                                                  \
    var = Nan::ObjectWrap::Unwrap<type>(info[num].As<Object>());                                                       \
    if (!var->isAlive()) {                                                                                             \
      Nan::ThrowError(#type " parameter already destroyed");                                                           \
      return;                                                                                                          \
    }                                                                                                                  \
  }

#define NODE_ARG_ARRAY_OPT(num, name, var)                                                                             \
  if (info.Length() > num) {                                                                                           \
    if (info[num]->IsArray()) {                                                                                        \
      var = info[num].As<Array>();                                                                                     \
    } else if (!info[num]->IsNull() && !info[num]->IsUndefined()) {                                                    \
      Nan::ThrowTypeError(name " must be an array");                                                                   \
      return;                                                                                                          \
    }                                                                                                                  \
  }

#define NODE_ARG_OBJECT_OPT(num, name, var)                                                                            \
  if (info.Length() > num && !info[num]->IsNull() && !info[num]->IsUndefined()) {                                      \
    if (!info[num]->IsObject()) { return Nan::ThrowTypeError((std::string(name) + " must be an object").c_str()); }    \
    var = info[num].As<Object>();                                                                                      \
  }

// delete is in GDALAsyncWorker::~GDALAsyncWorker
#define NODE_ARG_CB_OPT(num, name, var)                                                                                \
  if (!info[num]->IsNull() && !info[num]->IsUndefined()) {                                                             \
    if (info[num]->IsFunction()) {                                                                                     \
      var = new Nan::Callback(info[num].As<Function>());                                                               \
    } else {                                                                                                           \
      Nan::ThrowTypeError(name " must be a function");                                                                 \
      return;                                                                                                          \
    }                                                                                                                  \
  }

// ----- special case: progress callback in [options] argument -------

#define NODE_PROGRESS_CB_OPT(num, progress_cb, job)                                                                    \
  {                                                                                                                    \
    Local<Object> progress_obj;                                                                                        \
    progress_cb = nullptr;                                                                                             \
    if (info.Length() > num && !info[num]->IsNull() && !info[num]->IsUndefined()) {                                    \
      if (!info[num]->IsObject()) { return Nan::ThrowTypeError("options must be an object"); }                         \
      progress_obj = info[num].As<Object>();                                                                           \
      NODE_CB_FROM_OBJ_OPT(progress_obj, "progress_cb", progress_cb);                                                  \
    }                                                                                                                  \
    if (progress_cb) { job.progress = progress_cb; }                                                                   \
  }

// ----- wrapped methods w/ results-------

#define NODE_WRAPPED_METHOD_WITH_RESULT(klass, method, result_type, wrapped_method)                                    \
  NAN_METHOD(klass::method) {                                                                                          \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    info.GetReturnValue().Set(Nan::New<result_type>(obj->this_->wrapped_method()));                                    \
  }

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_WRAPPED_PARAM(                                                               \
  klass, method, result_type, wrapped_method, param_type, param_name)                                                  \
  NAN_METHOD(klass::method) {                                                                                          \
    param_type *param;                                                                                                 \
    NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) return Nan::ThrowError(#klass " object has already been destroyed");                          \
    info.GetReturnValue().Set(Nan::New<result_type>(obj->this_->wrapped_method(param->get())));                        \
  }

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_ENUM_PARAM(                                                                  \
  klass, method, result_type, wrapped_method, enum_type, param_name)                                                   \
  NAN_METHOD(klass::method) {                                                                                          \
    enum_type param;                                                                                                   \
    NODE_ARG_ENUM(0, #param_name, enum_type, param);                                                                   \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    info.GetReturnValue().Set(Nan::New<result_type>(obj->this_->wrapped_method(param)));                               \
  }

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_STRING_PARAM(klass, method, result_type, wrapped_method, param_name)         \
  NAN_METHOD(klass::method) {                                                                                          \
    std::string param;                                                                                                 \
    NODE_ARG_STR(0, #param_name, param);                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    info.GetReturnValue().Set(Nan::New<result_type>(obj->this_->wrapped_method(param.c_str())));                       \
  }

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_INTEGER_PARAM(klass, method, result_type, wrapped_method, param_name)        \
  NAN_METHOD(klass::method) {                                                                                          \
    int param;                                                                                                         \
    NODE_ARG_INT(0, #param_name, param);                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    info.GetReturnValue().Set(Nan::New<result_type>(obj->this_->wrapped_method(param)));                               \
  }

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_DOUBLE_PARAM(klass, method, result_type, wrapped_method, param_name)         \
  NAN_METHOD(klass::method) {                                                                                          \
    double param;                                                                                                      \
    NODE_ARG_DOUBLE(0, #param_name, param);                                                                            \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    info.GetReturnValue().Set(Nan::New<result_type>(obj->this_->wrapped_method(param)));                               \
  }

// ----- wrapped methods w/ lock -------

#define NODE_WRAPPED_METHOD_WITH_RESULT_LOCKED(klass, method, result_type, wrapped_method)                             \
  NAN_METHOD(klass::method) {                                                                                          \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    GDAL_LOCK_PARENT(obj);                                                                                             \
    info.GetReturnValue().Set(Nan::New<result_type>(obj->this_->wrapped_method()));                                    \
  }

#define NODE_WRAPPED_GETTER_WITH_STRING_LOCKED(klass, method, wrapped_method)                                          \
  NAN_GETTER(klass::method) {                                                                                          \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    GDAL_LOCK_PARENT(obj);                                                                                             \
    auto r = obj->this_->wrapped_method();                                                                             \
    info.GetReturnValue().Set(SafeString::New(r.c_str()));                                                             \
  }

#define NODE_WRAPPED_GETTER_WITH_RESULT_LOCKED(klass, method, result_type, wrapped_method)                             \
  NAN_GETTER(klass::method) {                                                                                          \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    GDAL_LOCK_PARENT(obj);                                                                                             \
    auto r = obj->this_->wrapped_method();                                                                             \
    info.GetReturnValue().Set(Nan::New<result_type>(r));                                                               \
  }

// ----- wrapped asyncable methods-------
#define NODE_WRAPPED_ASYNC_METHOD(klass, method, wrapped_method)                                                       \
  GDAL_ASYNCABLE_DEFINE(klass::method) {                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    auto *gdal_obj = obj->this_;                                                                                       \
    GDALAsyncableJob<int> job(0);                                                                                      \
    job.main = [gdal_obj](const GDALExecutionProgress &) {                                                             \
      gdal_obj->wrapped_method();                                                                                      \
      return 0;                                                                                                        \
    };                                                                                                                 \
    job.rval = [](int, const GetFromPersistentFunc &) { return Nan::Undefined().As<Value>(); };                        \
    job.run(info, async, 0);                                                                                           \
  }

// ----- wrapped asyncable methods w/ results-------

#define NODE_WRAPPED_ASYNC_METHOD_WITH_RESULT(klass, async_type, method, result_type, wrapped_method)                  \
  GDAL_ASYNCABLE_DEFINE(klass::method) {                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    auto *gdal_obj = obj->this_;                                                                                       \
    GDALAsyncableJob<async_type> job(0);                                                                               \
    job.main = [gdal_obj](const GDALExecutionProgress &) { return gdal_obj->wrapped_method(); };                       \
    job.rval = [](async_type r, const GetFromPersistentFunc &) { return Nan::New<result_type>(r); };                   \
    job.run(info, async, 0);                                                                                           \
  }

// param_type must be a node-gdal type
#define NODE_WRAPPED_ASYNC_METHOD_WITH_RESULT_1_WRAPPED_PARAM(                                                         \
  klass, async_type, method, result_type, wrapped_method, param_type, param_name)                                      \
  GDAL_ASYNCABLE_DEFINE(klass::method) {                                                                               \
    param_type *param;                                                                                                 \
    NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) return Nan::ThrowError(#klass " object has already been destroyed");                          \
    auto *gdal_obj = obj->this_;                                                                                       \
    auto *gdal_param = param->get();                                                                                   \
    GDALAsyncableJob<async_type> job(0);                                                                               \
    job.persist(info[0].As<Object>());                                                                                 \
    job.main = [gdal_obj, gdal_param](const GDALExecutionProgress &) { return gdal_obj->wrapped_method(gdal_param); }; \
    job.rval = [](async_type r, const GetFromPersistentFunc &) { return Nan::New<result_type>(r); };                   \
    job.run(info, async, 1);                                                                                           \
  }

#define NODE_WRAPPED_ASYNC_METHOD_WITH_RESULT_1_ENUM_PARAM(                                                            \
  klass, async_type, method, result_type, wrapped_method, enum_type, param_name)                                       \
  GDAL_ASYNCABLE_DEFINE(klass::method) {                                                                               \
    enum_type param;                                                                                                   \
    NODE_ARG_ENUM(0, #param_name, enum_type, param);                                                                   \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    auto *gdal_obj = obj->this_;                                                                                       \
    GDALAsyncableJob<async_type> job(0);                                                                               \
    job.main = [gdal_obj, param](const GDALExecutionProgress &) { return gdal_obj->wrapped_method(param); };           \
    job.rval = [](async_type r, const GetFromPersistentFunc &) { return Nan::New<result_type>(r); };                   \
    job.run(info, async, 1);                                                                                           \
  }

#define NODE_WRAPPED_ASYNC_METHOD_WITH_OGRERR_RESULT_LOCKED(klass, method, wrapped_method)                             \
  GDAL_ASYNCABLE_DEFINE(klass::method) {                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    auto gdal_obj = obj->this_;                                                                                        \
    GDALAsyncableJob<OGRErr> job(obj->parent_uid);                                                                     \
    job.main = [gdal_obj](const GDALExecutionProgress &) {                                                             \
      int err = gdal_obj->wrapped_method();                                                                            \
      if (err) throw getOGRErrMsg(err);                                                                                \
      return err;                                                                                                      \
    };                                                                                                                 \
    job.rval = [](OGRErr, const GetFromPersistentFunc &) { return Nan::Undefined().As<Value>(); };                     \
    job.run(info, async, 0);                                                                                           \
  }

#define NODE_WRAPPED_ASYNC_METHOD_WITH_OGRERR_RESULT_1_WRAPPED_PARAM(                                                  \
  klass, async_type, method, wrapped_method, param_type, param_name)                                                   \
  GDAL_ASYNCABLE_DEFINE(klass::method) {                                                                               \
    param_type *param;                                                                                                 \
    NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    auto gdal_obj = obj->this_;                                                                                        \
    auto gdal_param = param->get();                                                                                    \
    GDALAsyncableJob<async_type> job(0);                                                                               \
    job.persist(info[0].As<Object>());                                                                                 \
    job.main = [gdal_obj, gdal_param](const GDALExecutionProgress &) {                                                 \
      int err = gdal_obj->wrapped_method(gdal_param);                                                                  \
      if (err) throw getOGRErrMsg(err);                                                                                \
      return err;                                                                                                      \
    };                                                                                                                 \
    job.rval = [](async_type, const GetFromPersistentFunc &) { return Nan::Undefined().As<Value>(); };                 \
    job.run(info, async, 1);                                                                                           \
  }

// ----- wrapped methods w/ CPLErr result (throws) -------

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT(klass, method, wrapped_method)                                          \
  NAN_METHOD(klass::method) {                                                                                          \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method();                                                                            \
    if (err) {                                                                                                         \
      NODE_THROW_LAST_CPLERR;                                                                                          \
      return;                                                                                                          \
    }                                                                                                                  \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_WRAPPED_PARAM(klass, method, wrapped_method, param_type, param_name)  \
  NAN_METHOD(klass::method) {                                                                                          \
    param_type *param;                                                                                                 \
    NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param->get());                                                                \
    if (err) {                                                                                                         \
      NODE_THROW_LAST_CPLERR;                                                                                          \
      return;                                                                                                          \
    }                                                                                                                  \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_STRING_PARAM(klass, method, wrapped_method, param_name)               \
  NAN_METHOD(klass::method) {                                                                                          \
    std::string param;                                                                                                 \
    NODE_ARG_STR(0, #param_name, param);                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param.c_str());                                                               \
    if (err) {                                                                                                         \
      NODE_THROW_LAST_CPLERR;                                                                                          \
      return;                                                                                                          \
    }                                                                                                                  \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_INTEGER_PARAM(klass, method, wrapped_method, param_name)              \
  NAN_METHOD(klass::method) {                                                                                          \
    int param;                                                                                                         \
    NODE_ARG_INT(0, #param_name, param);                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param);                                                                       \
    if (err) {                                                                                                         \
      NODE_THROW_LAST_CPLERR;                                                                                          \
      return;                                                                                                          \
    }                                                                                                                  \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_INTEGER_PARAM_LOCKED(klass, method, wrapped_method, param_name)       \
  NAN_METHOD(klass::method) {                                                                                          \
    int param;                                                                                                         \
    NODE_ARG_INT(0, #param_name, param);                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param);                                                                       \
    if (err) {                                                                                                         \
      NODE_THROW_LAST_CPLERR;                                                                                          \
      return;                                                                                                          \
    }                                                                                                                  \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_CPLERR_RESULT_1_DOUBLE_PARAM(klass, method, wrapped_method, param_name)               \
  NAN_METHOD(klass::method) {                                                                                          \
    double param;                                                                                                      \
    NODE_ARG_DOUBLE(0, #param_name, param);                                                                            \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    GDAL_LOCK_PARENT(obj);                                                                                             \
    int err = obj->this_->wrapped_method(param);                                                                       \
    if (err) { NODE_THROW_LAST_CPLERR; }                                                                               \
    return;                                                                                                            \
  }

// ----- wrapped methods w/ OGRErr result (throws) -------

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT(klass, method, wrapped_method)                                          \
  NAN_METHOD(klass::method) {                                                                                          \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method();                                                                            \
    if (err) {                                                                                                         \
      NODE_THROW_OGRERR(err);                                                                                          \
      return;                                                                                                          \
    }                                                                                                                  \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_WRAPPED_PARAM(klass, method, wrapped_method, param_type, param_name)  \
  NAN_METHOD(klass::method) {                                                                                          \
    param_type *param;                                                                                                 \
    NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param->get());                                                                \
    if (err) {                                                                                                         \
      NODE_THROW_OGRERR(err);                                                                                          \
      return;                                                                                                          \
    }                                                                                                                  \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_STRING_PARAM(klass, method, wrapped_method, param_name)               \
  NAN_METHOD(klass::method) {                                                                                          \
    std::string param;                                                                                                 \
    NODE_ARG_STR(0, #param_name, param);                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param.c_str());                                                               \
    if (err) {                                                                                                         \
      NODE_THROW_OGRERR(err);                                                                                          \
      return;                                                                                                          \
    }                                                                                                                  \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_INTEGER_PARAM(klass, method, wrapped_method, param_name)              \
  NAN_METHOD(klass::method) {                                                                                          \
    int param;                                                                                                         \
    NODE_ARG_INT(0, #param_name, param);                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param);                                                                       \
    if (err) {                                                                                                         \
      NODE_THROW_OGRERR(err);                                                                                          \
      return;                                                                                                          \
    }                                                                                                                  \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_1_DOUBLE_PARAM(klass, method, wrapped_method, param_name)               \
  NAN_METHOD(klass::method) {                                                                                          \
    double param;                                                                                                      \
    NODE_ARG_DOUBLE(0, #param_name, param);                                                                            \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    int err = obj->this_->wrapped_method(param);                                                                       \
    if (err) {                                                                                                         \
      NODE_THROW_OGRERR(err);                                                                                          \
      return;                                                                                                          \
    }                                                                                                                  \
    return;                                                                                                            \
  }

// ----- wrapped methods w/ OGRErr result (throws) -------

#define NODE_WRAPPED_METHOD_WITH_OGRERR_RESULT_LOCKED(klass, method, wrapped_method)                                   \
  NAN_METHOD(klass::method) {                                                                                          \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    GDAL_LOCK_PARENT(obj);                                                                                             \
    int err = obj->this_->wrapped_method();                                                                            \
    if (err) {                                                                                                         \
      NODE_THROW_OGRERR(err);                                                                                          \
      return;                                                                                                          \
    }                                                                                                                  \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_RESULT_1_STRING_PARAM_LOCKED(klass, method, result_type, wrapped_method, param_name)  \
  NAN_METHOD(klass::method) {                                                                                          \
    std::string param;                                                                                                 \
    NODE_ARG_STR(0, #param_name, param);                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    GDAL_LOCK_PARENT(obj);                                                                                             \
    auto r = obj->this_->wrapped_method(param.c_str());                                                                \
    info.GetReturnValue().Set(Nan::New<result_type>(r));                                                               \
  }

// ----- wrapped methods -------

#define NODE_WRAPPED_METHOD(klass, method, wrapped_method)                                                             \
  NAN_METHOD(klass::method) {                                                                                          \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    obj->this_->wrapped_method();                                                                                      \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_1_WRAPPED_PARAM(klass, method, wrapped_method, param_type, param_name)                \
  NAN_METHOD(klass::method) {                                                                                          \
    param_type *param;                                                                                                 \
    NODE_ARG_WRAPPED(0, #param_name, param_type, param);                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    obj->this_->wrapped_method(param->get());                                                                          \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_1_INTEGER_PARAM(klass, method, wrapped_method, param_name)                            \
  NAN_METHOD(klass::method) {                                                                                          \
    int param;                                                                                                         \
    NODE_ARG_INT(0, #param_name, param);                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    obj->this_->wrapped_method(param);                                                                                 \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_1_DOUBLE_PARAM(klass, method, wrapped_method, param_name)                             \
  NAN_METHOD(klass::method) {                                                                                          \
    double param;                                                                                                      \
    NODE_ARG_DOUBLE(0, #param_name, param);                                                                            \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    obj->this_->wrapped_method(param);                                                                                 \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_1_BOOLEAN_PARAM(klass, method, wrapped_method, param_name)                            \
  NAN_METHOD(klass::method) {                                                                                          \
    bool param;                                                                                                        \
    NODE_ARG_BOOL(0, #param_name, param);                                                                              \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    obj->this_->wrapped_method(param);                                                                                 \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_1_ENUM_PARAM(klass, method, wrapped_method, enum_type, param_name)                    \
  NAN_METHOD(klass::method) {                                                                                          \
    enum_type param;                                                                                                   \
    NODE_ARG_ENUM(0, #param_name, enum_type, param);                                                                   \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    obj->this_->wrapped_method(param);                                                                                 \
    return;                                                                                                            \
  }

#define NODE_WRAPPED_METHOD_WITH_1_STRING_PARAM(klass, method, wrapped_method, param_name)                             \
  NAN_METHOD(klass::method) {                                                                                          \
    std::string param;                                                                                                 \
    NODE_ARG_STR(0, #param_name, param);                                                                               \
    klass *obj = Nan::ObjectWrap::Unwrap<klass>(info.This());                                                          \
    if (!obj->isAlive()) {                                                                                             \
      Nan::ThrowError(#klass " object has already been destroyed");                                                    \
      return;                                                                                                          \
    }                                                                                                                  \
    obj->this_->wrapped_method(param.c_str());                                                                         \
    return;                                                                                                            \
  }

#define MEASURE_EXECUTION_TIME(msg, op)                                                                                \
  {                                                                                                                    \
    auto start = std::chrono::high_resolution_clock::now();                                                            \
    if (msg != nullptr) fprintf(stderr, "%s", msg);                                                                    \
    op;                                                                                                                \
    auto elapsed = std::chrono::high_resolution_clock::now() - start;                                                  \
    if (msg != nullptr)                                                                                                \
      fprintf(                                                                                                         \
        stderr,                                                                                                        \
        "%ld µs\n",                                                                                                    \
        static_cast<long>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()));                    \
  }

#endif
