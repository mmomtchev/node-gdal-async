#ifndef __NODE_GDAL_BASEGROUP_COLLECTION_H__
#define __NODE_GDAL_BASEGROUP_COLLECTION_H__

// gdal
#include <gdal_priv.h>

#include "../async.hpp"
#include "../gdal_common.hpp"
#include "../gdal_dataset.hpp"
#include "../gdal_group.hpp"

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

namespace node_gdal {

template <typename SELF, typename GDALOBJ, typename GDALPARENT, typename NODEOBJ, typename NODEPARENT>
class GroupCollection : public GDALObject<SELF> {
    public:
  static constexpr const char *_className = "GroupCollection<abstract>";

  static void Initialize(Napi::Object target) {
    Napi::Env env = target.Env();

    // NOTE: the descriptor macros carry their own trailing comma
    Napi::Function lcons = GDALDefineClass<SELF>(env, SELF::_className,
      {
        METHOD(toString)
        METHOD_ASYNCABLE(count)
        METHOD_ASYNCABLE(get)
        ATTR_DONT_ENUM(lcons, "ds", dsGetter, READ_ONLY_SETTER)
        ATTR_DONT_ENUM(lcons, "parent", parentGetter, READ_ONLY_SETTER)
        ATTR(lcons, "names", namesGetter, READ_ONLY_SETTER)
      });

    target.Set(SELF::_className, lcons);

    SELF::constructor = Napi::Persistent(lcons);
    SELF::constructor.SuppressDestruct();
  }

  static NAPI_METHOD(toString) {
    return Napi::String::New(info.Env(), SELF::_className);
  }

  static Napi::Value New(Napi::Value parent, Napi::Value parent_ds) {
    // The parents are passed to the constructor so that the constructor can
    // tell an internal construction (from a parent object) apart from a direct
    // `new gdal.X()` from JS
    std::vector<napi_value> args = {parent, parent_ds};
    return SELF::constructor.Value().New(args);
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

    Napi::Object this_obj = info.This().As<Napi::Object>();
    Napi::Object parent_ds = GDAL_GET_PRIVATE(this_obj, "parent_ds_").As<Napi::Object>();
    Napi::Object parent_obj = GDAL_GET_PRIVATE(this_obj, "parent_").As<Napi::Object>();
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
    return job.run(info, async, 1);
  }

  GDAL_ASYNCABLE_TEMPLATE(count) {

    Napi::Object this_obj = info.This().As<Napi::Object>();
    Napi::Object parent_ds = GDAL_GET_PRIVATE(this_obj, "parent_ds_").As<Napi::Object>();
    Napi::Object parent_obj = GDAL_GET_PRIVATE(this_obj, "parent_").As<Napi::Object>();
    NODE_UNWRAP_CHECK(Dataset, parent_ds, ds);
    NODE_UNWRAP_CHECK(NODEPARENT, parent_obj, parent);

    std::shared_ptr<GDALPARENT> raw = parent->get();

    GDALAsyncableJob<int> job(ds->uid);
    job.persist(parent_obj);
    job.main = [raw](const GDALExecutionProgress &) {
      int r = SELF::__count(raw);
      return r;
    };
    job.rval = [](int r, const GetFromPersistentFunc &) { return Napi::Number::New(node_gdal::napi_env(), r); };
    return job.run(info, async, 0);
  }

  static NAPI_GETTER(namesGetter) {

    Napi::Object this_obj = info.This().As<Napi::Object>();
    Napi::Object parent_ds = GDAL_GET_PRIVATE(this_obj, "parent_ds_").As<Napi::Object>();
    Dataset *ds = node_gdal::UnwrapWrapped<Dataset>(parent_ds);

    Napi::Object parent_obj = GDAL_GET_PRIVATE(this_obj, "parent_").As<Napi::Object>();
    NODEPARENT *parent = node_gdal::UnwrapWrapped<NODEPARENT>(parent_obj);

    if (!ds->isAlive()) {
      Napi::Error::New(info.Env(), "Dataset object has already been destroyed").ThrowAsJavaScriptException();
      return info.Env().Undefined();
    }

    std::vector<std::string> names = SELF::__getNames(parent->get());

    Napi::Array results = Napi::Array::New(info.Env(), 0);
    int i = 0;
    for (std::string &n : names) { results.Set(i++, SafeString::New(info.Env(), n.c_str())); }

    return results;
  }

  static NAPI_GETTER(parentGetter) {
    return GDAL_GET_PRIVATE(info.This().As<Napi::Object>(), "parent_");
  }

  static NAPI_GETTER(dsGetter) {
    return GDAL_GET_PRIVATE(info.This().As<Napi::Object>(), "parent_ds_");
  }

  GroupCollection(const Napi::CallbackInfo &info) : GDALObject<SELF>(info) {
    if (info.Length() < 2 || !info[0].IsObject() || !info[1].IsObject()) {
      std::string msg = "Cannot create ";
      msg += SELF::_className;
      msg += " directly";
      Napi::Error::New(info.Env(), msg.c_str()).ThrowAsJavaScriptException();
      return;
    }
    Napi::Object this_obj = info.This().As<Napi::Object>();
    GDAL_SET_PRIVATE(this_obj, "parent_ds_", info[1]);
    GDAL_SET_PRIVATE(this_obj, "parent_", info[0]);
  }

    protected:
  ~GroupCollection() {
  }
};

} // namespace node_gdal
#endif
#endif
