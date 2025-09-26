#include "gdal_memfile.hpp"

namespace node_gdal {

/**
 * File operations specific to in-memory `/vsimem/` files.
 *
 * @namespace vsimem
 */

// These are the currently used by GDAL Node.js buffers
std::map<void *, Memfile *> Memfile::memfile_collection;
// These are the currently tracked GDAL internal buffers
std::map<void *, size_t> Memfile::tracked_buffers;

Memfile::Memfile(void *data, const std::string &filename) : data(data), filename(filename) {
}

Memfile::Memfile(void *data) : data(data) {
  char _filename[32];
  // The pointer makes for a perfect unique filename
  snprintf(_filename, sizeof(_filename), "/vsimem/%p", data);
  this->filename = _filename;
}

Memfile::~Memfile() {
  if (persistent && !persistent->IsEmpty()) { persistent->Reset(); }
  delete persistent;
}

void Memfile::weakCallback(const Nan::WeakCallbackInfo<Memfile> &file) {
  Memfile *mem = file.GetParameter();
  memfile_collection.erase(mem->data);
  VSIUnlink(mem->filename.c_str());
  delete mem->persistent;
  mem->persistent = nullptr;
  delete mem;
}

void Memfile::Initialize(Local<Object> target) {
  Local<Object> vsimem = Nan::New<Object>();
  Nan::Set(target, Nan::New("vsimem").ToLocalChecked(), vsimem);
  Nan::SetMethod(vsimem, "_anonymous", Memfile::vsimemAnonymous); // not a public API
  Nan::SetMethod(vsimem, "set", Memfile::vsimemSet);
  Nan::SetMethod(vsimem, "release", Memfile::vsimemRelease);
  Nan::SetMethod(vsimem, "copy", Memfile::vsimemCopy);
}

// Anonymous buffers are handled by the GC
// Whenever the JS buffer goes out of scope, the file is deleted
Memfile *Memfile::get(Local<Object> buffer) {
  if (!Buffer::HasInstance(buffer)) return nullptr;
  void *data = Buffer::Data(buffer);
  if (data == nullptr) return nullptr;
  if (memfile_collection.count(data)) return memfile_collection.find(data)->second;

  size_t len = Buffer::Length(buffer);
  Memfile *mem = nullptr;
  mem = new Memfile(data);

  VSILFILE *vsi = VSIFileFromMemBuffer(mem->filename.c_str(), (GByte *)data, len, 0);
  if (vsi == nullptr) return nullptr;
  VSIFCloseL(vsi);

  mem->persistent = new Nan::Persistent<Object>(buffer);
  mem->persistent->SetWeak(mem, weakCallback, Nan::WeakCallbackType::kParameter);
  memfile_collection[data] = mem;
  return mem;
}

// Named buffers are protected from the GC and are owned by Node
Memfile *Memfile::get(Local<Object> buffer, const std::string &filename) {
  if (!Buffer::HasInstance(buffer)) return nullptr;
  void *data = node::Buffer::Data(buffer);
  if (data == nullptr) { return nullptr; }

  size_t len = node::Buffer::Length(buffer);
  Memfile *mem = nullptr;
  mem = new Memfile(data, filename);

  VSILFILE *vsi = VSIFileFromMemBuffer(mem->filename.c_str(), (GByte *)data, len, 0);
  if (vsi == nullptr) return nullptr;
  VSIFCloseL(vsi);

  mem->persistent = new Nan::Persistent<Object>(buffer);
  memfile_collection[data] = mem;
  return mem;
}

// GDAL buffers handled by GDAL and are not referenced by node-gdal-async
bool Memfile::copy(Local<Object> buffer, const std::string &filename) {
  if (!Buffer::HasInstance(buffer)) return false;
  void *data = node::Buffer::Data(buffer);
  if (data == nullptr) return false;

  size_t len = node::Buffer::Length(buffer);

  void *dataCopy = CPLMalloc(len);
  if (dataCopy == nullptr) return false;

  memcpy(dataCopy, data, len);

  VSILFILE *vsi = VSIFileFromMemBuffer(filename.c_str(), (GByte *)dataCopy, len, 1);
  if (vsi == nullptr) {
    CPLFree(dataCopy);
    return false;
  }

  // If you malloc, you adjust external memory too (https://github.com/nodejs/node/issues/40936)
  Memfile::tracked_buffers.insert({dataCopy, len});
  Nan::AdjustExternalMemory(len);

  VSIFCloseL(vsi);
  return true;
}

/**
 * Create an in-memory `/vsimem/` file from a `Buffer`.
 * This is a zero-copy operation - GDAL will read from the Buffer which will be
 * protected by the GC even if it goes out of scope.
 *
 * The file will stay in memory until it is deleted with `gdal.vsimem.release`.
 *
 * The file will be in read-write mode, but GDAL won't
 * be able to extend it as the allocated memory will be tied to the `Buffer` object.
 * Use `gdal.vsimem.copy` to create an extendable copy.
 *
 * @static
 * @method set
 * @memberof vsimem
 * @throws {Error}
 * @param {Buffer} data A binary buffer containing the file data
 * @param {string} filename A file name beginning with `/vsimem/`
 */
NAN_METHOD(Memfile::vsimemSet) {
  Local<Object> buffer;
  std::string filename;

  NODE_ARG_OBJECT(0, "buffer", buffer);
  NODE_ARG_STR(1, "filename", filename);

  Memfile *memfile = Memfile::get(buffer, filename);
  if (memfile == nullptr) Nan::ThrowError("Failed creating in-memory file");
}

/**
 * Create an in-memory `/vsimem/` file copying a `Buffer`.
 * This method copies the `Buffer` into GDAL's own memory heap
 * creating an in-memory file that can be freely extended by GDAL.
 * `gdal.vsimem.set` is the better choice unless the file needs to be extended.
 *
 * The file will stay in memory until it is deleted with `gdal.vsimem.release`.
 *
 * @static
 * @method copy
 * @memberof vsimem
 * @throws {Error}
 * @param {Buffer} data A binary buffer containing the file data
 * @param {string} filename A file name beginning with `/vsimem/`
 */
NAN_METHOD(Memfile::vsimemCopy) {
  Local<Object> buffer;
  std::string filename;

  NODE_ARG_OBJECT(0, "buffer", buffer);
  NODE_ARG_STR(1, "filename", filename);

  if (!Memfile::copy(buffer, filename)) Nan::ThrowError("Failed creating in-memory file");
}

/*
 * This creates an anonymous vsimem file from a Buffer.
 * It is automatically deleted when the Buffer goes out of scope.
 * This is not a public method as it is not always safe.
 */
NAN_METHOD(Memfile::vsimemAnonymous) {
  Local<Object> buffer;

  NODE_ARG_OBJECT(0, "buffer", buffer);

  Memfile *memfile = Memfile::get(buffer);
  if (memfile == nullptr)
    Nan::ThrowError("Failed creating in-memory file");
  else
    info.GetReturnValue().Set(Nan::New<String>(memfile->filename).ToLocalChecked());
}

/**
 * Delete and retrieve the contents of an in-memory `/vsimem/` file.
 * This is a very fast zero-copy operation.
 * It does not block the event loop.
 * If the file was created by `vsimem.set`, it will return a reference
 * to the same `Buffer` that was used to create it.
 * Otherwise it will construct a new `Buffer` object with the GDAL
 * allocated buffer as its backing store.
 *
 * ***WARNING***!
 *
 * The file must not be open or random memory corruption is possible with GDAL <= 3.3.1.
 * GDAL >= 3.3.2 will gracefully fail further operations and this function will always be safe.
 *
 * @static
 * @method release
 * @memberof vsimem
 * @param {string} filename A file name beginning with `/vsimem/`
 * @throws {Error}
 * @return {Buffer} A binary buffer containing all the data
 */
NAN_METHOD(Memfile::vsimemRelease) {
  vsi_l_offset len;
  std::string filename;
  NODE_ARG_STR(0, "filename", filename);

  CPLErrorReset();
  void *data = VSIGetMemFileBuffer(filename.c_str(), &len, false);
  if (data == nullptr) {
    NODE_THROW_LAST_CPLERR;
    return;
  }

  // Two cases:
  if (memfile_collection.count(data)) {
    // the file comes from a named buffer and the buffer is owned by Node
    // -> a reference to the existing buffer is returned
    Memfile *mem = memfile_collection.find(data)->second;
    memfile_collection.erase(mem->data);
    VSIUnlink(mem->filename.c_str());
    info.GetReturnValue().Set(Nan::New(*mem->persistent));
    delete mem;
  } else {
    // the file has been created by GDAL and the buffer is owned by GDAL
    // -> a new Buffer is constructed and GDAL has to relinquish control
    // The GC will call the lambda at some point to free the backing storage
    VSIGetMemFileBuffer(filename.c_str(), &len, true);
    info.GetReturnValue().Set(Nan::NewBuffer(
                                static_cast<char *>(data),
                                static_cast<size_t>(len),
                                [](char *data, void *) {
                                  // If the returned internal buffer is tracked towards the heap
                                  // signal the GC that we are releasing the amount we
                                  // initially counted - even if by now this amount might be
                                  // different
                                  std::map<void *, size_t>::iterator b =
                                    Memfile::tracked_buffers.find(static_cast<void *>(data));
                                  if (b != Memfile::tracked_buffers.end()) {
                                    Nan::AdjustExternalMemory(-(static_cast<int>(b->second)));
                                    Memfile::tracked_buffers.erase(b);
                                  }
                                  CPLFree(data);
                                },
                                nullptr)
                                .ToLocalChecked());
  }
}

} // namespace node_gdal
