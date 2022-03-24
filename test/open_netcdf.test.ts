import * as gdal from 'gdal-async'
import * as path from 'path'
import { assert } from 'chai'

describe('Open', () => {
  afterEach(global.gc)

  describe('NetCDF', () => {
    let filename, ds: gdal.Dataset

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/gfs.t00z.alnsf.nc')
      ds = gdal.open(filename)
    })

    it('should be able to read raster size', () => {
      assert.equal(ds.rasterSize.x, 90)
      assert.equal(ds.rasterSize.y, 45)
      assert.equal(ds.bands.count(), 1)
    })

    it('should be able to read geotransform', () => {
      const expected_geotransform = [ -0.05859375, 4, 0, 89.9679127646579815, 0, -4 ]

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

    it('on Linux, with bundled GDAL, it should support should virtual IO', () => {
      assert.equal(gdal.drivers.get('netcdf').getMetadata().DCAP_VIRTUALIO, 'YES')
    })
  })
})
