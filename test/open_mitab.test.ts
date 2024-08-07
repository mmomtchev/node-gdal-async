import * as gdal from 'gdal-async'
import * as path from 'path'
import { assert } from 'chai'

describe('Open', () => {
  afterEach(global.gc!)

  describe('MapInfo', () => {
    let filename, ds: gdal.Dataset

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/mitab/EXTRACT_POLYGON.mif')
      ds = gdal.open(filename)
    })
    it('should be able to read layer count', () => {
      assert.equal(ds.layers.count(), 1)
    })
    it('should be able to read features', () => {
      const layer = ds.layers.get(0)
      const featureCount = layer.features.count()
      assert.equal(featureCount, 1, 'layer.features.count()')
      assert.instanceOf(layer.features.first().getGeometry(), gdal.Polygon)
    })
  })
})
