import * as gdal from 'gdal-async'
import { assert } from 'chai'

const unary_ops = [
  { name: 'abs', op: gdal.algebra.abs, test: (x) => Math.abs(x) },
  { name: 'sqrt', op: gdal.algebra.sqrt, test: (x) => Math.sqrt(x) },
  { name: 'log', op: gdal.algebra.log, test: (x) => Math.log(x) },
  { name: 'log10', op: gdal.algebra.log10, test: (x) => Math.log10(x), skipNegative: true }
] as {
  name: string;
  op: typeof gdal.algebra.abs,
  test: (x: number) => number,
  skipNegative?: boolean
}[]

const binary_ops = [
  { name: 'add', op: gdal.algebra.add, test: (a, b) => a + b },
  { name: 'sub', op: gdal.algebra.sub, test: (a, b) => a - b },
  { name: 'mul', op: gdal.algebra.mul, test: (a, b) => a * b },
  { name: 'div', op: gdal.algebra.div, test: (a, b) => a / b },
  { name: 'pow', op: gdal.algebra.pow, test: (a, b) => Math.pow(a, b) },
  { name: 'lt', op: gdal.algebra.lt, test: (a, b) => +(a < b) },
  { name: 'lte', op: gdal.algebra.lte, test: (a, b) => +(a <= b) },
  { name: 'gt', op: gdal.algebra.gt, test: (a, b) => +(a > b) },
  { name: 'gte', op: gdal.algebra.gte, test: (a, b) => +(a >= b) },
  { name: 'and', op: gdal.algebra.and, test: (a, b) => +(!!a && !!b), skipNan: true },
  { name: 'or', op: gdal.algebra.or, test: (a, b) => +(!!a || !!b), skipNan: true },
  { name: 'eq', op: gdal.algebra.eq, test: (a, b) => +(a == b) },
  { name: 'notEq', op: gdal.algebra.notEq, test: (a, b) => +(a != b) }
] as {
  name: string;
  op: typeof gdal.algebra.add,
  test: (a: number, b: number) => number,
  skipNan?: boolean
}[]

describe('algebra', () => {
  const w = 16
  const h = 16
  const buf1 = new Float32Array(w * h)
  const buf2 = new Float32Array(w * h)
  let ds: gdal.Dataset
  let arg1Band: gdal.RasterBand
  let arg2Band: gdal.RasterBand

  afterEach(() => void global.gc!())

  before(() => {
    // create a in-memory band
    ds = gdal.open('temp', 'w', 'MEM', w, h, 2, gdal.GDT_Float32)

    arg1Band = ds.bands.get(1)
    arg2Band = ds.bands.get(2)
    for (let i = 0; i < h * w; i++) {
      buf1[i] = Math.random() * 10 - 5
      buf2[i] = Math.random() * 5
    }
    buf1[0] = NaN
    buf2[0] = NaN
    buf1[10] = NaN
    buf2[20] = NaN
    arg1Band.pixels.write(0, 0, w, h, buf1)
    arg2Band.pixels.write(0, 0, w, h, buf2)
  })
  after(() => {
    ds.close()
  })

  for (const op of binary_ops) {
    describe(`${op.name}()`, () => {

      it('should add two bands', () => {
        const r = op.op(arg1Band, arg2Band)
        const data = r.pixels.read(0, 0, w, h)
        assert.lengthOf(data, w * h)
        for (let i = 0; i < w * h; i++) {
          if (op.skipNan && (isNaN(buf1[i]) || isNaN(buf2[i]))) {
            continue
          }
          if (isNaN(data[i])) {
            assert.isNaN(op.test(buf1[i], buf2[i]))
          } else {
            assert.closeTo(data[i], op.test(buf1[i], buf2[i]), 0.1)
          }
        }
      })

      it('should add a band and a number', () => {
        const r = op.op(arg1Band, 4.2)
        const data = r.pixels.read(0, 0, w, h)
        assert.lengthOf(data, w * h)
        for (let i = 0; i < w * h; i++) {
          if (op.skipNan && (isNaN(buf1[i]) || isNaN(buf2[i]))) {
            continue
          }
          if (isNaN(data[i])) {
            assert.isNaN(op.test(buf1[i], 4.2))
          } else {
            assert.closeTo(data[i], op.test(buf1[i], 4.2), 0.1)
          }
        }
      })

      it('should add a number and a band', () => {
        const r = op.op(4.2, arg2Band)
        const data = r.pixels.read(0, 0, w, h)
        assert.lengthOf(data, w * h)
        for (let i = 0; i < w * h; i++) {
          if (op.skipNan && (isNaN(buf1[i]) || isNaN(buf2[i]))) {
            continue
          }
          if (isNaN(data[i])) {
            assert.isNaN(op.test(4.2, buf2[i]))
          } else {
            assert.closeTo(data[i], op.test(4.2, buf2[i]), 0.1)
          }
        }
      })

      it('should throw on invalid arguments', () => {
        assert.throws(() => {
          op.op(-42, 42)
        }, /At least one RasterBand must be given/)
        assert.throws(() => {
        // @ts-expect-error voluntary error
          op.op(ds, arg1Band)
        }, /Argument must be either a number or a RasterBand/)
      })

      it('should handle destroying the computed dataset', () => {
        const r = op.op(-42, arg2Band)
        assert.instanceOf(r, gdal.RasterBand)
        assert.instanceOf(r.ds, gdal.Dataset)
        r.ds.close()
      })
    })
  }

  for (const op of unary_ops) {
    describe(`${op.name}()`, () => {

      it('should add two bands', () => {
        const r = op.op(arg1Band)
        const data = r.pixels.read(0, 0, w, h)
        assert.lengthOf(data, w * h)
        for (let i = 0; i < w * h; i++) {
          if (op.skipNegative && buf1[i] < 0) {
            continue
          }
          if (isNaN(data[i])) {
            assert.isNaN(op.test(buf1[i]))
          } else {
            assert.closeTo(data[i], op.test(buf1[i]), 0.1)
          }
        }
      })

      it('should throw on invalid arguments', () => {
        assert.throws(() => {
        // @ts-expect-error voluntary error
          op.op(42)
        }, /Argument must be an instance of RasterBand/)
      })
    })
  }

  describe('asType()', () => {
    it('should convert the band to another type', () => {
      const r = gdal.algebra.asType(arg1Band, gdal.GDT_Int32)
      assert.instanceOf(r, gdal.RasterBand)
      assert.strictEqual(r.dataType, gdal.GDT_Int32)
      const data = r.pixels.read(0, 0, w, h)
      assert.instanceOf(data, Int32Array)
      for (let i = 0; i < w * h; i++) {
        if (isNaN(buf1[i])) {
          assert.strictEqual(data[i], 0)
        } else {
          assert.closeTo(data[i], buf1[i], 1)
        }
      }
    })

    it('should throw on invalid arguments', () => {
      assert.throws(() => {
        gdal.algebra.asType(arg1Band, 'something')
      }, /Invalid data type/)
      assert.throws(() => {
        // @ts-expect-error voluntary error
        gdal.algebra.asType('invalid', 'something')
      }, /Argument must be an instance of RasterBand/)
    })
  })

  describe('ifThenElse()', () => {
    it('should support the ternary operator', () => {
      const r = gdal.algebra.ifThenElse(gdal.algebra.lt(arg1Band, 0), arg1Band, -1)
      const data = r.pixels.read(0, 0, w, h)
      for (let i = 0; i < w * h; i++) {
        if (isNaN(buf1[i])) {
          assert.strictEqual(data[i], -1)
        } else {
          if (buf1[i] < 0) {
            assert.strictEqual(data[i], buf1[i])
          } else {
            assert.strictEqual(data[i], -1)
          }
        }
      }
    })

    it('should throw on invalid arguments', () => {
      assert.throws(() => {
        // @ts-expect-error voluntary error
        gdal.algebra.ifThenElse(arg1Band, 'something', 13)
      }, /Argument must be either a number or a RasterBand/)
    })
  })

  it('should support chaining', () => {
    const r = gdal.algebra.mul(gdal.algebra.add(arg1Band, arg2Band), gdal.algebra.sqrt(arg2Band))
    assert.instanceOf(r, gdal.RasterBand)
  })
})
