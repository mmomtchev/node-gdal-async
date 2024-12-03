
#ifndef __OBJ_STORE_H__
#define __OBJ_STORE_H__

// node
#include <node.h>

// nan
#include "../nan-wrapper.h"

// gdal
#include <gdal_priv.h>

// ogr
#include <ogrsf_frmts.h>

#include <list>
#include <map>

using namespace v8;
using namespace std;

namespace node_gdal {

typedef shared_ptr<uv_sem_t> AsyncLock;

template <typename GDALPTR> struct ObjectStoreItem {
  long uid;
  Nan::Persistent<v8::Object> &obj;
  GDALPTR ptr;
  shared_ptr<ObjectStoreItem<GDALDataset *>> parent;
  ObjectStoreItem(Nan::Persistent<Object> &obj);
};

template <> struct ObjectStoreItem<OGRLayer *> {
  long uid;
  Nan::Persistent<v8::Object> &obj;
  OGRLayer *ptr;
  shared_ptr<ObjectStoreItem<GDALDataset *>> parent;
  bool is_result_set;
  ObjectStoreItem(Nan::Persistent<Object> &obj);
};

template <> struct ObjectStoreItem<GDALDataset *> {
  long uid;
  Nan::Persistent<v8::Object> &obj;
  GDALDataset *ptr;
  shared_ptr<ObjectStoreItem<GDALDataset *>> parent;
  list<long> children;
  AsyncLock async_lock;
  ObjectStoreItem(Nan::Persistent<Object> &obj);
};

struct uv_sem_deleter {
  void operator()(uv_sem_t *p);
};

class ObjectStore {
    public:
  template <typename GDALPTR> long add(GDALPTR ptr, Nan::Persistent<Object> &obj, long parent_uid);
  long add(OGRLayer *ptr, Nan::Persistent<Object> &obj, long parent_uid, bool is_result_set);
  long add(GDALDataset *ptr, Nan::Persistent<Object> &obj, long parent_uid);

  void dispose(long uid, bool manual = false);
  bool isAlive(long uid);
  inline void lockDataset(AsyncLock lock) {
    uv_sem_wait(lock.get());
  }
  inline void unlockDataset(AsyncLock lock) {
    uv_sem_post(lock.get());
    uv_mutex_lock(&master_lock);
    uv_cond_broadcast(&master_sleep);
    uv_mutex_unlock(&master_lock);
  }
  inline void unlockDatasets(vector<AsyncLock> locks) {
    for (const AsyncLock &l : locks) uv_sem_post(l.get());
    uv_mutex_lock(&master_lock);
    uv_cond_broadcast(&master_sleep);
    uv_mutex_unlock(&master_lock);
  }
  AsyncLock lockDataset(long uid);
  vector<AsyncLock> lockDatasets(vector<long> uids);
  AsyncLock tryLockDataset(long uid, bool &result);
  vector<AsyncLock> tryLockDatasets(vector<long> uids, bool &result);

  template <typename GDALPTR> bool has(GDALPTR ptr);
  template <typename GDALPTR> Local<Object> get(GDALPTR ptr);
  template <typename GDALPTR> Local<Object> get(long uid);

  void cleanup();

  ObjectStore();
  ~ObjectStore();

    private:
  long uid;
  uv_mutex_t master_lock;
  uv_cond_t master_sleep;
  vector<AsyncLock> _tryLockDatasets(vector<long> uids, bool &result);
  template <typename GDALPTR> void dispose(shared_ptr<ObjectStoreItem<GDALPTR>> item, bool manual);
  void do_dispose(long uid, bool manual = false);
};

} // namespace node_gdal

#endif
