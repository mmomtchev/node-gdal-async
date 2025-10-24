#ifndef __GDAL_ALGEBRA_H__
#define __GDAL_ALGEBRA_H__

// node
#include <node.h>
#include <node_object_wrap.h>

// nan
#include "nan-wrapper.h"

// gdal
#include <gdal_priv.h>

#include "async.hpp"

using namespace v8;
using namespace node;

// GDAL Raster Band Algebra
// https://gdal.org/en/latest/user/band_algebra.html

namespace node_gdal {
namespace Algebra {

void Initialize(Local<Object> target);

GDAL_ASYNCABLE_GLOBAL(add);
} // namespace Algebra
} // namespace node_gdal

#endif
