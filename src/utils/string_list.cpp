#include "string_list.hpp"

namespace node_gdal {

StringList::StringList() : list(NULL), strlist(NULL) {
}

StringList::~StringList() {
  if (list) delete[] list;
  if (strlist) delete[] strlist;
}

int StringList::parse(Napi::Value value) {
  unsigned int i;

  if (value.IsNull() || value.IsUndefined()) return 0;

  if (value.IsArray()) {
    Napi::Array array = value.As<Napi::Array>();
    if (array.Length() == 0) return 0;

    list = new char *[array.Length() + 1];
    strlist = new std::string[array.Length()];
    for (i = 0; i < array.Length(); ++i) {
      strlist[i] = Stringify(array.As<Napi::Object>().Get(i));
      list[i] = (char *)strlist[i].c_str();
    }
    list[i] = NULL;
  } else if (value.IsObject()) {
    Napi::Object obj = value.As<Napi::Object>();
    Napi::Array keys = obj.GetPropertyNames();
    if (keys.Length() == 0) return 0;

    list = new char *[keys.Length() + 1];
    strlist = new std::string[keys.Length()];
    for (i = 0; i < keys.Length(); ++i) {
      std::string key = Stringify(keys.As<Napi::Object>().Get(i));
      std::string val = Stringify(obj.As<Napi::Object>().Get(keys.As<Napi::Object>().Get(i)));
      strlist[i] = key + "=" + val;
      list[i] = (char *)strlist[i].c_str();
    }
    list[i] = NULL;
  } else {
    Napi::TypeError::New(value.Env(), "String list must be an array or object").ThrowAsJavaScriptException();
    return 1;
  }
  return 0;
}

} // namespace node_gdal
