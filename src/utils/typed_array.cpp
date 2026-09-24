#include "typed_array.hpp"

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
#include <cpl_float.h>
#endif

#include <climits>
#include <limits>
#include <sstream>

const double max_safe_integer = std::numeric_limits<double>::radix / std::numeric_limits<double>::epsilon();

namespace node_gdal {

// https://github.com/joyent/node/issues/4201#issuecomment-9837340

Napi::Value TypedArray::New(GDALDataType type, int64_t length) {
  Napi::Env env = node_gdal::napi_env();

  const char *name;
  switch (type) {
    case GDT_Byte: name = "Uint8Array"; break;
    case GDT_Int16: name = "Int16Array"; break;
    case GDT_UInt16: name = "Uint16Array"; break;
    case GDT_Int32: name = "Int32Array"; break;
    case GDT_UInt32: name = "Uint32Array"; break;
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
    case GDT_Int64: name = "BigInt64Array"; break;
    case GDT_UInt64: name = "BigUint64Array"; break;
#endif
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
    case GDT_Float16: name = "Float16Array"; break;
#endif
    case GDT_Float32: name = "Float32Array"; break;
    case GDT_Float64: name = "Float64Array"; break;
    default:
      Napi::Error::New(env, "Unsupported array type").ThrowAsJavaScriptException();
      return env.Undefined();
  }

  Napi::Object global = env.Global();

  // make ArrayBuffer
  Napi::Value val = global.Get("ArrayBuffer");
  if (!val.IsFunction()) {
    Napi::Error::New(env, "Error getting ArrayBuffer constructor").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  Napi::Function constructor = val.As<Napi::Function>();

  int64_t size = length * GDALGetDataTypeSizeBytes(type);
  if (size == 0) {
    Napi::Error::New(env, "Invalid GDAL data type").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  if (size > max_safe_integer) {
    Napi::Error::New(env, "Buffer size exceeds maximum safe JS integer").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Value array_buffer = constructor.New({Napi::Number::New(env, static_cast<double>(size))});
  if (array_buffer.IsEmpty() || !array_buffer.IsObject()) {
    Napi::Error::New(env, "Error allocating ArrayBuffer").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // make TypedArray
  val = global.Get(name);
  if (val.IsEmpty() || !val.IsFunction()) {
    Napi::Error::New(env, "Error getting typed array constructor").ThrowAsJavaScriptException();
    return env.Undefined();
  }
  constructor = val.As<Napi::Function>();

  Napi::Value array;
  try {
    array = constructor.New({array_buffer});
  } catch (const Napi::Error &) {
    Napi::RangeError::New(
      env, "Failed constructing a TypedArray, data is probably over the 4G elements limit")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }
  if (array.IsEmpty() || !array.IsObject()) {
    Napi::RangeError::New(
      env, "Failed constructing a TypedArray, data is probably over the 4G elements limit")
      .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  array.As<Napi::Object>().Set("_gdal_type", Napi::Number::New(env, type));

  return array;
}

// Create a new TypedArray view over an existing memory buffer
// This function throws because it is meant to be used inside a pixel function
Napi::Value TypedArray::New(GDALDataType type, void *data, int64_t length) {
  Napi::Env env = node_gdal::napi_env();

  const char *name;
  switch (type) {
    case GDT_Byte: name = "Uint8Array"; break;
    case GDT_Int16: name = "Int16Array"; break;
    case GDT_UInt16: name = "Uint16Array"; break;
    case GDT_Int32: name = "Int32Array"; break;
    case GDT_UInt32: name = "Uint32Array"; break;
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
    case GDT_Int64: name = "BigInt64Array"; break;
    case GDT_UInt64: name = "BigUint64Array"; break;
#endif
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
    case GDT_Float16: name = "Float16Array"; break;
#endif
    case GDT_Float32: name = "Float32Array"; break;
    case GDT_Float64: name = "Float64Array"; break;
    default: throw "Unsupported array type";
  }

  size_t size = GDALGetDataTypeSizeBytes(type);
  if (size == 0) { throw "Invalid GDAL data type"; }

  // make an ArrayBuffer with external storage, i.e. a Node.js Buffer without a
  // free callback
  Napi::Object buffer = Napi::Buffer<char>::New(env, reinterpret_cast<char *>(data), length * size);
  if (buffer.IsEmpty()) { throw "Error getting creating Node.js Buffer"; }

  // get the underlying ArrayBuffer
  Napi::Value underlyingAB = buffer.Get("buffer");

  // make TypedArray
  Napi::Value val = env.Global().Get(name);
  if (val.IsEmpty() || !val.IsFunction()) { throw "Error getting typed array constructor"; }
  Napi::Function constructor = val.As<Napi::Function>();

  Napi::Value array = constructor.New({underlyingAB});
  if (array.IsEmpty() || !array.IsObject()) { throw "Error creating TypedArray"; }

  array.As<Napi::Object>().Set("_gdal_type", Napi::Number::New(env, type));

  return array;
}

GDALDataType TypedArray::Identify(Napi::Object obj) {
  Napi::Env env = node_gdal::napi_env();

  if (!obj.HasOwnProperty("_gdal_type")) return GDT_Unknown;
  Napi::Value val = obj.Get("_gdal_type");
  if (!val.IsNumber()) return GDT_Unknown;

  return (GDALDataType)val.As<Napi::Number>().Int32Value();
}

// The old binding's typed arrays took any ArrayBufferView, DataView included -
// which is what the JS layer hands over for a Float16Array. Napi::TypedArrayOf
// only accepts a real typed array, and asking a DataView for its element length
// fails with napi_invalid_arg.
template <typename T> static void *validate(Napi::Object obj, GDALDataType type, int64_t min_length) {
  // `::napi_env`: node_gdal::napi_env() (the function) otherwise hides the type
  ::napi_env napiEnv = node_gdal::napi_env();
  napi_value value = obj;

  void *data = nullptr;
  size_t byte_length = 0;

  bool is_dataview = false;
  if (napi_is_dataview(napiEnv, value, &is_dataview) != napi_ok) return NULL;

  if (is_dataview) {
    napi_value arraybuffer = nullptr;
    size_t byte_offset = 0;
    if (napi_get_dataview_info(napiEnv, value, &byte_length, &data, &arraybuffer, &byte_offset) != napi_ok) return NULL;
  } else {
    napi_typedarray_type array_type;
    size_t element_length = 0;
    size_t byte_offset = 0;
    napi_value arraybuffer = nullptr;
    if (napi_get_typedarray_info(napiEnv, value, &array_type, &element_length, &data, &arraybuffer, &byte_offset) !=
      napi_ok)
      return NULL;
    byte_length = element_length * sizeof(T);
  }

  if (TypedArray::ValidateLength(byte_length / sizeof(T), min_length)) return NULL;
  return data;
}

void *TypedArray::Validate(Napi::Object obj, GDALDataType type, int64_t min_length) {
  // validate array

  GDALDataType src_type = TypedArray::Identify(obj);
  if (src_type == GDT_Unknown) {
    Napi::TypeError::New(node_gdal::napi_env(), "Unable to identify GDAL datatype of passed array object")
      .ThrowAsJavaScriptException();
    return NULL;
  }
  if (src_type != type) {
    std::ostringstream ss;
    ss << "Array type does not match band data type ("
       << "input: " << GDALGetDataTypeName(src_type) << ", target: " << GDALGetDataTypeName(type) << ")";

    Napi::TypeError::New(node_gdal::napi_env(), ss.str().c_str()).ThrowAsJavaScriptException();
    return NULL;
  }
  switch (type) {
    case GDT_Byte: return validate<GByte>(obj, type, min_length);
    case GDT_Int16: return validate<GInt16>(obj, type, min_length);
    case GDT_UInt16: return validate<GUInt16>(obj, type, min_length);
    case GDT_Int32: return validate<GInt32>(obj, type, min_length);
    case GDT_UInt32: return validate<GUInt32>(obj, type, min_length);
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
    case GDT_Int64: return validate<GInt64>(obj, type, min_length);
    case GDT_UInt64: return validate<GUInt64>(obj, type, min_length);
#endif
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
    case GDT_Float16: return validate<GFloat16>(obj, type, min_length);
#endif
    case GDT_Float32: return validate<float>(obj, type, min_length);
    case GDT_Float64: return validate<double>(obj, type, min_length);
    default:
      Napi::Error::New(node_gdal::napi_env(), "Unsupported array type").ThrowAsJavaScriptException();
      return NULL;
  }
}

bool TypedArray::ValidateLength(size_t length, int64_t min_length) {
  if (static_cast<int64_t>(length) < min_length) {
    std::ostringstream ss;
    ss << "Array length must be greater than or equal to " << min_length;

    Napi::Error::New(node_gdal::napi_env(), ss.str().c_str()).ThrowAsJavaScriptException();
    return true;
  }
  return false;
}

} // namespace node_gdal
