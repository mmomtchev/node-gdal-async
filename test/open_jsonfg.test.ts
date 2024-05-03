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

  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  describe('JSONFG', () => {
    let ds: gdal.Dataset

    it('should not throw', () => {
      ds = gdal.open(path.join(__dirname, 'data', 'crs_32631_fc_and_feat.json'))
    })

    it('should be able to read layer definition', () => {
      const fields = ds.layers.get(0).fields.map((f) => f)
      assert.lengthOf(fields, 1)
      assert.strictEqual(fields[0].type, 'string')
      assert.strictEqual(fields[0].name, 'notes')
    })

    it('should be able to read features', () => {
      const features = ds.layers.get(0).features.map((f) => f)
      assert.lengthOf(features, 1)
      assert.strictEqual(features[0].fid, 1)
      assert.strictEqual(features[0].fields.get('notes'), 'coordinates of geometry are not equivalent to place on purpose')
    })

    it('should have projection', () => {
      assert.isTrue(ds.layers.get(0).srs?.isSame(gdal.SpatialReference.fromEPSG(32631)))
    })
  })

})
