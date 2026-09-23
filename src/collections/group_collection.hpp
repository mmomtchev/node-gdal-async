#ifndef __NODE_GDAL_BASEGROUP_COLLECTION_H__
#define __NODE_GDAL_BASEGROUP_COLLECTION_H__

// node
#include <node.h>
#include <node_object_wrap.h>

// nan
#include "../nan-wrapper.h"

// gdal
#include <gdal_priv.h>

#include "../async.hpp"
#include "../gdal_dataset.hpp"
#include "../gdal_group.hpp"

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

using namespace v8;
using namespace node;

namespace node_gdal {

template <typename SELF, typename GDALOBJ, typename GDALPARENT, typename NODEOBJ, typename NODEPARENT>
class GroupCollection : public Nan::ObjectWrap {
    public:
  static constexpr const char *_className = "GroupCollection<abstract>";
  static void Initialize(Local<Object> target) {
    Nan::HandleScope scope;

    Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(SELF::New);
    lcons->InstanceTemplate()->SetInternalFieldCount(1);
    lcons->SetClassName(Nan::New(SELF::_className).ToLocalChecked());

    Nan::SetPrototypeMethod(lcons, "toString", toString);
    Nan__SetPrototypeAsyncableMethod(lcons, "count", count);
    Nan__SetPrototypeAsyncableMethod(lcons, "get", get);

    ATTR_DONT_ENUM(lcons, "ds", dsGetter, READ_ONLY_SETTER);
    ATTR_DONT_ENUM(lcons, "parent", parentGetter, READ_ONLY_SETTER);
    ATTR(lcons, "names", namesGetter, READ_ONLY_SETTER);

    Nan::Set(target, Nan::New(SELF::_className).ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

    SELF::constructor.Reset(lcons);
  }

  static NAN_METHOD(toString) {
    info.GetReturnValue().Set(Nan::New(SELF::_className).ToLocalChecked());
  }

  static NAN_METHOD(New) {

    if (!info.IsConstructCall()) {
      Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
      return;
    }
    if (info[0]->IsExternal()) {
      Local<External> ext = info[0].As<External>();
      void *ptr = ext->Value(V8_TYPE_TAG);
      SELF *f = static_cast<SELF *>(ptr);
      f->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
      return;
    } else {
      Nan::ThrowError("Cannot create GroupCollection directly");
      return;
    }
  }

  static Local<Value> New(Local<Value> parent, Local<Value> parent_ds) {
    Nan::EscapableHandleScope scope;

    SELF *wrapped = new SELF();

    Local<Value> ext = Nan::New<External>(wrapped);
    Local<Object> obj =
      Nan::NewInstance(Nan::GetFunction(Nan::New(SELF::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();
    Nan::SetPrivate(obj, Nan::New("parent_ds_").ToLocalChecked(), parent_ds);
    Nan::SetPrivate(obj, Nan::New("parent_").ToLocalChecked(), parent);

    return scope.Escape(obj);
  }

  static std::shared_ptr<GDALOBJ> __get(std::shared_ptr<GDALPARENT> parent, std::string const &name) {
    return nullptr;
  };
  static std::shared_ptr<GDALOBJ> __get(std::shared_ptr<GDALPARENT> parent, size_t idx) {
    return nullptr;
  };
  static std::vector<std::string> __getNames(std::shared_ptr<GDALPARENT> parent) {
    return {};
  };
  static int __count(std::shared_ptr<GDALPARENT> parent) {
    return 0;
  };

  GDAL_ASYNCABLE_TEMPLATE(get) {

    Local<Object> parent_ds =
      Nan::GetPrivate(info.This(), Nan::New("parent_ds_").ToLocalChecked()).ToLocalChecked().As<Object>();
    Local<Object> parent_obj =
      Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
    NODE_UNWRAP_CHECK(Dataset, parent_ds, ds);
    NODE_UNWRAP_CHECK(NODEPARENT, parent_obj, parent);

    std::shared_ptr<GDALPARENT> raw = parent->get();
    GDALDataset *gdal_ds = ds->get();
    std::string name = "";
    size_t idx = 0;
    NODE_ARG_STR_INT(0, "id", name, idx, isString);

    GDALAsyncableJob<std::shared_ptr<GDALOBJ>> job(ds->uid);
    job.persist(parent_obj);
    job.main = [raw, name, idx, isString](const GDALExecutionProgress &) {
      std::shared_ptr<GDALOBJ> r = nullptr;
      if (!isString)
        r = SELF::__get(raw, idx);
      else
        r = SELF::__get(raw, name);
      if (r == nullptr) throw "Invalid element";
      return r;
    };
    job.rval = [gdal_ds](std::shared_ptr<GDALOBJ> r, const GetFromPersistentFunc &) {
      return NODEOBJ::New(r, gdal_ds);
    };
    job.run(info, async, 1);
  }

  GDAL_ASYNCABLE_TEMPLATE(count) {

    Local<Object> parent_ds =
      Nan::GetPrivate(info.This(), Nan::New("parent_ds_").ToLocalChecked()).ToLocalChecked().As<Object>();
    Local<Object> parent_obj =
      Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
    NODE_UNWRAP_CHECK(Dataset, parent_ds, ds);
    NODE_UNWRAP_CHECK(NODEPARENT, parent_obj, parent);

    std::shared_ptr<GDALPARENT> raw = parent->get();

    GDALAsyncableJob<int> job(ds->uid);
    job.persist(parent_obj);
    job.main = [raw](const GDALExecutionProgress &) {
      int r = SELF::__count(raw);
      return r;
    };
    job.rval = [](int r, const GetFromPersistentFunc &) { return Nan::New<Number>(r); };
    job.run(info, async, 0);
  }

  static NAN_GETTER(namesGetter) {

    Local<Object> parent_ds =
      Nan::GetPrivate(info.This(), Nan::New("parent_ds_").ToLocalChecked()).ToLocalChecked().As<Object>();
    Dataset *ds = Nan::ObjectWrap::Unwrap<Dataset>(parent_ds);

    Local<Object> parent_obj =
      Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
    NODEPARENT *parent = Nan::ObjectWrap::Unwrap<NODEPARENT>(parent_obj);

    if (!ds->isAlive()) {
      Nan::ThrowError("Dataset object has already been destroyed");
      return;
    }

    std::vector<std::string> names = SELF::__getNames(parent->get());

    Local<Array> results = Nan::New<Array>(0);
    int i = 0;
    for (std::string &n : names) { Nan::Set(results, i++, SafeString::New(n.c_str())); }

    info.GetReturnValue().Set(results);
  }

  static NAN_GETTER(parentGetter) {
    info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked());
  }

  static NAN_GETTER(dsGetter) {
    info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("parent_ds_").ToLocalChecked()).ToLocalChecked());
  }

  GroupCollection() : Nan::ObjectWrap() {
  }

    protected:
  ~GroupCollection() {
  }
};

} // namespace node_gdal
#endif
#endif
