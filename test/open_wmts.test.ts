import * as gdal from 'gdal-async'
import * as chai from 'chai'
const assert: Chai.Assert = chai.assert
import * as chaiAsPromised from 'chai-as-promised'
chai.use(chaiAsPromised)

const shomTides = 'WMTS:https://services.data.shom.fr/INSPIRE/wmts?request=GetCapabilities&service=WMTS&version=1.0.0,layer=ZONES_MAREE_PYR_PNG_3857_WMTS'

describe('Open', () => {
  afterEach(() => void global.gc!())

  describe('WMTS w/Net', () => {
    let ds: gdal.Dataset

    after(() => (ds && ds.close()))

    it('should not throw', () => {
      ds = gdal.open(shomTides)
    })

    it('should be able to the bands', () => {
      assert.equal(ds.bands.count(), 4)
      assert.equal(ds.bands.get(1).overviews.get(16).size.x, 234)
    })

    it('should have projection', () => {
      assert.isTrue(ds.srs?.isSame(gdal.SpatialReference.fromEPSG(3857)))
    })
  })

  describe('WMTS/Async w/Net', () => {
    let ds: Promise<gdal.Dataset>

    after(() => ds && ds.then((r) => r.close()))

    it('should not throw', () => {
      ds = gdal.openAsync(shomTides)
      return assert.isFulfilled(ds)
    })

    it('should be able to read raster size', () =>
      assert.isFulfilled(Promise.all([
        assert.eventually.equal(ds.then((r) => r.bands.count()), 4),
        assert.eventually.equal(ds.then((r) => r.bands.get(1).overviews.get(16).size.x), 234)
      ]))
    )

    it('should have projection', () =>
      assert.eventually.isTrue(ds.then((r) =>
        r.srs?.isSame(gdal.SpatialReference.fromEPSG(3857))
      )))
  })
})
