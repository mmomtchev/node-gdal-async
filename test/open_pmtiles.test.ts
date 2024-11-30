import * as path from 'path'
import * as gdal from 'gdal-async'
import * as chai from 'chai'
import * as semver from 'semver'
const assert: Chai.Assert = chai.assert
import * as chaiAsPromised from 'chai-as-promised'
chai.use(chaiAsPromised)

describe('Open', () => {
  if (!semver.gte(gdal.version, '3.8.0')) {
    return
  }

  afterEach(() => void global.gc!())

  describe('PMTiles', () => {
    let ds: gdal.Dataset

    it('should not throw', () => {
      ds = gdal.open(path.join(__dirname, 'data', 'ne_10m_admin_0_france.pmtiles'))
    })

    it('should be able to read layer definition', () => {
      const fields = ds.layers.get(0).fields.map((f) => f)
      assert.lengthOf(fields, 64)
      assert.strictEqual(fields[0].type, 'integer64')
      assert.strictEqual(fields[0].name, 'mvt_id')
      assert.strictEqual(fields[63].type, 'integer')
      assert.strictEqual(fields[63].name, 'woe_id')
    })

    it('should be able to read features', () => {
      const features = ds.layers.get(0).features.map((f) => f)
      assert.lengthOf(features, 19)
      assert.strictEqual(features[0].fid, 335)
      assert.strictEqual(features[0].fields.get('name'), 'France')
      assert.strictEqual(features[18].fid, 5456)
      assert.strictEqual(features[18].fields.get('name'), 'Netherlands')
    })

    it('should have projection', () => {
      assert.isTrue(ds.layers.get(0).srs?.isSame(gdal.SpatialReference.fromEPSG(3857)))
    })
  })

})
