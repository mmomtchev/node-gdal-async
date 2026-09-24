#ifndef __NODE_GDAL_MAJOROBJECT_H__
#define __NODE_GDAL_MAJOROBJECT_H__

// gdal
#include <gdal_priv.h>

#include "napi-wrapper.h"

namespace node_gdal {

class MajorObject {
    public:
  static Napi::Object getMetadata(CSLConstList metadata);
};

} // namespace node_gdal
#endif
