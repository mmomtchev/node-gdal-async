#ifndef __NODE_GDAL_VSIFS_H__
#define __NODE_GDAL_VSIFS_H__

// node
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

// nan
#include "nan-wrapper.h"

// gdal
#include <gdal_priv.h>

#include "gdal_common.hpp"

#include "async.hpp"

using namespace v8;
using namespace node;

// A vsimem file

namespace node_gdal {

namespace VSI {

void Initialize(Local<Object> target);
GDAL_ASYNCABLE_GLOBAL(stat);
GDAL_ASYNCABLE_GLOBAL(readDir);
static NAN_METHOD(clearCurlCache);

} // namespace VSI
} // namespace node_gdal
#endif
