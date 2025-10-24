import * as gdal from 'gdal-async'
import { assert } from 'chai'

describe('algebra', () => {
  afterEach(() => void global.gc!())

  describe('add()', () => {
    let ds: gdal.Dataset
    let arg1Band: gdal.RasterBand
    let arg2Band: gdal.RasterBand

    before(() => {
      // create a simple ramp in memory
      const w = 64
      const h = 64
      ds = gdal.open('temp', 'w', 'MEM', w, h, 2)
      arg1Band = ds.bands.get(1)
      arg2Band = ds.bands.get(2)
      for (let y = 0; y < h; y++) {
        const buf = Buffer.alloc(w)
        buf.fill(y * 4)
        arg1Band.pixels.write(0, y, w, 1, new Uint8Array(buf))
        arg2Band.pixels.write(0, y, w, 1, new Uint8Array(buf))
      }
    })
    after(() => {
      ds.close()
    })

    it('should add two bands', () => {
      const r = gdal.algebra.add(arg1Band, arg2Band)
      assert.instanceOf(r, gdal.RasterBand)
      assert.instanceOf(r.ds, gdal.Dataset)
      assert.strictEqual(r.pixels.get(5, 5), arg1Band.pixels.get(5, 5) + arg2Band.pixels.get(5, 5))
    })

    it('should add a band and a number', () => {
      const r = gdal.algebra.add(arg1Band, 42)
      assert.instanceOf(r, gdal.RasterBand)
      assert.instanceOf(r.ds, gdal.Dataset)
      assert.strictEqual(r.pixels.get(5, 5), arg1Band.pixels.get(5, 5) + 42)
    })

    it('should add a number and a band', () => {
      const r = gdal.algebra.add(-42, arg2Band)
      assert.instanceOf(r, gdal.RasterBand)
      assert.instanceOf(r.ds, gdal.Dataset)
      assert.strictEqual(r.pixels.get(5, 5), -42 + arg2Band.pixels.get(5, 5))
    })

    it('should throw on invalid arguments', () => {
      assert.throws(() => {
        gdal.algebra.add(-42, 42)
      }, /At least one RasterBand must be given/)
      assert.throws(() => {
        // @ts-expect-error voluntary error
        gdal.algebra.add(ds, arg1Band)
      }, /Argument must be either a number or a RasterBand/)
    })

    it('should handle destroying the computed dataset', () => {
      const r = gdal.algebra.add(-42, arg2Band)
      assert.instanceOf(r, gdal.RasterBand)
      assert.instanceOf(r.ds, gdal.Dataset)
      r.ds.close()
    })
  })
})
