#ifndef __NODE_OGR_GEOMETRYBASE_H__
#define __NODE_OGR_GEOMETRYBASE_H__

#include "../gdal_common.hpp"

namespace node_gdal {

/*
 * Geometry class inheritance hierarchy.
 * It uses CRTP - https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
 * to get around the fact that the methods exposed to JS are static and cannot be virtual.
 *
 *
 * C++
 * (maximizes code reuse)
 *
 * GeometryBase<>
 *    Geometry
 *    Point
 *    CurveBase<>
 *        SimpleCurve
 *        LineString
 *        CircularString
 *        LinearRing
 *        Polygon
 *        CompoundCurve
 *    GeometryCollectionBase<>
 *        GeometryCollection
 *        Multi*
 *
 *
 * JS
 * (tries to follow GDAL and the ISO specification)
 *
 * Geometry
 *    Point
 *    SimpleCurve
 *        LineString
 *            LinearRing
 *        CircularString
 *    Polygon
 *    CompoundCurve
 *    GeometryCollection
 *        Multi*
 *
 *
 * The full GDAL OGRGeometry class hierarchy
 * https://gdal.org/doxygen/classOGRGeometry.html
 *
 * Every concrete class passes itself as T, which is exactly what
 * Napi::ObjectWrap needs, so the CRTP maps onto it one to one.
 */

#define UPDATE_AMOUNT_OF_GEOMETRY_MEMORY(geom)                                                                         \
  {                                                                                                                    \
    int new_size = geom->this_->WkbSize();                                                                             \
    if (geom->owned_) Napi::MemoryManagement::AdjustExternalMemory(geom->Env(), new_size - geom->size_);               \
    geom->size_ = new_size;                                                                                            \
  }

// OGRSimpleCurve and OGRGeometry are abstract (Geometry/SimpleCurve only exist
// so that the concrete classes can be `instanceof` them) - nothing to construct
template <typename OGRT> inline OGRT *MakeGeometry() {
  return new OGRT();
}
template <> inline OGRSimpleCurve *MakeGeometry<OGRSimpleCurve>() {
  return nullptr;
}
template <> inline OGRGeometry *MakeGeometry<OGRGeometry>() {
  return nullptr;
}

template <class T, class OGRT> class GeometryBase : public GDALObject<T> {
    public:
  static Napi::Value New(OGRT *geom);
  static Napi::Value New(OGRT *geom, bool owned);

  GeometryBase(const Napi::CallbackInfo &info);
  inline OGRT *get() {
    return this_;
  }
  inline bool isAlive() {
    return this_;
  }

    protected:
  ~GeometryBase();
  OGRT *this_;
  bool owned_;
  int size_;
  uv_sem_t *async_lock;
};

template <class T, class OGRT> Napi::Value GeometryBase<T, OGRT>::New(OGRT *geom) {
  return T::New(geom, true);
}

template <class T, class OGRT> Napi::Value GeometryBase<T, OGRT>::New(OGRT *geom, bool owned) {
  Napi::Env env = node_gdal::napi_env();

  if (!geom) { return env.Null(); }

  // make a copy of geometry owned by a feature
  // + no need to track when a feature is destroyed
  // + no need to throw errors when a method tries to modify an owned read-only
  // geometry
  // - is slower

  if (!owned) { geom = static_cast<OGRT *>(geom->clone()); }

  // node-addon-api allocates the wrapper itself, so the External carries the OGR
  // object and not the pre-built wrapper - unlike the old binding
  std::vector<napi_value> args = {Napi::External<OGRT>::New(env, geom)};
  Napi::Object obj = T::constructor.Value().New(args);

  T *wrapped = node_gdal::UnwrapWrapped<T>(obj);
  wrapped->owned_ = true;

  UPDATE_AMOUNT_OF_GEOMETRY_MEMORY(wrapped);

  return obj;
}

template <class T, class OGRT>
GeometryBase<T, OGRT>::GeometryBase(const Napi::CallbackInfo &info)
  : GDALObject<T>(info), this_(nullptr), owned_(true), size_(0) {
  // The async locks must live outside the V8 memory management,
  // otherwise they won't be accessible from the async threads
  async_lock = new uv_sem_t;
  uv_sem_init(async_lock, 1);

  if (info.Length() > 0 && info[0].IsExternal()) {
    this_ = info[0].As<Napi::External<OGRT>>().Data();
  } else {
    // Constructed from JS: the derived class interprets its own arguments
    this_ = MakeGeometry<OGRT>();
  }
  LOG("Created Geometry %s [%p]", typeid(T).name(), this_);
}

template <class T, class OGRT> GeometryBase<T, OGRT>::~GeometryBase() {
  if (this_) {
    LOG("Disposing Geometry %s [%p] (%s)", typeid(T).name(), this_, owned_ ? "owned" : "unowned");
    if (owned_) {
      OGRGeometryFactory::destroyGeometry(this_);
      Napi::MemoryManagement::AdjustExternalMemory(node_gdal::napi_env(), -size_);
    }
    LOG("Disposed Geometry [%p]", this_)
    this_ = NULL;
  }
  uv_sem_destroy(async_lock);
  delete async_lock;
}

} // namespace node_gdal
#endif
