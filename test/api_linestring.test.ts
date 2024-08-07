import * as gdal from 'gdal-async'
import { assert } from 'chai'

describe('gdal.LineString', () => {
  afterEach(global.gc!)

  it('should be instantiable', () => {
    new gdal.LineString()
  })
  it('should inherit from Geometry', () => {
    assert.instanceOf(new gdal.LineString(), gdal.LineString)
    assert.instanceOf(new gdal.LineString(), gdal.SimpleCurve)
    assert.instanceOf(new gdal.LineString(), gdal.Geometry)
  })
  describe('instance', () => {
    describe('getLength()', () => {
      it('should return length', () => {
        const line = new gdal.LineString()
        line.points.add(0, 0, 0)
        line.points.add(10, 10, 0)
        line.points.add(10, 20, 0)
        assert.closeTo(
          line.getLength(),
          Math.sqrt(10 * 10 + 10 * 10) + 10,
          0.001
        )
      })
    })
    describe('addSubLineString()', () => {
      it('should append to current linestring', () => {
        const a = gdal.Geometry.fromWKT('LINESTRING(0 0, 0 1, 0 2)') as gdal.LineString
        const b = gdal.Geometry.fromWKT('LINESTRING(0 2, 1 2, 2 2)') as gdal.LineString
        const c = gdal.Geometry.fromWKT(
          'LINESTRING(0 0, 0 1, 0 2, 0 2, 1 2, 2 2)'
        )
        assert.instanceOf(a, gdal.LineString)
        assert.instanceOf(b, gdal.LineString)
        a.addSubLineString(b)
        assert.isTrue(a.equals(c))
      })
      it('should use start option', () => {
        const a = gdal.Geometry.fromWKT('LINESTRING(0 0, 0 1, 0 2)') as gdal.LineString
        const b = gdal.Geometry.fromWKT('LINESTRING(0 2, 1 2, 2 2)') as gdal.LineString
        const c = gdal.Geometry.fromWKT('LINESTRING(0 0, 0 1, 0 2, 1 2, 2 2)')
        assert.instanceOf(a, gdal.LineString)
        assert.instanceOf(b, gdal.LineString)
        a.addSubLineString(b, 1)
        assert.isTrue(a.equals(c))
      })
      it('should use end option', () => {
        const a = gdal.Geometry.fromWKT('LINESTRING(0 0, 0 1, 0 2)') as gdal.LineString
        const b = gdal.Geometry.fromWKT('LINESTRING(0 2, 1 2, 2 2)') as gdal.LineString
        const c = gdal.Geometry.fromWKT('LINESTRING(0 0, 0 1, 0 2, 1 2)')
        assert.instanceOf(a, gdal.LineString)
        assert.instanceOf(b, gdal.LineString)
        a.addSubLineString(b, 1, 1)
        assert.isTrue(a.equals(c))
      })
      it('should throw if given a non-linestring', () => {
        const a = gdal.Geometry.fromWKT('LINESTRING(0 0, 0 1, 0 2)') as gdal.LineString
        const b = gdal.Geometry.fromWKT('POINT(0 2)')
        assert.instanceOf(a, gdal.LineString)
        assert.instanceOf(b, gdal.Point)
        assert.throws(() => {
          /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
          a.addSubLineString(b as any)
        })
      })
      it('should throw if given invalid indexes', () => {
        const a = gdal.Geometry.fromWKT('LINESTRING(0 0, 0 1, 0 2)') as gdal.LineString
        const b = gdal.Geometry.fromWKT('LINESTRING(1 2)') as gdal.LineString
        assert.instanceOf(a, gdal.LineString)
        assert.instanceOf(b, gdal.LineString)
        assert.throws(() => {
          a.addSubLineString(b, -1)
        })
        assert.throws(() => {
          a.addSubLineString(b, 1)
        })
        assert.throws(() => {
          a.addSubLineString(b, 0, 1)
        })
        assert.throws(() => {
          a.addSubLineString(b, 0, -2)
        })
      })
    })
    describe('"points" property', () => {
      describe('count()', () => {
        it('should return number of points', () => {
          const line = new gdal.LineString()
          line.points.add(0, 0, 0)
          line.points.add(10, 10, 0)
          line.points.add(10, 10, 2)
          assert.equal(line.points.count(), 3)
        })
      })
      describe('add()', () => {
        it('should accept gdal.Point instance', () => {
          const line = new gdal.LineString()
          const pt = new gdal.Point(2, 3)
          line.points.add(pt)
          const pt_result = line.points.get(0)
          assert.equal(pt_result.x, 2)
          assert.equal(pt_result.y, 3)
        })
        it('should accept gdal.Point array', () => {
          const line = new gdal.LineString()
          line.points.add([ new gdal.Point(2, 3), new gdal.Point(4, 5) ])
          const pt_result1 = line.points.get(0)
          assert.equal(pt_result1.x, 2)
          assert.equal(pt_result1.y, 3)
          const pt_result2 = line.points.get(1)
          assert.equal(pt_result2.x, 4)
          assert.equal(pt_result2.y, 5)
        })
        it('should accept object', () => {
          const line = new gdal.LineString()
          line.points.add({ x: 2, y: 3 })
          const pt_result = line.points.get(0)
          assert.equal(pt_result.x, 2)
          assert.equal(pt_result.y, 3)
        })
        it('should accept object array', () => {
          const line = new gdal.LineString()
          line.points.add([
            { x: 2, y: 3 },
            { x: 4, y: 5 }
          ])
          const pt_result1 = line.points.get(0)
          assert.equal(pt_result1.x, 2)
          assert.equal(pt_result1.y, 3)
          const pt_result2 = line.points.get(1)
          assert.equal(pt_result2.x, 4)
          assert.equal(pt_result2.y, 5)
        })
        it('should accept x,y,z arguments', () => {
          const line = new gdal.LineString()
          line.points.add(1, 2, 3)
          const pt_result = line.points.get(0)
          assert.equal(pt_result.x, 1)
          assert.equal(pt_result.y, 2)
          assert.equal(pt_result.z, 3)
        })
      })
      describe('set()', () => {
        it("should throw if point doesn't exist", () => {
          assert.throws(() => {
            const line = new gdal.LineString()
            line.points.set(1, new gdal.Point(1, 1))
            line.points.set(1, { x: 1, y: 1 })
            line.points.set(1, 1, 1, 1)
          })
        })
        it('should accept object', () => {
          const line = new gdal.LineString()
          line.points.add(0, 0)
          line.points.set(0, { x: 1, y: 2 })
          const pt = line.points.get(0)
          assert.equal(pt.x, 1)
          assert.equal(pt.y, 2)
        })
        it('should accept Point instance', () => {
          const line = new gdal.LineString()
          line.points.add(0, 0)
          line.points.set(0, new gdal.Point(1, 2))
          const pt = line.points.get(0)
          assert.equal(pt.x, 1)
          assert.equal(pt.y, 2)
        })
        it('should accept x,y,z arguments', () => {
          const line = new gdal.LineString()
          line.points.add(0, 0)
          line.points.set(0, 1, 2, 3)
          const pt = line.points.get(0)
          assert.equal(pt.x, 1)
          assert.equal(pt.y, 2)
          assert.equal(pt.z, 3)
        })
      })
      describe('get()', () => {
        it("should throw if point doesn't exist", () => {
          const line = new gdal.LineString()
          assert.throws(() => {
            line.points.get(2)
          })
        })
        it('should Point instance', () => {
          const line = new gdal.LineString()
          line.points.add(1, 2, 3)
          const pt = line.points.get(0)
          assert.instanceOf(pt, gdal.Point)
          assert.equal(pt.x, 1)
          assert.equal(pt.y, 2)
          assert.equal(pt.z, 3)
        })
      })
      describe('resize()', () => {
        it('should adjust the number of points', () => {
          const line = new gdal.LineString()
          line.points.add(1, 2, 3)
          line.points.add(2, 3, 4)
          line.points.add(3, 4, 5)
          line.points.resize(2)
          assert.equal(line.points.count(), 2)
        })
      })
      describe('reverse()', () => {
        it('should flip order of points', () => {
          const line = new gdal.LineString()
          line.points.add(1, 2, 3)
          line.points.add(2, 3, 4)
          line.points.add(3, 4, 5)
          line.points.reverse()
          const p1 = line.points.get(0)
          const p2 = line.points.get(1)
          const p3 = line.points.get(2)
          assert.equal(p1.x, 3)
          assert.equal(p2.x, 2)
          assert.equal(p3.x, 1)
        })
      })
      describe('forEach()', () => {
        it('should stop if callback returns false', () => {
          const line = new gdal.LineString()
          line.points.add(1, 2, 3)
          line.points.add(2, 3, 4)
          line.points.add(3, 4, 5)
          let count = 0
          line.points.forEach((pt, i) => {
            count++
            assert.isNumber(i)
            if (i === 1) return false
          })
          assert.equal(count, 2)
        })
        it('should iterate through all points', () => {
          const x_expected = [ 1, 2, 3 ]
          const x_actual = [] as number[]
          const line = new gdal.LineString()
          line.points.add(1, 2, 3)
          line.points.add(2, 3, 4)
          line.points.add(3, 4, 5)
          line.points.forEach((pt, i) => {
            assert.isNumber(i)
            x_actual.push(pt.x)
          })
          assert.deepEqual(x_actual, x_expected)
        })
      })
      describe('@@iterator()', () => {
        it('should iterate through all points', () => {
          const x_expected = [ 1, 2, 3 ]
          const x_actual = []
          const line = new gdal.LineString()
          line.points.add(1, 2, 3)
          line.points.add(2, 3, 4)
          line.points.add(3, 4, 5)
          for (const pt of line.points) {
            x_actual.push(pt.x)
          }
          assert.deepEqual(x_actual, x_expected)
        })
      })
      describe('map()', () => {
        it('should operate normally', () => {
          const line = new gdal.LineString()
          line.points.add(1, 2, 3)
          line.points.add(2, 3, 4)
          line.points.add(3, 4, 5)
          const result = line.points.map((pt, i) =>
            [ pt.x, pt.y, pt.z, i ].join('/')
          )

          assert.deepEqual(result, [ '1/2/3/0', '2/3/4/1', '3/4/5/2' ])
        })
      })
      describe('toArray()', () => {
        it('should return array of Point instances', () => {
          const line = new gdal.LineString()
          line.points.add(1, 2, 3)
          line.points.add(2, 3, 4)
          line.points.add(3, 4, 5)
          const points = line.points.toArray()
          assert.lengthOf(points, 3)
          assert.instanceOf(points[0], gdal.Point)
          assert.instanceOf(points[1], gdal.Point)
          assert.instanceOf(points[2], gdal.Point)
          assert.equal(points[0].x, 1)
          assert.equal(points[1].x, 2)
          assert.equal(points[2].x, 3)
        })
      })
    })
  })
})
