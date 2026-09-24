#include "gdal_common.hpp"

#include <string>

NAPI_SETTER(READ_ONLY_SETTER) {
  // The property name is passed through the descriptor's `data` field: an N-API
  // setter callback does not receive it, but the message has to name the
  // property ("name is a read-only property")
  const char *name = static_cast<const char *>(info.Data());
  std::string err = std::string(name ? name : "property") + " is a read-only property";
  Napi::Error::New(node_gdal::napi_env(), err.c_str()).ThrowAsJavaScriptException();
}
