#ifndef __NODE_GDAL_ASYNC_WORKER_H__
#define __NODE_GDAL_ASYNC_WORKER_H__

#include <thread>
#include <functional>
#include <chrono>
#include "nan-wrapper.h"
#include "gdal_common.hpp"

namespace node_gdal {

// The id of the main V8 thread
extern std::thread::id mainV8ThreadId;

// This generates method definitions for 2 methods: sync and async version and a hidden common block
#define GDAL_ASYNCABLE_DEFINE(method)                                                                                  \
  NAN_METHOD(method) {                                                                                                 \
    method##_do(info, false);                                                                                          \
  }                                                                                                                    \
  NAN_METHOD(method##Async) {                                                                                          \
    method##_do(info, true);                                                                                           \
  }                                                                                                                    \
  void method##_do(const Nan::FunctionCallbackInfo<v8::Value> &info, bool async)

// This generates getter definitions for 2 getters: sync and async version and a hidden common block
#define GDAL_ASYNCABLE_GETTER_DEFINE(method)                                                                           \
  NAN_GETTER(method) {                                                                                                 \
    method##_do(property, info, false);                                                                                \
  }                                                                                                                    \
  NAN_GETTER(method##Async) {                                                                                          \
    method##_do(property, info, true);                                                                                 \
  }                                                                                                                    \
  Nan::NAN_GETTER_RETURN_TYPE method##_do(v8::Local<v8::String> property, Nan::NAN_GETTER_ARGS_TYPE info, bool async)

// This generates method declarations for 2 methods: sync and async version and a hidden common block
#define GDAL_ASYNCABLE_DECLARE(method)                                                                                 \
  static NAN_METHOD(method);                                                                                           \
  static NAN_METHOD(method##Async);                                                                                    \
  static void method##_do(const Nan::FunctionCallbackInfo<v8::Value> &info, bool async)

// This generates getter declarations for 2 getters: sync and async version and a hidden common block
#define GDAL_ASYNCABLE_GETTER_DECLARE(method)                                                                          \
  static NAN_GETTER(method);                                                                                           \
  static NAN_GETTER(method##Async);                                                                                    \
  static Nan::NAN_GETTER_RETURN_TYPE method##_do(                                                                      \
    v8::Local<v8::String> property, Nan::NAN_GETTER_ARGS_TYPE info, bool async)

#define GDAL_ASYNCABLE_GLOBAL(method)                                                                                  \
  NAN_METHOD(method);                                                                                                  \
  NAN_METHOD(method##Async);                                                                                           \
  void method##_do(const Nan::FunctionCallbackInfo<v8::Value> &info, bool async)

#define GDAL_ASYNCABLE_TEMPLATE(method)                                                                                \
  static NAN_METHOD(method) {                                                                                          \
    method##_do(info, false);                                                                                          \
  }                                                                                                                    \
  static NAN_METHOD(method##Async) {                                                                                   \
    method##_do(info, true);                                                                                           \
  }                                                                                                                    \
  static void method##_do(const Nan::FunctionCallbackInfo<v8::Value> &info, bool async)

#define GDAL_ISASYNC async

#define THROW_OR_REJECT(msg)                                                                                           \
  if (async) {                                                                                                         \
    auto context = info.GetIsolate()->GetCurrentContext();                                                             \
    auto resolver = v8::Promise::Resolver::New(context).ToLocalChecked();                                              \
    resolver->Reject(context, Nan::Error(msg)).FromJust();                                                             \
    info.GetReturnValue().Set(resolver->GetPromise());                                                                 \
  } else                                                                                                               \
    Nan::ThrowError(msg);

// Handle locking (used only for sync methods)
#define GDAL_LOCK_PARENT(p)                                                                                            \
  AsyncGuard lock;                                                                                                     \
  try {                                                                                                                \
    lock.acquire((p)->parent_uid);                                                                                     \
  } catch (const char *err) {                                                                                          \
    Nan::ThrowError(err);                                                                                              \
    return;                                                                                                            \
  }

static const char eventLoopWarning[] =
  "Synchronous method called while an asynchronous operation is running in the background, check node_modules/gdal-async/ASYNCIO.md, event loop blocked for ";
// These constructors throw
// Only one use case never throws: on the main thread
// and after checking that the Dataset is alive
class AsyncGuard {
    public:
  inline AsyncGuard() : lock(nullptr), locks(nullptr) {
  }
  inline AsyncGuard(long uid) : locks(nullptr) {
    lock = object_store.lockDataset(uid);
  }
  inline AsyncGuard(vector<long> uids) : lock(nullptr), locks(nullptr) {
    if (uids.size() == 1)
      lock = object_store.lockDataset(uids[0]);
    else
      locks = make_shared<vector<AsyncLock>>(object_store.lockDatasets(uids));
  }
  inline AsyncGuard(vector<long> uids, bool warning) : lock(nullptr), locks(nullptr) {
    bool locked = true;
    if (uids.size() == 1) {
      if (uids[0] == 0) return;
      lock = warning ? object_store.tryLockDataset(uids[0], locked) : object_store.lockDataset(uids[0]);
      if (!locked) { MEASURE_EXECUTION_TIME(eventLoopWarning, lock = object_store.lockDataset(uids[0])); }
    } else {
      locks = warning ? make_shared<vector<AsyncLock>>(object_store.tryLockDatasets(uids, locked))
                      : make_shared<vector<AsyncLock>>(object_store.lockDatasets(uids));
      if (!locked) {
        MEASURE_EXECUTION_TIME(
          eventLoopWarning, locks = make_shared<vector<AsyncLock>>(object_store.lockDatasets(uids)));
      }
    }
  }
  inline void acquire(long uid) {
    if (lock != nullptr) throw "Trying to acquire multiple locks";
    lock = object_store.lockDataset(uid);
  }
  inline ~AsyncGuard() {
    if (lock != nullptr) object_store.unlockDataset(lock);
    if (locks != nullptr) object_store.unlockDatasets(*locks);
  }

    private:
  AsyncLock lock;
  shared_ptr<vector<AsyncLock>> locks;
};

// Node.js NAN null initializes and trivially copies objects of this class without asking permission
struct GDALProgressInfo {
  double complete;
  const char *message;

  GDALProgressInfo(const GDALProgressInfo &o);
  GDALProgressInfo(double, const char *);
  GDALProgressInfo();
  ~GDALProgressInfo() = default;
};

class GDALSyncExecutionProgress {
  Nan::Callback *progress_callback;

  GDALSyncExecutionProgress() = delete;

    public:
  GDALSyncExecutionProgress(Nan::Callback *);
  ~GDALSyncExecutionProgress();
  void Send(GDALProgressInfo *) const;
};

typedef std::function<v8::Local<v8::Value>(const char *)> GetFromPersistentFunc;
typedef Nan::AsyncProgressWorkerBase<GDALProgressInfo> GDALAsyncProgressWorker;
typedef GDALAsyncProgressWorker::ExecutionProgress GDALAsyncExecutionProgress;

// This an ExecutionContext that works both with Node.js' NAN ExecutionProgress when in async mode
// and with GDALSyncExecutionContext when in sync mode
class GDALExecutionProgress {
  // Only one of these is active at any given moment
  const GDALAsyncExecutionProgress *async;
  const GDALSyncExecutionProgress *sync;

  GDALExecutionProgress() = delete;

    public:
  GDALExecutionProgress(const GDALAsyncExecutionProgress *);
  GDALExecutionProgress(const GDALSyncExecutionProgress *);
  ~GDALExecutionProgress();
  void Send(GDALProgressInfo *info) const;
};

// This is the progress callback trampoline
// It can be invoked both in the main thread (in sync mode) or in auxillary thread (in async mode)
// It is essentially a gateway between the GDAL world and Node.js/V8 world
int ProgressTrampoline(double dfComplete, const char *pszMessage, void *pProgressArg);

//
// This is the common class for handling async operations
// It has two subclasses: GDALCallbackWorker and GDALPromiseWorker
// They are meant to be used by GDALAsyncableJob defined below
//
// It takes the lambdas as input
// GDALType is the type of the object that will be carried from
// the aux thread to the main thread
//
// JS-visible object creation is possible only in the main thread while
// ths JS world is not running
//
template <class GDALType> class GDALAsyncWorker : public GDALAsyncProgressWorker {
    public:
  typedef std::function<GDALType(const GDALExecutionProgress &)> GDALMainFunc;
  typedef std::function<v8::Local<v8::Value>(const GDALType, const GetFromPersistentFunc &)> GDALRValFunc;

    private:
  Nan::Callback *progressCallback;
  const GDALMainFunc doit;
  const GDALRValFunc rval;
  const std::vector<long> ds_uids;
  GDALType raw;

    public:
  explicit GDALAsyncWorker(
    Nan::Callback *resultCallback,
    Nan::Callback *progressCallback,
    const GDALMainFunc &doit,
    const GDALRValFunc &rval,
    const std::map<std::string, v8::Local<v8::Object>> &objects,
    const std::vector<long> &ds_uids);

  ~GDALAsyncWorker();

  void Execute(const ExecutionProgress &progress);
  Local<Value> ProduceRVal();
  void HandleProgressCallback(const GDALProgressInfo *data, size_t count);
};

template <class GDALType>
GDALAsyncWorker<GDALType>::GDALAsyncWorker(
  Nan::Callback *resultCallback,
  Nan::Callback *progressCallback,
  const GDALMainFunc &doit,
  const GDALRValFunc &rval,
  const std::map<std::string, v8::Local<v8::Object>> &objects,
  const std::vector<long> &ds_uids)
  : GDALAsyncProgressWorker(resultCallback, "node-gdal:GDALAsyncWorker"),
    progressCallback(progressCallback),
    // These members are not references! These functions must be copied
    // as they will be executed in async context!
    doit(doit),
    rval(rval),
    ds_uids(ds_uids) {
  // Main thread with the JS world is not running
  // Get persistent handles
  for (auto i = objects.begin(); i != objects.end(); i++) SaveToPersistent(i->first.c_str(), i->second);
  for (auto i = ds_uids.begin(); i != ds_uids.end(); i++)
    if (*i != 0) SaveToPersistent(("ds" + std::to_string(*i)).c_str(), object_store.get<GDALDataset *>(*i));
}

template <class GDALType> Local<Value> GDALAsyncWorker<GDALType>::ProduceRVal() {
  return rval(raw, [this](const char *key) { return this->GetFromPersistent(key); });
}

template <class GDALType> void GDALAsyncWorker<GDALType>::Execute(const ExecutionProgress &progress) {
  // Aux thread with the JS world running
  // V8 objects are not acessible here
  try {
    GDALExecutionProgress executionProgress(&progress);
    AsyncGuard lock(ds_uids);
    raw = doit(executionProgress);
  } catch (const char *err) { this->SetErrorMessage(err); }
}

template <class GDALType> GDALAsyncWorker<GDALType>::~GDALAsyncWorker() {
  if (progressCallback != nullptr) delete progressCallback;
}

template <class GDALType>
void GDALAsyncWorker<GDALType>::HandleProgressCallback(const GDALProgressInfo *data, size_t count) {
  if (progressCallback == nullptr) return;
  // Back to the main thread with the JS world not running
  Nan::HandleScope scope;
  // A mutex-protected pop in the calling function (in Node.js NAN) can sometimes produce a spurious call
  // with no data, handle gracefully this case -> no need to call JS if there is no data to deliver
  if (data == nullptr || count == 0) return;
  // Receiving more than one callback invocation at the same time is also possible
  // Send only the last one to JS
  const GDALProgressInfo *to_send = data + (count - 1);
  if (data != nullptr && count > 0) {
    v8::Local<v8::Value> argv[] = {Nan::New<Number>(to_send->complete), SafeString::New(to_send->message)};
    Nan::TryCatch try_catch;
    progressCallback->Call(2, argv, this->async_resource);
    if (try_catch.HasCaught()) this->SetErrorMessage("async progress callback exception");
  }
}

// This async worker calls a callback
template <class GDALType> class GDALCallbackWorker : public GDALAsyncWorker<GDALType> {
    public:
  using GDALAsyncWorker<GDALType>::GDALAsyncWorker;
  void HandleOKCallback();
  void HandleErrorCallback();
};

template <class GDALType> void GDALCallbackWorker<GDALType>::HandleOKCallback() {
  // Back to the main thread with the JS world not running
  Nan::HandleScope scope;

  // rval is the user function that will create the returned value
  // we give it a lambda that can access the persistent storage created for this operation
  // It uses our HandleScope so it can return a Local without escaping
  v8::Local<v8::Value> argv[] = {Nan::Null(), this->ProduceRVal()};
  this->callback->Call(2, argv, this->async_resource);
}

template <class GDALType> void GDALCallbackWorker<GDALType>::HandleErrorCallback() {
  // Back to the main thread with the JS world not running
  Nan::HandleScope scope;
  v8::Local<v8::Value> argv[] = {Nan::Error(this->ErrorMessage())};
  this->callback->Call(1, argv, this->async_resource);
}

// This async worker returns a Promise
template <class GDALType> class GDALPromiseWorker : public GDALAsyncWorker<GDALType> {
    public:
  typedef typename GDALAsyncWorker<GDALType>::GDALMainFunc GDALMainFunc;
  typedef typename GDALAsyncWorker<GDALType>::GDALRValFunc GDALRValFunc;

    private:
  Nan::Persistent<v8::Promise::Resolver> *resolver_handle;
  Nan::Persistent<v8::Context> *context_handle;

    public:
  explicit GDALPromiseWorker(
    Nan::NAN_GETTER_ARGS_TYPE info,
    const GDALMainFunc &doit,
    const GDALRValFunc &rval,
    const std::map<std::string, v8::Local<v8::Object>> &objects,
    const std::vector<long> &ds_uids);

  ~GDALPromiseWorker();

  void HandleOKCallback();
  void HandleErrorCallback();
  inline Local<Value> Promise() {
    return Nan::New(*resolver_handle)->GetPromise();
  };
};

template <class GDALType>
GDALPromiseWorker<GDALType>::GDALPromiseWorker(
  Nan::NAN_GETTER_ARGS_TYPE info,
  const GDALMainFunc &doit,
  const GDALRValFunc &rval,
  const std::map<std::string, v8::Local<v8::Object>> &objects,
  const std::vector<long> &ds_uids)
  : GDALAsyncWorker<GDALType>(nullptr, nullptr, doit, rval, objects, ds_uids) {
  auto context = info.GetIsolate()->GetCurrentContext();
  context_handle = new Nan::Persistent<v8::Context>(context);
  auto resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
  resolver_handle = new Nan::Persistent<v8::Promise::Resolver>(resolver);
}

template <class GDALType> void GDALPromiseWorker<GDALType>::HandleOKCallback() {
  Nan::HandleScope scope;
  v8::Local<v8::Context> context = Nan::New(*context_handle);
  v8::Local<v8::Promise::Resolver> resolver = Nan::New(*resolver_handle);
  resolver->Resolve(context, this->ProduceRVal()).FromJust();
}

template <class GDALType> void GDALPromiseWorker<GDALType>::HandleErrorCallback() {
  Nan::HandleScope scope;
  v8::Local<v8::Context> context = Nan::New(*context_handle);
  v8::Local<v8::Promise::Resolver> resolver = Nan::New(*resolver_handle);
  resolver->Reject(context, Nan::Error(this->ErrorMessage())).FromJust();
}

template <class GDALType> GDALPromiseWorker<GDALType>::~GDALPromiseWorker() {
  context_handle->Reset();
  resolver_handle->Reset();
  delete context_handle;
  delete resolver_handle;
}

// This the basic unit of the GDALAsyncable framework
// GDALAsyncableJob is a GDAL job consisting of a main
// lambda that calls GDAL and rval lambda that transforms
// the return value
// It handles persistence of the referenced objects and
// can be executed both synchronously and asynchronously
//
// GDALType is the intermediary type that will be carried
// across the context boundaries
//
// The caller must ensure that all the lambdas can be executed in
// another thread:
// * no capturing of automatic variables (C++ memory management)
// * no referencing of JS-visible objects in main() (V8 memory management)
// * protecting all JS-visible objects from the GC by calling persist() (V8 MM)
// * locking all GDALDatasets (GDAL limitation)
//
// If a GDALDataset is locked, but not persisted, the GC could still
// try to free it - in this case it will stop the JS world and then it will wait
// on the Dataset lock in PtrManager::dispose() blocking the event loop - the situation
// is safe but it is still best if it is avoided (see the comments in ptr_manager.cpp)

template <class GDALType> class GDALAsyncableJob {
    public:
  typedef std::function<GDALType(const GDALExecutionProgress &)> GDALMainFunc;
  typedef std::function<v8::Local<v8::Value>(const GDALType, const GetFromPersistentFunc &)> GDALRValFunc;
  // This is the lambda that produces the <GDALType> object
  GDALMainFunc main;
  // This is the lambda that produces the JS return object from the <GDALType> object
  GDALRValFunc rval;
  Nan::Callback *progress;

  GDALAsyncableJob(long ds_uid) : main(), rval(), progress(nullptr), persistent(), ds_uids({ds_uid}), autoIndex(0) {};
  GDALAsyncableJob(std::vector<long> ds_uids)
    : main(), rval(), progress(nullptr), persistent(), ds_uids(ds_uids), autoIndex(0) {};

  inline void persist(const std::string &key, const v8::Local<v8::Object> &obj) {
    persistent[key] = obj;
  }

  inline void persist(const v8::Local<v8::Object> &obj) {
    persistent[std::to_string(autoIndex++)] = obj;
  }

  inline void persist(const v8::Local<v8::Object> &obj1, const v8::Local<v8::Object> &obj2) {
    persist(obj1);
    persist(obj2);
  }

  inline void persist(const std::vector<v8::Local<v8::Object>> &objs) {
    for (auto const &i : objs) persist(i);
  }

  void run(const Nan::FunctionCallbackInfo<v8::Value> &info, bool async, int cb_arg) {
    if (!info.This().IsEmpty() && info.This()->IsObject()) persist("this", info.This());
    if (async) {
      if (progress) persist("progress_cb", progress->GetFunction());
      Nan::Callback *callback;
      NODE_ARG_CB(cb_arg, "callback", callback);
      Nan::AsyncQueueWorker(new GDALCallbackWorker<GDALType>(callback, progress, main, rval, persistent, ds_uids));
      return;
    }
    try {
      GDALExecutionProgress executionProgress(new GDALSyncExecutionProgress(progress));
      AsyncGuard lock(ds_uids, eventLoopWarn);
      GDALType obj = main(executionProgress);
      // rval is the user function that will create the returned value
      // we give it a lambda that can access the persistent storage created for this operation
      info.GetReturnValue().Set(rval(obj, [this](const char *key) { return this->persistent[key]; }));
    } catch (const char *err) { Nan::ThrowError(err); }
  }

  void run(Nan::NAN_GETTER_ARGS_TYPE info, bool async) {
    if (!info.This().IsEmpty() && info.This()->IsObject()) persist("this", info.This());
    if (async) {
      auto worker = new GDALPromiseWorker<GDALType>(info, main, rval, persistent, ds_uids);
      info.GetReturnValue().Set(worker->Promise());
      Nan::AsyncQueueWorker(worker);
      return;
    }
    try {
      GDALExecutionProgress executionProgress(new GDALSyncExecutionProgress(progress));
      AsyncGuard lock(ds_uids, eventLoopWarn);
      GDALType obj = main(executionProgress);
      // rval is the user function that will create the returned value
      // we give it a lambda that can access the persistent storage created for this operation
      info.GetReturnValue().Set(rval(obj, [this](const char *key) { return this->persistent[key]; }));
    } catch (const char *err) { Nan::ThrowError(err); }
  }

    private:
  std::map<std::string, v8::Local<v8::Object>> persistent;
  const std::vector<long> ds_uids;
  unsigned autoIndex;
};
} // namespace node_gdal
#endif
