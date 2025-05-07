const path = require('path')
const fs = require('fs')
const binary = require('@mapbox/node-pre-gyp')
const { Float16Array } = require('@petamoriken/float16')

const binding_path = binary.find(path.resolve(path.join(__dirname, '../package.json')))
const gdal = require(binding_path)

const data_path = path.resolve(__dirname, '..', 'deps', 'libgdal', 'gdal', 'data')
const proj_path = path.resolve(__dirname, '..', 'deps', 'libproj', 'proj', 'data')
const grib_resource_path = path.resolve(__dirname, '..', 'deps', 'libgdal', 'gdal', 'frmts', 'grib', 'data')

if (process.env.GDAL_DATA === undefined && !fs.existsSync(data_path)) {
  throw new Error(
    `The bundled data path for node-gdal is missing '${data_path}' and GDAL_DATA environment is not set`
  )
}

const proj_lib_env_undefined = process.env.PROJ_LIB === undefined
if (proj_lib_env_undefined && !fs.existsSync(proj_path)) {
  throw new Error(
    `The bundled proj data path for node-gdal is missing '${proj_path}' and PROJ_LIB environment is not set`
  )
}

module.exports = gdal

/**
 * Float16Array JS implementation
 *
 * @example
 *
 * const a = new gdal.Float16Array([1.0, 1.1, 1.2, 1.3])
 *
 * @name Float16Array
 * @kind member
 * @type {typeof import('@petamoriken/float16').Float16Array}
 * @readonly
 *
 */
gdal.Float16Array = Float16Array

if (proj_lib_env_undefined && gdal.bundled) {
  gdal.setPROJSearchPath(proj_path)
}

gdal.Point.Multi = gdal.MultiPoint
gdal.LineString.Multi = gdal.MultiLineString
gdal.LinearRing.Multi = gdal.MultiLineString
gdal.Polygon.Multi = gdal.MultiPolygon

gdal.quiet()

if (typeof gdal.config === 'undefined') {
  gdal.config = {}

  /**
 * @namespace config
 */

  /**
 * Gets a GDAL configuration setting.
 *
 * @example
 *
 * data_path = gdal.config.get('GDAL_DATA');
 *
 * @static
 * @method get
 * @memberof config
 * @param {string} key
 * @return {string|null}
 */
  gdal.config.get = gdal.getConfigOption

  /**
 * Sets a GDAL configuration setting.
 *
 * @example
 *
 * gdal.config.set('GDAL_DATA', data_path);
 *
 * @static
 * @method set
 * @memberof config
 * @param {string} key
 * @param {string|null} value
 * @return {void}
 */
  gdal.config.set = gdal.setConfigOption

  delete gdal.getConfigOption
  delete gdal.setConfigOption
}

if (process.env.CURL_CA_BUNDLE === undefined && gdal.bundled) {
  gdal.config.set('CURL_CA_BUNDLE', path.resolve(__dirname, '../deps/libcurl/cacert.pem'))
}

if (fs.existsSync(grib_resource_path)) {
  gdal.config.set('GRIB_RESOURCE_DIR', grib_resource_path)
} else {
  console.warn('GRIB resources files not found in ', grib_resource_path)
}

/**
 * Callback using the standard Node.js error convention
 * @callback callback
 * @typedef {(err: Error, obj: T) => void} callback<T>
 * @param {Error} err
 * @param {any} result
 */

if (process.env.GDAL_DATA === undefined && gdal.bundled) {
  gdal.config.set('GDAL_DATA', data_path)
}

gdal.Envelope = require('./envelope.js')(gdal)
gdal.Envelope3D = require('./envelope_3d.js')(gdal)

const getEnvelope = gdal.Geometry.prototype.getEnvelope
gdal.Geometry.prototype.getEnvelope = function () {
  const obj = getEnvelope.apply(this, arguments)
  return new gdal.Envelope(obj)
}

const getEnvelope3D = gdal.Geometry.prototype.getEnvelope3D
gdal.Geometry.prototype.getEnvelope3D = function () {
  const obj = getEnvelope3D.apply(this, arguments)
  return new gdal.Envelope3D(obj)
}

const getEnvelopeAsync = gdal.Geometry.prototype.getEnvelopeAsync
gdal.Geometry.prototype.getEnvelopeAsync = function () {
  // arguments[arguments.length - 1] is the callback
  const old_cb = arguments[arguments.length - 1]
  const new_cb = (e, r) => {
    const obj = e ? undefined : new gdal.Envelope(r)
    old_cb(e, obj)
  }
  arguments[arguments.length - 1] = new_cb
  getEnvelopeAsync.apply(this, arguments)
}

const getEnvelope3DAsync = gdal.Geometry.prototype.getEnvelope3DAsync
gdal.Geometry.prototype.getEnvelope3DAsync = function () {
  const old_cb = arguments[arguments.length - 1]
  const new_cb = (e, r) => {
    const obj = e ? undefined : new gdal.Envelope3D(r)
    old_cb(e, obj)
  }
  arguments[arguments.length - 1] = new_cb
  getEnvelope3DAsync.apply(this, arguments)
}

const getExtent = gdal.Layer.prototype.getExtent
gdal.Layer.prototype.getExtent = function () {
  const obj = getExtent.apply(this, arguments)
  return new gdal.Envelope(obj)
}

const readStream = require('./readable.js')
const writeStream = require('./writable.js')
const muxStream = require('./multiplexer.js')
gdal.RasterBandPixels.prototype.createReadStream = readStream.createReadStream
gdal.RasterReadStream = readStream.RasterReadStream
gdal.RasterBandPixels.prototype.createWriteStream = writeStream.createWriteStream
gdal.RasterWriteStream = writeStream.RasterWriteStream
gdal.RasterMuxStream = muxStream.RasterMuxStream
gdal.RasterTransform = muxStream.RasterTransform

gdal.calcAsync = require('./calc')(gdal)

gdal.wrapVRT = require('./wrapVRT')

/**
 * Create a GDAL pixel function from a JS expression for one pixel.
 *
 * Higher-level API of `gdal.toPixelFunc`.
 *
 * @static
 * @method createPixelFunc
 *
 * @example
 * // This example will register a new GDAL pixel function called sum2
 * // that requires a VRT dataset with 2 values per pixel
 * gdal.addPixelFunc('sum2', gdal.createPixelFunc((a, b) => a + b));
 *
 * @param {(...sources: number[]) => void} pixelFn
 * @returns {PixelFunction}
 */
gdal.createPixelFunc = function createPixelFunc(fn) {
  if (typeof fn !== 'function') throw TypeError('pixelFn must be a function')

  const makeThunk = `
    return function(sources, buffer) {
      for (let i = 0; i < buffer.length; i++)
        buffer[i] = fn(${Array(fn.length).fill(0).map((x, i) => `sources[${i}][i]`).join(',')});
    }
  `
  const thunk = new Function('fn', makeThunk)(fn)

  return gdal.toPixelFunc(thunk)
}

/**
 * Create a GDAL pixel function from a JS expression for one pixel.
 *
 * Same as `gdal.createPixelFunc` but passes an object with the static arguments from
 * the VRT descriptor.
 *
 * @static
 * @method createPixelFuncWithArgs
 *
 * @example
 * // This example will register a new GDAL pixel function called sum2
 * // that requires a VRT dataset with 2 values per pixel
 * gdal.addPixelFunc('sum2', gdal.createPixelFuncWithArgs((args, a, b) => args.k + a + b));
 *
 * @param {(args: Record<string, string|number>,...sources: number[]) => void} pixelFn
 * @returns {PixelFunction}
 */
gdal.createPixelFuncWithArgs = function createPixelFunc(fn) {
  if (typeof fn !== 'function') throw TypeError('pixelFn must be a function')

  const makeThunk = `
    return function(sources, buffer, args) {
      for (let i = 0; i < buffer.length; i++)
        buffer[i] = fn(args, ${Array(Math.max(fn.length-1, 0)).fill(0).map((x, i) => `sources[${i}][i]`).join(',')});
    }
  `
  const thunk = new Function('fn', makeThunk)(fn)

  return gdal.toPixelFunc(thunk)
}

/**
 * @interface xyz
 * @property {number} x
 * @property {number} y
 * @property {number} [z]
 */

/**
 * @callback ProgressCb
 * @param {number} complete
 * @param {string} msg
 * @typedef {( complete: number, msg: string ) => void} ProgressCb
 */

/**
 * @typedef {object} ProgressOptions
 * @property {ProgressCb} progress_cb
 */

/**
 * Returns a `TypedArray` constructor from a GDAL data type
 *
 * @example
 *
 * const array = new (gdal.fromDataType(band.dataType))(band.size.x * band.size.y)
 * `
 *
 * @method fromDataType
 * @throws TypeError
 * @param {string|null} dataType
 * @return {new (len: number) => TypedArray}
 */
gdal.fromDataType = (() => {
  const fromDataTypeList = {}
  fromDataTypeList[gdal.GDT_Byte] = Uint8Array
  fromDataTypeList[gdal.GDT_Int16] = Int16Array
  fromDataTypeList[gdal.GDT_UInt16] = Uint16Array
  fromDataTypeList[gdal.GDT_Int32] = Int32Array
  fromDataTypeList[gdal.GDT_UInt32] = Uint32Array
  fromDataTypeList[gdal.GDT_Float32] = Float32Array
  fromDataTypeList[gdal.GDT_Float64] = Float64Array

  return (dtype) => {
    if (!fromDataTypeList[dtype]) throw new TypeError('No such GDAL type')
    return fromDataTypeList[dtype]
  }
})()

/**
 * Returns a GDAL data type from a `TypedArray`
 *
 * @example
 *
 * const dataType = gdal.fromDataType(array)
 * `
 *
 * @method toDataType
 * @throws TypeError
 * @param {TypedArray} array
 * @return {string}
 */
gdal.toDataType = (() => {
  const toDataTypeList = {}
  toDataTypeList[Uint8Array] = gdal.GDT_Byte
  toDataTypeList[Int16Array] = gdal.GDT_Float64
  toDataTypeList[Uint16Array] = gdal.GDT_UInt16
  toDataTypeList[Int32Array] = gdal.GDT_Int32
  toDataTypeList[Uint32Array] = gdal.GDT_UInt32
  toDataTypeList[Float32Array] = gdal.GDT_Float32
  toDataTypeList[Float64Array] = gdal.GDT_Float64

  return (dtype) => {
    if (!toDataTypeList[dtype.constructor]) throw new TypeError('No such GDAL type')
    return toDataTypeList[dtype.constructor]
  }
})()

/**
 * Returns a {@link Envelope|Envelope object for the raster bands}
 *
 * @example
 *
 * const extent = dataset.getEnvelope()
 * `
 *
 * @memberof DatasetBands
 * @method getEnvelope
 * @return {Envelope}
 */
gdal.DatasetBands.prototype.getEnvelope = function () {
  const ulx = this.ds.geoTransform[0]
  const uly = this.ds.geoTransform[3]
  const lrx = this.ds.geoTransform[0] + this.ds.geoTransform[1] * this.ds.rasterSize.x
  const lry = this.ds.geoTransform[3] + this.ds.geoTransform[5] * this.ds.rasterSize.y
  return new gdal.Envelope({
    minX: Math.min(ulx, lrx),
    minY: Math.min(uly, lry),
    maxX: Math.max(ulx, lrx),
    maxY: Math.max(uly, lry)
  })
}

require('./iterators.js')(gdal)

/**
 * Creates or opens a dataset. Dataset should be explicitly closed with `dataset.close()` method if opened in `"w"` mode to flush any changes. Otherwise, datasets are closed when (and if) node decides to garbage collect them.
 *
 * @example
 *
 * var dataset = gdal.open('./data.shp');
 *
 * @example
 *
 * var dataset = gdal.open(fs.readFileSync('./data.shp'));
 *
 * @throws Error
 * @method open
 * @static
 * @param {string|Buffer} path Path to dataset or in-memory Buffer to open
 * @param {string} [mode="r"] The mode to use to open the file: `"r"`, `"r+"`, or `"w"`
 * @param {string|string[]} [drivers] Driver name, or list of driver names to attempt to use.
 *
 * @param {number} [x_size] Used when creating a raster dataset with the `"w"` mode.
 * @param {number} [y_size] Used when creating a raster dataset with the `"w"` mode.
 * @param {number} [band_count] Used when creating a raster dataset with the `"w"` mode.
 * @param {string} [data_type] Used when creating a raster dataset with the `"w"` mode.
 * @param {string[]|object} [creation_options] Used when creating a dataset with the `"w"` mode.
 *
 * @return {Dataset}
 */
gdal.open = (function () {
  const open = gdal.open

  // add 'w' mode to gdal.open() method and also GDAL2-style driver selection
  return function (
    filename,
    mode,
    drivers /* , x_size, y_size, n_bands, datatype, options */
  ) {
    if (filename instanceof Buffer) {
      const buffer = filename
      arguments[0] = gdal.vsimem._anonymous(filename)
      const ds = gdal.open.apply(gdal, arguments)
      ds.buffer = buffer
      return ds
    }

    if (typeof drivers === 'string') {
      drivers = [ drivers ]
    } else if (drivers && !Array.isArray(drivers)) {
      throw new Error('driver(s) must be a string or list of strings')
    }

    if (mode && mode.includes && mode.includes('w')) {
      // create file with given driver
      if (!drivers) {
        throw new Error('Driver must be specified')
      }
      if (drivers.length !== 1) {
        throw new Error('Only one driver can be used to create a file')
      }
      const driver = gdal.drivers.get(drivers[0])
      if (!driver) {
        throw new Error(`Cannot find driver: ${drivers[0]}`)
      }

      const args = Array.prototype.slice.call(arguments, 3) // x_size, y_size, ...
      args.unshift(filename)
      return driver.create.apply(driver, args)
    }

    if (arguments.length > 2) {
      // open file with driver list
      // loop through given drivers trying to open file
      let ds
      drivers.forEach((driver_name) => {
        const driver = gdal.drivers.get(driver_name)
        if (!driver) {
          throw new Error(`Cannot find driver: ${driver_name}`)
        }
        try {
          ds = driver.open(filename, mode)
          return false
        } catch (_err) {
          /* skip driver */
        }
      })

      if (!ds) throw new Error('Error opening dataset')
      return ds
    }

    // call gdal.open() method normally
    return open.apply(gdal, arguments)
  }
})()

const promisify = require('util').promisify
const callbackify = require('util').callbackify

/**
 * Asynchronously creates or opens a dataset. Dataset should be explicitly closed with `dataset.close()` method if opened in `"w"` mode to flush any changes. Otherwise, datasets are closed when (and if) node decides to garbage collect them.
 * If the last parameter is a callback, then this callback is called on completion and undefined is returned. Otherwise the function returns a Promise resolved with the result.
 *
 * @example
 *
 * var dataset = await gdal.openAsync('./data.shp');
 *
 * @example
 *
 * var dataset = await gdal.openAsync(await fd.readFile('./data.shp'));
 *
 * @example
 *
 * gdal.openAsync('./data.shp', (err, ds) => {...});
 *
 * @method openAsync
 * @static
 * @param {string|Buffer} path Path to dataset or in-memory Buffer to open
 * @param {string} [mode="r"] The mode to use to open the file: `"r"`, `"r+"`, or `"w"`
 * @param {string|string[]} [drivers] Driver name, or list of driver names to attempt to use.
 *
 * @param {number} [x_size] Used when creating a raster dataset with the `"w"` mode.
 * @param {number} [y_size] Used when creating a raster dataset with the `"w"` mode.
 * @param {number} [band_count] Used when creating a raster dataset with the `"w"` mode.
 * @param {string} [data_type] Used when creating a raster dataset with the `"w"` mode.
 * @param {string[]|object} [creation_options] Used when creating a dataset with the `"w"` mode.
 * @param {callback<Dataset>} [callback=undefined]
 * @return {Promise<Dataset>}
 */

/**
 * TypeScript shorthand version with callback and no optional arguments
 *
 * @method openAsync
 * @static
 * @param {string|Buffer} path Path to dataset or in-memory Buffer to open
 * @param {callback<Dataset>} callback
 * @return {void}
 */

gdal.openAsync = (function () {
  const openPromise = (function () {
    const openPromise = promisify(gdal.openAsync)

    // add 'w' mode to gdal.open() method and also GDAL2-style driver selection
    return function (
      filename,
      mode,
      drivers,
      _x_size,
      _y_size,
      _n_bands,
      _datatype,
      _options
    ) {
      if (filename instanceof Buffer) {
        const buffer = filename
        try {
          // vsimem._anonymous is always synchronous
          arguments[0] = gdal.vsimem._anonymous(filename)
        } catch (e) {
          return Promise.reject(e)
        }
        return gdal.openAsync.apply(gdal, arguments).then((ds) => {
          ds.buffer = buffer
          return ds
        })
      }
      if (typeof drivers === 'string') {
        drivers = [ drivers ]
      } else if (drivers && !Array.isArray(drivers)) {
        throw new Error('driver(s) must be a string or list of strings')
      }

      if (mode === 'w') {
        // create file with given driver
        if (!drivers) {
          throw new Error('Driver must be specified')
        }
        if (drivers.length !== 1) {
          throw new Error('Only one driver can be used to create a file')
        }
        const driver = gdal.drivers.get(drivers[0])
        if (!driver) {
          throw new Error(`Cannot find driver: ${drivers[0]}`)
        }

        const args = Array.prototype.slice.call(arguments, 3) // x_size, y_size, ...
        args.unshift(filename)
        return gdal.Driver.prototype.createAsync.apply(driver, args)
      }

      if (arguments.length > 2 && drivers) {
        const p = []
        for (const driver_name of drivers) {
          const driver = gdal.drivers.get(driver_name)
          if (!driver) {
            throw new Error(`Cannot find driver: ${driver_name}`)
          }
          p.push(gdal.Driver.prototype.openAsync.call(driver, filename, mode))
        }
        // first driver to open the file wins
        // normally, there are no formats supported by two drivers
        return Promise.any(p).catch(() => {
          throw new Error('Error opening dataset')
        })
      }

      // call gdal.open() method normally
      return openPromise.call(gdal, filename, mode)
    }
  })()

  const openCb = callbackify(openPromise)
  return function (filename,
    mode,
    drivers,
    x_size,
    y_size,
    n_bands,
    datatype,
    options,
    callback) {
    if (typeof arguments[arguments.length - 1] === 'function' && callback === undefined) {
      callback = arguments[arguments.length - 1]
      arguments[arguments.length - 1] = undefined
    }
    if (callback) {
      const args = arguments
      Array.prototype.push.call(args, callback)
      return openCb.apply(gdal, args)
    }
    return openPromise.apply(gdal, arguments)
  }
})()

function fieldTypeFromValue(val) {
  const type = typeof val
  if (type === 'number') {
    if (val % 1 === 0) return gdal.OFTInteger
    return gdal.OFTReal
  } else if (type === 'string') {
    return gdal.OFTString
  } else if (type === 'boolean') {
    return gdal.OFTInteger
  } else if (val instanceof Date) {
    return gdal.OFTDateTime
  } else if (val instanceof Array) {
    const sub_type = fieldTypeFromValue(val[0])
    switch (sub_type) {
      case gdal.OFTString:
        return gdal.OFTStringList
      case gdal.OFTInteger:
        return gdal.OFTIntegerList
      case gdal.OFTReal:
        return gdal.OFTRealList
      default:
        throw new Error('Array element cannot be converted into OGRFieldType')
    }
  } else if (val instanceof Buffer) {
    return gdal.OFTBinary
  }

  throw new Error('Value cannot be converted into OGRFieldType')
}

/**
 * Creates a LayerFields instance from an object of keys and values.
 *
 * @method fromJSON
 * @memberof LayerFields
 * @param {object} object
 * @param {boolean} [approx_ok=false]
 */
gdal.LayerFields.prototype.fromJSON = (function () {
  let warned = false
  return function (obj, approx_ok) {
    if (!warned) {
      console.warn(
        'NODE-GDAL Deprecation Warning: LayerFields fromJSON() is deprecated, use fromObject() instead'
      )
      warned = true
    }
    return this.fromObject(obj, approx_ok)
  }
})()

/**
 * Creates a LayerFields instance from an object of keys and values.
 *
 * @method fromObject
 * @memberof LayerFields
 * @param {Record<string, any>} object
 * @param {boolean} [approx_ok=false]
 */
gdal.LayerFields.prototype.fromObject = function (obj, approx_ok) {
  approx_ok = approx_ok || false
  Object.entries(obj).forEach(([ k, v ]) => {
    const type = fieldTypeFromValue(v)
    const def = new gdal.FieldDefn(k, type)
    this.add(def, approx_ok)
  })
}

gdal.Point.wkbType = gdal.wkbPoint
gdal.LineString.wkbType = gdal.wkbLineString
gdal.LinearRing.wkbType = gdal.wkbLinearRing
if (gdal.CircularString) {
  gdal.CircularString.wkbType = gdal.wkbCircularString
  gdal.CompoundCurve.wkbType = gdal.wkbCompoundCurve
  gdal.MultiCurve.wkbType = gdal.wkbMultiCurve
}
gdal.Polygon.wkbType = gdal.wkbPolygon
gdal.MultiPoint.wkbType = gdal.wkbMultiPoint
gdal.MultiLineString.wkbType = gdal.wkbMultiLineString
gdal.MultiPolygon.wkbType = gdal.wkbMultiPolygon
gdal.GeometryCollection.wkbType = gdal.wkbGeometryCollection

// enable passing geometry constructors as the geometry type
gdal.DatasetLayers.prototype.create = (function () {
  const create = gdal.DatasetLayers.prototype.create
  return function (name, srs, geom_type /* , creation_options */) {
    if (arguments.length > 2 && geom_type instanceof Function) {
      if (typeof geom_type.wkbType === 'undefined') {
        throw new Error('Function must be a geometry constructor')
      }
      arguments[2] = geom_type.wkbType
    }
    return create.apply(this, arguments)
  }
})()

function getTypedArrayType(array) {
  if (array instanceof Uint8Array) {
    array._gdal_type = 1 // gdal.GDT_Byte
  } else if (array instanceof Int8Array) {
    array._gdal_type = 1 // gdal.GDT_Byte
  } else if (array instanceof Int16Array) {
    array._gdal_type = 3 // gdal.GDT_Int16
  } else if (array instanceof Uint16Array) {
    array._gdal_type = 2 // gdal.GDT_UInt16
  } else if (array instanceof Int32Array) {
    array._gdal_type = 5 // gdal.GDT_Int32
  } else if (array instanceof Uint32Array) {
    array._gdal_type = 4 // gdal.GDT_UInt32
  } else if (array instanceof BigUint64Array) {
    array._gdal_type = 12 // gdal.GDT_UInt64
  } else if (array instanceof BigInt64Array) {
    array._gdal_type = 13 // gdal.GDT_Int64
  } else if (array instanceof Float32Array) {
    array._gdal_type = 6 // gdal.GDT_Float32
  } else if (array instanceof Float64Array) {
    array._gdal_type = 7 // gdal.GDT_Float64
  } else if (array instanceof Float16Array) {
    const dv = new DataView(array.buffer)
    dv._gdal_type = 15 // gdal.GDT_Float16
    array = dv
  } else {
    array._gdal_type = 0 // gdal.GDT_Unknown
  }
  return array
}

const mangleWrite = (args) => {
  let [ x, y, width, height, data, options ] = args
  if (!options) options = {}
  if (data) data = getTypedArrayType(data)
  return [
    x,
    y,
    width,
    height,
    data,
    options.buffer_width,
    options.buffer_height,
    options.pixel_space,
    options.line_space,
    options.progress_cb,
    options.offset
  ]
}

const mangleRead = (args) => {
  let [ x, y, width, height, data, options ] = args
  if (!options) options = {}
  if (data) data = getTypedArrayType(data)
  return [
    x,
    y,
    width,
    height,
    data,
    options.buffer_width,
    options.buffer_height,
    options.type || options.data_type,
    options.pixel_space,
    options.line_space,
    options.resampling,
    options.progress_cb,
    options.offset
  ]
}

const mangleBlock = (args) => {
  if (args[2]) args[2] = getTypedArrayType(args[2])
  return args
}

const mangleMDArray = (args) => {
  if (typeof args[0] === 'object' && typeof args[0].data === 'object') {
    args[0].data = getTypedArrayType(args[0].data)
  }
  return args
}

gdal.RasterBandPixels.prototype.read = (function () {
  const read = gdal.RasterBandPixels.prototype.read
  return function () {
    return read.apply(this, mangleRead(arguments))
  }
})()

gdal.RasterBandPixels.prototype.write = (function () {
  const write = gdal.RasterBandPixels.prototype.write
  return function () {
    return write.apply(this, mangleWrite(arguments))
  }
})()

gdal.RasterBandPixels.prototype.readBlock = (function () {
  const readBlock = gdal.RasterBandPixels.prototype.readBlock
  return function (x, y, data) {
    if (data) data = getTypedArrayType(data)
    return readBlock.apply(this, arguments)
  }
})()

gdal.RasterBandPixels.prototype.writeBlock = (function () {
  const writeBlock = gdal.RasterBandPixels.prototype.writeBlock
  return function (x, y, data) {
    arguments[2] = getTypedArrayType(data)
    return writeBlock.apply(this, arguments)
  }
})()

if (gdal.MDArray) {
  gdal.MDArray.prototype.read = (function () {
    const read = gdal.MDArray.prototype.read
    return function () {
      return read.apply(this, mangleMDArray(arguments))
    }
  })()
}

const GroupCollection = {
  countAsync: 0,
  getAsync: 1
}

// Generic promisifiable methods and the argument number of their callbacks
const promisifiables = {
  Driver: {
    createAsync: 6,
    createCopyAsync: 5,
    openAsync: 2
  },
  Dataset: {
    flushAsync: 0,
    buildOverviewsAsync: 4,
    executeSQLAsync: 3,
    getMetadataAsync: 1,
    setMetadataAsync: 2
  },
  Layer: {
    flushAsync: 0
  },
  RasterBand: {
    flushAsync: 0,
    fillAsync: 2,
    computeStatisticsAsync: 1,
    getMetadataAsync: 1,
    setMetadataAsync: 2
  },
  RasterBandPixels: {
    readAsync: 13,
    writeAsync: 11,
    readBlockAsync: 3,
    writeBlockAsync: 3,
    clampBlockAsync: 2,
    getAsync: 2,
    setAsync: 3
  },
  DatasetLayers: {
    getAsync: 1,
    createAsync: 4,
    countAsync: 0,
    copyAsync: 3,
    removeAsync: 1
  },
  LayerFeatures: {
    getAsync: 1,
    setAsync: 2,
    firstAsync: 0,
    nextAsync: 0,
    addAsync: 1,
    countAsync: 1,
    removeAsync: 1
  },
  DatasetBands: {
    getAsync: 1,
    createAsync: 2,
    countAsync: 0
  },
  RasterBandOverviews: {
    getAsync: 1,
    getBySampleCountAsync: 1,
    countAsync: 0
  },
  Geometry: {
    $fromWKTAsync: 2,
    $fromWKBAsync: 2,
    $fromGeoJsonAsync: 1,
    $fromGeoJsonBufferAsync: 1,
    toKMLAsync: 0,
    toGMLAsync: 0,
    toWKTAsync: 0,
    toWKBAsync: 2,
    toJSONAsync: 0,
    centroidAsync: 0,
    convexHullAsync: 0,
    boundaryAsync: 0,
    intersectionAsync: 1,
    flattenTo2DAsync: 0,
    unionAsync: 1,
    differenceAsync: 1,
    symDifferenceAsync: 1,
    simplifyAsync: 1,
    simplifyPreserveTopologyAsync: 1,
    bufferAsync: 2,
    getEnvelopeAsync: 0,
    getEnvelope3DAsync: 0,
    closeRingsAsync: 0,
    emptyAsync: 0,
    swapXYAsync: 0,
    isEmptyAsync: 0,
    isValidAsync: 0,
    makeValidAsync: 0,
    isSimpleAsync: 0,
    isRingAsync: 0,
    intersectsAsync: 1,
    equalsAsync: 1,
    disjointAsync: 1,
    touchesAsync: 1,
    crossesAsync: 1,
    withinAsync: 1,
    containsAsync: 1,
    overlapsAsync: 1,
    distanceAsync: 1,
    transformAsync: 1,
    transformToAsync: 1
  },
  SpatialReference: {
    $fromURLAsync: 1,
    $fromCRSURLAsync: 1,
    $fromUserInputAsync: 1
  },
  MDArray: {
    readAsync: 1
  },
  fs: {
    $statAsync: 2,
    $readDirAsync: 1
  },
  GroupArrays: GroupCollection,
  GroupDimensions: GroupCollection,
  GroupAttributes: GroupCollection,
  GroupGroups: GroupCollection,
  ArrayDimensions: GroupCollection,
  ArrayAttributes: GroupCollection,
  $: {
    $fillNodataAsync: 1,
    $contourGenerateAsync: 1,
    $sieveFilterAsync: 1,
    $checksumImageAsync: 5,
    $polygonizeAsync: 1,
    $reprojectImageAsync: 1,
    $suggestedWarpOutputAsync: 1,
    $translateAsync: 4,
    $vectorTranslateAsync: 4,
    $infoAsync: 2,
    $warpAsync: 5,
    $buildVRTAsync: 4,
    $rasterizeAsync: 4,
    $demAsync: 6,
    $_acquireLocksAsync: 3
  }
}

const argMangle = {
  RasterBandPixels: {
    readAsync: mangleRead,
    writeAsync: mangleWrite,
    readBlockAsync: mangleBlock,
    writeBlockAsync: mangleBlock
  },
  MDArray: {
    readAsync: mangleMDArray
  }
}

// For each *Async function create a function that checks if the last parameter is a callback
// Then call either the original, either the promisified version with the callback
// placed at the right argument number since the C++ code does not support floating callbacks
for (const c of Object.keys(promisifiables)) {
  const klass = c === '$' ? gdal : gdal[c]
  if (klass === undefined) {
    continue
  }
  for (const _m of Object.keys(promisifiables[c])) {
    const { base, m } = _m.startsWith('$') ?
      { base: klass, m: _m.slice(1) } :
      { base: klass.prototype, m: _m }
    if (base[m] === undefined) {
      continue
    }
    base[m] = (function () {
      const promisified = promisify(base[m])
      const original = base[m]
      const cbArg = promisifiables[c][_m]
      const mangle = argMangle[c] && argMangle[c][_m] ? argMangle[c][_m] : (a) => a
      return function () {
        let callback
        if (typeof arguments[arguments.length - 1] === 'function') {
          callback = arguments[arguments.length - 1]
          arguments[arguments.length - 1] = undefined
        }
        let args = Array.prototype.slice.call(mangle(arguments), 0, cbArg)
        if (callback) {
          args[cbArg] = callback
          return original.apply(this, args)
        }
        args = Object.assign(new Array(cbArg).fill(undefined), args)
        return promisified.apply(this, args)
      }
    })()
  }
}
