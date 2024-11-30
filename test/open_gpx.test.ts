import * as gdal from 'gdal-async'
import * as path from 'path'
import { assert } from 'chai'

describe('Open', () => {
  afterEach(() => void global.gc!())

  describe('GPX', () => {
    let filename, ds: gdal.Dataset

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/gpx/spiritmountain.gpx')
      ds = gdal.open(filename, 'r', 'GPX')
    })
    it('should be able to read layer count', () => {
      assert.equal(ds.layers.count(), 5)
    })

    describe('layer', () => {
      let layer: gdal.Layer
      it('should exist', () => {
        layer = ds.layers.get(0)
        assert.ok(layer)
        assert.instanceOf(layer, gdal.Layer)
      })
      it('should have all fields defined', () => {
        const names = layer.fields.getNames()
        assert.include(names, 'ele')
        assert.include(names, 'time')
        assert.include(names, 'name')
        assert.include(names, 'cmt')
        assert.include(names, 'sym')
      })
      describe('features', () => {
        it('should be readable', () => {
          assert.equal(layer.features.count(), 2)
          const feature = layer.features.get(0)
          const fields = feature.fields.toObject()

          assert.closeTo(fields.ele, 1975.311646, 0.000001)
          assert.deepEqual(fields.time, {
            year: 2014,
            month: 5,
            day: 27,
            hour: 17,
            minute: 25,
            second: 13,
            timezone: 100
          })
          assert.equal(fields.name, '005')
          assert.equal(fields.cmt, 'PARK')
          assert.equal(fields.sym, 'Flag, Blue')
        })
      })
    })
  })
})
