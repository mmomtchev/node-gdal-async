

module.exports = function (gdal) {

  function defaultForEach(callback) {
    const n = this.count()
    for (let i = 0; i < n; i++) {
      if (callback(this.get(i), i) === false) return
    }
  }

  /**
 * @callback mapCb
 * @param {T} object
 * @param {number} index
 * @returns {U}
 * @typedef {(obj: T, index: number) => U} mapCb<T,U>
 */
  function defaultMap(callback) {
    const result = []
    this.forEach((value, i) => {
      result.push(callback(value, i))
    })
    return result
  }

  function defaultToArray() {
    const array = []
    this.forEach((geom) => {
      array.push(geom)
    })
    return array
  }

  function defaultIterator() {
    let i = 0

    return {
      next: () => {
        const done = !(i < this.count())
        return {
          done,
          value: done ? null : this.get(i++)
        }
      }
    }
  }

  function defaultAsyncIterator() {
    let i = 0

    return {
      next: () => this.countAsync()
        .then((count) => {
          if (i < count) {
            return this.getAsync(i++).then((value) => ({ done: false, value }))
          }
          return { done: true, value: null }
        })
    }
  }


  /**
 * Iterates through all layers using a callback function.
 *
 * @example
 *
 * dataset.layers.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof DatasetLayers
 * @param {forEachCb<Layer>} callback The callback to be called with each {@link Layer}
 */
  gdal.DatasetLayers.prototype.forEach = defaultForEach


  /**
 * Iterates through layers using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = dataset.layers.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof DatasetLayers
 * @param {mapCb<Layer,U>} callback The callback to be called with each {@link Layer}
 * @return {U[]}
 */
  gdal.DatasetLayers.prototype.map = defaultMap


  /**
 * Iterates through all layers using an iterator
 *
 * @example
 *
 * for (const array of dataset.layers) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof DatasetLayers
 * @type {Layer}
 */
  gdal.DatasetLayers.prototype[Symbol.iterator] = defaultIterator


  /**
 * Iterates through all layers using an async iterator
 *
 * @example
 *
 * for await (const array of dataset.layers) {
 * }
 *
 * @method
 * @name @@asyncIterator
 * @instance
 * @memberof DatasetLayers
 * @type {Layer}
 */
  if (Symbol.asyncIterator) {
    gdal.DatasetLayers.prototype[Symbol.asyncIterator] = defaultAsyncIterator
  }


  /**
 * Iterates through raster bands using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = dataset.bands.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof DatasetBands
 * @param {mapCb<RasterBand,U>} callback The callback to be called with each {@link RasterBand}
 * @return {U[]}
 */
  gdal.DatasetBands.prototype.map = defaultMap


  /**
 * Iterates through all overviews using a callback function.
 *
 * @example
 *
 * band.overviews.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof RasterBandOverviews
 * @param {forEachCb<RasterBand>} callback The callback to be called with each {@link RasterBand}
 */
  gdal.RasterBandOverviews.prototype.forEach = defaultForEach


  /**
 * Iterates through overviews using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = band.overviews.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof RasterBandOverviews
 * @param {mapCb<RasterBand,U>} callback The callback to be called with each {@link RasterBand}
 * @return {U[]}
 */
  gdal.RasterBandOverviews.prototype.map = defaultMap


  /**
 * Iterates through all overviews using an iterator
 *
 * @example
 *
 * for (const array of band.overviews) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof RasterBandOverviews
 * @type {RasterBand}
 */
  gdal.RasterBandOverviews.prototype[Symbol.iterator] = defaultIterator


  /**
 * Iterates through all overviews using an async iterator
 *
 * @example
 *
 * for await (const array of band.overviews) {
 * }
 *
 * @method
 * @name @@asyncIterator
 * @instance
 * @memberof RasterBandOverviews
 * @type {RasterBand}
 */
  if (Symbol.asyncIterator) {
    gdal.RasterBandOverviews.prototype[Symbol.asyncIterator] = defaultAsyncIterator
  }


  /**
 * Iterates through all drivers using a callback function.
 *
 * @example
 *
 * gdal.drivers.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof GDALDrivers
 * @param {forEachCb<Driver>} callback The callback to be called with each {@link Driver}
 */
  gdal.GDALDrivers.prototype.forEach = defaultForEach


  /**
 * Iterates through drivers using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = gdal.drivers.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof GDALDrivers
 * @param {mapCb<Driver,U>} callback The callback to be called with each {@link Driver}
 * @return {U[]}
 */
  gdal.GDALDrivers.prototype.map = defaultMap


  /**
 * Iterates through all drivers using an iterator
 *
 * @example
 *
 * for (const array of gdal.drivers) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof GDALDrivers
 * @type {Driver}
 */
  gdal.GDALDrivers.prototype[Symbol.iterator] = defaultIterator


  /**
 * Iterates through features using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = layer.features.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof LayerFeatures
 * @param {mapCb<Feature,U>} callback The callback to be called with each {@link Feature}
 * @return {U[]}
 */
  gdal.LayerFeatures.prototype.map = defaultMap


  /**
 * Iterates through field definitions using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = layer.features.get(0).fields.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof FeatureFields
 * @param {mapCb<any,U>} callback The callback to be called with each {@link any}
 * @return {U[]}
 */
  gdal.FeatureFields.prototype.map = defaultMap


  /**
 * Iterates through all field definitions using a callback function.
 *
 * @example
 *
 * layer.fields.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof LayerFields
 * @param {forEachCb<FieldDefn>} callback The callback to be called with each {@link FieldDefn}
 */
  gdal.LayerFields.prototype.forEach = defaultForEach


  /**
 * Iterates through field definitions using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = layer.fields.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof LayerFields
 * @param {mapCb<FieldDefn,U>} callback The callback to be called with each {@link FieldDefn}
 * @return {U[]}
 */
  gdal.LayerFields.prototype.map = defaultMap


  /**
 * Iterates through all field definitions using an iterator
 *
 * @example
 *
 * for (const array of layer.fields) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof LayerFields
 * @type {FieldDefn}
 */
  gdal.LayerFields.prototype[Symbol.iterator] = defaultIterator


  /**
 * Iterates through all field definitions using an async iterator
 *
 * @example
 *
 * for await (const array of layer.fields) {
 * }
 *
 * @method
 * @name @@asyncIterator
 * @instance
 * @memberof LayerFields
 * @type {FieldDefn}
 */
  if (Symbol.asyncIterator) {
    gdal.LayerFields.prototype[Symbol.asyncIterator] = defaultAsyncIterator
  }


  /**
 * Iterates through all field definitions using a callback function.
 *
 * @example
 *
 * featureDefn.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof FeatureDefnFields
 * @param {forEachCb<FieldDefn>} callback The callback to be called with each {@link FieldDefn}
 */
  gdal.FeatureDefnFields.prototype.forEach = defaultForEach


  /**
 * Iterates through field definitions using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = featureDefn.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof FeatureDefnFields
 * @param {mapCb<FieldDefn,U>} callback The callback to be called with each {@link FieldDefn}
 * @return {U[]}
 */
  gdal.FeatureDefnFields.prototype.map = defaultMap


  /**
 * Iterates through all field definitions using an iterator
 *
 * @example
 *
 * for (const array of featureDefn) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof FeatureDefnFields
 * @type {FieldDefn}
 */
  gdal.FeatureDefnFields.prototype[Symbol.iterator] = defaultIterator


  /**
 * Iterates through all field definitions using an async iterator
 *
 * @example
 *
 * for await (const array of featureDefn) {
 * }
 *
 * @method
 * @name @@asyncIterator
 * @instance
 * @memberof FeatureDefnFields
 * @type {FieldDefn}
 */
  if (Symbol.asyncIterator) {
    gdal.FeatureDefnFields.prototype[Symbol.asyncIterator] = defaultAsyncIterator
  }


  /**
 * Iterates through all points using a callback function.
 *
 * @example
 *
 * lineString.points.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof LineStringPoints
 * @param {forEachCb<Point>} callback The callback to be called with each {@link Point}
 */
  gdal.LineStringPoints.prototype.forEach = defaultForEach


  /**
 * Iterates through points using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = lineString.points.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof LineStringPoints
 * @param {mapCb<Point,U>} callback The callback to be called with each {@link Point}
 * @return {U[]}
 */
  gdal.LineStringPoints.prototype.map = defaultMap


  /**
 * Iterates through all points using an iterator
 *
 * @example
 *
 * for (const array of lineString.points) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof LineStringPoints
 * @type {Point}
 */
  gdal.LineStringPoints.prototype[Symbol.iterator] = defaultIterator


  /**
 * Outputs all points as a regular javascript array.
 *
 * @method
 * @name toArray
 * @instance
 * @memberof LineStringPoints
 * @return {Point[]} List of {@link Point|Point instances}
 */
  gdal.LineStringPoints.prototype.toArray = defaultToArray


  /**
 * Iterates through all rings using a callback function.
 *
 * @example
 *
 * polygon.rings.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof PolygonRings
 * @param {forEachCb<LineString>} callback The callback to be called with each {@link LineString}
 */
  gdal.PolygonRings.prototype.forEach = defaultForEach


  /**
 * Iterates through rings using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = polygon.rings.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof PolygonRings
 * @param {mapCb<LineString,U>} callback The callback to be called with each {@link LineString}
 * @return {U[]}
 */
  gdal.PolygonRings.prototype.map = defaultMap


  /**
 * Iterates through all rings using an iterator
 *
 * @example
 *
 * for (const array of polygon.rings) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof PolygonRings
 * @type {LineString}
 */
  gdal.PolygonRings.prototype[Symbol.iterator] = defaultIterator


  /**
 * Outputs all rings as a regular javascript array.
 *
 * @method
 * @name toArray
 * @instance
 * @memberof PolygonRings
 * @return {LineString[]} List of {@link LineString|LineString instances}
 */
  gdal.PolygonRings.prototype.toArray = defaultToArray

  if (gdal.CompoundCurveCurves) {


    /**
 * Iterates through all curves using a callback function.
 *
 * @example
 *
 * compoundCurves.curves.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof CompoundCurveCurves
 * @param {forEachCb<SimpleCurve>} callback The callback to be called with each {@link SimpleCurve}
 */
    gdal.CompoundCurveCurves.prototype.forEach = defaultForEach


    /**
 * Iterates through curves using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = compoundCurves.curves.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof CompoundCurveCurves
 * @param {mapCb<SimpleCurve,U>} callback The callback to be called with each {@link SimpleCurve}
 * @return {U[]}
 */
    gdal.CompoundCurveCurves.prototype.map = defaultMap


    /**
 * Iterates through all curves using an iterator
 *
 * @example
 *
 * for (const array of compoundCurves.curves) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof CompoundCurveCurves
 * @type {SimpleCurve}
 */
    gdal.CompoundCurveCurves.prototype[Symbol.iterator] = defaultIterator


    /**
 * Iterates through all curves using an async iterator
 *
 * @example
 *
 * for await (const array of compoundCurves.curves) {
 * }
 *
 * @method
 * @name @@asyncIterator
 * @instance
 * @memberof CompoundCurveCurves
 * @type {SimpleCurve}
 */
    if (Symbol.asyncIterator) {
      gdal.CompoundCurveCurves.prototype[Symbol.asyncIterator] = defaultAsyncIterator
    }


    /**
 * Outputs all curves as a regular javascript array.
 *
 * @method
 * @name toArray
 * @instance
 * @memberof CompoundCurveCurves
 * @return {SimpleCurve[]} List of {@link SimpleCurve|SimpleCurve instances}
 */
    gdal.CompoundCurveCurves.prototype.toArray = defaultToArray

  }


  /**
 * Iterates through all child geometries using a callback function.
 *
 * @example
 *
 * geometryCollection.children.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof GeometryCollectionChildren
 * @param {forEachCb<Geometry>} callback The callback to be called with each {@link Geometry}
 */
  gdal.GeometryCollectionChildren.prototype.forEach = defaultForEach


  /**
 * Iterates through child geometries using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = geometryCollection.children.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof GeometryCollectionChildren
 * @param {mapCb<Geometry,U>} callback The callback to be called with each {@link Geometry}
 * @return {U[]}
 */
  gdal.GeometryCollectionChildren.prototype.map = defaultMap


  /**
 * Iterates through all child geometries using an iterator
 *
 * @example
 *
 * for (const array of geometryCollection.children) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof GeometryCollectionChildren
 * @type {Geometry}
 */
  gdal.GeometryCollectionChildren.prototype[Symbol.iterator] = defaultIterator


  /**
 * Iterates through all color entries using a callback function.
 *
 * @example
 *
 * band.colorTable.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof ColorTable
 * @param {forEachCb<Color>} callback The callback to be called with each {@link Color}
 */
  gdal.ColorTable.prototype.forEach = defaultForEach


  /**
 * Iterates through color entries using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = band.colorTable.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof ColorTable
 * @param {mapCb<Color,U>} callback The callback to be called with each {@link Color}
 * @return {U[]}
 */
  gdal.ColorTable.prototype.map = defaultMap


  /**
 * Iterates through all color entries using an iterator
 *
 * @example
 *
 * for (const array of band.colorTable) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof ColorTable
 * @type {Color}
 */
  gdal.ColorTable.prototype[Symbol.iterator] = defaultIterator


  if (gdal.MDArray) {


    /**
 * Iterates through all arrays using a callback function.
 *
 * @example
 *
 * group.arrays.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof GroupArrays
 * @param {forEachCb<MDArray>} callback The callback to be called with each {@link MDArray}
 */
    gdal.GroupArrays.prototype.forEach = defaultForEach


    /**
 * Iterates through arrays using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = group.arrays.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof GroupArrays
 * @param {mapCb<MDArray,U>} callback The callback to be called with each {@link MDArray}
 * @return {U[]}
 */
    gdal.GroupArrays.prototype.map = defaultMap


    /**
 * Iterates through all arrays using an iterator
 *
 * @example
 *
 * for (const array of group.arrays) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof GroupArrays
 * @type {MDArray}
 */
    gdal.GroupArrays.prototype[Symbol.iterator] = defaultIterator


    /**
 * Iterates through all arrays using an async iterator
 *
 * @example
 *
 * for await (const array of group.arrays) {
 * }
 *
 * @method
 * @name @@asyncIterator
 * @instance
 * @memberof GroupArrays
 * @type {MDArray}
 */
    if (Symbol.asyncIterator) {
      gdal.GroupArrays.prototype[Symbol.asyncIterator] = defaultAsyncIterator
    }


    /**
 * Iterates through all groups using a callback function.
 *
 * @example
 *
 * group.groups.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof GroupGroups
 * @param {forEachCb<Group>} callback The callback to be called with each {@link Group}
 */
    gdal.GroupGroups.prototype.forEach = defaultForEach


    /**
 * Iterates through groups using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = group.groups.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof GroupGroups
 * @param {mapCb<Group,U>} callback The callback to be called with each {@link Group}
 * @return {U[]}
 */
    gdal.GroupGroups.prototype.map = defaultMap


    /**
 * Iterates through all groups using an iterator
 *
 * @example
 *
 * for (const array of group.groups) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof GroupGroups
 * @type {Group}
 */
    gdal.GroupGroups.prototype[Symbol.iterator] = defaultIterator


    /**
 * Iterates through all groups using an async iterator
 *
 * @example
 *
 * for await (const array of group.groups) {
 * }
 *
 * @method
 * @name @@asyncIterator
 * @instance
 * @memberof GroupGroups
 * @type {Group}
 */
    if (Symbol.asyncIterator) {
      gdal.GroupGroups.prototype[Symbol.asyncIterator] = defaultAsyncIterator
    }


    /**
 * Iterates through all dimensions using a callback function.
 *
 * @example
 *
 * group.dimensions.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof GroupDimensions
 * @param {forEachCb<Dimension>} callback The callback to be called with each {@link Dimension}
 */
    gdal.GroupDimensions.prototype.forEach = defaultForEach


    /**
 * Iterates through dimensions using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = group.dimensions.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof GroupDimensions
 * @param {mapCb<Dimension,U>} callback The callback to be called with each {@link Dimension}
 * @return {U[]}
 */
    gdal.GroupDimensions.prototype.map = defaultMap


    /**
 * Iterates through all dimensions using an iterator
 *
 * @example
 *
 * for (const array of group.dimensions) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof GroupDimensions
 * @type {Dimension}
 */
    gdal.GroupDimensions.prototype[Symbol.iterator] = defaultIterator


    /**
 * Iterates through all dimensions using an async iterator
 *
 * @example
 *
 * for await (const array of group.dimensions) {
 * }
 *
 * @method
 * @name @@asyncIterator
 * @instance
 * @memberof GroupDimensions
 * @type {Dimension}
 */
    if (Symbol.asyncIterator) {
      gdal.GroupDimensions.prototype[Symbol.asyncIterator] = defaultAsyncIterator
    }


    /**
 * Iterates through all attributes using a callback function.
 *
 * @example
 *
 * group.attributes.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof GroupAttributes
 * @param {forEachCb<Attribute>} callback The callback to be called with each {@link Attribute}
 */
    gdal.GroupAttributes.prototype.forEach = defaultForEach


    /**
 * Iterates through attributes using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = group.attributes.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof GroupAttributes
 * @param {mapCb<Attribute,U>} callback The callback to be called with each {@link Attribute}
 * @return {U[]}
 */
    gdal.GroupAttributes.prototype.map = defaultMap


    /**
 * Iterates through all attributes using an iterator
 *
 * @example
 *
 * for (const array of group.attributes) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof GroupAttributes
 * @type {Attribute}
 */
    gdal.GroupAttributes.prototype[Symbol.iterator] = defaultIterator


    /**
 * Iterates through all attributes using an async iterator
 *
 * @example
 *
 * for await (const array of group.attributes) {
 * }
 *
 * @method
 * @name @@asyncIterator
 * @instance
 * @memberof GroupAttributes
 * @type {Attribute}
 */
    if (Symbol.asyncIterator) {
      gdal.GroupAttributes.prototype[Symbol.asyncIterator] = defaultAsyncIterator
    }


    /**
 * Iterates through all dimensions using a callback function.
 *
 * @example
 *
 * array.dimensions.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof ArrayDimensions
 * @param {forEachCb<Dimension>} callback The callback to be called with each {@link Dimension}
 */
    gdal.ArrayDimensions.prototype.forEach = defaultForEach


    /**
 * Iterates through dimensions using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = array.dimensions.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof ArrayDimensions
 * @param {mapCb<Dimension,U>} callback The callback to be called with each {@link Dimension}
 * @return {U[]}
 */
    gdal.ArrayDimensions.prototype.map = defaultMap


    /**
 * Iterates through all dimensions using an iterator
 *
 * @example
 *
 * for (const array of array.dimensions) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof ArrayDimensions
 * @type {Dimension}
 */
    gdal.ArrayDimensions.prototype[Symbol.iterator] = defaultIterator


    /**
 * Iterates through all dimensions using an async iterator
 *
 * @example
 *
 * for await (const array of array.dimensions) {
 * }
 *
 * @method
 * @name @@asyncIterator
 * @instance
 * @memberof ArrayDimensions
 * @type {Dimension}
 */
    if (Symbol.asyncIterator) {
      gdal.ArrayDimensions.prototype[Symbol.asyncIterator] = defaultAsyncIterator
    }


    /**
 * Iterates through all attributes using a callback function.
 *
 * @example
 *
 * array.attributes.forEach(function(array, i) { ... });
 *
 * @method
 * @name forEach
 * @instance
 * @memberof ArrayAttributes
 * @param {forEachCb<Attribute>} callback The callback to be called with each {@link Attribute}
 */
    gdal.ArrayAttributes.prototype.forEach = defaultForEach


    /**
 * Iterates through attributes using a callback function and builds
 * an array of the returned values.
 *
 * @example
 *
 * var result = array.attributes.map(function(array, i) {
 *     return value;
 * });
 *
 * @method
 * @name map<U>
 * @instance
 * @memberof ArrayAttributes
 * @param {mapCb<Attribute,U>} callback The callback to be called with each {@link Attribute}
 * @return {U[]}
 */
    gdal.ArrayAttributes.prototype.map = defaultMap


    /**
 * Iterates through all attributes using an iterator
 *
 * @example
 *
 * for (const array of array.attributes) {
 * }
 *
 * @method
 * @name @@iterator
 * @instance
 * @memberof ArrayAttributes
 * @type {Attribute}
 */
    gdal.ArrayAttributes.prototype[Symbol.iterator] = defaultIterator


    /**
 * Iterates through all attributes using an async iterator
 *
 * @example
 *
 * for await (const array of array.attributes) {
 * }
 *
 * @method
 * @name @@asyncIterator
 * @instance
 * @memberof ArrayAttributes
 * @type {Attribute}
 */
    if (Symbol.asyncIterator) {
      gdal.ArrayAttributes.prototype[Symbol.asyncIterator] = defaultAsyncIterator
    }


  }

}
