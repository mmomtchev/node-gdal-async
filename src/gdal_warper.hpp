#ifndef __GDAL_WARPER_H__
#define __GDAL_WARPER_H__

// node

// nan
#include "gdal_common.hpp"

// gdal
#include <gdal_alg.h>
#include <gdal_priv.h>

// ogr
#include <ogrsf_frmts.h>

#include "async.hpp"


// Methods and classes from gdalwarper.h
// https://gdal.org/doxygen/gdalwarper_8h.html

namespace node_gdal {
namespace Warper {

void Initialize(Napi::Object target);

GDAL_ASYNCABLE_GLOBAL(reprojectImage);
GDAL_ASYNCABLE_GLOBAL(suggestedWarpOutput);

} // namespace Warper
} // namespace node_gdal

#endif
