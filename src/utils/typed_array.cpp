#include "typed_array.hpp"

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
#include <cpl_float.h>
#endif

#include <climits>
#include <sstream>

const double max_safe_integer = std::numeric_limits<double>::radix / std::numeric_limits<double>::epsilon();

namespace node_gdal {

// https://github.com/joyent/node/issues/4201#issuecomment-9837340

Local<Value> TypedArray::New(GDALDataType type, int64_t length) {
  Nan::EscapableHandleScope scope;

  Local<Value> val;
  Local<Function> constructor;
  Local<Object> global = Nan::GetCurrentContext()->Global();

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
    default: Nan::ThrowError("Unsupported array type"); return scope.Escape(Nan::Undefined());
  }

  // make ArrayBuffer
  val = Nan::Get(global, Nan::New("ArrayBuffer").ToLocalChecked()).ToLocalChecked();
  if (!val->IsFunction()) {
    Nan::ThrowError("Error getting ArrayBuffer constructor");
    return Local<Value>();
  }

  constructor = val.As<Function>();
  int64_t size = length * GDALGetDataTypeSize(type) / 8;
  if (size == 0) {
    Nan::ThrowError("Invalid GDAL data type");
    return Local<Value>();
  }
  if (size > max_safe_integer) {
    Nan::ThrowError("Buffer size exceeds maximum safe JS integer");
    return Local<Value>();
  }
  Local<Value> v8_size = Nan::New<v8::Number>(size);
  MaybeLocal<Object> array_buffer_maybe = Nan::NewInstance(constructor, 1, &v8_size);
  if (array_buffer_maybe.IsEmpty()) { return Local<Value>(); }
  Local<Value> array_buffer = array_buffer_maybe.ToLocalChecked();
  if (!array_buffer->IsObject()) {
    Nan::ThrowError("Error allocating ArrayBuffer");
    return Local<Value>();
  }

  // make TypedArray
  val = Nan::Get(global, Nan::New(name).ToLocalChecked()).ToLocalChecked();
  if (val.IsEmpty()) { return Local<Value>(); }
  if (!val->IsFunction()) {
    Nan::ThrowError("Error getting typed array constructor");
    return Local<Value>();
  }
  constructor = val.As<Function>();
  MaybeLocal<Object> array_maybe = Nan::NewInstance(constructor, 1, &array_buffer);
  if (array_maybe.IsEmpty()) {
    Nan::ThrowRangeError("Failed constructing a TypedArray, data is probably over the 4G elements limit");
    return Local<Value>();
  }
  Local<Object> array = array_maybe.ToLocalChecked();

  Nan::Set(array, Nan::New("_gdal_type").ToLocalChecked(), Nan::New(type));

  return scope.Escape(array);
}

// Create a new TypedArray view over an existing memory buffer
// This function throws because it is meant to be used inside a pixel function
Local<Value> TypedArray::New(GDALDataType type, void *data, int64_t length) {
  Nan::EscapableHandleScope scope;

  Local<Object> global = Nan::GetCurrentContext()->Global();

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
    case GDT_Float16: throw "Float16Array cannot be created at the moment, pass an existing array"; break;
#endif
    case GDT_Float32: name = "Float32Array"; break;
    case GDT_Float64: name = "Float64Array"; break;
    default: throw "Unsupported array type";
  }

  size_t size = GDALGetDataTypeSizeBytes(type);
  if (size == 0) { throw "Invalid GDAL data type"; }

  // make ArrayBuffer with external storage by creating a Node.js Buffer w/ an empty free callback
  Local<Object> buffer =
    Nan::NewBuffer(reinterpret_cast<char *>(data), length * size, [](char *, void *) {}, nullptr).ToLocalChecked();

  if (buffer.IsEmpty() || !buffer->IsObject()) { throw "Error getting creating Node.js Buffer"; }

  // get the underlying ArrayBuffer
  Local<Value> underlyingAB = Nan::Get(buffer, Nan::New("buffer").ToLocalChecked()).ToLocalChecked();

  // make TypedArray
  Local<Value> val = Nan::Get(global, Nan::New(name).ToLocalChecked()).ToLocalChecked();
  if (val.IsEmpty() || !val->IsFunction()) { throw "Error getting typed array constructor"; }
  Local<Function> constructor = val.As<Function>();

  Local<Object> array = Nan::NewInstance(constructor, 1, &underlyingAB).ToLocalChecked();

  if (array.IsEmpty() || !array->IsObject()) { throw "Error creating TypedArray"; }

  Nan::Set(array, Nan::New("_gdal_type").ToLocalChecked(), Nan::New(type));

  return scope.Escape(array);
}

GDALDataType TypedArray::Identify(Local<Object> obj) {
  Nan::HandleScope scope;

  Local<String> sym = Nan::New("_gdal_type").ToLocalChecked();
  if (!Nan::HasOwnProperty(obj, sym).FromMaybe(false)) return GDT_Unknown;
  Local<Value> val = Nan::Get(obj, sym).ToLocalChecked();
  if (!val->IsNumber()) return GDT_Unknown;

  return (GDALDataType)Nan::To<int32_t>(val).ToChecked();
}

void *TypedArray::Validate(Local<Object> obj, GDALDataType type, int64_t min_length) {
  // validate array
  Nan::HandleScope scope;

  GDALDataType src_type = TypedArray::Identify(obj);
  if (src_type == GDT_Unknown) {
    Nan::ThrowTypeError("Unable to identify GDAL datatype of passed array object");
    return NULL;
  }
  if (src_type != type) {
    std::ostringstream ss;
    ss << "Array type does not match band data type ("
       << "input: " << GDALGetDataTypeName(src_type) << ", target: " << GDALGetDataTypeName(type) << ")";

    Nan::ThrowTypeError(ss.str().c_str());
    return NULL;
  }
  switch (type) {
    case GDT_Byte: {
      Nan::TypedArrayContents<GByte> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    case GDT_Int16: {
      Nan::TypedArrayContents<GInt16> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    case GDT_UInt16: {
      Nan::TypedArrayContents<GUInt16> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    case GDT_Int32: {
      Nan::TypedArrayContents<GInt32> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    case GDT_UInt32: {
      Nan::TypedArrayContents<GUInt32> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 5)
    case GDT_Int64: {
      Nan::TypedArrayContents<GInt64> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    case GDT_UInt64: {
      Nan::TypedArrayContents<GUInt64> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
#endif
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 11)
    case GDT_Float16: {
      Nan::TypedArrayContents<GFloat16> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
#endif
    case GDT_Float32: {
      Nan::TypedArrayContents<float> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    case GDT_Float64: {
      Nan::TypedArrayContents<double> contents(obj);
      if (ValidateLength(contents.length(), min_length)) return NULL;
      return *contents;
    }
    default: Nan::ThrowError("Unsupported array type"); return NULL;
  }
}
bool TypedArray::ValidateLength(size_t length, int64_t min_length) {
  if (static_cast<int64_t>(length) < min_length) {
    std::ostringstream ss;
    ss << "Array length must be greater than or equal to " << min_length;

    Nan::ThrowError(ss.str().c_str());
    return true;
  }
  return false;
}

} // namespace node_gdal
