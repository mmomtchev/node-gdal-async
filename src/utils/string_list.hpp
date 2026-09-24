
#ifndef __STRING_LIST_H__
#define __STRING_LIST_H__

#include <string>

#include "../napi-wrapper.h"

namespace node_gdal {

// The old binding stringified whatever it was handed, numbers included,
// which is how the string lists accept { blockysize: 4096 }. Napi::String only
// casts an actual string, so the coercion has to be explicit.
inline std::string Stringify(const Napi::Value &v) {
  napi_value str = nullptr;
  if (napi_coerce_to_string(v.Env(), v, &str) != napi_ok || str == nullptr) return std::string();
  return Napi::Value(v.Env(), str).As<Napi::String>().Utf8Value();
}

// A class for parsing a Napi::Value and constructing a GDAL string list
//
// inputs:
// {key: value, ...}, ["key=value", ...]
//
// outputs:
// ["key=value", ...]

class StringList {
    public:
  int parse(Napi::Value value);

  StringList();
  ~StringList();

  inline char **get() {
    return list;
  }

    private:
  char **list;
  std::string *strlist;
};

} // namespace node_gdal

#endif
