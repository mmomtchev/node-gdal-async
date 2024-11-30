/* eslint no-new: 0 */
import { assert } from 'chai'
import * as gdal from 'gdal-async'

describe('gdal.Envelope3D', () => {
  afterEach(() => void global.gc!())

  it('should be instantiable', () => {
    new gdal.Envelope3D()
  })
  describe('instance', () => {
    it('should have "minX" property', () => {
      const envelope = new gdal.Envelope3D({
        minX: 5,
        maxX: 0,
        minY: 0,
        maxY: 0,
        minZ: 0,
        maxZ: 0
      })
      assert.equal(envelope.minX, 5)
    })
    it('should have "maxX" property', () => {
      const envelope = new gdal.Envelope3D({
        minX: 0,
        maxX: 5,
        minY: 0,
        maxY: 0,
        minZ: 0,
        maxZ: 0
      })
      assert.equal(envelope.maxX, 5)
    })
    it('should have "minY" property', () => {
      const envelope = new gdal.Envelope3D({
        minX: 0,
        maxX: 0,
        minY: 5,
        maxY: 0,
        minZ: 0,
        maxZ: 0
      })
      assert.equal(envelope.minY, 5)
    })
    it('should have "maxY" property', () => {
      const envelope = new gdal.Envelope3D({
        minX: 0,
        maxX: 0,
        minY: 0,
        maxY: 5,
        minZ: 0,
        maxZ: 0
      })
      assert.equal(envelope.maxY, 5)
    })
    it('should have "minZ" property', () => {
      const envelope = new gdal.Envelope3D({
        minX: 0,
        maxX: 0,
        minY: 0,
        maxY: 0,
        minZ: 5,
        maxZ: 0
      })
      assert.equal(envelope.minZ, 5)
    })
    it('should have "maxZ" property', () => {
      const envelope = new gdal.Envelope3D({
        minX: 0,
        maxX: 0,
        minY: 0,
        maxY: 0,
        minZ: 0,
        maxZ: 5
      })
      assert.equal(envelope.maxZ, 5)
    })
    describe('isEmpty()', () => {
      it('should return true when all components zero', () => {
        const envelope = new gdal.Envelope3D()
        assert.isTrue(envelope.isEmpty())
      })
      it('should return false when a component is non-zero', () => {
        const envelope = new gdal.Envelope3D({
          minX: 0,
          maxX: 0,
          minY: 0,
          maxY: 0,
          minZ: 10,
          maxZ: 20
        })
        assert.isFalse(envelope.isEmpty())
      })
    })
    describe('getEnvelope3D', () => {
      it('should return the 3D envelope', () => {
        const ring = new gdal.LinearRing()
        ring.points.add({ x: 0, y: 0, z: 0 })
        ring.points.add({ x: 10, y: 0, z: 0 })
        ring.points.add({ x: 10, y: 10, z: 10 })
        ring.points.add({ x: 0, y: 10, z: 10 })
        ring.closeRings()
        const square = new gdal.Polygon()
        square.rings.add(ring)

        const envelope = square.getEnvelope3D()
        assert.propertyVal(envelope, 'minX', 0)
        assert.propertyVal(envelope, 'maxX', 10)
        assert.propertyVal(envelope, 'minY', 0)
        assert.propertyVal(envelope, 'maxY', 10)
        assert.propertyVal(envelope, 'minZ', 0)
        assert.propertyVal(envelope, 'maxZ', 10)
      })
    })
    describe('getEnvelope3DAsync', () => {
      it('should return the 3D envelope', () => {
        const ring = new gdal.LinearRing()
        ring.points.add({ x: 0, y: 0, z: 0 })
        ring.points.add({ x: 10, y: 0, z: 0 })
        ring.points.add({ x: 10, y: 10, z: 10 })
        ring.points.add({ x: 0, y: 10, z: 10 })
        ring.closeRings()
        const square = new gdal.Polygon()
        square.rings.add(ring)

        const envelope = square.getEnvelope3DAsync()
        return assert.isFulfilled(Promise.all([ assert.eventually.propertyVal(envelope, 'minX', 0),
          assert.eventually.propertyVal(envelope, 'maxX', 10),
          assert.eventually.propertyVal(envelope, 'minY', 0),
          assert.eventually.propertyVal(envelope, 'maxY', 10),
          assert.eventually.propertyVal(envelope, 'minZ', 0),
          assert.eventually.propertyVal(envelope, 'maxZ', 10) ]))
      })
    })
    describe('merge()', () => {
      describe('w/x,y,z arguments', () => {
        it('should expand envelope', () => {
          const envelope = new gdal.Envelope3D({
            minX: -1,
            maxX: 1,
            minY: -2,
            maxY: 2,
            minZ: 0,
            maxZ: 1
          })
          envelope.merge(2, 3, 5)
          assert.equal(envelope.minX, -1)
          assert.equal(envelope.minY, -2)
          assert.equal(envelope.minZ, 0)
          assert.equal(envelope.maxX, 2)
          assert.equal(envelope.maxY, 3)
          assert.equal(envelope.maxZ, 5)
        })
      })
      describe('w/envelope argument', () => {
        it('should expand envelope', () => {
          const envelopeA = new gdal.Envelope3D({
            minX: -1,
            maxX: 1,
            minY: -2,
            maxY: 2,
            minZ: 0,
            maxZ: 1
          })
          const envelopeB = new gdal.Envelope3D({
            minX: -2,
            maxX: 10,
            minY: -1,
            maxY: 1,
            minZ: -1,
            maxZ: 2
          })
          envelopeA.merge(envelopeB)
          assert.equal(envelopeA.minX, -2)
          assert.equal(envelopeA.minY, -2)
          assert.equal(envelopeA.minZ, -1)
          assert.equal(envelopeA.maxX, 10)
          assert.equal(envelopeA.maxY, 2)
          assert.equal(envelopeA.maxZ, 2)
        })
      })
    })
    describe('intersects()', () => {
      it('should determine if envelopes touch', () => {
        const envelopeA = new gdal.Envelope3D({
          minX: 1,
          maxX: 2,
          minY: 1,
          maxY: 2,
          minZ: 0,
          maxZ: 1
        })
        const envelopeB = new gdal.Envelope3D({
          minX: 2,
          maxX: 4,
          minY: 1,
          maxY: 2,
          minZ: 0.5,
          maxZ: 2
        })
        const envelopeC = new gdal.Envelope3D({
          minX: 1,
          maxX: 2,
          minY: 1,
          maxY: 2,
          minZ: 2,
          maxZ: 3
        })
        assert.isTrue(envelopeA.intersects(envelopeB))
        assert.isFalse(envelopeA.intersects(envelopeC))
      })
    })
    describe('contains()', () => {
      it('should determine if it fully contains the other envelope', () => {
        const envelopeA = new gdal.Envelope3D({
          minX: -10,
          maxX: 10,
          minY: -10,
          maxY: 10,
          minZ: -10,
          maxZ: 10
        })
        const envelopeB = new gdal.Envelope3D({
          minX: -1,
          maxX: 1,
          minY: -1,
          maxY: 1,
          minZ: -1,
          maxZ: 1
        })
        const envelopeC = new gdal.Envelope3D({
          minX: -1,
          maxX: 1,
          minY: -1,
          maxY: 1,
          minZ: -20,
          maxZ: 1
        })
        assert.isTrue(envelopeA.contains(envelopeB))
        assert.isFalse(envelopeA.contains(envelopeC))
      })
    })
    describe('intersect()', () => {
      it("should yield empty envelope if the two don't intersect", () => {
        const envelopeA = new gdal.Envelope3D({
          minX: 1,
          maxX: 2,
          minY: 1,
          maxY: 2,
          minZ: -10,
          maxZ: 10
        })
        const envelopeC = new gdal.Envelope3D({
          minX: 10,
          maxX: 20,
          minY: 10,
          maxY: 20,
          minZ: -1,
          maxZ: 2
        })
        envelopeA.intersect(envelopeC)
        assert.equal(envelopeA.minX, 0)
        assert.equal(envelopeA.maxX, 0)
        assert.equal(envelopeA.minY, 0)
        assert.equal(envelopeA.maxY, 0)
      })
      it('should yield other envelope if current envelope is empty and other envelope intersects 0,0,0', () => {
        const envelopeA = new gdal.Envelope3D()
        const envelopeC = new gdal.Envelope3D({
          minX: -10,
          maxX: 20,
          minY: -10,
          maxY: 20,
          minZ: -10,
          maxZ: 20
        })
        envelopeA.intersect(envelopeC)
        assert.equal(envelopeA.minX, -10)
        assert.equal(envelopeA.maxX, 20)
        assert.equal(envelopeA.minY, -10)
        assert.equal(envelopeA.maxY, 20)
        assert.equal(envelopeA.minZ, -10)
        assert.equal(envelopeA.maxZ, 20)
      })
      it('should otherwise yield intersection', () => {
        const envelopeA = new gdal.Envelope3D({
          minX: -10,
          maxX: 10,
          minY: -10,
          maxY: 10,
          minZ: -10,
          maxZ: 10
        })
        const envelopeB = new gdal.Envelope3D({
          minX: -2,
          maxX: 12,
          minY: -1,
          maxY: 1,
          minZ: -2,
          maxZ: 2
        })
        envelopeA.intersect(envelopeB)
        assert.equal(envelopeA.minX, -2)
        assert.equal(envelopeA.maxX, 10)
        assert.equal(envelopeA.minY, -1)
        assert.equal(envelopeA.maxY, 1)
        assert.equal(envelopeA.minZ, -2)
        assert.equal(envelopeA.maxZ, 2)
      })
    })
  })
})
