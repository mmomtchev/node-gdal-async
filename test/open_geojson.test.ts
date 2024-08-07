import * as gdal from 'gdal-async'
import * as path from 'path'
import { assert } from 'chai'

describe('Open', () => {
  afterEach(global.gc!)

  describe('GeoJSON', () => {
    let filename, ds: gdal.Dataset

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/park.geo.json')
      ds = gdal.open(filename)
    })
    it('should be able to read layer count', () => {
      assert.equal(ds.layers.count(), 1)
    })

    describe('layer', () => {
      let layer: gdal.Layer
      it('should exist', () => {
        layer = ds.layers.get(0)
        assert.ok(layer)
        assert.instanceOf(layer, gdal.Layer)
      })
      it('should have all fields defined', () => {
        assert.equal(layer.fields.count(), 3)
        assert.deepEqual(layer.fields.getNames(), [ 'kind', 'name', 'state' ])
      })
      describe('features', () => {
        it('should be readable', () => {
          assert.equal(layer.features.count(), 1)
          const feature = layer.features.get(0)
          const fields = feature.fields.toObject()

          assert.deepEqual(fields, {
            kind: 'county',
            state: 'WY',
            name: 'Park'
          })
        })
      })
    })
  })
})
