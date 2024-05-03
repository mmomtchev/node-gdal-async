import * as gdal from 'gdal-async'
import * as chai from 'chai'
import * as path from 'path'
import * as semver from 'semver'
const assert: Chai.Assert = chai.assert
import * as chaiAsPromised from 'chai-as-promised'
chai.use(chaiAsPromised)

describe('gdal', () => {
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  describe('contourGenerate()', () => {
    let src: gdal.Dataset, srcband: gdal.RasterBand, dst: gdal.Dataset, lyr: gdal.Layer

    before(() => {
      // create a simple ramp in memory
      const w = 64
      const h = 64
      src = gdal.open('temp', 'w', 'MEM', w, h, 1)
      srcband = src.bands.get(1)
      for (let y = 0; y < h; y++) {
        const buf = Buffer.alloc(w)
        buf.fill(y * 4)
        srcband.pixels.write(0, y, w, 1, new Uint8Array(buf))
      }
    })
    after(() => {
      src.close()
    })
    beforeEach(() => {
      dst = gdal.open('temp', 'w', 'Memory')

      lyr = dst.layers.create('temp', null, gdal.LineString)
      lyr.fields.add(new gdal.FieldDefn('id', gdal.OFTInteger))
      lyr.fields.add(new gdal.FieldDefn('elev', gdal.OFTReal))
    })
    afterEach(() => {
      try {
        dst.close()
      } catch (err) {
        /* ignore */
      }
    })
    it('should generate contours when passed an interval / base', () => {
      const offset = 7
      const interval = 32

      gdal.contourGenerate({
        src: srcband,
        dst: lyr,
        offset: offset,
        interval: interval,
        idField: 0,
        elevField: 1
      })

      assert.isTrue(lyr.features.count() > 0, 'features were created')

      lyr.features.forEach((feature) => {
        assert.isTrue(
          (feature.fields.get('elev') - offset) % interval === 0,
          'contour used correct interval / base'
        )
        assert.isFalse(feature.getGeometry().isEmpty())
      })
    })
    it('should accept an array of fixed levels', () => {
      const order = (a: number, b: number) => a - b
      const levels = [ 53, 43, 193 ].sort(order)

      gdal.contourGenerate({
        src: srcband,
        dst: lyr,
        fixedLevels: levels,
        idField: 0,
        elevField: 1
      })

      assert.isTrue(lyr.features.count() > 0, 'features were created')

      const actual_levels = [] as number[]

      lyr.features.forEach((feature) => {
        const elev = feature.fields.get('elev')
        assert.include(
          levels,
          elev,
          'contour elevation in array of fixed levels'
        )
        assert.isFalse(feature.getGeometry().isEmpty())
        if (actual_levels.indexOf(elev) === -1) actual_levels.push(elev)
      })

      assert.deepEqual(levels, actual_levels.sort(order), 'all fixed levels used')
    })
    it('should accept a "progress_cb"', () => {
      const offset = 7
      const interval = 32

      let calls = 0
      gdal.contourGenerate({
        src: srcband,
        dst: lyr,
        offset: offset,
        interval: interval,
        idField: 0,
        elevField: 1,
        progress_cb: () => {
          calls++
        }
      })

      assert.isAbove(calls, 0)
    })
  })
  describe('contourGenerateAsync()', () => {
    let src: gdal.Dataset, srcband: gdal.RasterBand, dst: gdal.Dataset, lyr: gdal.Layer

    before(() => {
      // create a simple ramp in memory
      const w = 64
      const h = 64
      src = gdal.open('temp', 'w', 'MEM', w, h, 1)
      srcband = src.bands.get(1)
      for (let y = 0; y < h; y++) {
        const buf = Buffer.alloc(w)
        buf.fill(y * 4)
        srcband.pixels.write(0, y, w, 1, new Uint8Array(buf))
      }
    })
    beforeEach(() => {
      dst = gdal.open('temp', 'w', 'Memory')

      lyr = dst.layers.create('temp', null, gdal.LineString)
      lyr.fields.add(new gdal.FieldDefn('id', gdal.OFTInteger))
      lyr.fields.add(new gdal.FieldDefn('elev', gdal.OFTReal))
    })
    it('should generate contours when passed an interval / base', () => {
      const offset = 7
      const interval = 32

      const p = gdal.contourGenerateAsync({
        src: srcband,
        dst: lyr,
        offset: offset,
        interval: interval,
        idField: 0,
        elevField: 1
      })

      return assert.eventually.isTrue(p.then(() => lyr.features.count() > 0), 'features were created')
    })
  })
  describe('fillNodata()', () => {
    let src: gdal.Dataset, srcband: gdal.RasterBand
    const holes_x = [ 53, 61, 61, 1, 43, 44, 5 ]
    const holes_y = [ 11, 5, 6, 33, 22, 11, 0 ]
    const nodata = 33

    beforeEach(() => {
      const w = 64
      const h = 64

      src = gdal.open('temp', 'w', 'MEM', w, h, 1)
      srcband = src.bands.get(1)
      srcband.noDataValue = nodata

      // sprinkle a solid fill with nodata
      const buf = Buffer.alloc(w * h)
      buf.fill(128)
      srcband.pixels.write(0, 0, w, h, new Uint8Array(buf))
      for (let i = 0; i < holes_x.length; i++) {
        srcband.pixels.set(holes_x[i], holes_y[i], nodata)
        assert.equal(srcband.pixels.get(holes_x[i], holes_y[i]), nodata)
      }
    })
    afterEach(() => {
      try {
        src.close()
      } catch (err) {
        /* ignore */
      }
    })
    it('should fill nodata values', () => {
      gdal.fillNodata({
        src: srcband,
        searchDist: 3,
        smoothingIterations: 2
      })

      for (let i = 0; i < holes_x.length; i++) {
        assert.notEqual(srcband.pixels.get(holes_x[i], holes_y[i]), nodata)
      }
    })
  })
  describe('fillNodataAsync()', () => {
    let src: gdal.Dataset, srcband: gdal.RasterBand
    const holes_x = [ 53, 61, 61, 1, 43, 44, 5 ]
    const holes_y = [ 11, 5, 6, 33, 22, 11, 0 ]
    const nodata = 33

    beforeEach(() => {
      const w = 64
      const h = 64

      src = gdal.open('temp', 'w', 'MEM', w, h, 1)
      srcband = src.bands.get(1)
      srcband.noDataValue = nodata

      // sprinkle a solid fill with nodata
      const buf = Buffer.alloc(w * h)
      buf.fill(128)
      srcband.pixels.write(0, 0, w, h, new Uint8Array(buf))
      for (let i = 0; i < holes_x.length; i++) {
        srcband.pixels.set(holes_x[i], holes_y[i], nodata)
        assert.equal(srcband.pixels.get(holes_x[i], holes_y[i]), nodata)
      }
    })
    it('should fill nodata values', () => {
      const p = gdal.fillNodataAsync({
        src: srcband,
        searchDist: 3,
        smoothingIterations: 2
      })

      const r = []
      for (let i = 0; i < holes_x.length; i++) {
        r.push(assert.eventually.notEqual(p.then(() => srcband.pixels.get(holes_x[i], holes_y[i])), nodata))
      }
      return assert.isFulfilled(Promise.all(r))
    })
  })

  describe('checksumImage()', () => {
    let src: gdal.Dataset, band: gdal.RasterBand
    const w = 16
    const h = 16
    beforeEach(() => {
      src = gdal.open('temp', 'w', 'MEM', w, h, 1)
      band = src.bands.get(1)
    })
    afterEach(() => {
      try {
        src.close()
      } catch (err) {
        /* ignore */
      }
    })
    it('should generate unique checksum for the given region', () => {
      for (let x = 0; x < w; x++) {
        for (let y = 0; y < h; y++) {
          band.pixels.set(x, y, (x * h + y) % 255)
        }
      }
      band.pixels.set(4, 4, 25)
      const a = gdal.checksumImage(band)
      band.pixels.set(4, 4, 93)
      const b = gdal.checksumImage(band)
      const c = gdal.checksumImage(band, 8, 0, w / 2, h)

      assert.notEqual(a, b)
      assert.notEqual(b, c)
    })
  })
  describe('checksumImageAsync()', () => {
    let src: gdal.Dataset, band: gdal.RasterBand
    const w = 16
    const h = 16
    beforeEach(() => {
      src = gdal.open('temp', 'w', 'MEM', w, h, 1)
      band = src.bands.get(1)
    })
    it('should generate unique checksum for the given region', () => {
      for (let x = 0; x < w; x++) {
        for (let y = 0; y < h; y++) {
          band.pixels.set(x, y, (x * h + y) % 255)
        }
      }
      band.pixels.set(4, 4, 25)
      const a = gdal.checksumImageAsync(band)
      band.pixels.set(4, 4, 93)
      const b = gdal.checksumImageAsync(band)
      const c = gdal.checksumImageAsync(band, 8, 0, w / 2, h)

      return assert.isFulfilled(Promise.all([ assert.eventually.notEqual(a, b), assert.eventually.notEqual(b, c) ]))
    })
  })

  describe('sieveFilter()', () => {
    let src: gdal.Dataset, band: gdal.RasterBand
    const w = 64
    const h = 64
    beforeEach(() => {
      src = gdal.open('temp', 'w', 'MEM', w, h, 1)
      band = src.bands.get(1)

      // create two rectangles next to eachother of differing sizes
      const small_buffer = Buffer.alloc(4 * 4)
      small_buffer.fill(10)
      const big_buffer = Buffer.alloc(32 * 32)
      big_buffer.fill(20)

      band.pixels.write(5, 5, 32, 32, new Uint8Array(big_buffer))
      band.pixels.write(7, 7, 4, 4, new Uint8Array(small_buffer))
    })
    afterEach(() => {
      try {
        src.close()
      } catch (err) {
        /* ignore */
      }
    })
    it('should fill smaller polygons with value from neighboring bigger polygon', () => {
      assert.equal(band.pixels.get(8, 8), 10)

      gdal.sieveFilter({
        src: band,
        dst: band, // in place
        threshold: 4 * 4 + 1,
        connectedness: 8
      })

      assert.equal(band.pixels.get(8, 8), 20)
    })
    it('accept "progress_cb"', () => {
      assert.equal(band.pixels.get(8, 8), 10)

      let calls = 0
      gdal.sieveFilter({
        src: band,
        dst: band, // in place
        threshold: 4 * 4 + 1,
        connectedness: 8,
        progress_cb: () => {
          calls++
        }
      })
      assert.isAbove(calls, 0)

      assert.equal(band.pixels.get(8, 8), 20)
    })
  })
  describe('sieveFilterAsync()', () => {
    let src: gdal.Dataset, band: gdal.RasterBand
    const w = 64
    const h = 64
    beforeEach(() => {
      src = gdal.open('temp', 'w', 'MEM', w, h, 1)
      band = src.bands.get(1)

      // create two rectangles next to eachother of differing sizes
      const small_buffer = Buffer.alloc(4 * 4)
      small_buffer.fill(10)
      const big_buffer = Buffer.alloc(32 * 32)
      big_buffer.fill(20)

      band.pixels.write(5, 5, 32, 32, new Uint8Array(big_buffer))
      band.pixels.write(7, 7, 4, 4, new Uint8Array(small_buffer))
    })
    it('should fill smaller polygons with value from neighboring bigger polygon', () => {
      assert.equal(band.pixels.get(8, 8), 10)

      const p = gdal.sieveFilterAsync({
        src: band,
        dst: band, // in place
        threshold: 4 * 4 + 1,
        connectedness: 8
      })

      return assert.eventually.equal(p.then(() => band.pixels.get(8, 8)), 20)
    })
  })
  describe('polygonize()', () => {
    let src: gdal.Dataset, srcband: gdal.RasterBand, dst: gdal.Dataset, lyr: gdal.Layer

    before(() => {
      // create two identical rectangles
      const w = 64
      const h = 64
      src = gdal.open('temp', 'w', 'MEM', w, h, 1)
      srcband = src.bands.get(1)
      for (let y = 0; y < h; y++) {
        const buf = Buffer.alloc(w)
        buf.fill(y & 32)
        srcband.pixels.write(0, y, w, 1, new Uint8Array(buf))
      }
    })
    after(() => {
      try {
        src.close()
      } catch (err) {
        /* ignore */
      }
    })
    beforeEach(() => {
      dst = gdal.open('temp', 'w', 'Memory')
      lyr = dst.layers.create('temp', null, gdal.Polygon)
      lyr.fields.add(new gdal.FieldDefn('val', gdal.OFTInteger))
    })
    afterEach(() => {
      try {
        dst.close()
      } catch (err) {
        /* ignore */
      }
    })
    it('should generate polygons from a RasterBand', () => {
      gdal.polygonize({
        src: srcband,
        dst: lyr,
        pixValField: 0,
        connectedness: 8
      })

      assert.equal(lyr.features.count(), 2)
      lyr.features.forEach((f) => {
        const geom = f.getGeometry()
        assert.isFalse(geom.isEmpty())
        assert.instanceOf(geom, gdal.Polygon)
      })
    })
    it('should accept a "progress_cb"', () => {
      let calls = 0
      gdal.polygonize({
        src: srcband,
        dst: lyr,
        pixValField: 0,
        connectedness: 8,
        progress_cb: () => {
          calls++
        }
      })
      assert.isAbove(calls, 0)
    })
  })

  describe('addPixelFunc()', () => {
    it('should throw with invalid arguments', () => {
      assert.throws(() => {
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        (gdal.addPixelFunc as any)(1, 2)
      }, /name must be a string/)
      assert.throws(() => {
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        (gdal.addPixelFunc as any)('func', 2)
      }, /pixelFn must be an object/)
      assert.throws(() => {
        gdal.addPixelFunc('func', new Uint8Array(48))
      }, /pixelFn must be a native code pixel function/)
    })
  })

  describe('toPixelFunc()', () => {
    let band1: gdal.RasterBand, band2: gdal.RasterBand
    before(() => {
      band1 = gdal.open(path.resolve(__dirname, 'data', 'AROME_T2m_10.tiff')).bands.get(1)
      band2 = gdal.open(path.resolve(__dirname, 'data', 'AROME_D2m_10.tiff')).bands.get(1)
    })

    it('should produce a binary pixel function from a JS function', function () {
      if (!semver.gte(gdal.version, '3.5.0-git')) this.skip()
      const sum2 = (sources: gdal.TypedArray[], buffer: gdal.TypedArray, args: Record<string, string|number>) => {
        assert.equal(sources.length, 2)
        assert.instanceOf(sources[0], Float64Array)
        assert.instanceOf(sources[1], Float64Array)
        assert.instanceOf(buffer, Float64Array)
        assert.isEmpty(args)
        for (let i = 0; i < buffer.length; i++) {
          buffer[i] = sources[0][i] + sources[1][i] + 1
        }
      }
      gdal.addPixelFunc('sum2', gdal.toPixelFunc(sum2))

      const vrt = gdal.wrapVRT({
        bands: [
          {
            sources: [ band1, band2 ],
            pixelFunc: 'sum2'
          }
        ]
      })
      const ds = gdal.open(vrt)

      assert.equal(ds.bands.count(), 1)
      const input1 = band1.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const input2 = band2.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const q = ds.bands.get(1).pixels.readAsync(0, 0, ds.rasterSize.x, ds.rasterSize.y).then((result) => {
        for (let i = 0; i < ds.rasterSize.x * ds.rasterSize.y; i += 256) {
          assert.closeTo(result[i], input1[i] + input2[i] + 1, 1e-6)
        }
      })
      return assert.isFulfilled(q)
    })

    it('should propagate exceptions to the calling code', function () {
      if (!semver.gte(gdal.version, '3.5.0-git')) this.skip()
      const fail = () => {
        throw new Error('pixel function failed')
      }
      gdal.addPixelFunc('fail', gdal.toPixelFunc(fail))

      const vrt = gdal.wrapVRT({
        bands: [
          {
            sources: [ band1, band2 ],
            pixelFunc: 'fail'
          }
        ]
      })
      const ds = gdal.open(vrt)

      return assert.isRejected(ds.bands.get(1).pixels.readAsync(0, 0, ds.rasterSize.x, ds.rasterSize.y),
        /pixel function failed/)
    })

    it('should support being called synchronously', function () {
      if (!semver.gte(gdal.version, '3.5.0-git')) this.skip()
      const sync = (sources: gdal.TypedArray[], buffer: gdal.TypedArray) => {
        for (let i = 0; i < buffer.length; i++) {
          buffer[i] = sources[0][i] + sources[1][i] + 2
        }
      }
      gdal.addPixelFunc('sync', gdal.toPixelFunc(sync))

      const vrt = gdal.wrapVRT({
        bands: [
          {
            sources: [ band1, band2 ],
            pixelFunc: 'sync'
          }
        ]
      })
      const ds = gdal.open(vrt)

      assert.equal(ds.bands.count(), 1)
      const input1 = band1.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const input2 = band2.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const result = ds.bands.get(1).pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      for (let i = 0; i < ds.rasterSize.x * ds.rasterSize.y; i += 256) {
        assert.closeTo(result[i], input1[i] + input2[i] + 2, 1e-6)
      }
    })

    it('should support converting the data type', function () {
      if (!semver.gte(gdal.version, '3.5.0-git')) this.skip()
      const sum2 = (sources: gdal.TypedArray[], buffer: gdal.TypedArray) => {
        assert.equal(sources.length, 2)
        assert.instanceOf(sources[0], Float32Array)
        assert.instanceOf(sources[1], Float32Array)
        assert.instanceOf(buffer, Int32Array)
        for (let i = 0; i < buffer.length; i++) {
          buffer[i] = sources[0][i] + sources[1][i] + 3
        }
      }
      gdal.addPixelFunc('sum2int', gdal.toPixelFunc(sum2))

      const vrt = gdal.wrapVRT({
        bands: [
          {
            sources: [ band1, band2 ],
            pixelFunc: 'sum2int',
            dataType: gdal.GDT_Int32,
            sourceTransferType: gdal.GDT_Float32
          }
        ]
      })
      const ds = gdal.open(vrt)

      assert.equal(ds.bands.count(), 1)
      assert.equal(ds.bands.get(1).dataType, gdal.GDT_Int32)
      const input1 = band1.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const input2 = band2.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const q = ds.bands.get(1).pixels.readAsync(0, 0, ds.rasterSize.x, ds.rasterSize.y).then((result) => {
        assert.instanceOf(result, Int32Array)
        for (let i = 0; i < ds.rasterSize.x * ds.rasterSize.y; i += 256) {
          assert.closeTo(result[i], input1[i] + input2[i] + 3, 1)
        }
      })
      return assert.isFulfilled(q)
    })

    it('should pass any additional arguments', function () {
      if (!semver.gte(gdal.version, '3.5.0-git')) this.skip()
      const withArgs = (sources: gdal.TypedArray[], buffer: gdal.TypedArray, args: Record<string, string|number>) => {
        assert.deepEqual(args, { s: 'stringArg', k: 20, t: 15, pi: 3.14 })
        assert.isString(args.s)
        assert.isNumber(args.k)
        assert.isNumber(args.t)
        assert.isNumber(args.pi)
        for (let i = 0; i < buffer.length; i++) {
          buffer[i] = +args.k + sources[0][i] + sources[1][i]
        }
      }
      gdal.addPixelFunc('withArgs', gdal.toPixelFunc(withArgs))

      const vrt = gdal.wrapVRT({
        bands: [
          {
            sources: [ band1, band2 ],
            pixelFunc: 'withArgs',
            pixelFuncArgs: { s: 'stringArg', k: 20, t: '15', pi: '3.14' }
          }
        ]
      })
      const ds = gdal.open(vrt)

      assert.equal(ds.bands.count(), 1)
      const input1 = band1.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const input2 = band2.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const result = ds.bands.get(1).pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      for (let i = 0; i < ds.rasterSize.x * ds.rasterSize.y; i += 256) {
        assert.closeTo(result[i], input1[i] + input2[i] + 20, 1e-6)
      }
    })
  })

  describe('createPixelFunc()', () => {
    let band1: gdal.RasterBand, band2: gdal.RasterBand
    before(() => {
      band1 = gdal.open(path.resolve(__dirname, 'data', 'AROME_T2m_10.tiff')).bands.get(1)
      band2 = gdal.open(path.resolve(__dirname, 'data', 'AROME_D2m_10.tiff')).bands.get(1)
    })

    it('should create a pixel function from a JS function for a single pixel', function () {
      if (!semver.gte(gdal.version, '3.5.0-git')) this.skip()

      gdal.addPixelFunc('createPxFn', gdal.createPixelFunc((a, b) => a + b + 4))

      const vrt = gdal.wrapVRT({
        bands: [
          {
            sources: [ band1, band2 ],
            pixelFunc: 'createPxFn'
          }
        ]
      })
      const ds = gdal.open(vrt)

      assert.equal(ds.bands.count(), 1)
      const input1 = band1.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const input2 = band2.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const result = ds.bands.get(1).pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      for (let i = 0; i < ds.rasterSize.x * ds.rasterSize.y; i += 256) {
        assert.closeTo(result[i], input1[i] + input2[i] + 4, 1e-6)
      }
    })

    it('should support converting the data type', function () {
      if (!semver.gte(gdal.version, '3.5.0-git')) this.skip()

      gdal.addPixelFunc('createPxFnInt', gdal.createPixelFunc((a, b) => Math.round(a + b + 5)))

      const vrt = gdal.wrapVRT({
        bands: [
          {
            sources: [ band1, band2 ],
            pixelFunc: 'createPxFnInt',
            dataType: gdal.GDT_Int32,
            sourceTransferType: gdal.GDT_Float32
          }
        ]
      })
      const ds = gdal.open(vrt)

      assert.equal(ds.bands.count(), 1)
      assert.equal(ds.bands.get(1).dataType, gdal.GDT_Int32)
      const input1 = band1.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const input2 = band2.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const result = ds.bands.get(1).pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      for (let i = 0; i < ds.rasterSize.x * ds.rasterSize.y; i += 256) {
        assert.instanceOf(result, Int32Array)
        assert.closeTo(result[i], input1[i] + input2[i] + 5, 0.5, `${input1[i]} + ${input2[i]}`)
      }
    })
  })

  describe('createPixelFuncWithArgs()', () => {
    let band1: gdal.RasterBand, band2: gdal.RasterBand
    before(() => {
      band1 = gdal.open(path.resolve(__dirname, 'data', 'AROME_T2m_10.tiff')).bands.get(1)
      band2 = gdal.open(path.resolve(__dirname, 'data', 'AROME_D2m_10.tiff')).bands.get(1)
    })

    it('should create a pixel function with arguments from a JS function for a single pixel', function () {
      if (!semver.gte(gdal.version, '3.5.0-git')) this.skip()

      const k = 42
      gdal.addPixelFunc('createPxFn', gdal.createPixelFuncWithArgs((args, a, b) => a + b + +args.k))

      const vrt = gdal.wrapVRT({
        bands: [
          {
            sources: [ band1, band2 ],
            pixelFunc: 'createPxFn',
            pixelFuncArgs: { k }
          }
        ]
      })
      const ds = gdal.open(vrt)

      assert.equal(ds.bands.count(), 1)
      const input1 = band1.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const input2 = band2.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      const result = ds.bands.get(1).pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y)
      for (let i = 0; i < ds.rasterSize.x * ds.rasterSize.y; i += 256) {
        assert.closeTo(result[i], input1[i] + input2[i] + k, 1e-6)
      }
    })
  })
})
