import * as gdal from 'gdal-async'
import { assert } from 'chai'

describe('gdal.CoordinateTransformation', () => {
  afterEach(global.gc)

  it('should be exposed', () => {
    assert.ok(gdal.CoordinateTransformation)
  })
  it('should handle Proj.4', () => {
    const srs0 = gdal.SpatialReference.fromProj4(
      '+proj=lcc +lat_1=49 +lat_2=45 +lat_0=44.25 +lon_0=-109.5 +x_0=599999.9999976 +y_0=0 +ellps=GRS80 +datum=NAD83 +to_meter=0.3048 +no_defs'
    )
    const srs1 = gdal.SpatialReference.fromProj4('+init=epsg:4326')

    const pt = new gdal.Point(1, 2)
    const ct = new gdal.CoordinateTransformation(srs0, srs1)
    pt.transform(ct)

    assert.closeTo(pt.x, -116.98546236548084, 0.1)
    assert.closeTo(pt.y, 43.99194884775841, 0.1)
  })
  it('should handle Proj.4 (EPSG)', () => {
    const srs0 = gdal.SpatialReference.fromProj4('+init=epsg:4326')
    const srs1 = gdal.SpatialReference.fromProj4('+init=epsg:32632')

    const pt = new gdal.Point(20, 30)
    const ct = new gdal.CoordinateTransformation(srs0, srs1)
    pt.transform(ct)

    assert.closeTo(pt.x, 1564201.4044502454, 0.1)
    assert.closeTo(pt.y, 3370263.469590679, 0.1)
  })
  describe('transformPoint()', () => {
    let ct: gdal.CoordinateTransformation
    beforeEach(() => {
      const srs0 = gdal.SpatialReference.fromProj4('+init=epsg:4326')
      const srs1 = gdal.SpatialReference.fromProj4('+init=epsg:32632')
      ct = new gdal.CoordinateTransformation(srs0, srs1)
    })
    it('should accept xyz objects', () => {
      const pt = ct.transformPoint({ x: 20, y: 30 })

      assert.closeTo(pt.x, 1564201.4044502454, 0.1)
      assert.closeTo(pt.y, 3370263.469590679, 0.1)
    })
    it('should accept x, y, z arguments', () => {
      const pt = ct.transformPoint(20, 30)

      assert.closeTo(pt.x, 1564201.4044502454, 0.1)
      assert.closeTo(pt.y, 3370263.469590679, 0.1)
    })
    it('with bundled GDAL, should throw on invalid coordinates', () => {
      assert.throws(() => {
        ct.transformPoint({ x: 400, y: 120 })
      }, /Error transforming point: Invalid coordinate/)
    })
    it('should throw on invalid arguments', () => {
      assert.throws(() => {
      // eslint-disable-next-line @typescript-eslint/no-explicit-any
        ct.transformPoint({ x: 'a', y: 30 } as any)
      }, /point must contain numerical properties x and y/)
    })
  })
})
