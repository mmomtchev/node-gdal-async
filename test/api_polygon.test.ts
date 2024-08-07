import { assert } from 'chai'
import * as path from 'path'
import * as gdal from 'gdal-async'

describe('gdal.Polygon', () => {
  afterEach(global.gc!)

  it('should be instantiable', () => {
    new gdal.Polygon()
  })
  it('should inherit from Geometry', () => {
    assert.instanceOf(new gdal.Polygon(), gdal.Polygon)
    assert.instanceOf(new gdal.Polygon(), gdal.Geometry)
  })
  describe('instance', () => {
    describe('"rings" property', () => {
      describe('get()', () => {
        it("should throw if ring doesn't exist", () => {
          const polygon = new gdal.Polygon()
          assert.throws(() => {
            polygon.rings.get(2)
          })
        })
        it('should return LinearRing instance', () => {
          const polygon = new gdal.Polygon()
          const ring = new gdal.LinearRing()
          ring.points.add(0, 0, 0)
          ring.points.add(10, 0, 0)
          ring.points.add(10, 10, 0)
          ring.points.add(0, 10, 0)
          ring.points.add(0, 0, 0)
          polygon.rings.add(ring)
          assert.instanceOf(polygon.rings.get(0), gdal.LinearRing)
        })
      })
      describe('count()', () => {
        it('should return ring count', () => {
          const polygon = new gdal.Polygon()
          assert.equal(polygon.rings.count(), 0)

          const ring = new gdal.LinearRing()
          ring.points.add(0, 0, 0)
          ring.points.add(10, 0, 0)
          ring.points.add(10, 10, 0)
          ring.points.add(0, 10, 0)
          ring.points.add(0, 0, 0)
          polygon.rings.add(ring)
          assert.equal(polygon.rings.count(), 1)
        })
      })
      describe('add()', () => {
        const ring1 = new gdal.LinearRing()
        ring1.points.add(0, 0, 0)
        ring1.points.add(10, 0, 0)
        ring1.points.add(10, 11, 0)
        ring1.points.add(0, 10, 0)
        ring1.points.add(0, 0, 0)
        it('should add a ring', () => {
          const polygon = new gdal.Polygon()
          polygon.rings.add(ring1)
          const ring_result = polygon.rings.get(0)
          assert.instanceOf(ring_result, gdal.LinearRing)
          assert.equal(ring_result.points.get(2).y, 11)
        })
        it('should accept multiple LinearRing instances', () => {
          const ring2 = new gdal.LinearRing()
          ring2.points.add(1, 0, 0)
          ring2.points.add(11, 0, 0)
          ring2.points.add(11, 11, 0)
          ring2.points.add(1, 10, 0)
          ring2.points.add(1, 0, 0)

          const polygon = new gdal.Polygon()
          polygon.rings.add([ ring1, ring2 ])
          assert.equal(polygon.rings.count(), 2)
        })
        it('should reject invalid geometries', () => {
          const polygon = new gdal.Polygon()
          polygon.rings.add(ring1)
          const ring2 = new gdal.LineString()
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            polygon.rings.add(ring2 as any)
          }, /must be a LinearRing/)
        })
      })
      let polygon: gdal.Polygon
      const createRings = () => {
        const ring1 = new gdal.LinearRing()
        ring1.points.add(0, 0, 0)
        ring1.points.add(10, 0, 0)
        ring1.points.add(10, 11, 0)
        ring1.points.add(0, 10, 0)
        ring1.points.add(0, 0, 0)
        const ring2 = new gdal.LinearRing()
        ring2.points.add(1, 0, 0)
        ring2.points.add(11, 0, 0)
        ring2.points.add(11, 11, 0)
        ring2.points.add(1, 10, 0)
        ring2.points.add(1, 0, 0)
        const ring3 = new gdal.LinearRing()
        ring3.points.add(2, 0, 0)
        ring3.points.add(20, 0, 0)
        ring3.points.add(20, 11, 0)
        ring3.points.add(3, 10, 0)
        ring3.points.add(3, 0, 0)

        polygon = new gdal.Polygon()
        polygon.rings.add([ ring1, ring2, ring3 ])
      }
      const original = [
        [
          '{ "type": "Point", "coordinates": [ 0.0, 0.0, 0.0 ] }',
          '{ "type": "Point", "coordinates": [ 10.0, 0.0, 0.0 ] }',
          '{ "type": "Point", "coordinates": [ 10.0, 11.0, 0.0 ] }',
          '{ "type": "Point", "coordinates": [ 0.0, 10.0, 0.0 ] }',
          '{ "type": "Point", "coordinates": [ 0.0, 0.0, 0.0 ] }'
        ],
        [
          '{ "type": "Point", "coordinates": [ 1.0, 0.0, 0.0 ] }',
          '{ "type": "Point", "coordinates": [ 11.0, 0.0, 0.0 ] }',
          '{ "type": "Point", "coordinates": [ 11.0, 11.0, 0.0 ] }',
          '{ "type": "Point", "coordinates": [ 1.0, 10.0, 0.0 ] }',
          '{ "type": "Point", "coordinates": [ 1.0, 0.0, 0.0 ] }'
        ],
        [
          '{ "type": "Point", "coordinates": [ 2.0, 0.0, 0.0 ] }',
          '{ "type": "Point", "coordinates": [ 20.0, 0.0, 0.0 ] }',
          '{ "type": "Point", "coordinates": [ 20.0, 11.0, 0.0 ] }',
          '{ "type": "Point", "coordinates": [ 3.0, 10.0, 0.0 ] }',
          '{ "type": "Point", "coordinates": [ 3.0, 0.0, 0.0 ] }'
        ]
      ]
      describe('forEach()', () => {
        before(createRings)
        it('should stop if callback returns false', () => {
          let count = 0
          polygon.rings.forEach((pt, i) => {
            count++
            assert.isNumber(i)
            if (i === 1) return false
          })
          assert.equal(count, 2)
        })
        it('should iterate through all points', () => {
          const result = [] as string[][]
          polygon.rings.forEach((ring) => {
            result.push(ring.points.toArray().map((pt) => pt.toJSON()))
          })

          assert.deepEqual(result, original)
        })
      })
      describe('map()', () => {
        it('should operate normally', () => {
          const polygon = new gdal.Polygon()
          const ring = new gdal.LinearRing()
          ring.points.add(0, 0, 0)
          ring.points.add(10, 0, 0)
          ring.points.add(10, 10, 0)
          ring.points.add(0, 11, 0)
          ring.points.add(0, 0, 0)
          polygon.rings.add(ring)

          const result = polygon.rings.map((ring, i) => {
            assert.isNumber(i)
            assert.instanceOf(ring, gdal.LinearRing)
            return 'a'
          })

          assert.isArray(result)
          assert.lengthOf(result, 1)
          assert.equal(result[0], 'a')
        })
      })
      describe('@@iterator()', () => {
        before(createRings)
        it('should iterate through all points', () => {
          const result = []
          for (const ring of polygon.rings) {
            result.push(ring.points.toArray().map((pt) => pt.toJSON()))
          }

          assert.deepEqual(result, original)
        })
      })
      describe('toArray()', () => {
        it('should return array of LinearRing instances', () => {
          const polygon = new gdal.Polygon()
          const ring = new gdal.LinearRing()
          ring.points.add(0, 0, 0)
          ring.points.add(10, 0, 0)
          ring.points.add(10, 10, 0)
          ring.points.add(0, 11, 0)
          ring.points.add(0, 0, 0)
          polygon.rings.add(ring)
          const array = polygon.rings.toArray()
          assert.isArray(array)
          assert.lengthOf(array, 1)
          assert.instanceOf(array[0], gdal.LinearRing)
          assert.equal(array[0].points.get(3).y, 11)
        })
      })
    })
    describe('getArea()', () => {
      it('should return area', () => {
        const polygon = new gdal.Polygon()
        const ring = new gdal.LinearRing()
        ring.points.add(0, 0, 0)
        ring.points.add(10, 0, 0)
        ring.points.add(10, 10, 0)
        ring.points.add(0, 10, 0)
        ring.points.add(0, 0, 0)
        polygon.rings.add(ring)
        assert.closeTo(ring.getArea(), 100, 0.001)
      })
    })
  })
})

describe('gdal.MultiPolygon', () => {
  afterEach(global.gc!)
  let multiPolygon: gdal.MultiPolygon

  beforeEach(() => {
    multiPolygon = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
      .layers.get(0).features.get(0).getGeometry() as gdal.MultiPolygon
  })

  it('unionCascaded() should return a Geometry', () => {
    const geom = gdal.Geometry.fromWKT(
      'MULTIPOLYGON(((0 0,0 1,1 1,1 0,0 0)),((0.5 0.5,0.5 1.5,1.5 1.5,1.5 0.5,0.5 0.5)))') as gdal.MultiPolygon
    const union = geom.unionCascaded()
    assert.instanceOf(union, gdal.Geometry)
  })

  it('unionCascaded() should throw on Error', () => {
    assert.throws(() => {
      multiPolygon.unionCascaded()
    })
  })


  it('getArea() should return a number', () => {
    const area = multiPolygon.getArea()
    assert.isNumber(area)
    assert.closeTo(area, 2.04665, 1e-3)
  })

  describe('"children" property', () => {
    it('should be an instance of GeometryCollectionChildren', () => {
      assert.instanceOf(multiPolygon.children, gdal.GeometryCollectionChildren)
    })
    describe('get()', () => {
      it('should return a Geometry', () => {
        const geom = multiPolygon.children.get(0)
        assert.instanceOf(geom, gdal.Geometry)
        assert.instanceOf(geom, gdal.Polygon)
      })
      it('should throw if the geometry does not exist', () => {
        assert.throws(() => {
          multiPolygon.children.get(112)
        })
      })
    })
    it('count() should return a number', () => {
      const n = multiPolygon.children.count()
      assert.isNumber(n)
      assert.equal(n, 1)
    })
    describe('add()', () => {
      it('should add a new element', () => {
        const geom = new gdal.Polygon()
        const count = multiPolygon.children.count()
        multiPolygon.children.add(geom)
        assert.equal(multiPolygon.children.count(), count + 1)
        assert.deepEqual(multiPolygon.children.get(count), geom)
      })
      it('should add an array of new elements', () => {
        const geom = new gdal.Polygon()
        const count = multiPolygon.children.count()
        multiPolygon.children.add([ geom ])
        assert.equal(multiPolygon.children.count(), count + 1)
        assert.deepEqual(multiPolygon.children.get(count), geom)
      })
      it('should throw on no arguments', () => {
        assert.throws(() => {
          (multiPolygon.children.add as () => void)()
        }, /must be given/)
      })
      it('should throw on invalid elements', () => {
        assert.throws(() => {
          multiPolygon.children.add({} as gdal.Geometry)
        }, /child must be a geometry/)
      })
      it('should throw on invalid array elements', () => {
        assert.throws(() => {
          multiPolygon.children.add([ {} as gdal.Geometry ])
        }, /All array elements must be geometry objects/)
      })
    })
    describe('remove()', () => {
      it('should remove an element', () => {
        const count = multiPolygon.children.count()
        multiPolygon.children.remove(count - 1)
        assert.equal(multiPolygon.children.count(), count - 1)
      })
      it('should throw on error', () => {
        assert.throws(() => {
          multiPolygon.children.remove(-2)
        })
      })
    })
  })
})
