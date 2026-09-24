#ifndef __NODE_GDAL_VSIFS_H__
#define __NODE_GDAL_VSIFS_H__

// gdal-async
#include "gdal_common.hpp"

// gdal
#include <gdal_priv.h>

#include "gdal_common.hpp"

#include "async.hpp"


// A vsimem file

namespace node_gdal {

namespace VSI {

void Initialize(Napi::Object target);
GDAL_ASYNCABLE_GLOBAL(stat);
GDAL_ASYNCABLE_GLOBAL(readDir);
NAPI_METHOD(clearCurlCache);

} // namespace VSI
} // namespace node_gdal
#endif
