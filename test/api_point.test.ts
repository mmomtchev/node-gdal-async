import * as gdal from 'gdal-async'
import { assert } from 'chai'

describe('gdal.Point', () => {
  afterEach(() => void global.gc!())

  let point2d: gdal.Point
  let point3d: gdal.Point
  before(() => {
    point2d = new gdal.Point(1, 2)
    point3d = new gdal.Point(1, 2, 3)
  })
  it('should inherit from Geometry', () => {
    assert.instanceOf(point2d, gdal.Point)
    assert.instanceOf(point2d, gdal.Geometry)
    assert.instanceOf(point3d, gdal.Point)
    assert.instanceOf(point3d, gdal.Geometry)
  })
  it('should be valid', () => {
    assert.equal(point2d.isValid(), true)
    assert.equal(point3d.isValid(), true)
  })
  it('should be simple', () => {
    assert.equal(point2d.isSimple(), true)
    assert.equal(point3d.isSimple(), true)
  })
  describe('instance', () => {
    describe('"x","y","z" properties', () => {
      it('should be gettable', () => {
        assert.equal(point2d.x, 1)
        assert.equal(point2d.y, 2)
        assert.equal(point2d.z, 0)
        assert.equal(point3d.x, 1)
        assert.equal(point3d.y, 2)
        assert.equal(point3d.z, 3)
      })
      it('should be settable', () => {
        const pt = new gdal.Point(1, 2, 3)
        pt.x = 4
        pt.y = 5
        pt.z = 6
        assert.equal(pt.x, 4)
        assert.equal(pt.y, 5)
        assert.equal(pt.z, 6)
      })
    })
    describe('swapXY()', () => {
      it('should flip x,y coordinates', () => {
        const pt = new gdal.Point(1, 2, 3)
        pt.swapXY()
        assert.equal(pt.x, 2)
        assert.equal(pt.y, 1)
        assert.equal(pt.z, 3)
      })
    })
  })
})
