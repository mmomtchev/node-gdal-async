const { Readable } = require('stream')

const debug = process.env.NODE_DEBUG && process.env.NODE_DEBUG.match(/gdal_read|gdal([^_]|$)/) ?
  console.debug.bind(console, 'RasterReadStream:') :
  () => undefined

/**
 * @interface RasterReadableOptions
 * @extends stream.ReadableOptions
 * @property {boolean} [blockOptimize]
 * @property {boolean} [convertNoData]
 * @property {new (len: number) => TypedArray} [type]
 */

/**
 * create a Readable stream from a raster band
 *
 * @memberof RasterBandPixels
 * @instance
 * @method createReadStream
 * @param {RasterReadableOptions} [options]
 * @param {boolean} [options.blockOptimize=true] Read by file blocks when possible (when `rasterSize.x == blockSize.x`)
 * @param {boolean} [options.convertNoData=true] Automatically convert `RasterBand.noDataValue` to `NaN`
 * @param {new (len: number) => TypedArray} [options.readAs=undefined] Data type to convert to, must be a `TypedArray` constructor
 * @returns {RasterReadStream}
 */
function createReadStream(options) {
  const readable = new RasterReadStream({ ...options ||{}, band: this.band })
  return readable
}

/**
 * Class implementing {@link RasterBand} reading as a stream of pixels}
 *
 * Reading is buffered and it is aligned on the underlying
 * compression blocks for maximum efficiency when possible
 *
 * Pixels are streamed in row-major order
 *
 * @class RasterReadStream
 * @extends stream.Readable
 * @constructor
 * @param {RasterReadableOptions} [options]
 * @param {RasterBand} options.band RasterBand to use
 * @param {boolean} [options.blockOptimize=true] Read by file blocks when possible (when `rasterSize.x == blockSize.x`)
 * @param {boolean} [options.convertNoData=false] Automatically convert `RasterBand.noDataValue` to `NaN`, requires float data types
 * @param {new (len: number) => TypedArray} [options.type=undefined] Data type to convert to, must be a `TypedArray` constructor, default is the raster band data type
 */
class RasterReadStream extends Readable {
  constructor(options) {
    super({ ...options, objectMode: true })
    this.band = options.band
    this.readingPos = 0
    this.blockPos = 0
    this.readingInProgress = false
    this.rasterEnded = false

    if (typeof options.type !== 'undefined') {
      try {
        new options.type(0)
      } catch (e) {
        throw new TypeError(`"type" must be a constructor: ${e}`, { cause: e })
      }
    }

    if (!options.band.pixels) {
      throw new TypeError('"band" must be a gdal.RasterBand')
    }

    // This part is an ideal candidate for Node 16 _construct,
    // but alas our baseline is Node 12 so some rather
    // cumbersome acrobatics are needed
    this.initQ = Promise.all([ this.band.blockSizeAsync, this.band.sizeAsync, this.band.noDataValueAsync ])
      .then(([ blockSize, rasterSize, noDataValue ]) => {
        this.blockSize = blockSize
        this.rasterSize = rasterSize
        if (options.convertNoData && noDataValue !== null) {
          this._convertNoData = this._doConvertNoData.bind(this, noDataValue)
        } else {
          this._convertNoData = () => undefined
        }
        if (blockSize.x == rasterSize.x && options.blockOptimize !== false) {
          debug('init done, optimized block read', blockSize, rasterSize)
          this._readNextBuffer = RasterReadStream.prototype._readNextBlock
          if (options.type) {
            this.arrayConstructor = () => new options.type(blockSize.x * blockSize.y)
          }
          return
        }
        debug('init done, line by line read', blockSize, rasterSize)
        this._readNextBuffer = RasterReadStream.prototype._readNextLine
        if (options.type) {
          this.arrayConstructor = () => new options.type(rasterSize.x)
        }
      })
  }
}

RasterReadStream.prototype._doConvertNoData = function (noData, buffer) {
  for (let i = 0; i < buffer.length; i++) {
    if (buffer[i] === noData) buffer[i] = NaN
  }
}

RasterReadStream.prototype._readNext = function () {
  debug('reading next block', this.readingPos, this.readingInProgress)
  if (this.readingInProgress || this.rasterEnded) return
  this.readingInProgress = true
  this.initQ.then(() => {
    debug('do read')
    this._readNextBuffer()
      .then((data) => {
        this.readingInProgress = false
        this._convertNoData(data)

        debug('adding a new buffer', data.length)
        const flowing = this.push(data)
        if (this.readingPos == this.rasterSize.y) {
          debug('raster ended at ', this.readingPos)
          this.rasterEnded = true
          this.push(null)
          return
        }
        if (flowing) {
          this._readNext()
        } else {
          debug('push buffer is full')
        }
      })
      .catch((e) => {
        debug('emitting error', e)
        this.destroy(e)
      })
  })
}

// Optimized reading when horizontally there is only one block (blockSize.x == rasterSize.x)
// This is more often the case than not
RasterReadStream.prototype._readNextBlock = function () {
  const actualSize = this.readingPos + this.blockSize.y > this.rasterSize.y ?
    this.rasterSize.y - this.readingPos :
    this.blockSize.y
  const array = this.arrayConstructor ? this.arrayConstructor() : undefined
  const dataq = this.band.pixels.readBlockAsync(0, this.blockPos, array)

  return dataq
    .then((data) => {
      this.readingPos += actualSize
      this.blockPos++
      // Edge blocks, need to be clamped as the data is smaller than the block
      if (actualSize != this.blockSize.y) {
        debug('clamping', this.blockSize, actualSize)
        return data.subarray(0, actualSize * this.rasterSize.x)
      }
      return data
    })
}

// Line by line reading, in this case we are better off with
// GDAL's own block cache handling the block reading
// If GDAL can fit all the blocks from one horizontal line in its cache
// this case will be almost as fast as the previous one
RasterReadStream.prototype._readNextLine = function () {
  let array
  try {
    array = this.arrayConstructor ? this.arrayConstructor() : undefined
  } catch (e) {
    console.error(e)
  }
  return this.band.pixels.readAsync(0, this.blockPos, this.rasterSize.x, 1, array)
    .then((data) => {
      this.readingPos++
      this.blockPos++
      return data
    })
}

RasterReadStream.prototype._read = function () {
  this._readNext()
}

module.exports = {
  createReadStream,
  RasterReadStream
}
