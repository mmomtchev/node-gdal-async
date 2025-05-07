#ifndef __NODE_TYPEDARRAY_H__
#define __NODE_TYPEDARRAY_H__

// node
#include <node.h>
#include <node_object_wrap.h>

// nan
#include "../nan-wrapper.h"

// gdal
#include <gdal_priv.h>

using namespace v8;
using namespace node;

namespace node_gdal {

// V8 Typed Arrays
//
// Int8Array      1  8-bit twos complement  signed integer  signed char
// Uint8Array     1  8-bit unsigned integer  unsigned  char
// Int16Array     2  16-bit twos complement  signed integer  short
// Uint16Array    2  16-bit unsigned integer  unsigned short
// Int32Array     4  32-bit twos complement  signed integer  int
// Uint32Array    4  32-bit unsigned integer  unsigned int
// BigInt64Array  8 64-bit signed integer GInt64 (GDAL type)
// BigUint64Array 8 64-bit unsigned integer GUInt64 (GDAL type)
// Float16Array   2  16-bit IEEE floating point number  GFloat16 (GDAL type)
// Float32Array   4  32-bit IEEE floating point number  float
// Float64Array   8  64-bit IEEE floating point number  double

// GDALDataType {
//   GDT_Unknown = 0,
//   GDT_Byte = 1,
//   GDT_UInt16 = 2,
//   GDT_Int16 = 3,
//   GDT_UInt32 = 4,
//   GDT_Int32 = 5,
//   GDT_Float32 = 6,
//   GDT_Float64 = 7,
//   GDT_CInt16 = 8,
//   GDT_CInt32 = 9,
//   GDT_CFloat32 = 10,
//   GDT_CFloat64 = 11,
//   GDT_UInt64 = 12,
//   GDT_Int64 = 13,
//   GDT_Float16 = 15,
//   GDT_CFloat16 = 16
// }

namespace TypedArray {

Local<Value> New(GDALDataType type, int64_t length);
Local<Value> New(GDALDataType type, void *data, int64_t length);
GDALDataType Identify(Local<Object> array);
void *Validate(Local<Object> obj, GDALDataType type, int64_t min_length);
bool ValidateLength(size_t length, int64_t min_length);
} // namespace TypedArray

} // namespace node_gdal
#endif
