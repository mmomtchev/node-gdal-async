import { assert } from 'chai'
import * as gdal from 'gdal-async'

describe('gdal.LinearRing', () => {
  afterEach(global.gc!)

  it('should be instantiable', () => {
    new gdal.LinearRing()
  })
  it('should inherit from LineString', () => {
    assert.instanceOf(new gdal.LinearRing(), gdal.LinearRing)
    assert.instanceOf(new gdal.LinearRing(), gdal.LineString)
    assert.instanceOf(new gdal.LinearRing(), gdal.SimpleCurve)
    assert.instanceOf(new gdal.LinearRing(), gdal.Geometry)
  })
  describe('instance', () => {
    describe('getArea()', () => {
      it('should return area', () => {
        const ring = new gdal.LinearRing()
        ring.points.add(0, 0, 0)
        ring.points.add(10, 0, 0)
        ring.points.add(10, 10, 0)
        ring.points.add(0, 10, 0)
        ring.points.add(0, 0, 0)
        assert.closeTo(ring.getArea(), 100, 0.001)
      })
    })
  })
})
