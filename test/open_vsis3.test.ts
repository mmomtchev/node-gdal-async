import * as gdal from 'gdal-async'
import * as chai from 'chai'
import * as semver from 'semver'
const assert: Chai.Assert = chai.assert
import * as chaiAsPromised from 'chai-as-promised'
chai.use(chaiAsPromised)

describe('Open', () => {
  if (!semver.gte(gdal.version, '2.3.0')) {
    return
  }

  afterEach(() => void global.gc!())
  before(() => {
    gdal.config.set('AWS_NO_SIGN_REQUEST', 'YES')
  })

  describe('vsis3 w/Net', () => {
    let ds: gdal.Dataset

    it('should not throw', () => {
      ds = gdal.open('/vsis3/elevation-tiles-prod/geotiff/12/1/1.tif')
    })

    it('should be able to read raster size', () => {
      assert.equal(ds.rasterSize.x, 512)
      assert.equal(ds.rasterSize.y, 512)
      assert.equal(ds.bands.count(), 1)
    })

    it('should have projection', () => {
      assert.isTrue(ds.srs?.isSame(gdal.SpatialReference.fromEPSG(3857)))
    })
  })

  describe('vsis3/Async w/Net', () => {
    let ds: Promise<gdal.Dataset>

    it('should not throw', () => {
      ds = gdal.openAsync('/vsis3/elevation-tiles-prod/geotiff/12/1/1.tif')
      return assert.isFulfilled(ds)
    })

    it('should be able to read raster size', () => assert.isFulfilled(Promise.all([ assert.eventually.equal(ds.then((r) => r.rasterSize.x), 512),
      assert.eventually.equal(ds.then((r) => r.rasterSize.y), 512),
      assert.eventually.equal(ds.then((r) => r.bands.count()), 1) ])))

    it('should have projection', () => {
      assert.eventually.isTrue(ds.then((r) => r.srs?.isSame(gdal.SpatialReference.fromEPSG(3857))))
    })
  })
})
