import * as gdal from 'gdal-async'
import * as path from 'path'
import { assert } from 'chai'

describe('Open', () => {
  afterEach(global.gc!)

  before(() => {
    gdal.config.set('GDAL_PAM_ENABLED', 'NO')
  })

  after(() => {
    gdal.config.set('GDAL_PAM_ENABLED', 'YES')
  })

  describe('TIFF', () => {
    let filename, ds: gdal.Dataset

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/sample.tif')
      ds = gdal.open(filename)
    })

    it('should be able to read raster size', () => {
      assert.equal(ds.rasterSize.x, 984)
      assert.equal(ds.rasterSize.y, 804)
      assert.equal(ds.bands.count(), 1)
    })

    it('should be able to read geotransform', () => {
      const expected_geotransform = [
        -1134675.2952829634,
        7.502071930146189,
        0,
        2485710.4658232867,
        0,
        -7.502071930145942
      ]

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
        min: 0,
        max: 100,
        mean: 29.725628716175223,
        std_dev: 36.98885954363488
      }

      const actual_stats = band.getStatistics(false, true) as typeof expected_stats
      const delta = 0.00001
      assert.closeTo(actual_stats.min, expected_stats.min, delta)
      assert.closeTo(actual_stats.max, expected_stats.max, delta)
      assert.closeTo(actual_stats.mean, expected_stats.mean, delta)
      assert.closeTo(actual_stats.std_dev, expected_stats.std_dev, delta)
    })
  })
})
