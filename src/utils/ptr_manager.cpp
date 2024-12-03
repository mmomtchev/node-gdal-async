#include "ptr_manager.hpp"
#include "../gdal_dataset.hpp"
#include "../gdal_group.hpp"
#include "../gdal_mdarray.hpp"
#include "../gdal_dimension.hpp"
#include "../gdal_attribute.hpp"
#include "../gdal_layer.hpp"
#include "../gdal_rasterband.hpp"

#include <sstream>
#include <thread>

// Here used to be dragons, but now there is a shopping mall
//
// This is the object store, a singleton
//
// It serves 2 purposes:
//
// First, it keeps track of created objects so that they can be reused
// The point of this mechanism is that it returns a reference to the same object
// for two successive calls of `ds.bands.get(1)` for example
// For this use, the V8 objects are indexed with the pointer to the GDAL
// base object
// uids won't work for this use
//
// Second, it is allocated entirely outside of the V8 memory management and the GC
// Thus, it is accessible from the worker threads
// The async locks live here
// For this use, the V8 objects are indexed with numeric uids
// ptrs won't be safe for this use

// Async lock semantics:
//
// * There is one global master lock, all operations on the ObjectStore structures
//   must acquire it
// * There is one async lock per dataset and it is a semaphore because it needs
//   to support being acquired by the main thread and being unlocked in a worker
// * Sync operations can sleep on the semaphore as only the main thread can
//   delete a semaphore
// * Async operations should sleep on the master_sleep condition as semaphores
//   can be deleted by the main thread (but this would also mean that someone forgot
//   to protect his object from the GC)
//   - Failing to protect an object from the GC means that GC could potentially sleep
//   on a semaphore when disposing
//   - GC that sleeps -> event loop that does run
// * Acquiring a semaphore requires acquiring the master look otherwise the
//   semaphore may disappear
// * When waking from master_sleep, the presence of the semaphore (isAlive) must be
//   checked again
// * When unlocking a semaphore, the master_sleep condition is to be broadcasted
// * Never acquire the master lock while holding a semaphore (deadlock avoidance)
// * Multiple datasets are to be locked with .lockDataset which sorts locks (deadlock avoidance)
// * Never sleep with the master lock held (performance)
// * All objects carry the dataset uid
// * All GDAL operations on a dependant object require locking the parent dataset
// - This is best accomplished though .lockDataset
// * Dependant Datasets share a semaphore with their parent through a shared_ptr

namespace node_gdal {

// Because of severe bugs linked to C++14 template variables in MSVC
// these two must be here and must have file scope
// MSVC throws an Internal Compiler Error when specializing templated variables
// and the linker doesn't use the right address when processing exported symbols
template <typename GDALPTR> using UidMap = map<long, shared_ptr<ObjectStoreItem<GDALPTR>>>;
template <typename GDALPTR> using PtrMap = map<GDALPTR, shared_ptr<ObjectStoreItem<GDALPTR>>>;
template <typename GDALPTR> static UidMap<GDALPTR> uidMap;
template <typename GDALPTR> static PtrMap<GDALPTR> ptrMap;

void uv_sem_deleter::operator()(uv_sem_t *p) {
  uv_sem_destroy(p);
  delete p;
}

class uv_scoped_mutex {
    public:
  inline uv_scoped_mutex(uv_mutex_t *lock) : lock(lock) {
    uv_mutex_lock(lock);
  }
  inline ~uv_scoped_mutex() {
    uv_mutex_unlock(lock);
  }

    private:
  uv_mutex_t *lock;
};

ObjectStore::ObjectStore() : uid(1) {
#ifdef PTHREAD_MUTEX_DEBUG
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&master_lock, &attr);
#else
  uv_mutex_init(&master_lock);
#endif
  uv_cond_init(&master_sleep);
}

ObjectStore::~ObjectStore() {
  uv_mutex_destroy(&master_lock);
  uv_cond_destroy(&master_sleep);
}

bool ObjectStore::isAlive(long uid) {
  if (uid == 0) return true;
  return uidMap<GDALRasterBand *>.count(uid) > 0 || uidMap<OGRLayer *>.count(uid) > 0 ||
    uidMap<GDALDataset *>.count(uid) > 0 || uidMap<GDALColorTable *>.count(uid)
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
    || uidMap<shared_ptr<GDALGroup>>.count(uid) > 0 || uidMap<shared_ptr<GDALMDArray>>.count(uid) > 0 ||
    uidMap<shared_ptr<GDALDimension>>.count(uid) > 0 || uidMap<shared_ptr<GDALAttribute>>.count(uid) > 0
#endif
    ;
}

static inline void sortUnique(vector<long> &uids) {
  // Avoid deadlocks
  sort(uids.begin(), uids.end());
  // Eliminate dupes and 0s
  uids.erase(unique(uids.begin(), uids.end()), uids.end());
  if (uids.front() == 0) uids.erase(uids.begin());
}

/*
 * Lock a Dataset by uid, throws when the Dataset has been destroyed.
 * There is a single global condition which allows to avoid active spinning.
 * Every time a Dataset releases a lock it must broadcast the condition.
 */
AsyncLock ObjectStore::lockDataset(long uid) {
  if (uid == 0) return nullptr;
  uv_scoped_mutex lock(&master_lock);
  while (true) {
    auto parent = uidMap<GDALDataset *>.find(uid);
    if (parent == uidMap<GDALDataset *>.end()) { throw "Parent Dataset object has already been destroyed"; }
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 10)
    // Do not lock thread-safe datasets
    if (parent->second->ptr->IsThreadSafe(GDAL_OF_RASTER)) { return nullptr; }
#endif
    int r = uv_sem_trywait(parent->second->async_lock.get());
    if (r == 0) { return parent->second->async_lock; }
    uv_cond_wait(&master_sleep, &master_lock);
  }
}

/*
 * Lock several Datasets by uid avoiding deadlocks, same semantics as the previous one.
 */
vector<AsyncLock> ObjectStore::lockDatasets(vector<long> uids) {
  // There is lots of copying around here but these vectors are never longer than 3 elements
  sortUnique(uids);
  if (uids.size() == 0) return {};
  uv_scoped_mutex lock(&master_lock);
  while (true) {
    try {
      bool locked;
      vector<AsyncLock> locks = _tryLockDatasets(uids, locked);
      if (locked) { return locks; }
    } catch (const char *msg) { throw msg; }
    uv_cond_wait(&master_sleep, &master_lock);
  }
}

/*
 * Acquire the lock only if it is free, do not block.
 */
AsyncLock ObjectStore::tryLockDataset(long uid, bool &result) {
  if (uid == 0) {
    result = true;
    return nullptr;
  }
  uv_scoped_mutex lock(&master_lock);
  auto parent = uidMap<GDALDataset *>.find(uid);
  if (parent == uidMap<GDALDataset *>.end()) { throw "Parent Dataset object has already been destroyed"; }
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 10)
  if (parent->second->ptr->IsThreadSafe(GDAL_OF_RASTER)) {
    result = true;
    return nullptr;
  }
#endif
  int r = uv_sem_trywait(parent->second->async_lock.get());
  if (r == 0) {
    result = true;
    return parent->second->async_lock;
  }
  result = false;
  return nullptr;
}

vector<AsyncLock> ObjectStore::_tryLockDatasets(vector<long> uids, bool &result) {
  vector<AsyncLock> locks;
  for (long uid : uids) {
    auto parent = uidMap<GDALDataset *>.find(uid);
    if (parent == uidMap<GDALDataset *>.end()) { throw "Parent Dataset object has already been destroyed"; }
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 10)
    // Skip locking thread-safe datasets
    if (parent->second->ptr->IsThreadSafe(GDAL_OF_RASTER)) continue;
#endif
    locks.push_back(parent->second->async_lock);
  }
  vector<AsyncLock> locked;
  int r = 0;
  for (AsyncLock &async_lock : locks) {
    r = uv_sem_trywait(async_lock.get());
    if (r == 0) {
      locked.push_back(async_lock);
    } else {
      // We failed acquiring one of the locks =>
      // free all acquired locks and start a new cycle
      for (AsyncLock &lock : locked) { uv_sem_post(lock.get()); }
      uv_cond_broadcast(&master_sleep);
      break;
    }
  }
  if (r == 0) {
    result = true;
    return locks;
  }
  result = false;
  return {};
}

/*
 * Try to acquire several locks avoiding deadlocks without blocking.
 */
vector<AsyncLock> ObjectStore::tryLockDatasets(vector<long> uids, bool &result) {
  // There is lots of copying around here but these vectors are never longer than 3 elements
  sortUnique(uids);
  if (uids.size() == 0) return {};
  uv_scoped_mutex lock(&master_lock);
  return _tryLockDatasets(uids, result);
}

// The basic unit of the ObjectStore is the ObjectStoreItem<GDALPTR>
// There is only one such item per GDALPTR
// There are two shared_ptr to it:
// * one in the uidMap
// * one in the ptrMap
// There is alo a reference to the Persistent in Nan::ObjectWrap
// This is a Weak Persistent and Nan::ObjectWrap will call the C++ destructor
// when the GC calls the Weak Callback which will trigger the dispose functions below

template <typename GDALPTR> ObjectStoreItem<GDALPTR>::ObjectStoreItem(Nan::Persistent<Object> &obj) : obj(obj) {
}
ObjectStoreItem<GDALDataset *>::ObjectStoreItem(Nan::Persistent<Object> &obj) : obj(obj) {
}
ObjectStoreItem<OGRLayer *>::ObjectStoreItem(Nan::Persistent<Object> &obj) : obj(obj) {
}

template <typename GDALPTR> long ObjectStore::add(GDALPTR ptr, Nan::Persistent<Object> &obj, long parent_uid) {
  uv_scoped_mutex lock(&master_lock);
  shared_ptr<ObjectStoreItem<GDALPTR>> item(new ObjectStoreItem<GDALPTR>(obj));
  item->uid = uid++;
  if (parent_uid) {
    shared_ptr<ObjectStoreItem<GDALDataset *>> parent = uidMap<GDALDataset *>[parent_uid];
    item->parent = parent;
    parent->children.push_back(item->uid);
  } else {
    item->parent = nullptr;
  }
  item->ptr = ptr;

  uidMap<GDALPTR>[item->uid] = item;
  ptrMap<GDALPTR>[ptr] = item;
  LOG("ObjectStore: Add %s [%ld]<[%ld]", typeid(ptr).name(), item->uid, parent_uid);
  return item->uid;
}

// Creating a Layer object is a special case - it can contain SQL results
long ObjectStore::add(OGRLayer *ptr, Nan::Persistent<Object> &obj, long parent_uid, bool is_result_set) {
  long uid = ObjectStore::add<OGRLayer *>(ptr, obj, parent_uid);
  uidMap<OGRLayer *>[uid] -> is_result_set = is_result_set;
  return uid;
}

// Creating a Dataset object is a special case
// It contains a lock (unless it is a dependant Dataset)
long ObjectStore::add(GDALDataset *ptr, Nan::Persistent<Object> &obj, long parent_uid) {
  long uid = ObjectStore::add<GDALDataset *>(ptr, obj, parent_uid);
  if (parent_uid == 0) {
    uidMap<GDALDataset *>[uid] -> async_lock = shared_ptr<uv_sem_t>(new uv_sem_t(), uv_sem_deleter());
    uv_sem_init(uidMap<GDALDataset *>[uid] -> async_lock.get(), 1);
  } else {
    uidMap<GDALDataset *>[uid] -> async_lock = uidMap<GDALDataset *>[parent_uid] -> async_lock;
  }
  return uid;
}

template <typename GDALPTR> bool ObjectStore::has(GDALPTR ptr) {
  uv_scoped_mutex lock(&master_lock);
  return ptrMap<GDALPTR>.count(ptr) > 0;
}
template <typename GDALPTR> Local<Object> ObjectStore::get(GDALPTR ptr) {
  uv_scoped_mutex lock(&master_lock);
  Nan::EscapableHandleScope scope;
  return scope.Escape(Nan::New(ptrMap<GDALPTR>[ptr] -> obj));
}
template <typename GDALPTR> Local<Object> ObjectStore::get(long uid) {
  uv_scoped_mutex lock(&master_lock);
  Nan::EscapableHandleScope scope;
  return scope.Escape(Nan::New(uidMap<GDALPTR>[uid] -> obj));
}

// Explicit instantiation:
// * allows calling object_store.add without <>
// * makes sure that this class template won't be accidentally instantiated with an unsupported type
template long ObjectStore::add(GDALDriver *, Nan::Persistent<Object> &, long);
template long ObjectStore::add(GDALRasterBand *, Nan::Persistent<Object> &, long);
template long ObjectStore::add(OGRSpatialReference *, Nan::Persistent<Object> &, long);
template long ObjectStore::add(GDALColorTable *, Nan::Persistent<Object> &, long);
template bool ObjectStore::has(GDALDriver *);
template bool ObjectStore::has(GDALDataset *);
template bool ObjectStore::has(OGRLayer *);
template bool ObjectStore::has(GDALRasterBand *);
template bool ObjectStore::has(OGRSpatialReference *);
template bool ObjectStore::has(GDALColorTable *);
template Local<Object> ObjectStore::get(GDALDriver *);
template Local<Object> ObjectStore::get(GDALDataset *);
template Local<Object> ObjectStore::get(OGRLayer *);
template Local<Object> ObjectStore::get(GDALRasterBand *);
template Local<Object> ObjectStore::get(OGRSpatialReference *);
template Local<Object> ObjectStore::get(GDALColorTable *);
template Local<Object> ObjectStore::get<GDALDataset *>(long uid);
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
template long ObjectStore::add(shared_ptr<GDALAttribute>, Nan::Persistent<Object> &, long);
template long ObjectStore::add(shared_ptr<GDALDimension>, Nan::Persistent<Object> &, long);
template long ObjectStore::add(shared_ptr<GDALGroup>, Nan::Persistent<Object> &, long);
template long ObjectStore::add(shared_ptr<GDALMDArray>, Nan::Persistent<Object> &, long);
template bool ObjectStore::has(shared_ptr<GDALAttribute>);
template bool ObjectStore::has(shared_ptr<GDALDimension>);
template bool ObjectStore::has(shared_ptr<GDALGroup>);
template bool ObjectStore::has(shared_ptr<GDALMDArray>);
template Local<Object> ObjectStore::get(shared_ptr<GDALAttribute>);
template Local<Object> ObjectStore::get(shared_ptr<GDALDimension>);
template Local<Object> ObjectStore::get(shared_ptr<GDALGroup>);
template Local<Object> ObjectStore::get(shared_ptr<GDALMDArray>);
#endif

const char warningGCBug[] =
  "Sleeping on semaphore in garbage collector, this is a bug in gdal-async, event loop blocked for ";
const char warningManualClose[] =
  "Closing a dataset while background async operations are still running, event loop blocked for ";
static inline void uv_sem_wait_with_warning(uv_sem_t *sem, const char *warning) {
  if (uv_sem_trywait(sem) != 0) { MEASURE_EXECUTION_TIME(warning, uv_sem_wait(sem)); }
}

// dispose is called by the C++ destructor which is called by Nan::ObjectWrap
// which is called by the WeakCallback of the GC on its Persistent
// This is the same Persistent that has a reference in the ObjectStoreItem
// Removes the object and all its children for the ObjectStore

// Disposing a Dataset is a special case - it has children (called with the master lock held)
template <> void ObjectStore::dispose(shared_ptr<ObjectStoreItem<GDALDataset *>> item, bool manual) {
  uv_sem_wait_with_warning(
    item->async_lock.get(), manual ? (eventLoopWarn ? warningManualClose : nullptr) : warningGCBug);
  uidMap<GDALDataset *>.erase(item->uid);
  ptrMap<GDALDataset *>.erase(item->ptr);
  if (item->parent != nullptr) item->parent->children.remove(item->uid);

  uv_sem_post(item->async_lock.get());
  uv_cond_broadcast(&master_sleep);
  // Beyond this point the Dataset is not alive anymore ->
  // anyone who was waiting for this semaphore should fail

  // All the children are removed from the ObjectStore
  // but the Node/V8 objects still exist
  // They can be deleted only by the GC
  // When this happens, they will skip this in do_dispose
  while (!item->children.empty()) { do_dispose(item->children.back()); }

  if (item->ptr) {
    LOG("Closing GDALDataset %ld [%p]", item->uid, item->ptr);
    GDALClose(item->ptr);
    item->ptr = nullptr;
  }
}

const char warningSQL[] =
  "Sleeping on semaphore in garbage collector while destroying an SQL results layers, this is a known issue in gdal-async, event loop blocked for ";
// Closing a Layer is a special case - it can contain SQL results
// This is the only case where we could sleep in the GC:
//   A Dataset has multiple layers, one of them is an SQL results layer
//   An asynchronous operation is running on one of the other layers
//   The GC decides it is time to reclaim the SQL results
template <> void ObjectStore::dispose(shared_ptr<ObjectStoreItem<OGRLayer *>> item, bool) {
  ptrMap<OGRLayer *>.erase(item->ptr);
  uidMap<OGRLayer *>.erase(item->uid);
  if (item->parent != nullptr) { item->parent->children.remove(item->uid); }
  if (item->is_result_set) {
    LOG("Closing OGRLayer with SQL results [%ld] [%p]", uid, item->ptr);
    if (item->parent) {
      uv_sem_wait_with_warning(item->parent->async_lock.get(), warningSQL);
      GDALDataset *parent_ds = item->parent->ptr;
      parent_ds->ReleaseResultSet(item->ptr);
      uv_sem_post(item->parent->async_lock.get());
      uv_cond_broadcast(&object_store.master_sleep);
    }
  }
}

// Generic disposal (called with the master lock held)
template <typename GDALPTR> void ObjectStore::dispose(shared_ptr<ObjectStoreItem<GDALPTR>> item, bool) {
  ptrMap<GDALPTR>.erase(item->ptr);
  uidMap<GDALPTR>.erase(item->uid);
  if (item->parent != nullptr) { item->parent->children.remove(item->uid); }
}

// Called from the C++ destructor
void ObjectStore::dispose(long uid, bool manual) {
  LOG("ObjectStore: Dispose [%ld]", uid);
  uv_scoped_mutex lock(&master_lock);
  do_dispose(uid, manual);
}

// The locked section of the above function
void ObjectStore::do_dispose(long uid, bool manual) {
  if (uidMap<GDALDataset *>.count(uid))
    dispose(uidMap<GDALDataset *>[uid], manual);
  else if (uidMap<OGRLayer *>.count(uid))
    dispose(uidMap<OGRLayer *>[uid], manual);
  else if (uidMap<GDALRasterBand *>.count(uid))
    dispose(uidMap<GDALRasterBand *>[uid], manual);
  else if (uidMap<GDALDriver *>.count(uid))
    dispose(uidMap<GDALDriver *>[uid], manual);
  else if (uidMap<OGRSpatialReference *>.count(uid))
    dispose(uidMap<OGRSpatialReference *>[uid], manual);
  else if (uidMap<GDALColorTable *>.count(uid))
    dispose(uidMap<GDALColorTable *>[uid], manual);
#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)
  else if (uidMap<shared_ptr<GDALGroup>>.count(uid))
    dispose(uidMap<shared_ptr<GDALGroup>>[uid], manual);
  else if (uidMap<shared_ptr<GDALMDArray>>.count(uid))
    dispose(uidMap<shared_ptr<GDALMDArray>>[uid], manual);
  else if (uidMap<shared_ptr<GDALDimension>>.count(uid))
    dispose(uidMap<shared_ptr<GDALDimension>>[uid], manual);
  else if (uidMap<shared_ptr<GDALAttribute>>.count(uid))
    dispose(uidMap<shared_ptr<GDALAttribute>>[uid], manual);
#endif
}

// Closes still open Datasets on process exit
// Called on the main thread after the event loop has exited
void ObjectStore::cleanup() {
  // This unusual loop is needed since dispose deletes elements from the map
  while (uidMap<GDALDataset *>.size() > 0) { dispose(uidMap<GDALDataset *>.cbegin()->second->uid, true); }
}

} // namespace node_gdal
