import * as gdal from 'gdal-async'
import * as path from 'path'
import { assert } from 'chai'

describe('Open', () => {
  afterEach(() => void global.gc!())

  describe('HDF5', () => {
    let filename, ds: gdal.Dataset

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/h5ex_t_array.h5')
      ds = gdal.open(filename)
    })

    it('should be able to read raster size', () => {
      assert.equal(ds.rasterSize.x, 512)
      assert.equal(ds.rasterSize.y, 512)
      assert.equal(ds.bands.count(), 0)
    })

    it('should not have projection', () => {
      assert.isNull(ds.srs)
    })
  })

  describe('HDF5/gzip', () => {
    let filename, ds: gdal.Dataset

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/h5ex_d_gzip.h5')
      ds = gdal.open(filename)
    })

    it('should be able to read raster size', () => {
      assert.equal(ds.rasterSize.x, 64)
      assert.equal(ds.rasterSize.y, 32)
      assert.equal(ds.bands.count(), 1)
    })

    it('should not have projection', () => {
      assert.isNull(ds.srs)
    })

    it('should be able to read statistics', () => {
      const band = ds.bands.get(1)
      const expected_stats = {
        min: -63,
        max: 1890,
        mean: 456.75,
        std_dev: 430.61431409093
      }

      const actual_stats = band.getStatistics(false, true) as typeof expected_stats
      const delta = 0.00001
      assert.closeTo(expected_stats.min, actual_stats.min, delta)
      assert.closeTo(expected_stats.max, actual_stats.max, delta)
      assert.closeTo(expected_stats.mean, actual_stats.mean, delta)
      assert.closeTo(expected_stats.std_dev, actual_stats.std_dev, delta)
    })
  })
})
