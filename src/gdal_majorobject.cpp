#include "gdal_majorobject.hpp"
#include "gdal_common.hpp"
#include "gdal_driver.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::Object MajorObject::getMetadata(CSLConstList metadata) {

  Napi::Object result = Napi::Object::New(node_gdal::napi_env());

  if (metadata) {
    int i = 0;
    while (metadata[i]) {
      std::string pair = metadata[i];
      std::size_t i_equal = pair.find_first_of('=');
      if (i_equal != std::string::npos) {
        std::string key = pair.substr(0, i_equal);
        std::string val = pair.substr(i_equal + 1);
        result.Set( Napi::String::New(node_gdal::napi_env(), key.c_str()), Napi::String::New(node_gdal::napi_env(), val.c_str()));
      }
      i++;
    }
  }

  return result;
}

} // namespace node_gdal
