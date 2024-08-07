/* eslint no-new: 0 */
import { assert } from 'chai'
import * as gdal from 'gdal-async'

describe('gdal.Envelope', () => {
  afterEach(global.gc!)

  it('should be instantiable', () => {
    new gdal.Envelope()
  })
  describe('instance', () => {
    it('should have "minX" property', () => {
      const envelope = new gdal.Envelope({
        minX: 5,
        maxX: 0,
        minY: 0,
        maxY: 0
      })
      assert.equal(envelope.minX, 5)
    })
    it('should have "maxX" property', () => {
      const envelope = new gdal.Envelope({
        minX: 0,
        maxX: 5,
        minY: 0,
        maxY: 0
      })
      assert.equal(envelope.maxX, 5)
    })
    it('should have "minY" property', () => {
      const envelope = new gdal.Envelope({
        minX: 0,
        maxX: 0,
        minY: 5,
        maxY: 0
      })
      assert.equal(envelope.minY, 5)
    })
    it('should have "maxY" property', () => {
      const envelope = new gdal.Envelope({
        minX: 0,
        maxX: 0,
        minY: 0,
        maxY: 5
      })
      assert.equal(envelope.maxY, 5)
    })
    describe('isEmpty()', () => {
      it('should return true when all components zero', () => {
        const envelope = new gdal.Envelope()
        assert.isTrue(envelope.isEmpty())
      })
      it('should return false when a component is non-zero', () => {
        const envelope = new gdal.Envelope({
          minX: 0,
          maxX: 5,
          minY: 0,
          maxY: 0
        })
        assert.isFalse(envelope.isEmpty())
      })
    })
    describe('merge()', () => {
      describe('w/x,y arguments', () => {
        it('should expand envelope', () => {
          const envelope = new gdal.Envelope({
            minX: -1,
            maxX: 1,
            minY: -2,
            maxY: 2
          })
          envelope.merge(2, 3)
          assert.equal(envelope.minX, -1)
          assert.equal(envelope.minY, -2)
          assert.equal(envelope.maxX, 2)
          assert.equal(envelope.maxY, 3)
        })
      })
      describe('w/envelope argument', () => {
        it('should expand envelope', () => {
          const envelopeA = new gdal.Envelope({
            minX: -1,
            maxX: 1,
            minY: -2,
            maxY: 2
          })
          const envelopeB = new gdal.Envelope({
            minX: -2,
            maxX: 10,
            minY: -1,
            maxY: 1
          })
          envelopeA.merge(envelopeB)
          assert.equal(envelopeA.minX, -2)
          assert.equal(envelopeA.minY, -2)
          assert.equal(envelopeA.maxX, 10)
          assert.equal(envelopeA.maxY, 2)
        })
      })
    })
    describe('intersects()', () => {
      it('should determine if envelopes touch', () => {
        const envelopeA = new gdal.Envelope({
          minX: 1,
          maxX: 2,
          minY: 1,
          maxY: 2
        })
        const envelopeB = new gdal.Envelope({
          minX: 2,
          maxX: 4,
          minY: 1,
          maxY: 2
        })
        const envelopeC = new gdal.Envelope({
          minX: 10,
          maxX: 20,
          minY: 10,
          maxY: 20
        })
        assert.isTrue(envelopeA.intersects(envelopeB))
        assert.isFalse(envelopeA.intersects(envelopeC))
      })
    })
    describe('contains()', () => {
      it('should determine if it fully contains the other envelope', () => {
        const envelopeA = new gdal.Envelope({
          minX: -10,
          maxX: 10,
          minY: -10,
          maxY: 10
        })
        const envelopeB = new gdal.Envelope({
          minX: -1,
          maxX: 1,
          minY: -1,
          maxY: 1
        })
        const envelopeC = new gdal.Envelope({
          minX: -1,
          maxX: 1,
          minY: -1,
          maxY: 20
        })
        assert.isTrue(envelopeA.contains(envelopeB))
        assert.isFalse(envelopeA.contains(envelopeC))
      })
    })
    describe('intersect()', () => {
      it("should yield empty envelope if the two don't intersect", () => {
        const envelopeA = new gdal.Envelope({
          minX: 1,
          maxX: 2,
          minY: 1,
          maxY: 2
        })
        const envelopeC = new gdal.Envelope({
          minX: 10,
          maxX: 20,
          minY: 10,
          maxY: 20
        })
        envelopeA.intersect(envelopeC)
        assert.equal(envelopeA.minX, 0)
        assert.equal(envelopeA.maxX, 0)
        assert.equal(envelopeA.minY, 0)
        assert.equal(envelopeA.maxY, 0)
      })
      it('should yield other envelope if current envelope is empty and other envelope intersects 0,0', () => {
        const envelopeA = new gdal.Envelope()
        const envelopeC = new gdal.Envelope({
          minX: -10,
          maxX: 20,
          minY: -10,
          maxY: 20
        })
        envelopeA.intersect(envelopeC)
        assert.equal(envelopeA.minX, -10)
        assert.equal(envelopeA.maxX, 20)
        assert.equal(envelopeA.minY, -10)
        assert.equal(envelopeA.maxY, 20)
      })
      it('should otherwise yield intersection', () => {
        const envelopeA = new gdal.Envelope({
          minX: -10,
          maxX: 10,
          minY: -10,
          maxY: 10
        })
        const envelopeB = new gdal.Envelope({
          minX: -2,
          maxX: 12,
          minY: -1,
          maxY: 1
        })
        envelopeA.intersect(envelopeB)
        assert.equal(envelopeA.minX, -2)
        assert.equal(envelopeA.maxX, 10)
        assert.equal(envelopeA.minY, -1)
        assert.equal(envelopeA.maxY, 1)
      })
    })
    describe('toPolygon()', () => {
      it('should return Polygon', () => {
        let envelope = new gdal.Envelope({
          minX: -1,
          maxX: 1,
          minY: -2,
          maxY: 2
        })
        const polygon = envelope.toPolygon()
        envelope = polygon.getEnvelope()
        assert.instanceOf(polygon, gdal.Polygon)
        assert.equal(envelope.minX, -1)
        assert.equal(envelope.maxX, 1)
        assert.equal(envelope.minY, -2)
        assert.equal(envelope.maxY, 2)
      })
    })
    describe('toPolygon() w/Async', () => {
      it('should return Polygon', () => {
        const envelope = new gdal.Envelope({
          minX: -1,
          maxX: 1,
          minY: -2,
          maxY: 2
        })
        const polygon = envelope.toPolygon()
        assert.instanceOf(polygon, gdal.Polygon)
        const envelopeQ = polygon.getEnvelopeAsync()
        return assert.isFulfilled(Promise.all([ assert.eventually.propertyVal(envelopeQ, 'minX', -1),
          assert.eventually.propertyVal(envelopeQ, 'maxX', 1),
          assert.eventually.propertyVal(envelopeQ, 'minY', -2),
          assert.eventually.propertyVal(envelopeQ, 'maxY', 2) ]))
      })
    })
  })
})
