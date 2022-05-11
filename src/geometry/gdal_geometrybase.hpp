#ifndef __NODE_OGR_GEOMETRYBASE_H__
#define __NODE_OGR_GEOMETRYBASE_H__

// node
#include <node.h>
#include <node_object_wrap.h>

// nan
#include "../nan-wrapper.h"

#include "../gdal_common.hpp"

using namespace v8;
using namespace node;

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
 */

#define UPDATE_AMOUNT_OF_GEOMETRY_MEMORY(geom)                                                                         \
  {                                                                                                                    \
    int new_size = geom->this_->WkbSize();                                                                             \
    if (geom->owned_) Nan::AdjustExternalMemory(new_size - geom->size_);                                               \
    geom->size_ = new_size;                                                                                            \
  }

template <class T, class OGRT> class GeometryBase : public Nan::ObjectWrap {
    public:
  static Local<Value> New(OGRT *geom);
  static Local<Value> New(OGRT *geom, bool owned);

  GeometryBase();
  GeometryBase(OGRT *geom);
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

template <class T, class OGRT> Local<Value> GeometryBase<T, OGRT>::New(OGRT *geom) {
  Nan::EscapableHandleScope scope;
  return scope.Escape(T::New(geom, true));
}

template <class T, class OGRT> Local<Value> GeometryBase<T, OGRT>::New(OGRT *geom, bool owned) {
  Nan::EscapableHandleScope scope;

  if (!geom) { return scope.Escape(Nan::Null()); }

  // make a copy of geometry owned by a feature
  // + no need to track when a feature is destroyed
  // + no need to throw errors when a method tries to modify an owned read-only
  // geometry
  // - is slower

  if (!owned) { geom = static_cast<OGRT *>(geom->clone()); }

  T *wrapped = new T(geom);
  wrapped->owned_ = true;

  UPDATE_AMOUNT_OF_GEOMETRY_MEMORY(wrapped);

  Local<Value> ext = Nan::New<External>(wrapped);
  Local<Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(T::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();

  return scope.Escape(obj);
}

template <class T, class OGRT>
GeometryBase<T, OGRT>::GeometryBase(OGRT *geom) : Nan::ObjectWrap(), this_(geom), owned_(true), size_(0) {
  LOG("Created Geometry %s [%p]", typeid(T).name(), geom);
  // The async locks must live outside the V8 memory management,
  // otherwise they won't be accessible from the async threads
  async_lock = new uv_sem_t;
  uv_sem_init(async_lock, 1);
}

template <class T, class OGRT>
GeometryBase<T, OGRT>::GeometryBase() : Nan::ObjectWrap(), this_(NULL), owned_(true), size_(0) {
  async_lock = new uv_sem_t;
  uv_sem_init(async_lock, 1);
}

template <class T, class OGRT> GeometryBase<T, OGRT>::~GeometryBase() {
  if (this_) {
    LOG("Disposing Geometry %s [%p] (%s)", typeid(T).name(), this_, owned_ ? "owned" : "unowned");
    if (owned_) {
      OGRGeometryFactory::destroyGeometry(this_);
      Nan::AdjustExternalMemory(-size_);
    }
    LOG("Disposed Geometry [%p]", this_)
    this_ = NULL;
  }
  uv_sem_destroy(async_lock);
  delete async_lock;
}

} // namespace node_gdal
#endif
