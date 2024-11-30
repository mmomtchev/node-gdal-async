import * as gdal from 'gdal-async'
import * as path from 'path'
import { assert } from 'chai'

describe('Open', () => {
  afterEach(() => void global.gc!())

  describe('JPG', () => {
    let filename, ds: gdal.Dataset

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/a39se10.jpg')
      ds = gdal.open(filename)
    })

    it('should be able to read raster size', () => {
      assert.equal(ds.rasterSize.x, 1361)
      assert.equal(ds.rasterSize.y, 1744)
      assert.equal(ds.bands.count(), 3)
    })

    it('should be able to read geotransform', () => {
      const expected_geotransform = [
        1485309.3937136543,
        3.28077925649814,
        0,
        603662.5109447651,
        0,
        -3.28077925649814
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

    it('should not have projection', () => {
      assert.isNull(ds.srs)
    })

    it('should be able to read statistics', () => {
      const band = ds.bands.get(1)
      const expected_stats = {
        min: 0,
        max: 255,
        mean: 118.37471983296146,
        std_dev: 50.89389997162665
      }

      const actual_stats = band.getStatistics(false, true) as typeof expected_stats
      const delta = 0.00001
      assert.closeTo(actual_stats.min, expected_stats.min, delta)
      assert.closeTo(actual_stats.max, expected_stats.max, delta)
      assert.closeTo(actual_stats.mean, expected_stats.mean, delta)
      assert.closeTo(actual_stats.std_dev, expected_stats.std_dev, delta)
    })

    it('should be able to read block size', () => {
      const band = ds.bands.get(1)
      const size = band.blockSize
      assert.equal(size.x, 1361)
      assert.equal(size.y, 1)
    })

    it('should have file list', () => {
      const files = [
        path.join(__dirname, 'data/a39se10.jpg'),
        path.join(__dirname, 'data/a39se10.jpg.aux.xml'),
        path.join(__dirname, 'data/a39se10.jgw')
      ]

      const actual_files = ds.getFileList()
      actual_files.sort()
      files.sort()
      assert.deepEqual(actual_files, files)
    })
  })
})
