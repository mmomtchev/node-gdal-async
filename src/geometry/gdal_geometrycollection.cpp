
#include "gdal_geometrycollection.hpp"
#include "../collections/geometry_collection_children.hpp"
#include "../gdal_common.hpp"
#include "gdal_geometry.hpp"

#include <stdlib.h>

namespace node_gdal {

Napi::FunctionReference GeometryCollection::constructor;

/**
 * A collection of 1 or more geometry objects.
 *
 * @constructor
 * @class GeometryCollection
 * @extends Geometry
 */
void GeometryCollection::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  SELF_CLASS(GeometryCollection);

  // NOTE: the descriptor macros carry their own trailing comma
  Napi::Function lcons = GDALDefineClass<SELF>(env, "GeometryCollection",
    {
        METHOD(toString)
        METHOD(getArea)
        METHOD(getLength)
        ATTR(lcons, "children", childrenGetter, READ_ONLY_SETTER)
    });

  // lcons->Inherit() has no DefineClass equivalent
  node_gdal::Inherit(lcons, Geometry::constructor.Value());

  target.Set("GeometryCollection", lcons);

  constructor = Napi::Persistent(lcons);
  constructor.SuppressDestruct();
}

NAPI_METHOD(GeometryCollection::toString) {
  return Napi::String::New(node_gdal::napi_env(), "GeometryCollection");
}

/**
 * Computes the combined area of the geometries.
 *
 * @method getArea
 * @instance
 * @memberof GeometryCollection
 * @return {number}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(GeometryCollection, getArea, Number, get_Area);

/**
 * Compute the length of a multicurve.
 *
 * @method getLength
 * @instance
 * @memberof GeometryCollection
 * @return {number}
 */
NODE_WRAPPED_METHOD_WITH_RESULT(GeometryCollection, getLength, Number, get_Length);

/**
 * All geometries represented by this collection.
 *
 * @kind member
 * @name children
 * @instance
 * @memberof GeometryCollection
 * @type {GeometryCollectionChildren}
 */
NAPI_GETTER(GeometryCollection::childrenGetter) {
  return GDAL_GET_PRIVATE(info.This(), "children_");
}

} // namespace node_gdal
