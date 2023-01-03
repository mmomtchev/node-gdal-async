const { Readable, Transform } = require('stream')

const debug = process.env.NODE_DEBUG && process.env.NODE_DEBUG.match(/gdal_mux|gdal([^_]|$)/) ?
  console.debug.bind(console, 'RasterMuxStream:') :
  () => undefined


/**
 * Multiplexer stream
 *
 * Reads multiple input {@link RasterReadStream} streams and outputs a single}
 * synchronized stream with multiple data elements.
 *
 * All the input streams must have the same length.
 *
 * Can be used with {@link RasterTransform}
 * which will automatically apply a function over the whole chunk.
 *
 * @example
 *
 *  const dsT2m = gdal.open('AROME_T2m_10.tiff'));
 *  const dsD2m = gdal.open('AROME_D2m_10.tiff'));
 *
 *  const dsCloudBase = gdal.open('CLOUDBASE.tiff', 'w', 'GTiff',
 *    dsT2m.rasterSize.x, dsT2m.rasterSize.y, 1, gdal.GDT_Float64);
 *
 *  const mux = new gdal.RasterMuxStream({
 *    T2m: dsT2m.bands.get(1).pixels.createReadStream(),
 *    D2m: dsD2m.bands.get(1).pixels.createReadStream()
 *  });
 *  const ws = dsCloudBase.bands.get(1).pixels.createWriteStream();
 *
 *  // Espy's estimation for cloud base height (lifted condensation level)
 *  // LCL = 125 * (T2m - Td2m)
 *  // where T2m is the temperature at 2m and Td2m is the dew point at 2m
 *  const espyEstimation = new Transform({
 *    objectMode: true,
 *    transform(chunk, _, cb) {
 *      const lcl = new Float64Array(chunk.T2m.length)
 *      for (let i = 0; i < chunk.T2m.length; i++) {
 *        lcl[i] = 125 * (chunk.T2m[i] - chunk.D2m[i])
 *      }
 *      cb(null, lcl)
 *    }
 *  })
 *
 *  mux.pipe(espyEstimation).pipe(ws);
 *
 *
 * @class RasterMuxStream
 * @extends stream.Readable
 * @constructor
 * @param {Record<string,RasterReadStream>} inputs Input streams
 * @param {RasterReadableOptions} [options]
 * @param {boolean} [options.blockOptimize=true] Read by file blocks when possible (when rasterSize.x == blockSize.x)
 */
class RasterMuxStream extends Readable {
  constructor(inputs, options) {
    super({ ...options, objectMode: true })
    this.inputs = {}
    this.buffers = {}
    this.buffersTotalData = {}
    this.ids = Object.keys(inputs)
    this.dataHandlers = {}
    this.endHandlers = {}
    this.blockOptimize = (options || {}).blockOptimize
    this.rasterHighWaterMark = Infinity

    for (const id of this.ids) {
      const inp = inputs[id]
      if (!(inp instanceof Readable)) throw new TypeError('inputs must be a map of Readables')
      if (!inp.readableObjectMode) throw new TypeError('All inputs must be in object mode')

      this.inputs[id] = inp
      this.buffers[id] = []
      this.buffersTotalData[id] = 0
      this.dataHandlers[id] = this.handleIncoming.bind(this, id)

      this.inputs[id].pause()
      this.inputs[id].on('data', this.dataHandlers[id])

      this.endHandlers[id] = this.inputEnded.bind(this, id)
      this.inputs[id].on('end', this.endHandlers[id])

      this.inputs[id].on('error', this.errorHandler.bind(this))
    }
  }

  handleIncoming(recv, chunk) {
    debug('received on', recv, 'chunk', 'chunk.length')
    this.buffers[recv].push(chunk)
    this.buffersTotalData[recv] += chunk.length

    const maxReady = Math.min.apply(Math, this.ids.map((id) => this.buffersTotalData[id]))
    if (maxReady == 0) {
      return
    }

    debug('sending', maxReady)
    const send = {}
    for (const id of this.ids) {
      if (this.buffers[id][0] === null) {
        // one of the inputs ended before the others
        debug('destroy on premature end', id)
        this.destroy(`premature end on ${id}`)
        return
      } else if (this.buffers[id][0].length == maxReady && this.blockOptimize !== false) {
        // zero-copy mode, this input blocks are same size as the output
        debug('zero-copy from', id)
        send[id] = this.buffers[id][0]
        this.buffers[id].shift()
      } else {
        // block consolidation mode
        debug('block consolidation from', id, this.buffers[id].map((buf) => buf && buf.length || 'null').join(','))
        send[id] = new this.buffers[id][0].constructor(maxReady)
        let len = 0
        while (len + this.buffers[id][0].length < maxReady) {
          debug('chunking')
          send[id].set(this.buffers[0], len)
          len += this.buffers[id][0].length
          this.buffers[id].shift()
          if (this.buffers[id][0] === null) {
            // one of the inputs ended before the others
            debug('destroy on premature end', id)
            this.destroy('premature end on', id)
            return
          }
        }
        debug('final chunk', maxReady - len, this.buffers[id][0].length)
        send[id].set(this.buffers[id][0].subarray(0, maxReady - len), len)
        if (maxReady - len < this.buffers[id][0].length) {
          this.buffers[id][0] = this.buffers[id][0].subarray(maxReady - len)
        } else {
          this.buffers[id].shift()
        }
      }
      this.buffersTotalData[id] -= maxReady
    }

    // send the consolidated result and eventually start pausing
    const flowing = this.push(send)

    if (this.tryEnd()) return

    this.rasterHighWaterMark = maxReady * this.readableHighWaterMark
    this.throttle(flowing)
  }

  throttle(flowing) {
    //debug('trying to throttle', flowing, this.rasterHighWaterMark, this.ids.map((id) => this.buffersTotalData[id]))
    let id
    for (id of this.ids) {
      if (!flowing || this.buffersTotalData[id] >= this.rasterHighWaterMark) this.inputs[id].pause()
      else if (flowing) this.inputs[id].resume()
    }
    //debug('after throttle', this.ids.map((id) => this.isRunning[id]))
  }

  tryEnd() {
    if (this.buffers[this.ids[0]][0] === null) {
      const max = Math.max.apply(Math, this.ids.map((id) => this.buffersTotalData[id]))
      if (max > 0) {
        debug('destroy on premature end', this.ids[0])
        this.destroy(`premature end on ${this.ids[0]}`)
        return true
      }
      // this is the normal end
      debug('ended')
      this.push(null)
      for (const id of this.ids) {
        this.inputs[id].off('data', this.dataHandlers[id])
        this.inputs[id].off('end', this.endHandlers[id])
      }
      return true
    }

    return false
  }

  inputEnded(id) {
    debug('id', 'ended', this.ids.map((id) => `${id}:${this.buffersTotalData[id]}`))
    for (const other of this.ids) {
      const eos = this.buffers[other].findIndex((buf) => buf === null)
      let first
      if (eos >= 0 && eos != this.buffers[id].length) {
        if (eos < this.buffers[other].length) {
          first = other
        } else {
          first = id
        }
        debug('destroy on premature end', first)
        this.destroy(`premature end on ${first}`)
        return
      }
    }
    this.buffers[id].push(null)
    this.tryEnd()
  }

  errorHandler(e) {
    debug('emitting error', e)
    this.destroy(e)
  }

  _read() {
    debug('readStart')
    this.throttle(true)
  }
}

/**
 * @interface RasterTransformOptions
 * @extends stream.TransformOptions
 * @property {Function} fn Function to be applied on all data
 * @property {new (len: number) => TypedArray} type Typed array constructor
 */

/**
 * A raster Transform stream
 *
 * Applies a function on all data elements.
 *
 * Input must be a {@link RasterMuxStream}
 *
 * {@link calcAsync} provides a higher-level interface for the same feature, while
 * an even lower-level API is available by manually extending `stream.Transform` as illustrated
 * in the {@link gdal.RasterMuxStream} example.
 *
 * @example
 *  const dsT2m = gdal.open('AROME_T2m_10.tiff'));
 *  const dsD2m = gdal.open('AROME_D2m_10.tiff'));
 *
 *  const dsCloudBase = gdal.open('CLOUDBASE.tiff', 'w', 'GTiff',
 *    dsT2m.rasterSize.x, dsT2m.rasterSize.y, 1, gdal.GDT_Float64);
 *
 *  const mux = new gdal.RasterMuxStream({
 *    T2m: dsT2m.bands.get(1).pixels.createReadStream(),
 *    D2m: dsD2m.bands.get(1).pixels.createReadStream()
 *  });
 *  const ws = dsCloudBase.bands.get(1).pixels.createWriteStream();
 *
 *  // Espy's estimation for cloud base height (lifted condensation level)
 *  // LCL = 125 * (T2m - Td2m)
 *  // where T2m is the temperature at 2m and Td2m is the dew point at 2m
 *  const fn = (t,td) => 125 * (t - td);
 *  const espyEstimation = new RasterTransform({ type: Float64Array, fn });
 *
 *  mux.pipe(espyEstimation).pipe(ws);
 *
 * @class RasterTransform
 * @extends stream.Transform
 * @constructor
 * @param {RasterTransformOptions} [options]
 * @param {Function} options.fn Function to be applied on all data
 * @param {new (len: number) => TypedArray} options.type Typed array constructor
 */
class RasterTransform extends Transform {
  constructor(opts) {
    super({ ...opts, objectMode: true })
    this.type = opts.type
    this.fn = opts.fn
  }

  _transform(chunk, _, cb) {
    if (!this.xform) {
      const thunk = `
        for (let i = 0; i < len; i++)
          out[i] = this.fn(${Object.keys(chunk).map((key) => `inp.${key}[i]`).join(',')})`
      this.xform = new Function('inp', 'out', 'len', thunk)
    }

    const len = chunk[Object.keys(chunk)[0]].length
    const out = new this.type(len)
    try {
      this.xform(chunk, out, len)
      cb(null, out)
    } catch (err) {
      cb(err)
    }
  }
}

module.exports = {
  RasterMuxStream,
  RasterTransform
}
