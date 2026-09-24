
#ifndef __WARP_OPTIONS_H__
#define __WARP_OPTIONS_H__

// node

// nan
#include "../gdal_common.hpp"

// gdal
#include <gdal_priv.h>
#include <gdalwarper.h>

#include "../gdal_dataset.hpp"
#include "number_list.hpp"
#include "string_list.hpp"


namespace node_gdal {

// A class for parsing a V8::Value and constructing a GDALWarpOptions struct
//
// see: https://www.gdal.org/doxygen/structGDALWarpOptions.html
//
// {
//   options : string[] | object
//   memoryLimit : int
//   resampleAlg : string
//   src: Dataset
//   dst: Dataset
//   srcBands: int | int[]
//   dstBands: int | int[]
//   nBands: int
//   srcAlphaBand: int
//   dstAlphaBand: int
//   srcNoData: double
//   dstNoData: double
//   cutline: geometry
//   blend: double
// }

class WarpOptions {
    public:
  int parse(Napi::Value value);
  int parseResamplingAlg(Napi::Value value);

  WarpOptions();
  ~WarpOptions();

  inline GDALWarpOptions *get() {
    return options;
  }
  inline bool useMultithreading() {
    return multi;
  }
  inline std::vector<Napi::Object> datasetObjects() {
    return {src_obj, dst_obj};
  }
  inline std::vector<long> datasetUids() {
    return {src ? src->uid : 0, dst ? dst->uid : 0};
  }

    private:
  GDALWarpOptions *options;
  Napi::Object src_obj;
  Napi::Object dst_obj;
  Dataset *src;
  Dataset *dst;
  StringList additional_options;
  IntegerList src_bands;
  IntegerList dst_bands;
  double *src_nodata;
  double *dst_nodata;
  bool multi;
};

} // namespace node_gdal

#endif
