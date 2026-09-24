

#include "number_list.hpp"

namespace node_gdal {

IntegerList::IntegerList() : list(NULL), len(0), name("") {
}

IntegerList::IntegerList(const char *name) : list(NULL), len(0) {
  name = (std::string(" ") + name).c_str();
}

IntegerList::~IntegerList() {
  if (list) delete[] list;
}

int IntegerList::parse(Napi::Value value) {
  unsigned int i;
  Napi::Array arr;

  if (value.IsNull() || value.IsUndefined()) return 0;

  if (value.IsArray()) {
    arr = value.As<Napi::Array>();
    len = arr.Length();
    if (len == 0) return 0;

    list = new int[len];
    for (i = 0; i < len; ++i) {
      Napi::Value element = arr.As<Napi::Object>().Get(i);
      if (element.IsNumber()) {
        list[i] = element.As<Napi::Number>().Int32Value();
      } else {
        std::string err = std::string("Every element in the") + name + " array must be a number";
        Napi::TypeError::New(node_gdal::napi_env(), err.c_str()).ThrowAsJavaScriptException();
        return 1;
      }
    }
  } else if (value.IsNumber()) {
    list = new int[1];
    list[0] = value.As<Napi::Number>().Int32Value();
    len = 1;
  } else {
    std::string err = std::string(name) + "integer list must be an array or single integer";
    Napi::TypeError::New(node_gdal::napi_env(), err.c_str()).ThrowAsJavaScriptException();
    return 1;
  }
  return 0;
}

DoubleList::DoubleList() : list(NULL), len(0), name("") {
}

DoubleList::DoubleList(const char *name) : list(NULL), len(0) {
  name = (std::string(" ") + name).c_str();
}

DoubleList::~DoubleList() {
  if (list) delete[] list;
}

int DoubleList::parse(Napi::Value value) {
  unsigned int i;
  Napi::Array arr;

  if (value.IsNull() || value.IsUndefined()) return 0;

  if (value.IsArray()) {
    arr = value.As<Napi::Array>();
    len = arr.Length();
    if (len == 0) return 0;

    list = new double[len];
    for (i = 0; i < len; ++i) {
      Napi::Value element = arr.As<Napi::Object>().Get(i);
      if (element.IsNumber()) {
        list[i] = element.As<Napi::Number>().DoubleValue();
      } else {
        std::string err = std::string("Every element in the") + name + " array must be a number";
        Napi::TypeError::New(node_gdal::napi_env(), err.c_str()).ThrowAsJavaScriptException();
        return 1;
      }
    }
  } else if (value.IsNumber()) {
    list = new double[1];
    list[0] = value.As<Napi::Number>().DoubleValue();
    len = 1;
  } else {
    std::string err = std::string(name) + "double list must be an array or single number";
    Napi::TypeError::New(node_gdal::napi_env(), err.c_str()).ThrowAsJavaScriptException();
    return 1;
  }
  return 0;
}

} // namespace node_gdal
