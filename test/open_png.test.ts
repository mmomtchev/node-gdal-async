import * as gdal from 'gdal-async'
import * as path from 'path'
import { assert } from 'chai'

describe('Open', () => {
  afterEach(global.gc!)

  describe('PNG', () => {
    let filename, ds: gdal.Dataset

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/CM13ct.png')
      ds = gdal.open(filename)
    })

    it('should be able to read raster size', () => {
      assert.equal(ds.rasterSize.x, 1951)
      assert.equal(ds.rasterSize.y, 3109)
      assert.equal(ds.bands.count(), 1)
    })

    it('should be able to read geotransform', () => {
      const expected_geotransform = [
        674425.2950174398,
        10,
        0,
        6462458.631486551,
        0,
        -10
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
        min: 1,
        max: 253,
        mean: 125.58997597595655,
        std_dev: 59.3013235226549
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
      assert.equal(size.x, 1951)
      assert.equal(size.y, 1)
    })

    it('should have file list', () => {
      const files = [
        path.join(__dirname, 'data/CM13ct.png'),
        path.join(__dirname, 'data/CM13ct.pgw')
      ]

      const actual_files = ds.getFileList()

      // get statistics may or may not create an .aux.xml file
      assert.include(actual_files, files[0])
      assert.include(actual_files, files[1])
      assert.isAtMost(actual_files.length, 3)
    })
  })
})
