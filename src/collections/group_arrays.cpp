#include <memory>
#include "group_arrays.hpp"
#include "../gdal_common.hpp"
#include "../gdal_dataset.hpp"
#include "../gdal_group.hpp"
#include "../gdal_mdarray.hpp"
#include "../utils/string_list.hpp"

namespace node_gdal {

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

Nan::Persistent<FunctionTemplate> GroupArrays::constructor;

std::shared_ptr<GDALMDArray> GroupArrays::__get(std::shared_ptr<GDALGroup> parent, std::string const &name) {
  return parent->OpenMDArray(name);
}

std::shared_ptr<GDALMDArray> GroupArrays::__get(std::shared_ptr<GDALGroup> parent, size_t idx) {
  std::vector<std::string> arrays = parent->GetMDArrayNames();
  if (idx >= arrays.size()) return nullptr;
  return parent->OpenMDArray(arrays[idx]);
}

std::vector<std::string> GroupArrays::__getNames(std::shared_ptr<GDALGroup> parent) {
  return parent->GetMDArrayNames();
}

int GroupArrays::__count(std::shared_ptr<GDALGroup> parent) {
  int r = parent->GetMDArrayNames().size();
  return r;
}

/**
 * An encapsulation of a {@link Group}
 * descendant arrays.
 *
 * @class GroupArrays
 */

/**
 * Returns the array with the given name/index.
 *
 * @method get
 * @instance
 * @memberof GroupArrays
 * @param {string|number} array
 * @return {MDArray}
 */

/**
 * Returns the array with the given name/index.
 * @async
 *
 * @method getAsync
 * @instance
 * @memberof GroupArrays
 *
 * @param {string|number} array
 * @param {callback<MDArray>} [callback=undefined]
 * @return {Promise<MDArray>}
 */

/**
 * Returns the number of arrays in the collection.
 *
 * @method count
 * @instance
 * @memberof GroupArrays
 * @return {number}
 */

/**
 * Returns the number of arrays in the collection.
 * @async
 *
 * @method countAsync
 * @instance
 * @memberof GroupArrays
 *
 * @param {callback<number>} [callback=undefined]
 * @return {Promise<number>}
 */

/**
 * @readonly
 * @kind member
 * @name names
 * @instance
 * @memberof GroupArrays
 * @type {string[]}
 */

/**
 * Returns the parent dataset.
 *
 * @readonly
 * @kind member
 * @name ds
 * @instance
 * @memberof GroupArrays
 * @type {Dataset}
 */
#endif

} // namespace node_gdal
