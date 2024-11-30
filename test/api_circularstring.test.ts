import { assert } from 'chai'
import * as gdal from 'gdal-async'

describe('gdal.CircularString', () => {
  afterEach(() => void global.gc!())

  it('should be instantiable', () => {
    new gdal.CircularString()
  })
  it('should inherit from Curve', () => {
    assert.instanceOf(new gdal.CircularString(), gdal.CircularString)
    assert.instanceOf(new gdal.CircularString(), gdal.SimpleCurve)
    assert.instanceOf(new gdal.CircularString(), gdal.Geometry)
  })
  describe('instance', () => {
    it('should support getArea()', () => {
      const arc: gdal.CircularString = new gdal.CircularString()
      arc.points.add(-5, 0)
      arc.points.add(0, 2.5)
      arc.points.add(5, 0)
      assert.closeTo(arc.getLength(), 11.5911, 0.001)
    })
    it('should support addSubLineString', () => {
      const arc = new gdal.CircularString()
      arc.points.add(-5, 0)
      arc.points.add(0, 2.5)
      arc.points.add(5, 0)

      const line = new gdal.LineString()
      line.points.add(0, 0, 0)
      line.points.add(10, 10, 0)
      line.points.add(10, 20, 0)

      arc.addSubLineString(line)

      assert.equal(arc.points.count(), 6)
    })
  })
})
