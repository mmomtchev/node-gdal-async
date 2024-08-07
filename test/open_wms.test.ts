import * as gdal from 'gdal-async'
import * as chai from 'chai'
const assert: Chai.Assert = chai.assert
import * as chaiAsPromised from 'chai-as-promised'
chai.use(chaiAsPromised)

// https://magosm.magellium.com/aide.html © Magellium pour les flux WMS/WFS

const magosm = 'https://magosm.magellium.com/geoserver/ows?SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=magosm%3Afrance_telecom_fibre_connection_point&CRS=CRS:84&BBOX=2,48,3,49'

describe('Open', () => {
  afterEach(global.gc!)

  describe('WMS w/Net', () => {
    let ds: gdal.Dataset

    after(() => (ds && ds.close()))

    it('should not throw', () => {
      ds = gdal.open(magosm)
    })

    it('should be able to the bands', () => {
      assert.equal(ds.bands.count(), 3)
      assert.equal(ds.bands.get(1).overviews.get(19).size.x, 1024)
    })

    it('should have projection', () => {
      assert.isTrue(ds.srs?.isSame(gdal.SpatialReference.fromEPSG(4326)))
    })
  })

  describe('WMS/Async w/Net', () => {
    let ds: Promise<gdal.Dataset>

    after(() => ds && ds.then((r) => r.close()))

    it('should not throw', () => {
      ds = gdal.openAsync(magosm)
      return assert.isFulfilled(ds)
    })

    it('should be able to read raster size', () =>
      assert.isFulfilled(Promise.all([
        assert.eventually.equal(ds.then((r) => r.bands.count()), 3),
        assert.eventually.equal(ds.then((r) => r.bands.get(1).overviews.get(19).size.x), 1024)
      ]))
    )

    it('should have projection', () =>
      assert.eventually.isTrue(ds.then((r) =>
        r.srs?.isSame(gdal.SpatialReference.fromEPSG(4326))
      )))
  })
})
