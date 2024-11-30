import * as gdal from 'gdal-async'
import * as path from 'path'
import { assert } from 'chai'

describe('Open', () => {
  afterEach(() => void global.gc!())

  describe('SDTS (DDF)', () => {
    let filename, ds: gdal.Dataset

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/sdts/8821CATD.DDF')
      ds = gdal.open(filename)
    })

    it('should be able to read raster size', () => {
      assert.equal(ds.rasterSize.x, 343)
      assert.equal(ds.rasterSize.y, 471)
      assert.equal(ds.bands.count(), 1)
    })

    it('should be able to read geotransform', () => {
      const expected_geotransform = [ 658995, 30, 0, 4929375, 0, -30 ]

      const actual_geotransform = ds.geoTransform
      const delta = 0.00001
      if (actual_geotransform === null) throw new Error('No GeoTransform')
      assert.closeTo(actual_geotransform[0], expected_geotransform[0], delta)
      assert.closeTo(actual_geotransform[1], expected_geotransform[1], delta)
      assert.closeTo(actual_geotransform[2], expected_geotransform[2], delta)
      assert.closeTo(actual_geotransform[3], expected_geotransform[3], delta)
      assert.closeTo(actual_geotransform[4], expected_geotransform[4], delta)
      assert.closeTo(actual_geotransform[5], expected_geotransform[5], delta)
    })

    it('should have projection', () => {
      if (ds.srs === null) throw new Error('No SRS')
      assert.match(ds.srs.toWKT(), /PROJCS/)
    })

    it('should be able to read statistics', () => {
      const band = ds.bands.get(1)
      const expected_stats = {
        min: 1520,
        max: 1826,
        mean: 1625.9093194071836,
        std_dev: 48.585305943514605
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
