#include <memory>
#include "group_dimensions.hpp"
#include "../gdal_common.hpp"
#include "../gdal_dataset.hpp"
#include "../gdal_group.hpp"
#include "../gdal_mdarray.hpp"
#include "../utils/string_list.hpp"

namespace node_gdal {

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

Nan::Persistent<FunctionTemplate> GroupDimensions::constructor;

std::shared_ptr<GDALDimension> GroupDimensions::__get(std::shared_ptr<GDALGroup> parent, std::string const &name) {
  std::vector<std::shared_ptr<GDALDimension>> dims = parent->GetDimensions();
  for (const std::shared_ptr<GDALDimension> &d : dims)
    if (d->GetName() == name) return d;
  return nullptr;
}

std::shared_ptr<GDALDimension> GroupDimensions::__get(std::shared_ptr<GDALGroup> parent, size_t idx) {
  std::vector<std::shared_ptr<GDALDimension>> dims = parent->GetDimensions();
  if (idx >= dims.size()) return nullptr;
  return dims[idx];
}

std::vector<std::string> GroupDimensions::__getNames(std::shared_ptr<GDALGroup> parent) {
  std::vector<std::shared_ptr<GDALDimension>> dims = parent->GetDimensions();
  std::vector<std::string> r;
  for (const std::shared_ptr<GDALDimension> &d : dims) r.push_back(d->GetName());
  return r;
}

int GroupDimensions::__count(std::shared_ptr<GDALGroup> parent) {
  int r = parent->GetDimensions().size();
  return r;
}

/**
 * An encapsulation of a {@link Group}
 * descendant dimensions.
 *
 * @example
 * const dimensions = group.dimensions;
 *
 * @class GroupDimensions
 */

/**
 * Returns the array with the given name/index.
 *
 * @method get
 * @instance
 * @memberof GroupDimensions
 * @param {string|number} array
 * @return {Dimension}
 */

/**
 * Returns the array with the given name/index.
 * @async
 *
 * @method getAsync
 * @instance
 * @memberof GroupDimensions
 *
 * @param {string|number} array
 * @param {callback<Dimension>} [callback=undefined]
 * @return {Promise<Dimension>}
 */

/**
 * Returns the number of dimensions in the collection.
 *
 * @method count
 * @instance
 * @memberof GroupDimensions
 * @return {number}
 */

/**
 * Returns the number of dimensions in the collection.
 * @async
 *
 * @method countAsync
 * @instance
 * @memberof GroupDimensions
 *
 * @param {callback<number>} [callback=undefined]
 * @return {Promise<number>}
 */

/**
 * @readonly
 * @kind member
 * @name names
 * @instance
 * @memberof GroupDimensions
 * @type {string[]}
 */

/**
 * Returns the parent dataset.
 *
 * @readonly
 * @kind member
 * @name ds
 * @instance
 * @memberof GroupDimensions
 * @type {Dataset}
 */
#endif

} // namespace node_gdal
