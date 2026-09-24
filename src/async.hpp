#ifndef __NODE_GDAL_ASYNC_WORKER_H__
#define __NODE_GDAL_ASYNC_WORKER_H__

#include <thread>
#include <cstdint>
#include <functional>
#include <chrono>
#include "napi-wrapper.h"
#include "gdal_common.hpp"

namespace node_gdal {

// The id of the main V8 thread
extern std::thread::id mainV8ThreadId;

// Set by the env cleanup hook: from then on the JS world may not be touched
extern bool gdalShuttingDown;

// This generates method definitions for 2 methods: sync and async version and a hidden common block
#define GDAL_ASYNCABLE_DEFINE(method)                                                                                  \
  NAPI_METHOD(method) {                                                                                                 \
    return method##_do(info, false);                                                                                   \
  }                                                                                                                    \
  NAPI_METHOD(method##Async) {                                                                                          \
    return method##_do(info, true);                                                                                    \
  }                                                                                                                    \
  Napi::Value method##_do(const Napi::CallbackInfo &info, bool async)

// This generates getter definitions for 2 getters: sync and async version and a hidden common block
#define GDAL_ASYNCABLE_GETTER_DEFINE(method)                                                                           \
  NAPI_GETTER(method) {                                                                                                 \
    return method##_do(info, false);                                                                                   \
  }                                                                                                                    \
  NAPI_GETTER(method##Async) {                                                                                          \
    return method##_do(info, true);                                                                                    \
  }                                                                                                                    \
  Napi::Value method##_do(const Napi::CallbackInfo &info, bool async)

// This generates method declarations for 2 methods: sync and async version and a hidden common block
#define GDAL_ASYNCABLE_DECLARE(method)                                                                                 \
  static NAPI_METHOD(method);                                                                                           \
  static NAPI_METHOD(method##Async);                                                                                    \
  static Napi::Value method##_do(const Napi::CallbackInfo &info, bool async)

// This generates getter declarations for 2 getters: sync and async version and a hidden common block
#define GDAL_ASYNCABLE_GETTER_DECLARE(method)                                                                          \
  static NAPI_GETTER(method);                                                                                           \
  static NAPI_GETTER(method##Async);                                                                                    \
  static Napi::Value method##_do(const Napi::CallbackInfo &info, bool async)

#define GDAL_ASYNCABLE_GLOBAL(method)                                                                                  \
  Napi::Value method(const Napi::CallbackInfo &info);                                                                  \
  Napi::Value method##Async(const Napi::CallbackInfo &info);                                                           \
  Napi::Value method##_do(const Napi::CallbackInfo &info, bool async)

#define GDAL_ASYNCABLE_TEMPLATE(method)                                                                                \
  static NAPI_METHOD(method) {                                                                                          \
    return method##_do(info, false);                                                                                   \
  }                                                                                                                    \
  static NAPI_METHOD(method##Async) {                                                                                   \
    return method##_do(info, true);                                                                                    \
  }                                                                                                                    \
  static Napi::Value method##_do(const Napi::CallbackInfo &info, bool async)

#define GDAL_ISASYNC async

// Handle locking (used only for sync methods)
#define GDAL_LOCK_PARENT(p)                                                                                            \
  AsyncGuard lock;                                                                                                     \
  try {                                                                                                                \
    lock.acquire((p)->parent_uid);                                                                                     \
  } catch (const char *err) {                                                                                          \
    Napi::Error::New(info.Env(), err).ThrowAsJavaScriptException();                                                    \
    return info.Env().Undefined();                                                                                     \
  } catch (const std::exception &err) {                                                                                \
    Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();                                             \
    return info.Env().Undefined();                                                                                     \
  }

// GDAL_LOCK_PARENT returns a value on error, so a setter (void) needs this
#define GDAL_LOCK_PARENT_VOID(p)                                                                                       \
  AsyncGuard lock;                                                                                                     \
  try {                                                                                                                \
    lock.acquire((p)->parent_uid);                                                                                     \
  } catch (const char *err) {                                                                                          \
    Napi::Error::New(info.Env(), err).ThrowAsJavaScriptException();                                                    \
    return;                                                                                                            \
  } catch (const std::exception &err) {                                                                                \
    Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();                                             \
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

// This is trivially copyable so that it can be queued across threads
struct GDALProgressInfo {
  double complete;
  const char *message;

  GDALProgressInfo(const GDALProgressInfo &o);
  GDALProgressInfo(double, const char *);
  GDALProgressInfo();
  ~GDALProgressInfo() = default;
};

class GDALSyncExecutionProgress {
  Napi::FunctionReference *progress_callback;

  GDALSyncExecutionProgress() = delete;

    public:
  GDALSyncExecutionProgress(Napi::FunctionReference *);
  ~GDALSyncExecutionProgress();
  void Send(GDALProgressInfo *) const;
};

typedef std::function<Napi::Value(const char *)> GetFromPersistentFunc;
typedef Napi::AsyncProgressQueueWorker<GDALProgressInfo> GDALAsyncProgressWorker;
typedef GDALAsyncProgressWorker::ExecutionProgress GDALAsyncExecutionProgress;

// This an ExecutionContext that works both with Node.js' async ExecutionProgress when in async mode
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
  typedef std::function<Napi::Value(const GDALType, const GetFromPersistentFunc &)> GDALRValFunc;

    protected:
  Napi::FunctionReference *progressCallback;
  const GDALMainFunc doit;
  const GDALRValFunc rval;
  const std::vector<long> ds_uids;
  GDALType raw;

  // Napi::AsyncWorker has no inheritable persistent store of its own (the old
  // binding had SaveToPersistent/GetFromPersistent), so the worker keeps the referenced
  // objects alive itself. Both creation and release happen on the main thread.
  std::map<std::string, Napi::Reference<Napi::Object>> persistent;

  inline void SaveToPersistent(const std::string &key, const Napi::Object &value) {
    persistent.emplace(key, Napi::Persistent(value));
  }

  inline Napi::Value GetFromPersistent(const char *key) {
    auto i = persistent.find(key);
    return i != persistent.end() ? i->second.Value() : Napi::Value();
  }

  inline void RestoreObjects(const std::map<std::string, Napi::Object> &objects) {
    for (auto i = objects.begin(); i != objects.end(); i++) SaveToPersistent(i->first, i->second);
    for (auto i = ds_uids.begin(); i != ds_uids.end(); i++)
      if (*i != 0) SaveToPersistent("ds" + std::to_string(*i), object_store.get<GDALDataset *>(*i));
  }

    public:
  GDALAsyncWorker(
    const Napi::Function &resultCallback,
    Napi::FunctionReference *progressCallback,
    const GDALMainFunc &doit,
    const GDALRValFunc &rval,
    const std::map<std::string, Napi::Object> &objects,
    const std::vector<long> &ds_uids);

  GDALAsyncWorker(
    Napi::Env env,
    Napi::FunctionReference *progressCallback,
    const GDALMainFunc &doit,
    const GDALRValFunc &rval,
    const std::map<std::string, Napi::Object> &objects,
    const std::vector<long> &ds_uids);

  ~GDALAsyncWorker();

  void Execute(const ExecutionProgress &progress) override;
  Napi::Value ProduceRVal();
  void OnProgress(const GDALProgressInfo *data, size_t count) override;
};

template <class GDALType>
GDALAsyncWorker<GDALType>::GDALAsyncWorker(
  const Napi::Function &resultCallback,
  Napi::FunctionReference *progressCallback,
  const GDALMainFunc &doit,
  const GDALRValFunc &rval,
  const std::map<std::string, Napi::Object> &objects,
  const std::vector<long> &ds_uids)
  : GDALAsyncProgressWorker(resultCallback, "node-gdal:GDALAsyncWorker"),
    progressCallback(progressCallback),
    // These members are not references! These functions must be copied
    // as they will be executed in async context!
    doit(doit),
    rval(rval),
    ds_uids(ds_uids) {
  // Main thread with the JS world running
  // Turn the JS handles collected by the job into persistent references
  RestoreObjects(objects);
}

template <class GDALType>
GDALAsyncWorker<GDALType>::GDALAsyncWorker(
  Napi::Env env,
  Napi::FunctionReference *progressCallback,
  const GDALMainFunc &doit,
  const GDALRValFunc &rval,
  const std::map<std::string, Napi::Object> &objects,
  const std::vector<long> &ds_uids)
  : GDALAsyncProgressWorker(env, "node-gdal:GDALAsyncWorker"),
    progressCallback(progressCallback),
    doit(doit),
    rval(rval),
    ds_uids(ds_uids) {
  RestoreObjects(objects);
}

template <class GDALType> Napi::Value GDALAsyncWorker<GDALType>::ProduceRVal() {
  if (gdalShuttingDown) return Napi::Value();
  return rval(raw, [this](const char *key) { return this->GetFromPersistent(key); });
}

template <class GDALType> void GDALAsyncWorker<GDALType>::Execute(const ExecutionProgress &progress) {
  // Aux thread with the JS world running
  // V8 objects are not acessible here
  try {
    GDALExecutionProgress executionProgress(&progress);
    AsyncGuard lock(ds_uids);
    raw = doit(executionProgress);
  } catch (const char *err) {
    // node-addon-api decides success from `_error.size() == 0`, so an empty
    // message would be taken for a success and the uninitialized `raw` would be
    // converted (the GDAL calls that fail without setting a CPL error report an
    // empty message)
    this->SetError(err != nullptr && *err != '\0' ? err : "Operation failed");
  } catch (const std::exception &err) {
    const char *what = err.what();
    this->SetError(what != nullptr && *what != '\0' ? what : "Operation failed");
  }
}

template <class GDALType> GDALAsyncWorker<GDALType>::~GDALAsyncWorker() {
  // FunctionReference has its destructor suppressed by the argument macros,
  // Reset() releases the reference explicitly
  if (progressCallback != nullptr) {
    progressCallback->Reset();
    delete progressCallback;
  }
}

template <class GDALType>
void GDALAsyncWorker<GDALType>::OnProgress(const GDALProgressInfo *data, size_t count) {
  if (progressCallback == nullptr) return;
  // Back to the main thread with the JS world not running
  // A mutex-protected pop in the calling function can sometimes produce a spurious call
  // with no data, handle gracefully this case -> no need to call JS if there is no data to deliver
  if (data == nullptr || count == 0) return;
  // Receiving more than one callback invocation at the same time is also possible
  // Send only the last one to JS
  const GDALProgressInfo *to_send = data + (count - 1);
  std::vector<napi_value> argv;
  argv.push_back(Napi::Number::New(Env(), to_send->complete));
  argv.push_back(SafeString::New(Env(), to_send->message));
  try {
    progressCallback->Value().Call(argv);
  } catch (const Napi::Error &) {
    this->SetError("async progress callback exception");
  }
}

// This async worker calls a callback
// The result is delivered as (null, rval) or (error), matching the old behaviour
template <class GDALType> class GDALCallbackWorker : public GDALAsyncWorker<GDALType> {
    public:
  using GDALAsyncWorker<GDALType>::GDALAsyncWorker;

    protected:
  std::vector<napi_value> GetResult(Napi::Env env) override {
    return {env.Null(), this->ProduceRVal()};
  }
};

// This async worker returns a Promise
template <class GDALType> class GDALPromiseWorker : public GDALAsyncWorker<GDALType> {
    public:
  typedef typename GDALAsyncWorker<GDALType>::GDALMainFunc GDALMainFunc;
  typedef typename GDALAsyncWorker<GDALType>::GDALRValFunc GDALRValFunc;

    private:
  Napi::Promise::Deferred deferred;

    public:
  GDALPromiseWorker(
    Napi::Env env,
    const GDALMainFunc &doit,
    const GDALRValFunc &rval,
    const std::map<std::string, Napi::Object> &objects,
    const std::vector<long> &ds_uids);

  inline Napi::Value Promise() {
    return deferred.Promise();
  }

    protected:
  void OnOK() override;
  void OnError(const Napi::Error &e) override;
};

template <class GDALType>
GDALPromiseWorker<GDALType>::GDALPromiseWorker(
  Napi::Env env,
  const GDALMainFunc &doit,
  const GDALRValFunc &rval,
  const std::map<std::string, Napi::Object> &objects,
  const std::vector<long> &ds_uids)
  : GDALAsyncWorker<GDALType>(env, nullptr, doit, rval, objects, ds_uids), deferred(env) {
}

template <class GDALType> void GDALPromiseWorker<GDALType>::OnOK() {
  if (gdalShuttingDown) return;
  deferred.Resolve(this->ProduceRVal());
}

template <class GDALType> void GDALPromiseWorker<GDALType>::OnError(const Napi::Error &e) {
  if (gdalShuttingDown) return;
  deferred.Reject(e.Value());
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
  typedef std::function<Napi::Value(const GDALType, const GetFromPersistentFunc &)> GDALRValFunc;
  // This is the lambda that produces the <GDALType> object
  GDALMainFunc main;
  // This is the lambda that produces the JS return object from the <GDALType> object
  GDALRValFunc rval;
  Napi::FunctionReference *progress;

  GDALAsyncableJob(long ds_uid) : main(), rval(), progress(nullptr), persistent(), ds_uids({ds_uid}), autoIndex(0) {};
  GDALAsyncableJob(std::vector<long> ds_uids)
    : main(), rval(), progress(nullptr), persistent(), ds_uids(ds_uids), autoIndex(0) {};

  inline void persist(const std::string &key, const Napi::Object &obj) {
    persistent[key] = obj;
  }

  inline void persist(const std::string &key, const Napi::Value &v) {
    persistent[key] = v.As<Napi::Object>();
  }

  inline void persist(const Napi::Value &v) {
    persistent[std::to_string(autoIndex++)] = v.As<Napi::Object>();
  }

  inline void persist(const Napi::Object &obj1, const Napi::Object &obj2) {
    persist(obj1);
    persist(obj2);
  }

  inline void persist(const std::vector<Napi::Object> &objs) {
    for (auto const &i : objs) persist(i);
  }

  Napi::Value run(const Napi::CallbackInfo &info, bool async, int cb_arg) {
    // The rval lambdas look this up by name ("this"), so it cannot go through
    // the auto-indexed overload
    if (!info.This().IsEmpty() && info.This().IsObject()) persist("this", info.This());
    if (async) {
      if (progress) persist("progress_cb", progress->Value());
      Napi::FunctionReference *callback;
      NODE_ARG_CB(cb_arg, "callback", callback);
      // The worker makes its own reference to the callback, ours is not needed anymore
      auto *worker =
        new GDALCallbackWorker<GDALType>(callback->Value(), progress, main, rval, persistent, ds_uids);
      callback->Reset();
      delete callback;
      worker->Queue();
      return info.Env().Undefined();
    }
    try {
      GDALExecutionProgress executionProgress(new GDALSyncExecutionProgress(progress));
      AsyncGuard lock(ds_uids, eventLoopWarn);
      GDALType obj = main(executionProgress);
      // rval is the user function that will create the returned value
      // we give it a lambda that can access the persistent storage created for this operation
      return rval(obj, [this](const char *key) { return this->persistent[key]; });
    } catch (const char *err) {
      Napi::Error::New(info.Env(), err).ThrowAsJavaScriptException();
    } catch (const std::exception &err) {
      Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();
    }
    return info.Env().Undefined();
  }

  Napi::Value run(const Napi::CallbackInfo &info, bool async) {
    // See the note in the callback overload above
    if (!info.This().IsEmpty() && info.This().IsObject()) persist("this", info.This());
    if (async) {
      auto *worker = new GDALPromiseWorker<GDALType>(info.Env(), main, rval, persistent, ds_uids);
      Napi::Value promise = worker->Promise();
      worker->Queue();
      return promise;
    }
    try {
      GDALExecutionProgress executionProgress(new GDALSyncExecutionProgress(progress));
      AsyncGuard lock(ds_uids, eventLoopWarn);
      GDALType obj = main(executionProgress);
      // rval is the user function that will create the returned value
      // we give it a lambda that can access the persistent storage created for this operation
      return rval(obj, [this](const char *key) { return this->persistent[key]; });
    } catch (const char *err) {
      Napi::Error::New(info.Env(), err).ThrowAsJavaScriptException();
    } catch (const std::exception &err) {
      Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();
    }
    return info.Env().Undefined();
  }

    private:
  std::map<std::string, Napi::Object> persistent;
  const std::vector<long> ds_uids;
  unsigned autoIndex;
};
} // namespace node_gdal
#endif
