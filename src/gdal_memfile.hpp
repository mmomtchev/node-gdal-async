#ifndef __NODE_GDAL_MEMFILE_H__
#define __NODE_GDAL_MEMFILE_H__

// node
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>

// nan
#include "nan-wrapper.h"

// gdal
#include <gdal_priv.h>

#include "gdal_common.hpp"

using namespace v8;
using namespace node;

// A vsimem file

namespace node_gdal {

class Memfile {
  void *data;
  Nan::Persistent<Object> *persistent;
  static void weakCallback(const Nan::WeakCallbackInfo<Memfile> &);

    public:
  std::string filename;
  Memfile(void *);
  Memfile(void *, const std::string &filename);
  ~Memfile();
  static Memfile *get(Local<Object>);
  static Memfile *get(Local<Object>, const std::string &filename);
  static bool copy(Local<Object>, const std::string &filename);
  static std::map<void *, Memfile *> memfile_collection;
  static std::map<void *, size_t> tracked_buffers;

  static void Initialize(Local<Object> target);
  static NAN_METHOD(vsimemSet);
  static NAN_METHOD(vsimemAnonymous);
  static NAN_METHOD(vsimemRelease);
  static NAN_METHOD(vsimemCopy);
};
} // namespace node_gdal
#endif
