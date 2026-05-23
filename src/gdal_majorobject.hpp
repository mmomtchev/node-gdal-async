#ifndef __NODE_GDAL_MAJOROBJECT_H__
#define __NODE_GDAL_MAJOROBJECT_H__

// node
#include <node.h>
#include <node_object_wrap.h>

// nan
#include "nan-wrapper.h"

// gdal
#include <gdal_priv.h>

using namespace v8;
using namespace node;

namespace node_gdal {

class MajorObject {
    public:
  static Local<Object> getMetadata(CSLConstList metadata);
};

} // namespace node_gdal
#endif
