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

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 12)

void Initialize(Local<Object> target);

GDAL_ASYNCABLE_GLOBAL(abs);
GDAL_ASYNCABLE_GLOBAL(sqrt);
GDAL_ASYNCABLE_GLOBAL(log);
GDAL_ASYNCABLE_GLOBAL(log10);
GDAL_ASYNCABLE_GLOBAL(gdal_not);

GDAL_ASYNCABLE_GLOBAL(add);
GDAL_ASYNCABLE_GLOBAL(sub);
GDAL_ASYNCABLE_GLOBAL(mul);
GDAL_ASYNCABLE_GLOBAL(div);
GDAL_ASYNCABLE_GLOBAL(pow);
GDAL_ASYNCABLE_GLOBAL(lt);
GDAL_ASYNCABLE_GLOBAL(lte);
GDAL_ASYNCABLE_GLOBAL(gt);
GDAL_ASYNCABLE_GLOBAL(gte);
GDAL_ASYNCABLE_GLOBAL(eq);
GDAL_ASYNCABLE_GLOBAL(notEq);
GDAL_ASYNCABLE_GLOBAL(gdal_and);
GDAL_ASYNCABLE_GLOBAL(gdal_or);

GDAL_ASYNCABLE_GLOBAL(gdal_min);
GDAL_ASYNCABLE_GLOBAL(gdal_max);
GDAL_ASYNCABLE_GLOBAL(gdal_mean);

GDAL_ASYNCABLE_GLOBAL(ifThenElse);

GDAL_ASYNCABLE_GLOBAL(asType);

#endif

} // namespace Algebra
} // namespace node_gdal

#endif
