import * as gdal from 'gdal-async'
import * as chai from 'chai'
const assert: Chai.Assert = chai.assert
import * as chaiAsPromised from 'chai-as-promised'
import * as semver from 'semver'
chai.use(chaiAsPromised)

const wcsURL = 'WCS:https://demo.mapserver.org/cgi-bin/wcs?VERSION=1.0.0&COVERAGE=ndvi'

describe('Open', () => {
  afterEach(() => void global.gc!())

  // System-installed versions do not always have the WCS driver
  if (!gdal.bundled && gdal.drivers.get('WCS') === null) {
    return
  }
  if (semver.lt(gdal.version, '2.3.0')) {
    return
  }

  describe('WCS w/Net', () => {
    let ds: gdal.Dataset

    after(() => (ds && ds.close()))

    it('should not throw', () => {
      ds = gdal.open(wcsURL)
    })

    it('should be able to the bands', () => {
      assert.equal(ds.bands.count(), 1)
    })

    it('should have projection', () => {
      assert.isTrue(ds.srs?.isSame(gdal.SpatialReference.fromEPSG(26915)))
    })
  })

  describe('WCS/Async w/Net', () => {
    let ds: Promise<gdal.Dataset>

    after(() => ds && ds.then((r) => r.close()))

    it('should not throw', () => {
      ds = gdal.openAsync(wcsURL)
      return assert.isFulfilled(ds)
    })

    it('should be able to read raster size', () =>
      assert.isFulfilled(Promise.all([
        assert.eventually.equal(ds.then((r) => r.bands.count()), 1)
      ]))
    )

    it('should have projection', () =>
      assert.eventually.isTrue(ds.then((r) =>
        r.srs?.isSame(gdal.SpatialReference.fromEPSG(26915))
      )))
  })
})
