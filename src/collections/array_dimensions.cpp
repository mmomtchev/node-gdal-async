#include <memory>
#include "array_dimensions.hpp"
#include "../gdal_common.hpp"
#include "../gdal_dataset.hpp"
#include "../gdal_group.hpp"
#include "../gdal_mdarray.hpp"
#include "../utils/string_list.hpp"

namespace node_gdal {

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

Nan::Persistent<FunctionTemplate> ArrayDimensions::constructor;

std::shared_ptr<GDALDimension> ArrayDimensions::__get(std::shared_ptr<GDALMDArray> parent, std::string const &name) {
  std::vector<std::shared_ptr<GDALDimension>> dims = parent->GetDimensions();
  for (const std::shared_ptr<GDALDimension> &d : dims)
    if (d->GetName() == name) return d;
  return nullptr;
}

int ArrayDimensions::__getIdx(std::shared_ptr<GDALMDArray> parent, std::string const &name) {
  std::vector<std::shared_ptr<GDALDimension>> dims = parent->GetDimensions();
  for (size_t i = 0; i < dims.size(); i++) {
    if (dims[i]->GetName() == name) return i;
  }
  return -1;
}

std::shared_ptr<GDALDimension> ArrayDimensions::__get(std::shared_ptr<GDALMDArray> parent, size_t idx) {
  std::vector<std::shared_ptr<GDALDimension>> dims = parent->GetDimensions();
  if (idx >= dims.size()) return nullptr;
  return dims[idx];
}

std::vector<std::string> ArrayDimensions::__getNames(std::shared_ptr<GDALMDArray> parent) {
  std::vector<std::shared_ptr<GDALDimension>> dims = parent->GetDimensions();
  std::vector<std::string> r;
  for (const std::shared_ptr<GDALDimension> &d : dims) r.push_back(d->GetName());
  return r;
}

int ArrayDimensions::__count(std::shared_ptr<GDALMDArray> parent) {
  int r = parent->GetDimensionCount();
  return r;
}

/**
 * An encapsulation of a {@link MDArray}
 * descendant dimensions.
 *
 * @example
 * const dimensions = group.dimensions;
 *
 * @class ArrayDimensions
 */

/**
 * Returns the array with the given name/index.
 *
 * @method get
 * @instance
 * @memberof ArrayDimensions
 * @param {string|number} array
 * @return {Dimension}
 */

/**
 * Returns the array with the given name/index.
 * @async
 *
 * @method getAsync
 * @instance
 * @memberof ArrayDimensions
 * @param {string|number} array
 * @param {callback<Dimension>} [callback=undefined]
 * @return {Promise<Dimension>}
 */

/**
 * Returns the number of dimensions in the collection.
 *
 * @method count
 * @instance
 * @memberof ArrayDimensions
* @return {number}
 */

/**
 * Returns the number of dimensions in the collection.
 * @async
 *
 * @method countAsync
 * @instance
 * @memberof ArrayDimensions
 * @param {callback<number>} [callback=undefined]
 * @return {Promise<number>}
 */

/**
 * Dimensions' names.
 *
 * @readonly
 * @kind member
 * @instance
 * @memberof ArrayDimensions
 * @name names
 * @type {string[]}
 */

/**
 * Returns the parent group.
 *
 * @readonly
 * @kind member
 * @instance
 * @memberof ArrayDimensions
 * @name parent
 * @type {Group}
 */

/**
 * Returns the parent dataset.
 *
 * @readonly
 * @kind member
 * @instance
 * @memberof ArrayDimensions
 * @name ds
 * @type {Dataset}
 */
#endif

} // namespace node_gdal
