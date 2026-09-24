#include "warp_options.hpp"
#include "../gdal_common.hpp"
#include "../geometry/gdal_geometry.hpp"
#include <stdio.h>
namespace node_gdal {

WarpOptions::WarpOptions()
  : options(NULL),
    src(nullptr),
    dst(nullptr),
    additional_options(),
    src_bands("src band ids"),
    dst_bands("dst band ids"),
    src_nodata(NULL),
    dst_nodata(NULL),
    multi(false) {
  options = GDALCreateWarpOptions();
}

WarpOptions::~WarpOptions() {

  // Dont use: GDALDestroyWarpOptions( options ); - it assumes ownership of
  // everything
  if (options) CPLFree(options);
  if (src_nodata) delete src_nodata;
  if (dst_nodata) delete dst_nodata;
}

int WarpOptions::parseResamplingAlg(Napi::Value value) {
  if (value.IsUndefined() || value.IsNull()) {
    options->eResampleAlg = GRA_NearestNeighbour;
    return 0;
  }
  if (!value.IsString()) {
    Napi::TypeError::New(node_gdal::napi_env(), "resampling property must be a string").ThrowAsJavaScriptException();
    return 1;
  }
  std::string name = value.As<Napi::String>().Utf8Value();

  if (name == "NearestNeighbor") {
    options->eResampleAlg = GRA_NearestNeighbour;
    return 0;
  }
  if (name == "NearestNeighbour") {
    options->eResampleAlg = GRA_NearestNeighbour;
    return 0;
  }
  if (name == "Bilinear") {
    options->eResampleAlg = GRA_Bilinear;
    return 0;
  }
  if (name == "Cubic") {
    options->eResampleAlg = GRA_Cubic;
    return 0;
  }
  if (name == "CubicSpline") {
    options->eResampleAlg = GRA_CubicSpline;
    return 0;
  }
  if (name == "Lanczos") {
    options->eResampleAlg = GRA_Lanczos;
    return 0;
  }
  if (name == "Average") {
    options->eResampleAlg = GRA_Average;
    return 0;
  }
  if (name == "Mode") {
    options->eResampleAlg = GRA_Mode;
    return 0;
  }

  Napi::Error::New(node_gdal::napi_env(), "Invalid resampling algorithm").ThrowAsJavaScriptException();
  return 1;
}

/*
 * {
 *   options : string[] | object
 *   memoryLimit : int
 *   resampleAlg : string
 *   src: Dataset
 *   dst: Dataset
 *   srcBands: int | int[]
 *   dstBands: int | int[]
 *   nBands: int
 *   srcAlphaBand: int
 *   dstAlphaBand: int
 *   srcNoData: double
 *   dstNoData: double
 *   cutline: geometry
 *   blend: double
 * }
 */
int WarpOptions::parse(Napi::Value value) {

  if (!value.IsObject() || value.IsNull()) Napi::TypeError::New(node_gdal::napi_env(), "Warp options must be an object").ThrowAsJavaScriptException();

  Napi::Object obj = value.As<Napi::Object>();
  Napi::Value prop;

  if (
    obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "options")) &&
    additional_options.parse(obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "options")))) {
    return 1; // error parsing string list
  }

  options->papszWarpOptions = additional_options.get();

  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "memoryLimit"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "memoryLimit"));
    if (prop.IsNumber()) {
      options->dfWarpMemoryLimit = prop.As<Napi::Number>().Int32Value();
    } else if (!prop.IsUndefined() && !prop.IsNull()) {
      Napi::TypeError::New(node_gdal::napi_env(), "memoryLimit property must be an integer").ThrowAsJavaScriptException();
      return 1;
    }
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "resampling"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "resampling"));
    if (parseResamplingAlg(prop)) {
      return 1; // error parsing resampling algorithm
    }
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "src"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "src"));
    if (prop.IsObject() && !prop.IsNull() && prop.As<Napi::Object>().InstanceOf(Dataset::constructor.Value())) {
      this->src_obj = prop.As<Napi::Object>();
      this->src = node_gdal::UnwrapWrapped<Dataset>(this->src_obj);
#if GDAL_VERSION_MAJOR == 2 && GDAL_VERSION_MINOR < 3
      options->hSrcDS = static_cast<GDALDatasetH>(this->src->get());
#else
      options->hSrcDS = GDALDataset::ToHandle(this->src->get());
#endif
      if (!options->hSrcDS) {
        Napi::Error::New(node_gdal::napi_env(), "src dataset already closed").ThrowAsJavaScriptException();
        return 1;
      }
    } else {
      Napi::TypeError::New(node_gdal::napi_env(), "src property must be a Dataset object").ThrowAsJavaScriptException();
      return 1;
    }
  } else {
    Napi::Error::New(node_gdal::napi_env(), "Warp options must include a source dataset").ThrowAsJavaScriptException();
    return 1;
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "dst"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "dst"));
    if (prop.IsObject() && !prop.IsNull() && prop.As<Napi::Object>().InstanceOf(Dataset::constructor.Value())) {
      this->dst_obj = prop.As<Napi::Object>();
      this->dst = node_gdal::UnwrapWrapped<Dataset>(this->dst_obj);
#if GDAL_VERSION_MAJOR == 2 && GDAL_VERSION_MINOR < 3
      options->hDstDS = static_cast<GDALDatasetH>(this->dst->get());
#else
      options->hDstDS = GDALDataset::ToHandle(this->dst->get());
#endif
      if (!options->hDstDS) {
        Napi::Error::New(node_gdal::napi_env(), "dst dataset already closed").ThrowAsJavaScriptException();
        return 1;
      }
    } else if (!prop.IsUndefined() && !prop.IsNull()) {
      Napi::TypeError::New(node_gdal::napi_env(), "dst property must be a Dataset object").ThrowAsJavaScriptException();
      return 1;
    }
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "srcBands"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "srcBands"));
    if (src_bands.parse(prop)) {
      return 1; // error parsing number list
    }
    options->panSrcBands = src_bands.get();
    options->nBandCount = src_bands.length();
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "dstBands"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "dstBands"));
    if (dst_bands.parse(prop)) {
      return 1; // error parsing number list
    }
    options->panDstBands = dst_bands.get();

    if (!options->panSrcBands) {
      Napi::Error::New(node_gdal::napi_env(), "srcBands must be provided if dstBands option is used").ThrowAsJavaScriptException();
      return 1;
    }
    if (dst_bands.length() != options->nBandCount) {
      Napi::Error::New(node_gdal::napi_env(), "Number of dst bands must equal number of src bands").ThrowAsJavaScriptException();
      return 1;
    }
  }
  if (options->panSrcBands && !options->panDstBands) {
    Napi::Error::New(node_gdal::napi_env(), "dstBands must be provided if srcBands option is used").ThrowAsJavaScriptException();
    return 1;
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "srcNodata"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "srcNodata"));
    if (prop.IsNumber()) {
      src_nodata = new double(prop.As<Napi::Number>().DoubleValue());
      options->padfSrcNoDataReal = src_nodata;
    } else if (!prop.IsUndefined() && !prop.IsNull()) {
      Napi::TypeError::New(node_gdal::napi_env(), "srcNodata property must be a number").ThrowAsJavaScriptException();
      return 1;
    }
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "dstNodata"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "dstNodata"));
    if (prop.IsNumber()) {
      dst_nodata = new double(prop.As<Napi::Number>().DoubleValue());
      options->padfDstNoDataReal = dst_nodata;
    } else if (!prop.IsUndefined() && !prop.IsNull()) {
      Napi::TypeError::New(node_gdal::napi_env(), "dstNodata property must be a number").ThrowAsJavaScriptException();
      return 1;
    }
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "srcAlphaBand"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "srcAlphaBand"));
    if (prop.IsNumber()) {
      options->nSrcAlphaBand = prop.As<Napi::Number>().Int32Value();
    } else if (!prop.IsUndefined() && !prop.IsNull()) {
      Napi::TypeError::New(node_gdal::napi_env(), "srcAlphaBand property must be an integer").ThrowAsJavaScriptException();
      return 1;
    }
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "dstAlphaBand"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "dstAlphaBand"));
    if (prop.IsNumber()) {
      options->nDstAlphaBand = prop.As<Napi::Number>().Int32Value();
    } else if (!prop.IsUndefined() && !prop.IsNull()) {
      Napi::TypeError::New(node_gdal::napi_env(), "dstAlphaBand property must be an integer").ThrowAsJavaScriptException();
      return 1;
    }
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "blend"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "blend"));
    if (prop.IsNumber()) {
      options->dfCutlineBlendDist = prop.As<Napi::Number>().DoubleValue();
    } else if (!prop.IsUndefined() && !prop.IsNull()) {
      Napi::TypeError::New(node_gdal::napi_env(), "cutline blend distance must be a number").ThrowAsJavaScriptException();
      return 1;
    }
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "cutline"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "cutline"));
    if (prop.IsObject() && !prop.IsNull() && prop.As<Napi::Object>().InstanceOf(Geometry::constructor.Value())) {
      options->hCutline = node_gdal::UnwrapWrapped<Geometry>(prop.As<Napi::Object>())->get();
    } else if (!prop.IsUndefined() && !prop.IsNull()) {
      Napi::TypeError::New(node_gdal::napi_env(), "cutline property must be a Geometry object").ThrowAsJavaScriptException();
      return 1;
    }
  }
  if (obj.As<Napi::Object>().HasOwnProperty(Napi::String::New(node_gdal::napi_env(), "multi"))) {
    prop = obj.As<Napi::Object>().Get(Napi::String::New(node_gdal::napi_env(), "multi"));
    if (prop.IsBoolean() && prop.As<Napi::Boolean>().Value()) { multi = true; }
  }
  return 0;
}

} // namespace node_gdal
