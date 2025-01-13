#include "async.hpp"

namespace node_gdal {

std::thread::id mainV8ThreadId;

// *message coming from GDAL points to a statically allocated buffer
GDALProgressInfo::GDALProgressInfo(double complete, const char *message) : complete(complete), message(message) {
}

GDALProgressInfo::GDALProgressInfo(const GDALProgressInfo &o) : complete(o.complete), message(o.message) {
}

GDALProgressInfo::GDALProgressInfo() : complete(0), message(nullptr) {
}

// This is the GDAL form of the progress callback trampoline
// It can be invoked both in the main thread (in sync mode) or in auxillary thread (in async mode)
// It is essentially a gateway between the GDAL world and Node.js/V8 world
int ProgressTrampoline(double dfComplete, const char *pszMessage, void *pProgressArg) {
  GDALExecutionProgress *context = (GDALExecutionProgress *)pProgressArg;
  // The dispatcher in async.hpp will delete it
  GDALProgressInfo *info = new GDALProgressInfo(dfComplete, pszMessage);
  // Go to the dispatcher
  context->Send(info);
  return 1;
}

// From async.hpp:
// typedef Nan::AsyncProgressWorkerBase<GDALProgressInfo> GDALAsyncProgressWorker;
// typedef GDALAsyncProgressWorker::ExecutionProgress GDALAsyncExecutionProgress;
// GDALAsyncExecutionProgress is an instance of a NAN templated class, in this case
// the AsyncWorker is the final owner of the progress_callback
GDALExecutionProgress::GDALExecutionProgress(const GDALAsyncExecutionProgress *async) : async(async), sync(nullptr) {
}
GDALExecutionProgress::GDALExecutionProgress(const GDALSyncExecutionProgress *sync) : async(nullptr), sync(sync) {
}

GDALExecutionProgress::~GDALExecutionProgress() {
  // The GDALAsyncExecutionProgress is owned by Node.js
  if (sync) delete sync;
}

// sync/async dispatcher
void GDALExecutionProgress::Send(GDALProgressInfo *info) const {
  auto infoHolder = std::unique_ptr<GDALProgressInfo>(info);
  // async mode -> we are in an aux thread, we can't go back to JS
  // we must enqueue a job on the event loop and wait for the JS world to stop
  // the enqueuing is in Nan::AsyncWorker, then once the JS world is not running
  // AsyncWorker::HandleProgressCallback will get invoked on the main thread
  if (async) async->Send(info, 1);
  // sync mode -> the JS world is not running, we can go back directly
  // this code is below
  if (sync) sync->Send(info);
}

// This is the sync execution context, it is the final owner of the progress_callback
GDALSyncExecutionProgress::GDALSyncExecutionProgress(Nan::Callback *cb) : progress_callback(cb) {};
GDALSyncExecutionProgress::~GDALSyncExecutionProgress() {
  delete progress_callback;
};

// Going back to JS in sync mode
void GDALSyncExecutionProgress::Send(GDALProgressInfo *info) const {
  Nan::HandleScope scope;
  v8::Local<v8::Value> argv[] = {Nan::New<Number>(info->complete), SafeString::New(info->message)};
  Nan::TryCatch try_catch;
  Nan::Call(progress_callback->GetFunction(), Nan::GetCurrentContext()->Global(), 2, argv);
  if (try_catch.HasCaught()) throw "sync progress callback exception";
}

} // namespace node_gdal
