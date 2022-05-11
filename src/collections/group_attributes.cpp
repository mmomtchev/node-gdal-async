#include <memory>
#include "group_attributes.hpp"
#include "../gdal_common.hpp"
#include "../gdal_dataset.hpp"
#include "../gdal_group.hpp"
#include "../gdal_mdarray.hpp"
#include "../utils/string_list.hpp"

namespace node_gdal {

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 1)

Nan::Persistent<FunctionTemplate> GroupAttributes::constructor;

std::shared_ptr<GDALAttribute> GroupAttributes::__get(std::shared_ptr<GDALGroup> parent, std::string const &name) {
  return parent->GetAttribute(name);
}

std::shared_ptr<GDALAttribute> GroupAttributes::__get(std::shared_ptr<GDALGroup> parent, size_t idx) {
  std::vector<std::shared_ptr<GDALAttribute>> attrs = parent->GetAttributes();
  if (idx >= attrs.size()) return nullptr;
  return attrs[idx];
}

std::vector<std::string> GroupAttributes::__getNames(std::shared_ptr<GDALGroup> parent) {
  std::vector<std::shared_ptr<GDALAttribute>> attrs = parent->GetAttributes();
  std::vector<std::string> r;
  for (const std::shared_ptr<GDALAttribute> &d : attrs) r.push_back(d->GetName());
  return r;
}

int GroupAttributes::__count(std::shared_ptr<GDALGroup> parent) {
  int r = parent->GetAttributes().size();
  return r;
}

/**
 * An encapsulation of a {@link Group}
 * descendant attributes.
 *
 * @class GroupAttributes
 */

/**
 * Returns the attribute with the given name/index.
 *
 * @method get
 * @instance
 * @memberof GroupAttributes
 * @param {string|number} attribute
 * @return {Attribute}
 */

/**
 * Returns the attribute with the given name/index.
 * @async
 *
 * @method getAsync
 * @instance
 * @memberof GroupAttributes
 *
 * @param {string|number} attribute
 * @param {callback<Attribute>} [callback=undefined]
 * @return {Promise<Attribute>}
 */

/**
 * Returns the number of attributes in the collection.
 *
 * @method count
 * @instance
 * @memberof GroupAttributes
 * @return {number}
 */

/**
 * Returns the number of attributes in the collection.
 * @async
 *
 * @method countAsync
 * @instance
 * @memberof GroupAttributes
 *
 * @param {callback<number>} [callback=undefined]
 * @return {Promise<number>}
 */

/**
 * @readonly
 * @kind member
 * @name names
 * @instance
 * @memberof GroupAttributes
 * @type {string[]}
 */

/**
 * Returns the parent dataset.
 *
 * @readonly
 * @kind member
 * @name ds
 * @instance
 * @memberof GroupAttributes
 * @type {Dataset}
 */
#endif

} // namespace node_gdal
