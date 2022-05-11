#include "geometry_collection_children.hpp"
#include "../gdal_common.hpp"
#include "../geometry/gdal_geometry.hpp"
#include "../geometry/gdal_geometrycollection.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> GeometryCollectionChildren::constructor;

void GeometryCollectionChildren::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(GeometryCollectionChildren::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("GeometryCollectionChildren").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan::SetPrototypeMethod(lcons, "count", count);
  Nan::SetPrototypeMethod(lcons, "get", get);
  Nan::SetPrototypeMethod(lcons, "remove", remove);
  Nan::SetPrototypeMethod(lcons, "add", add);

  Nan::Set(target, Nan::New("GeometryCollectionChildren").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

GeometryCollectionChildren::GeometryCollectionChildren() : Nan::ObjectWrap() {
}

GeometryCollectionChildren::~GeometryCollectionChildren() {
}

/**
 * A collection of Geometries, used by {@link GeometryCollection}.
 *
 * @class GeometryCollectionChildren
 */
NAN_METHOD(GeometryCollectionChildren::New) {

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }
  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    GeometryCollectionChildren *geom = static_cast<GeometryCollectionChildren *>(ptr);
    geom->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
    return;
  } else {
    Nan::ThrowError("Cannot create GeometryCollectionChildren directly");
    return;
  }
}

Local<Value> GeometryCollectionChildren::New(Local<Value> geom) {
  Nan::EscapableHandleScope scope;

  GeometryCollectionChildren *wrapped = new GeometryCollectionChildren();

  v8::Local<v8::Value> ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(GeometryCollectionChildren::constructor)).ToLocalChecked(), 1, &ext)
      .ToLocalChecked();
  Nan::SetPrivate(obj, Nan::New("parent_").ToLocalChecked(), geom);

  return scope.Escape(obj);
}

NAN_METHOD(GeometryCollectionChildren::toString) {
  info.GetReturnValue().Set(Nan::New("GeometryCollectionChildren").ToLocalChecked());
}

/**
 * Returns the number of items.
 *
 * @method count
 * @instance
 * @memberof GeometryCollectionChildren
 * @return {number}
 */
NAN_METHOD(GeometryCollectionChildren::count) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  GeometryCollection *geom = Nan::ObjectWrap::Unwrap<GeometryCollection>(parent);

  info.GetReturnValue().Set(Nan::New<Integer>(geom->get()->getNumGeometries()));
}

/**
 * Returns the geometry at the specified index.
 *
 * @method get
 * @instance
 * @memberof GeometryCollectionChildren
 * @param {number} index 0-based index
 * @throws {Error}
 * @return {Geometry}
 */
NAN_METHOD(GeometryCollectionChildren::get) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  GeometryCollection *geom = Nan::ObjectWrap::Unwrap<GeometryCollection>(parent);

  int i;
  NODE_ARG_INT(0, "index", i);

  auto r = geom->get()->getGeometryRef(i);
  if (r == nullptr) {
    NODE_THROW_LAST_CPLERR;
    return;
  }
  info.GetReturnValue().Set(Geometry::New(r, false));
}

/**
 * Removes the geometry at the specified index.
 *
 * @method remove
 * @instance
 * @memberof GeometryCollectionChildren
 * @param {number} index 0-based index, -1 for all geometries
 */
NAN_METHOD(GeometryCollectionChildren::remove) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  GeometryCollection *geom = Nan::ObjectWrap::Unwrap<GeometryCollection>(parent);

  int i;
  NODE_ARG_INT(0, "index", i);

  OGRErr err = geom->get()->removeGeometry(i);
  if (err) {
    NODE_THROW_OGRERR(err);
    return;
  }

  return;
}

/**
 * Adds geometry(s) to the collection.
 *
 * @example
 *
 * // one at a time:
 * geometryCollection.children.add(new Point(0,0,0));
 *
 * // add many at once:
 * geometryCollection.children.add([
 *     new Point(1,0,0),
 *     new Point(1,0,0)
 * ]);
 *
 * @method add
 * @instance
 * @memberof GeometryCollectionChildren
 * @param {Geometry|Geometry[]} geometry
 */
NAN_METHOD(GeometryCollectionChildren::add) {

  Local<Object> parent =
    Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
  GeometryCollection *geom = Nan::ObjectWrap::Unwrap<GeometryCollection>(parent);

  Geometry *child;

  if (info.Length() < 1) {
    Nan::ThrowError("child(ren) must be given");
    return;
  }
  if (info[0]->IsArray()) {
    // set from array of geometry objects
    Local<Array> array = info[0].As<Array>();
    int length = array->Length();
    for (int i = 0; i < length; i++) {
      Local<Value> element = Nan::Get(array, i).ToLocalChecked();
      if (IS_WRAPPED(element, Geometry)) {
        child = Nan::ObjectWrap::Unwrap<Geometry>(element.As<Object>());
        OGRErr err = geom->get()->addGeometry(child->get());
        if (err) {
          NODE_THROW_OGRERR(err);
          return;
        }
      } else {
        Nan::ThrowError("All array elements must be geometry objects");
        return;
      }
    }
  } else if (IS_WRAPPED(info[0], Geometry)) {
    child = Nan::ObjectWrap::Unwrap<Geometry>(info[0].As<Object>());
    OGRErr err = geom->get()->addGeometry(child->get());
    if (err) {
      NODE_THROW_OGRERR(err);
      return;
    }
  } else {
    Nan::ThrowError("child must be a geometry object or array of geometry objects");
    return;
  }

  return;
}

} // namespace node_gdal
