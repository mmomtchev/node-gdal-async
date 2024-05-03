import * as gdal from 'gdal-async'
import * as chai from 'chai'
const assert: Chai.Assert = chai.assert
import * as chaiAsPromised from 'chai-as-promised'
chai.use(chaiAsPromised)

describe('Open', () => {
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  describe('vsicurl w/Net', () => {
    let ds: gdal.Dataset

    after(() => (ds && ds.close()))

    it('should not throw', () => {
      ds = gdal.open('/vsicurl/https://s3.amazonaws.com/elevation-tiles-prod/geotiff/12/1/1.tif')
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

  describe('vsicurl/Async w/Net', () => {
    let ds: Promise<gdal.Dataset>

    after(() => ds && ds.then((r) => r.close()))

    it('should not throw', () => {
      ds = gdal.openAsync('/vsicurl/https://s3.amazonaws.com/elevation-tiles-prod/geotiff/12/1/1.tif')
      return assert.isFulfilled(ds)
    })

    it('should be able to read raster size', () =>
      assert.isFulfilled(Promise.all([
        assert.eventually.equal(ds.then((r) => r.rasterSize.x), 512),
        assert.eventually.equal(ds.then((r) => r.rasterSize.y), 512),
        assert.eventually.equal(ds.then((r) => r.bands.count()), 1)
      ]))
    )

    it('should have projection', () =>
      assert.eventually.isTrue(ds.then((r) => r.srs?.isSame(gdal.SpatialReference.fromEPSG(3857))))
    )
  })

  describe('vsigzip/vsicurl chaining w/Net', () => {
    let ds: gdal.Dataset

    after(() => ds && ds.close())

    it('should not throw', () => {
      ds = gdal.open('/vsigzip//vsicurl/https://s3.amazonaws.com/elevation-tiles-prod/skadi/N40/N40E001.hgt.gz')
    })

    it('should be able to read raster size', () => {
      assert.equal(ds.rasterSize.x, 3601)
      assert.equal(ds.rasterSize.y, 3601)
      assert.equal(ds.bands.count(), 1)
    })

    it('should have projection', () => {
      assert.isTrue(ds.srs?.isSame(gdal.SpatialReference.fromEPSG(4326)))
    })
  })
})
