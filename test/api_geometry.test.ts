import * as gdal from 'gdal-async'
import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as fs from 'fs'
import * as path from 'path'
const assert = chai.assert
chai.use(chaiAsPromised)
import * as semver from 'semver'

const WGS84 =
  'GEOGCS["WGS_84",DATUM["WGS_1984",SPHEROID["WGS_84",6378137,298.257223563]],PRIMEM["Greenwich",0],UNIT["Degree",0.0174532925199433],AXIS["Longitude",EAST],AXIS["Latitude",NORTH]]'

describe('gdal.Geometry', () => {
  afterEach(global.gc)

  describe('toJSON()', () => {
    it('should return valid result', () => {
      const point2d = new gdal.Point(1, 2)
      assert.deepEqual(JSON.parse(point2d.toJSON()), {
        type: 'Point',
        coordinates: [ 1, 2 ]
      })
      const point3d = new gdal.Point(1, 2, 3)
      assert.deepEqual(JSON.parse(point3d.toJSON()), {
        type: 'Point',
        coordinates: [ 1, 2, 3 ]
      })
    })
  })
  describe('toJSONAsync()', () => {
    it('should return valid result', () => {
      const point2d = new gdal.Point(1, 2)
      const json2d = point2d.toJSONAsync()
      const point3d = new gdal.Point(1, 2, 3)
      const json3d = point3d.toJSONAsync()
      return assert.isFulfilled(Promise.all([ assert.eventually.typeOf(json2d, 'string'),
        json2d.then((s) => assert.deepEqual(JSON.parse(s), {
          type: 'Point',
          coordinates: [ 1, 2 ]
        })),
        assert.eventually.typeOf(json3d, 'string'),
        json3d.then((s) =>
          assert.deepEqual(JSON.parse(s), {
            type: 'Point',
            coordinates: [ 1, 2, 3 ]
          })) ]))
    })
  })
  describe('toObject()', () => {
    it('should return valid result', () => {
      const point2d = new gdal.Point(1, 2)
      assert.deepEqual(point2d.toObject(), {
        type: 'Point',
        coordinates: [ 1, 2 ]
      })
      const point3d = new gdal.Point(1, 2, 3)
      assert.deepEqual(point3d.toObject(), {
        type: 'Point',
        coordinates: [ 1, 2, 3 ]
      })
    })
  })
  describe('toString()', () => {
    it('should return valid result', () => {
      const point = new gdal.Point(1, 2)
      assert.equal(point.toString(), 'Point')
    })
  })
  describe('toKML()', () => {
    it('should return valid result', () => {
      const point2d = new gdal.Point(1, 2)
      assert.equal(
        point2d.toKML(),
        '<Point><coordinates>1,2</coordinates></Point>'
      )
      const point3d = new gdal.Point(1, 2, 3)
      assert.equal(
        point3d.toKML(),
        '<Point><coordinates>1,2,3</coordinates></Point>'
      )
    })
  })
  describe('toKMLAsync()', () => {
    it('should return valid result', () => {
      const point2d = new gdal.Point(1, 2)
      const point3d = new gdal.Point(1, 2, 3)
      return assert.isFulfilled(Promise.all([ assert.eventually.equal(
        point2d.toKMLAsync(),
        '<Point><coordinates>1,2</coordinates></Point>'
      ),
      assert.eventually.equal(
        point3d.toKMLAsync(),
        '<Point><coordinates>1,2,3</coordinates></Point>'
      ) ]))
    })
  })
  describe('toWKT()', () => {
    it('should return valid result', () => {
      const point2d = new gdal.Point(1, 2)
      assert.equal(point2d.toWKT(), 'POINT (1 2)')
      const point3d = new gdal.Point(1, 2, 3)
      assert.equal(point3d.toWKT(), 'POINT (1 2 3)')
    })
  })
  describe('toWKTAsync()', () => {
    it('should return valid result', () => {
      const point2d = new gdal.Point(1, 2)
      const point3d = new gdal.Point(1, 2, 3)
      return assert.isFulfilled(Promise.all([ assert.eventually.equal(point2d.toWKTAsync(), 'POINT (1 2)'),
        assert.eventually.equal(point3d.toWKTAsync(), 'POINT (1 2 3)') ]))
    })
  })
  describe('toGML()', () => {
    it('should return valid result', () => {
      const point2d = new gdal.Point(1, 2)
      assert.equal(
        point2d.toGML(),
        '<gml:Point><gml:coordinates>1,2</gml:coordinates></gml:Point>'
      )
      const point3d = new gdal.Point(1, 2, 3)
      assert.equal(
        point3d.toGML(),
        '<gml:Point><gml:coordinates>1,2,3</gml:coordinates></gml:Point>'
      )
    })
  })
  describe('toGMLAsync()', () => {
    it('should return valid result', () => {
      const point2d = new gdal.Point(1, 2)
      const point3d = new gdal.Point(1, 2, 3)
      return assert.isFulfilled(Promise.all([ assert.eventually.equal(
        point2d.toGMLAsync(),
        '<gml:Point><gml:coordinates>1,2</gml:coordinates></gml:Point>'
      ),
      assert.eventually.equal(
        point3d.toGMLAsync(),
        '<gml:Point><gml:coordinates>1,2,3</gml:coordinates></gml:Point>'
      ) ]))
    })
  })
  describe('toWKBAsync()', () => {
    it('should return valid result', () => {
      const point2d = new gdal.Point(1, 2)
      const wkb = point2d.toWKBAsync()

      return assert.isFulfilled(wkb.then((wkb) => {
        let expected
        if (wkb[0] === 0) {
          expected = '00000000013ff00000000000004000000000000000'
        } else {
          expected = '0101000000000000000000f03f0000000000000040'
        }
        assert.equal(wkb.toString('hex'), expected)
      }))
    })
  })
  describe('fromWKT()', () => {
    it('should return valid result', () => {
      const point2d = gdal.Geometry.fromWKT('POINT (1 2)') as gdal.Point
      assert.equal(point2d.wkbType, gdal.wkbPoint)
      assert.equal(point2d.x, 1)
      assert.equal(point2d.y, 2)
    })
  })
  describe('fromWKTAsync()', () => {
    it('should return valid result', () => {
      const point2d = gdal.Geometry.fromWKTAsync('POINT (1 2)')
      return assert.isFulfilled(Promise.all([ assert.eventually.propertyVal(point2d, 'wkbType', gdal.wkbPoint),
        assert.eventually.propertyVal(point2d, 'x', 1),
        assert.eventually.propertyVal(point2d, 'y', 2)
      ]))
    })
  })
  describe('fromWKB()', () => {
    it('should return valid result', () => {
      const wkb = new gdal.Point(1, 2).toWKB()
      const point2d = gdal.Geometry.fromWKB(wkb) as gdal.Point
      assert.equal(point2d.wkbType, gdal.wkbPoint)
      assert.equal(point2d.x, 1)
      assert.equal(point2d.y, 2)
    })
  })
  describe('fromWKBAsync()', () => {
    it('should return valid result', () => {
      const wkb = new gdal.Point(1, 2).toWKB()
      const point2d = gdal.Geometry.fromWKBAsync(wkb)
      return assert.isFulfilled(Promise.all([ assert.eventually.propertyVal(point2d, 'wkbType', gdal.wkbPoint),
        assert.eventually.propertyVal(point2d, 'x', 1),
        assert.eventually.propertyVal(point2d, 'y', 2)
      ]))
    })
  })
  if (semver.gte(gdal.version, '2.3.0')) {
    describe('fromGeoJson()', () => {
      it('should return valid result', () => {
        const point2d = gdal.Geometry.fromGeoJson({ type: 'Point', coordinates: [ 2, 1 ] }) as gdal.Point
        assert.equal(point2d.wkbType, gdal.wkbPoint)
        assert.equal(point2d.x, 2)
        assert.equal(point2d.y, 1)
      })
      it('should throw on error', () => {
        assert.throws(() => {
          gdal.Geometry.fromGeoJson({ type: 'Garga', quantity: [ 2 ] })
        })
      })
    })
    describe('fromGeoJsonAsync()', () => {
      it('should return valid result', () => {
        const point2d = gdal.Geometry.fromGeoJsonAsync({ type: 'Point', coordinates: [ 2, 1 ] })
        return assert.isFulfilled(Promise.all([ assert.eventually.propertyVal(point2d, 'wkbType', gdal.wkbPoint),
          assert.eventually.propertyVal(point2d, 'x', 2),
          assert.eventually.propertyVal(point2d, 'y', 1)
        ]))
      })
      it('should reject on error', () =>
        assert.isRejected(gdal.Geometry.fromGeoJsonAsync({ type: 'Garga', quantity: [ 2 ] }))
      )
    })
    describe('fromGeoJsonBuffer()', () => {
      it('should return valid result', () => {
        const polygon = gdal.Geometry.fromGeoJsonBuffer(fs.readFileSync(path.join(__dirname, 'data', 'Ain.json'))) as gdal.Polygon
        assert.equal(polygon.wkbType, gdal.wkbPolygon)
      })
      it('should throw on error', () => {
        assert.throws(() => {
          gdal.Geometry.fromGeoJsonBuffer(Buffer.from('Garga'))
        })
      })
    })
    describe('fromGeoJsonBufferAsync()', () => {
      it('should return valid result', () => {
        const geom = gdal.Geometry.fromGeoJsonBufferAsync(fs.readFileSync(path.join(__dirname, 'data', 'Ain.json')))
        return assert.eventually.propertyVal(geom, 'wkbType', gdal.wkbPolygon)
      })
      it('should throw on error', () => {
        assert.isRejected(gdal.Geometry.fromGeoJsonBufferAsync(Buffer.from('Garga')))
      })
    })
  }
  describe('getConstructor()', () => {
    //  wkbUnknown = 0, wkbPoint = 1, wkbLineString = 2, wkbPolygon = 3,
    //  wkbMultiPoint = 4, wkbMultiLineString = 5, wkbMultiPolygon = 6, wkbGeometryCollection = 7,
    //  wkbNone = 100, wkbLinearRing = 101, wkbPoint25D = 0x80000001, wkbLineString25D = 0x80000002,
    //  wkbPolygon25D = 0x80000003, wkbMultiPoint25D = 0x80000004, wkbMultiLineString25D = 0x80000005, wkbMultiPolygon25D = 0x80000006,
    //  wkbGeometryCollection25D = 0x80000007
    it('should return proper constructor from wkbType', () => {
      assert.isNull(gdal.Geometry.getConstructor(0))
      assert.equal(gdal.Geometry.getConstructor(1), gdal.Point)
      assert.equal(gdal.Geometry.getConstructor(2), gdal.LineString)
      assert.equal(gdal.Geometry.getConstructor(3), gdal.Polygon)
      assert.equal(gdal.Geometry.getConstructor(4), gdal.MultiPoint)
      assert.equal(gdal.Geometry.getConstructor(5), gdal.MultiLineString)
      assert.equal(gdal.Geometry.getConstructor(6), gdal.MultiPolygon)
      assert.equal(gdal.Geometry.getConstructor(7), gdal.GeometryCollection)
      assert.equal(gdal.Geometry.getConstructor(8), gdal.CircularString)
      assert.equal(gdal.Geometry.getConstructor(9), gdal.CompoundCurve)
      assert.equal(gdal.Geometry.getConstructor(11), gdal.MultiCurve)
      assert.equal(gdal.Geometry.getConstructor(101), gdal.LinearRing)
    })
  })
  describe('"wkbType" property', () => {
    it('should be set', () => {
      assert.equal(gdal.Point.wkbType, 1)
      assert.equal(gdal.LineString.wkbType, 2)
      assert.equal(gdal.Polygon.wkbType, 3)
      assert.equal(gdal.MultiPoint.wkbType, 4)
      assert.equal(gdal.MultiLineString.wkbType, 5)
      assert.equal(gdal.MultiPolygon.wkbType, 6)
      assert.equal(gdal.GeometryCollection.wkbType, 7)
      assert.equal(gdal.CircularString.wkbType, 8)
      assert.equal(gdal.CompoundCurve.wkbType, 9)
      assert.equal(gdal.MultiCurve.wkbType, 11)
      assert.equal(gdal.LinearRing.wkbType, 101)
    })
  })

  describe('instance', () => {
    describe('"wkbType" property', () => {
      it('should be set', () => {
        assert.equal(new gdal.Point(0, 0).wkbType, 1)
        assert.equal(new gdal.LineString().wkbType, 2)
        assert.equal(new gdal.Polygon().wkbType, 3)
        assert.equal(new gdal.MultiPoint().wkbType, 4)
        assert.equal(new gdal.MultiLineString().wkbType, 5)
        assert.equal(new gdal.MultiPolygon().wkbType, 6)
        assert.equal(new gdal.GeometryCollection().wkbType, 7)
        assert.equal(new gdal.CircularString().wkbType, 8)
        assert.equal(new gdal.CompoundCurve().wkbType, 9)
        assert.equal(new gdal.MultiCurve().wkbType, 11)
        assert.equal(new gdal.LinearRing().wkbType, 101)
      })
    })
    describe('"coordinateDimension" property', () => {
      it('should be set', () => {
        assert.equal(new gdal.Point(1, 2, 3).coordinateDimension, 3)
      })
      it('should be writable', () => {
        const point = new gdal.Point(1, 2, 3)
        assert.equal(point.coordinateDimension, 3)
        assert.equal(point.z, 3)
        point.coordinateDimension = 2
        assert.equal(point.coordinateDimension, 2)
        assert.equal(point.z, 0)
      })
    })
    describe('"srs" property', () => {
      it('should be able to be get', () => {
        const point = new gdal.Point(0, 0)
        assert.equal(point.srs, null)

        point.srs = gdal.SpatialReference.fromWKT(WGS84)
        assert.instanceOf(point.srs, gdal.SpatialReference)
        assert.equal(point.srs.toWKT(), WGS84)
      })
      it('should be able to be set', () => {
        const point = new gdal.Point(1, 2)
        point.srs = gdal.SpatialReference.fromWKT(WGS84)
        point.srs = null
      })
      it('must require SpatialReference when setting', () => {
        const point = new gdal.Point(1, 2)
        assert.throws(() => {
          /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
          point.srs = 'invalid' as any
        })
      })
    })

    describe('empty()', () => {
      it('should empty the geometry', () => {
        const ring = new gdal.LinearRing()
        ring.points.add({ x: 0, y: 0 })
        ring.points.add({ x: 10, y: 0 })
        ring.points.add({ x: 10, y: 10 })
        ring.points.add({ x: 0, y: 10 })
        ring.closeRings()
        assert.isFalse(ring.isEmpty())
        ring.empty()
        assert.isTrue(ring.isEmpty())
      })
    });

    // comparison functions
    (function () {
      let ring: gdal.LinearRing, square: gdal.Polygon, point_inner: gdal.Point,
        point_inner_clone: gdal.Point, point_outer: gdal.Point, arc: gdal.SimpleCurve
      before(() => {
        ring = new gdal.LinearRing()
        ring.points.add({ x: 0, y: 0 })
        ring.points.add({ x: 10, y: 0 })
        ring.points.add({ x: 10, y: 10 })
        ring.points.add({ x: 0, y: 10 })
        ring.closeRings()

        square = new gdal.Polygon()
        square.rings.add(ring)

        point_inner = new gdal.Point(5, 5)
        point_outer = new gdal.Point(0, 20)
        point_inner_clone = new gdal.Point(5, 5)

        arc = new gdal.CircularString()
        arc.points.add({ x: 0, y: 5 })
        arc.points.add({ x: 10, y: 0 })
        arc.points.add({ x: 0, y: -5 })
      })
      describe('contains()', () => {
        it('should return correct result', () => {
          assert.equal(square.contains(point_inner), true)
          assert.equal(square.contains(point_outer), false)
        })
      })
      describe('within()', () => {
        it('should return correct result', () => {
          assert.equal(point_inner.within(square), true)
          assert.equal(point_outer.within(square), false)
        })
      })
      describe('intersects()', () => {
        it('should return correct result', () => {
          assert.equal(point_inner.intersects(square), true)
          assert.equal(point_outer.intersects(square), false)
        })
      })
      describe('equals()', () => {
        it('should return correct result', () => {
          assert.equal(point_inner.equals(square), false)
          assert.equal(point_outer.equals(square), false)
          assert.equal(point_inner.equals(point_inner_clone), true)
        })
        it('should determine if geometries are identical', () => {
          const point1 = new gdal.Point(3, 3)
          const point2 = new gdal.Point(3, 3)
          const point3 = new gdal.Point(5, 3)
          assert.equal(point1.equals(point2), true)
          assert.equal(point1.equals(point3), false)
        })
      })
      describe('disjoint()', () => {
        it('should return correct result', () => {
          assert.equal(point_inner.disjoint(square), false)
          assert.equal(point_outer.disjoint(square), true)
        })
      })
      describe('disjointAsync()', () => {
        it('should return correct result', () => Promise.all([ assert.eventually.equal(point_inner.disjointAsync(square), false),
          assert.eventually.equal(point_outer.disjointAsync(square), true)
        ]))
      })
      describe('crosses()', () => {
        it('should return correct result', () => {
          const line_cross = new gdal.LineString()
          line_cross.points.add(-1, -1)
          line_cross.points.add(11, 11)
          const line_nocross = new gdal.LineString()
          line_nocross.points.add(-1, -1)
          line_nocross.points.add(-1, 11)

          assert.equal(point_inner.crosses(square), false)
          assert.equal(point_outer.crosses(square), false)
          assert.equal(line_cross.crosses(square), true)
          assert.equal(line_nocross.crosses(square), false)
          assert.equal(line_cross.crosses(arc), true)
          assert.equal(line_nocross.crosses(arc), false)
        })
      })
      describe('overlaps()', () => {
        it('should return correct result', () => {
          const ring1 = new gdal.LinearRing()
          ring1.points.add({ x: 1, y: 0 })
          ring1.points.add({ x: 11, y: 0 })
          ring1.points.add({ x: 11, y: 10 })
          ring1.points.add({ x: 1, y: 10 })
          ring1.closeRings()

          const square1 = new gdal.Polygon()
          square1.rings.add(ring1)

          const ring2 = new gdal.LinearRing()
          ring2.points.add({ x: 100, y: 0 })
          ring2.points.add({ x: 110, y: 0 })
          ring2.points.add({ x: 110, y: 10 })
          ring2.points.add({ x: 100, y: 10 })
          ring2.closeRings()

          const square2 = new gdal.Polygon()
          square2.rings.add(ring2)

          assert.equal(square1.overlaps(square), true)
          assert.equal(square2.overlaps(square), false)
        })
      })
      describe('overlapsAsync()', () => {
        it('should return correct result', () => {
          const ring1 = new gdal.LinearRing()
          ring1.points.add({ x: 1, y: 0 })
          ring1.points.add({ x: 11, y: 0 })
          ring1.points.add({ x: 11, y: 10 })
          ring1.points.add({ x: 1, y: 10 })
          ring1.closeRings()

          const square1 = new gdal.Polygon()
          square1.rings.add(ring1)

          const ring2 = new gdal.LinearRing()
          ring2.points.add({ x: 100, y: 0 })
          ring2.points.add({ x: 110, y: 0 })
          ring2.points.add({ x: 110, y: 10 })
          ring2.points.add({ x: 100, y: 10 })
          ring2.closeRings()

          const square2 = new gdal.Polygon()
          square2.rings.add(ring2)

          return assert.isFulfilled(Promise.all([ assert.eventually.equal(square1.overlapsAsync(square), true),
            assert.eventually.equal(square2.overlapsAsync(square), false)
          ]))
        })
      })

      describe('touches()', () => {
        it('should return correct result', () => {
          const point_edge = new gdal.Point(10, 0)
          assert.equal(point_edge.touches(square), true)
          assert.equal(point_outer.touches(square), false)
        })
      })

      describe('isRing()', () => {
        it('should return correct result', () => {
          assert.isFalse(square.isRing())
          const ring1 = new gdal.LineString()
          ring1.points.add({ x: 1, y: 0 })
          ring1.points.add({ x: 11, y: 0 })
          ring1.points.add({ x: 11, y: 10 })
          ring1.points.add({ x: 1, y: 0 })
          assert.isTrue(ring1.isRing())
        })
      })
    })()

    // calculation functions
    describe('distance()', () => {
      it('should return correct result', () => {
        const point1 = new gdal.Point(0, 0)
        const point2 = new gdal.Point(10, 10)
        const distance_expected = Math.sqrt(10 * 10 + 10 * 10)
        const distance_actual = point1.distance(point2)
        assert.closeTo(distance_actual, distance_expected, 0.001)
      })
    })
    describe('distanceAsync()', () => {
      it('should return correct result', () => {
        const point1 = new gdal.Point(0, 0)
        const point2 = new gdal.Point(10, 10)
        const distance_expected = Math.sqrt(10 * 10 + 10 * 10)
        const distance_actual = point1.distanceAsync(point2)
        // TODO: this seems to be a bug in the TS bindings of chai
        // distance_actual should be a Promise<number>, it is a number
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        return assert.eventually.closeTo(distance_actual as any, distance_expected, 0.001)
      })
    })
    describe('convexHull()', () => {
      it('should return geometry without inner rings', () => {
        const outerRing = new gdal.LinearRing()
        outerRing.points.add({ x: 0, y: 0 })
        outerRing.points.add({ x: 20, y: 0 })
        outerRing.points.add({ x: 20, y: 10 })
        outerRing.points.add({ x: 0, y: 10 })
        outerRing.closeRings()
        const innerRing = new gdal.LinearRing()
        innerRing.points.add({ x: 1, y: 9 })
        innerRing.points.add({ x: 19, y: 9 })
        innerRing.points.add({ x: 19, y: 1 })
        innerRing.points.add({ x: 1, y: 1 })
        innerRing.closeRings()

        const squareDonut = new gdal.Polygon()
        squareDonut.rings.add(outerRing)
        squareDonut.rings.add(innerRing)

        const hull = squareDonut.convexHull()
        assert.instanceOf(hull, gdal.Polygon)
      })
    })
    describe('convexHullAsync()', () => {
      it('should return geometry without inner rings', () => {
        const outerRing = new gdal.LinearRing()
        outerRing.points.add({ x: 0, y: 0 })
        outerRing.points.add({ x: 20, y: 0 })
        outerRing.points.add({ x: 20, y: 10 })
        outerRing.points.add({ x: 0, y: 10 })
        outerRing.closeRings()
        const innerRing = new gdal.LinearRing()
        innerRing.points.add({ x: 1, y: 9 })
        innerRing.points.add({ x: 19, y: 9 })
        innerRing.points.add({ x: 19, y: 1 })
        innerRing.points.add({ x: 1, y: 1 })
        innerRing.closeRings()

        const squareDonut = new gdal.Polygon()
        squareDonut.rings.add(outerRing)
        squareDonut.rings.add(innerRing)

        const hull = squareDonut.convexHullAsync()
        return assert.eventually.instanceOf(hull, gdal.Polygon)
      })
    })
    describe('boundary()', () => {
      it('should return geometry without inner rings', () => {
        const outerRing = new gdal.LinearRing()
        outerRing.points.add({ x: 0, y: 0 })
        outerRing.points.add({ x: 20, y: 0 })
        outerRing.points.add({ x: 20, y: 10 })
        outerRing.points.add({ x: 0, y: 10 })
        outerRing.closeRings()
        const innerRing = new gdal.LinearRing()
        innerRing.points.add({ x: 1, y: 9 })
        innerRing.points.add({ x: 19, y: 9 })
        innerRing.points.add({ x: 19, y: 1 })
        innerRing.points.add({ x: 1, y: 1 })
        innerRing.closeRings()

        const squareDonut = new gdal.Polygon()
        squareDonut.rings.add(outerRing)
        squareDonut.rings.add(innerRing)

        const boundary = squareDonut.boundary()
        assert.instanceOf(boundary, gdal.MultiLineString)
      })
    })
    describe('boundaryAsync()', () => {
      it('should return geometry without inner rings', () => {
        const outerRing = new gdal.LinearRing()
        outerRing.points.add({ x: 0, y: 0 })
        outerRing.points.add({ x: 20, y: 0 })
        outerRing.points.add({ x: 20, y: 10 })
        outerRing.points.add({ x: 0, y: 10 })
        outerRing.closeRings()
        const innerRing = new gdal.LinearRing()
        innerRing.points.add({ x: 1, y: 9 })
        innerRing.points.add({ x: 19, y: 9 })
        innerRing.points.add({ x: 19, y: 1 })
        innerRing.points.add({ x: 1, y: 1 })
        innerRing.closeRings()

        const squareDonut = new gdal.Polygon()
        squareDonut.rings.add(outerRing)
        squareDonut.rings.add(innerRing)

        const boundary = squareDonut.boundaryAsync()
        return assert.eventually.instanceOf(boundary, gdal.MultiLineString)
      })
    })
    describe('centroid()', () => {
      it('should return correct result', () => {
        const ring = new gdal.LinearRing()
        ring.points.add({ x: 0, y: 0 })
        ring.points.add({ x: 20, y: 0 })
        ring.points.add({ x: 20, y: 10 })
        ring.points.add({ x: 0, y: 10 })
        ring.closeRings()

        const square = new gdal.Polygon()
        square.rings.add(ring)

        const centroid = square.centroid()

        assert.instanceOf(centroid, gdal.Point)
        assert.closeTo(centroid.x, 10, 0.0001)
        assert.closeTo(centroid.y, 5, 0.0001)
      })
    })
    describe('centroidAsync()', () => {
      it('should return correct result', () => {
        const ring = new gdal.LinearRing()
        ring.points.add({ x: 0, y: 0 })
        ring.points.add({ x: 20, y: 0 })
        ring.points.add({ x: 20, y: 10 })
        ring.points.add({ x: 0, y: 10 })
        ring.closeRings()

        const square = new gdal.Polygon()
        square.rings.add(ring)

        const centroid = square.centroidAsync()

        return assert.isFulfilled(Promise.all([ assert.eventually.instanceOf(centroid, gdal.Point),
          assert.eventually.propertyVal(centroid, 'x', 10),
          assert.eventually.propertyVal(centroid, 'y', 5)
        ]))
      })
    })
    describe('buffer()', () => {
      it('should return correct result', () => {
        const point = new gdal.Point(0, 0)
        const circle = point.buffer(1, 1000) as gdal.Polygon
        assert.instanceOf(circle, gdal.Polygon)
        assert.closeTo(circle.getArea(), 3.1415, 0.0001)
      })
    })
    describe('bufferAsync()', () => {
      it('should return correct result', () => {
        const point = new gdal.Point(0, 0)
        const circle = point.bufferAsync(1, 1000) as Promise<gdal.Polygon>
        return assert.isFulfilled(Promise.all([ assert.eventually.instanceOf(circle, gdal.Polygon),
          // TODO: this seems to be a bug in the TS bindings of chai
          /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
          assert.eventually.closeTo(circle.then((r) => r.getArea()) as any, 3.1415, 0.0001)
        ]))
      })
    })
    describe('simplify()', () => {
      it('should return simplified LineString', () => {
        const line = new gdal.LineString()
        line.points.add(0, 0)
        line.points.add(1, 1)
        line.points.add(10, 10)
        line.points.add(2, 2)
        line.points.add(5, 5)

        const simplified = line.simplify(0.1) as gdal.LineString
        assert.instanceOf(simplified, gdal.LineString)
        assert.equal(simplified.points.count(), 4)
        assert.closeTo(simplified.points.get(0).x, 0, 0.001)
        assert.closeTo(simplified.points.get(0).y, 0, 0.001)
        assert.closeTo(simplified.points.get(1).x, 10, 0.001)
        assert.closeTo(simplified.points.get(1).y, 10, 0.001)
        assert.closeTo(simplified.points.get(2).x, 2, 0.001)
        assert.closeTo(simplified.points.get(2).y, 2, 0.001)
        assert.closeTo(simplified.points.get(3).x, 5, 0.001)
        assert.closeTo(simplified.points.get(3).y, 5, 0.001)
      })
    })
    describe('simplifyAsync()', () => {
      it('should return simplified LineString', () => {
        const line = new gdal.LineString()
        line.points.add(0, 0)
        line.points.add(1, 1)
        line.points.add(10, 10)
        line.points.add(2, 2)
        line.points.add(5, 5)

        const simplified = line.simplifyAsync(0.1) as Promise<gdal.LineString>
        return assert.isFulfilled(Promise.all([ assert.eventually.instanceOf(simplified, gdal.LineString),
          assert.eventually.equal(simplified.then((r) => r.points.count()), 4)
        ]))
      })
    })
    describe('simplifyPreserveTopology()', () => {
      it('should return simplified LineString', () => {
        const line = new gdal.LineString()
        line.points.add(0, 0)
        line.points.add(10, 10)
        line.points.add(1, 1)
        line.points.add(2, 2)
        line.points.add(3, 3)
        line.points.add(4, 4)
        line.points.add(5, 5)

        const simplified = line.simplifyPreserveTopology(0.1) as gdal.LineString
        assert.instanceOf(simplified, gdal.LineString)
        assert.equal(simplified.points.count(), 7)
        assert.closeTo(simplified.points.get(0).x, 0, 0.001)
        assert.closeTo(simplified.points.get(0).y, 0, 0.001)
        assert.closeTo(simplified.points.get(1).x, 10, 0.001)
        assert.closeTo(simplified.points.get(1).y, 10, 0.001)
        assert.closeTo(simplified.points.get(6).x, 5, 0.001)
        assert.closeTo(simplified.points.get(6).y, 5, 0.001)
      })
    })
    describe('simplifyPreserveTopologyAsync()', () => {
      it('should return simplified LineString', () => {
        const line = new gdal.LineString()
        line.points.add(0, 0)
        line.points.add(10, 10)
        line.points.add(1, 1)
        line.points.add(2, 2)
        line.points.add(3, 3)
        line.points.add(4, 4)
        line.points.add(5, 5)

        const simplified = line.simplifyPreserveTopologyAsync(0.1) as Promise<gdal.LineString>
        return assert.isFulfilled(Promise.all([ assert.eventually.instanceOf(simplified, gdal.LineString),
          assert.eventually.equal(simplified.then((r) => r.points.count()), 7)
        ]))
      })
    })
    describe('segmentize()', () => {
      it('should segmentize the geometry', () => {
        const line = new gdal.LineString()
        line.points.add(0, 0)
        line.points.add(1, 1)
        line.points.add(10, 10)

        line.segmentize(2)
        assert.instanceOf(line, gdal.LineString)
        assert.equal(line.points.count(), 9)
      })
    })
    describe('flattenTo2D()', () => {
      it('should flatten a LineString', () => {
        const points = [
          [ 0, 0, 1 ],
          [ 1, 1, 2 ],
          [ 10, 10, 3 ],
          [ 2, 2, 0 ],
          [ 5, 5, 10 ]
        ]
        const line = new gdal.LineString()
        points.forEach((p) => line.points.add(p[0], p[1], p[2]))

        line.flattenTo2D()
        assert.instanceOf(line, gdal.LineString)
        assert.equal(line.points.count(), points.length)

        points.forEach((p, idx) => {
          const p2 = line.points.get(idx)
          assert.equal(p2.x, p[0])
          assert.equal(p2.y, p[1])
          assert.equal(p2.z, 0)
        })
      })
    })
    describe('flattenTo2DAsync()', () => {
      it('should flatten a LineString', () => {
        const points = [
          [ 0, 0, 1 ],
          [ 1, 1, 2 ],
          [ 10, 10, 3 ],
          [ 2, 2, 0 ],
          [ 5, 5, 10 ]
        ]
        const line = new gdal.LineString()
        points.forEach((p) => line.points.add(p[0], p[1], p[2]))

        return assert.isFulfilled(line.flattenTo2DAsync().then(() => {
          assert.instanceOf(line, gdal.LineString)
          assert.equal(line.points.count(), points.length)

          points.forEach((p, idx) => {
            const p2 = line.points.get(idx)
            assert.equal(p2.x, p[0])
            assert.equal(p2.y, p[1])
            assert.equal(p2.z, 0)
          })
        }))
      })
    })
    describe('union()', () => {
      it('should merge geometries', () => {
        const ring1 = new gdal.LinearRing()
        ring1.points.add({ x: 0, y: 0 })
        ring1.points.add({ x: 10, y: 0 })
        ring1.points.add({ x: 10, y: 10 })
        ring1.points.add({ x: 0, y: 10 })
        ring1.closeRings()

        const square1 = new gdal.Polygon()
        square1.rings.add(ring1)

        const ring2 = new gdal.LinearRing()
        ring2.points.add({ x: 10, y: 0 })
        ring2.points.add({ x: 20, y: 0 })
        ring2.points.add({ x: 20, y: 10 })
        ring2.points.add({ x: 10, y: 10 })
        ring2.closeRings()

        const square2 = new gdal.Polygon()
        square2.rings.add(ring2)

        const result = square1.union(square2) as gdal.Polygon
        assert.instanceOf(result, gdal.Polygon)
        assert.equal(result.getArea(), 200)
      })
    })
    describe('unionAsync()', () => {
      it('should merge geometries', () => {
        const ring1 = new gdal.LinearRing()
        ring1.points.add({ x: 0, y: 0 })
        ring1.points.add({ x: 10, y: 0 })
        ring1.points.add({ x: 10, y: 10 })
        ring1.points.add({ x: 0, y: 10 })
        ring1.closeRings()

        const square1 = new gdal.Polygon()
        square1.rings.add(ring1)

        const ring2 = new gdal.LinearRing()
        ring2.points.add({ x: 10, y: 0 })
        ring2.points.add({ x: 20, y: 0 })
        ring2.points.add({ x: 20, y: 10 })
        ring2.points.add({ x: 10, y: 10 })
        ring2.closeRings()

        const square2 = new gdal.Polygon()
        square2.rings.add(ring2)

        const result = square1.unionAsync(square2) as Promise<gdal.Polygon>
        return assert.isFulfilled(Promise.all([ assert.eventually.instanceOf(result, gdal.Polygon),
          assert.eventually.equal(result.then((r) => r.getArea()), 200)
        ]))
      })
    })
    describe('intersection()', () => {
      it('should return the intersection of two geometries', () => {
        const ring1 = new gdal.LinearRing()
        ring1.points.add({ x: 0, y: 0 })
        ring1.points.add({ x: 10, y: 0 })
        ring1.points.add({ x: 10, y: 10 })
        ring1.points.add({ x: 0, y: 10 })
        ring1.closeRings()

        const square1 = new gdal.Polygon()
        square1.rings.add(ring1)

        const ring2 = new gdal.LinearRing()
        ring2.points.add({ x: 5, y: 0 })
        ring2.points.add({ x: 20, y: 0 })
        ring2.points.add({ x: 20, y: 10 })
        ring2.points.add({ x: 5, y: 10 })
        ring2.closeRings()

        const square2 = new gdal.Polygon()
        square2.rings.add(ring2)

        const result = square1.intersection(square2) as gdal.Polygon
        assert.instanceOf(result, gdal.Polygon)
        assert.equal(result.getArea(), 50)
      })
    })
    describe('intersectionAsync()', () => {
      it('should return the intersection of two geometries', () => {
        const ring1 = new gdal.LinearRing()
        ring1.points.add({ x: 0, y: 0 })
        ring1.points.add({ x: 10, y: 0 })
        ring1.points.add({ x: 10, y: 10 })
        ring1.points.add({ x: 0, y: 10 })
        ring1.closeRings()

        const square1 = new gdal.Polygon()
        square1.rings.add(ring1)

        const ring2 = new gdal.LinearRing()
        ring2.points.add({ x: 5, y: 0 })
        ring2.points.add({ x: 20, y: 0 })
        ring2.points.add({ x: 20, y: 10 })
        ring2.points.add({ x: 5, y: 10 })
        ring2.closeRings()

        const square2 = new gdal.Polygon()
        square2.rings.add(ring2)

        const result = square1.intersectionAsync(square2) as Promise<gdal.Polygon>
        return assert.isFulfilled(Promise.all([ assert.eventually.instanceOf(result, gdal.Polygon),
          assert.eventually.equal(result.then((r) => r.getArea()), 50)
        ]))
      })
    })
    describe('difference()', () => {
      it('should return the difference of two geometries', () => {
        const ring1 = new gdal.LinearRing()
        ring1.points.add({ x: 0, y: 0 })
        ring1.points.add({ x: 10, y: 0 })
        ring1.points.add({ x: 10, y: 10 })
        ring1.points.add({ x: 0, y: 10 })
        ring1.closeRings()

        const square1 = new gdal.Polygon()
        square1.rings.add(ring1)

        const ring2 = new gdal.LinearRing()
        ring2.points.add({ x: 5, y: 0 })
        ring2.points.add({ x: 20, y: 0 })
        ring2.points.add({ x: 20, y: 10 })
        ring2.points.add({ x: 5, y: 10 })
        ring2.closeRings()

        const square2 = new gdal.Polygon()
        square2.rings.add(ring2)

        const result = square1.difference(square2) as gdal.Polygon
        assert.instanceOf(result, gdal.Polygon)
        assert.equal(result.getArea(), 50)
      })
    })
    describe('differenceAsync()', () => {
      it('should return the difference of two geometries', () => {
        const ring1 = new gdal.LinearRing()
        ring1.points.add({ x: 0, y: 0 })
        ring1.points.add({ x: 10, y: 0 })
        ring1.points.add({ x: 10, y: 10 })
        ring1.points.add({ x: 0, y: 10 })
        ring1.closeRings()

        const square1 = new gdal.Polygon()
        square1.rings.add(ring1)

        const ring2 = new gdal.LinearRing()
        ring2.points.add({ x: 5, y: 0 })
        ring2.points.add({ x: 20, y: 0 })
        ring2.points.add({ x: 20, y: 10 })
        ring2.points.add({ x: 5, y: 10 })
        ring2.closeRings()

        const square2 = new gdal.Polygon()
        square2.rings.add(ring2)

        const result = square1.differenceAsync(square2) as Promise<gdal.Polygon>
        return assert.isFulfilled(Promise.all([ assert.eventually.instanceOf(result, gdal.Polygon),
          assert.eventually.equal(result.then((r) => r.getArea()), 50)
        ]))
      })
    })
    describe('symDifference()', () => {
      it('should return the symetric difference of two geometries', () => {
        const ring1 = new gdal.LinearRing()
        ring1.points.add({ x: 0, y: 0 })
        ring1.points.add({ x: 10, y: 0 })
        ring1.points.add({ x: 10, y: 10 })
        ring1.points.add({ x: 0, y: 10 })
        ring1.closeRings()

        const square1 = new gdal.Polygon()
        square1.rings.add(ring1)

        const ring2 = new gdal.LinearRing()
        ring2.points.add({ x: 5, y: 0 })
        ring2.points.add({ x: 20, y: 0 })
        ring2.points.add({ x: 20, y: 10 })
        ring2.points.add({ x: 5, y: 10 })
        ring2.closeRings()

        const square2 = new gdal.Polygon()
        square2.rings.add(ring2)

        const result = square1.symDifference(square2) as gdal.MultiPolygon
        assert.instanceOf(result, gdal.MultiPolygon)
        assert.equal(result.getArea(), 150)
      })
    })
    it('with bundled GDAL, makeValid', () => {
      const json = JSON.parse(fs.readFileSync(path.join(__dirname, 'data', 'makeValid.json'), 'utf-8'))
      const invalid = gdal.Geometry.fromGeoJson(json)
      const valid = invalid.makeValid()
      assert.instanceOf(valid, gdal.GeometryCollection)
      assert.closeTo((valid as gdal.GeometryCollection).getArea(), 0.012428372488501122, 1e-6)
    })
    it('with bundled GDAL, makeValidAsync', () => {
      const json = JSON.parse(fs.readFileSync(path.join(__dirname, 'data', 'makeValid.json'), 'utf-8'))
      const invalid = gdal.Geometry.fromGeoJson(json)
      const valid = invalid.makeValidAsync()
      return assert.eventually.instanceOf(valid, gdal.GeometryCollection)
    })
  })
})
