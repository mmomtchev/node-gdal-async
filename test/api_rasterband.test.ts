import { assert } from 'chai'
import * as semver from 'semver'
import * as gdal from 'gdal-async'
import * as fileUtils from './utils/file'

describe('gdal.RasterBand', () => {
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  it('should not be instantiable', () => {
    assert.throws(() => {
      new gdal.RasterBand()
    })
  })
  describe('instance', () => {
    describe('"ds" property', () => {
      describe('getter', () => {
        it('should return gdal.Dataset', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.equal(band.ds, ds)
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (band as any).ds = null
          })
        })
      })
    })
    describe('"colorInterpretation" property', () => {
      describe('getter', () => {
        it('should return colorInterpretation', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          assert.equal(band.colorInterpretation, gdal.GCI_GrayIndex)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.colorInterpretation)
          })
        })
      })
      describe('setter', () => {
        it('should set colorInterpretation', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          band.colorInterpretation = gdal.GCI_RedBand
          assert.equal(band.colorInterpretation, gdal.GCI_RedBand)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            band.colorInterpretation = gdal.GCI_RedBand
          })
        })
      })
    })
    describe('"colorTable" property', () => {
      describe('getter', () => {
        it('should return a read-only colorTable', () => {
          const ds = gdal.open(`${__dirname}/data/CM13ct.png`)
          const band = ds.bands.get(1)
          assert.instanceOf(band.colorTable, gdal.ColorTable)
          assert.deepEqual(band.colorTable?.get(1), { c1: 7, c2: 8, c3: 45, c4: 255 })
          assert.throws(() => {
            band.colorTable?.set(0, { c1: 0, c2: 0, c3: 0, c4: 0 })
          }, /read-only/)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open(`${__dirname}/data/CM13ct.png`)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.colorTable)
          }, /already been destroyed/)
        })
      })
      describe('setter', () => {
        it('should set and clear the colorTable', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          const colorTable = new gdal.ColorTable(gdal.GPI_CMYK)
          band.colorTable = colorTable
          assert.isTrue(band.colorTable.isSame(colorTable))
          band.colorTable = null
          assert.isUndefined(band.colorTable)
        })
        it('should throw error on invalid value', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            band.colorTable = 12 as unknown as gdal.ColorTable
          }, /must be a gdal.ColorTable/)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            band.colorTable = null
          }, /already been destroyed/)
        })
      })
      describe('gdal.ColorTable', () => {
        it('@@iterator()', () => {
          const ds = gdal.open(`${__dirname}/data/CM13ct.png`)
          const band = ds.bands.get(1)
          let count = 0
          for (const color of band.colorTable || []) {
            count++
            assert.hasAllKeys(color, [ 'c1', 'c2', 'c3', 'c4' ])
          }
          assert.isAbove(count, 0)
          assert.equal(count, band.colorTable?.count())
        })
        it('clone()', () => {
          const ds = gdal.open(`${__dirname}/data/CM13ct.png`)
          const band = ds.bands.get(1)
          assert.isNotNull(band.colorTable)
          const colorTable = band.colorTable?.clone()
          assert.instanceOf(colorTable, gdal.ColorTable)
          assert.equal(colorTable?.count(), band?.colorTable?.count())
          for (let i = 0; i < (colorTable?.count() || 0); i++) {
            assert.deepEqual(colorTable?.get(i), band?.colorTable?.get(i))
          }
          colorTable?.set(0, { c1: 0, c2: 0, c3: 0, c4: 0 })
        })
        it('"interpretation" property', () => {
          const ds = gdal.open(`${__dirname}/data/CM13ct.png`)
          const band = ds.bands.get(1)
          const interp = band.colorTable?.interpretation
          assert.equal(interp, gdal.GPI_RGB)
        })
        it('ramp()', () => {
          const colorTable = new gdal.ColorTable(gdal.GPI_RGB)
          assert.instanceOf(colorTable, gdal.ColorTable)
          assert.equal(colorTable.count(), 0)
          colorTable.ramp(0, { c1: 0, c2: 99, c3: 0, c4: 0 }, 99, { c1: 99, c2: 0, c3: 0, c4: 0 })
          assert.equal(colorTable.count(), 100)
          for (let i = 0; i < colorTable.count(); i++) {
            assert.deepEqual(colorTable.get(i), { c1: i, c2: 99 - i , c3: 0, c4: 0 })
          }
        })
      })
    })
    describe('"description" property', () => {
      describe('getter', () => {
        it('should return string', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          const band = ds.bands.get(1)
          assert.equal(band.description, 'hshade17')
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.description)
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          const band = ds.bands.get(1)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (band as any).description = 'test'
          })
        })
      })
    })
    describe('"id" property', () => {
      describe('getter', () => {
        it('should return number', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.equal(band.id, 1)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.id)
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (band as any).id = 5
          })
        })
      })
    })
    describe('"size" property', () => {
      describe('getter', () => {
        it('should return object', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 128, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.deepEqual(band.size, { x: 128, y: 256 })
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.size)
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (band as any).size = { x: 128, y: 128 }
          })
        })
      })
    })
    describe('"blockSize" property', () => {
      describe('getter', () => {
        it('should return object', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          assert.deepEqual(band.blockSize, { x: 984, y: 8 })
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.blockSize)
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (band as any).blockSize = { x: 128, y: 128 }
          })
        })
      })
    })
    describe('"unitType" property', () => {
      describe('getter', () => {
        it('should return string', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          assert.isString(band.unitType)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.unitType)
          })
        })
      })
      describe('setter', () => {
        it('should set unitType', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          band.unitType = 'm'
          assert.equal(band.unitType, 'm')
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            band.unitType = 'm'
          })
        })
      })
    })
    describe('"dataType" property', () => {
      describe('getter', () => {
        it('should return dataType', () => {
          const ds = gdal.open(
            'temp',
            'w',
            'MEM',
            256,
            256,
            1,
            gdal.GDT_Float64
          )
          const band = ds.bands.get(1)
          assert.equal(band.dataType, gdal.GDT_Float64)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.dataType)
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (band as any).dataType = gdal.GDT_Float64
          })
        })
      })
    })
    describe('"readOnly" property', () => {
      describe('getter', () => {
        it('should return true on readOnly dataset', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          assert.isTrue(band.readOnly)
        })
        it('should return false on writable dataset', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.isFalse(band.readOnly)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.readOnly)
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (band as any).readOnly = true
          })
        })
      })
    })
    describe('"minimum" property', () => {
      describe('getter', () => {
        it('should return number', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          const band = ds.bands.get(1)
          assert.equal(band.minimum, 177)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.minimum)
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (band as any).minimum = 5
          })
        })
      })
    })
    describe('"maximum" property', () => {
      describe('getter', () => {
        it('should return number', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          const band = ds.bands.get(1)
          assert.equal(band.maximum, 182)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.maximum)
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (band as any).maximum = 5
          })
        })
      })
    })
    describe('"offset" property', () => {
      describe('getter', () => {
        it('should return number', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          if (semver.gt(gdal.version, '3.4.999')) {
            assert.isNull(band.offset)
          } else {
            assert.equal(band.offset, 0)
          }
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.offset)
          })
        })
      })
      describe('setter', () => {
        it('should set offset', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          band.offset = 16
          assert.equal(band.offset, 16)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            band.offset = 16
          })
        })
      })
    })
    describe('"scale" property', () => {
      describe('getter', () => {
        it('should return number', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          if (semver.gt(gdal.version, '3.4.999')) {
            assert.isNull(band.scale)
          } else {
            assert.equal(band.scale, 1)
          }
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.scale)
          })
        })
      })
      describe('setter', () => {
        it('should set scale', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          band.scale = 2
          assert.equal(band.scale, 2)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            band.scale = 2
          })
        })
      })
    })
    describe('"noDataValue" property', () => {
      describe('getter', () => {
        it('should return number', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          const band = ds.bands.get(1)
          assert.equal(band.noDataValue, 0)
        })
        it('should return null if not set', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.isNull(band.noDataValue)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            console.log(band.noDataValue)
          })
        })
      })
      describe('setter', () => {
        it('should set noDataValue', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          band.noDataValue = 5
          assert.equal(band.noDataValue, 5)
        })
        it('should support NaN', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          band.noDataValue = NaN
          assert.isNaN(band.noDataValue)
        })
        it('should clear the noDataValue when setting to null', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          band.noDataValue = null
          assert.isNull(band.noDataValue)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            band.noDataValue = 5
          })
        })
      })
    })
    describe('"pixels" property', () => {
      describe('getter', () => {
        it('should return pixel collection', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.instanceOf(band.pixels, gdal.RasterBandPixels)
          assert.strictEqual(band.pixels.band, band)
        })
      })
      describe('setter', () => {
        it('should throw an error', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (band as any).pixels = null
          })
        })
      })
      describe('get()', () => {
        it('should return a number', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          assert.equal(band.pixels.get(200, 300), 10)
        })
        it('should throw an error if x,y is out of bounds', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            band.pixels.get(-1, -1)
          })
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            band.pixels.get(200, 300)
          })
        })
      })
      describe('set()', () => {
        it('should set the pixel to the value', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          band.pixels.set(10, 20, 30)
          assert.equal(band.pixels.get(10, 20), 30)
          band.pixels.set(10, 20, 33.6)
          assert.equal(band.pixels.get(10, 20), 34)
        })
        it('should throw an error if x,y is out of bounds', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            band.pixels.set(-1, -1, 20)
          })
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            band.pixels.set(10, 20, 30)
          })
        })
      })
      describe('read()', () => {
        it('should return a TypedArray', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          const w = 20
          const h = 30
          const data = band.pixels.read(190, 290, w, h)
          assert.instanceOf(data, Uint8Array)
          assert.equal(data.length, w * h)
          assert.equal(data[10 * 20 + 10], 10)
        })
        describe('w/data over 4GB', () => {
          // These tests require at least 16GB of memory to be reliable
          const size = 66000
          it('when returning a new TypedArray', () => {
            const ds = gdal.open(`${__dirname}/data/huge-sparse.tiff`)
            const band = ds.bands.get(1)
            assert.deepEqual(ds.rasterSize, { x: size, y: size })
            const data = band.pixels.read(ds.rasterSize.x / 2, ds.rasterSize.y / 2,
              ds.rasterSize.x / 2, ds.rasterSize.y / 2)
            assert.instanceOf(data, Int32Array)
            assert.equal(data.length, ds.rasterSize.x * ds.rasterSize.y / 4)
            for (let i = 0; i < data.length; i++) {
              if (data[i] !== 42) throw new Error('data error')
            }
          })
          it('w/data argument', () => {
            const ds = gdal.open(`${__dirname}/data/huge-sparse.tiff`)
            const band = ds.bands.get(1)
            assert.deepEqual(ds.rasterSize, { x: size, y: size })
            const data = new Int32Array(size * size / 4)
            const r = band.pixels.read(ds.rasterSize.x / 2, ds.rasterSize.y / 2,
              ds.rasterSize.x / 2, ds.rasterSize.y / 2, data)
            assert.strictEqual(r, data)
            for (let i = 0; i < data.length; i++) {
              if (data[i] !== 42) throw new Error('data error')
            }
          })
          it('w/data argument w/error', () => {
            const ds = gdal.open(`${__dirname}/data/huge-sparse.tiff`)
            const band = ds.bands.get(1)
            assert.deepEqual(ds.rasterSize, { x: size, y: size })
            const data = new Int32Array(size * size / 4 - 1)
            assert.throws(() => {
              band.pixels.read(ds.rasterSize.x / 2, ds.rasterSize.y / 2,
                ds.rasterSize.x / 2, ds.rasterSize.y / 2, data)
            }, /Array length must be greater than/)
          })
          it('w/file over the 4G elements limit', () => {
            const ds = gdal.open(`${__dirname}/data/huge-sparse.tiff`)
            const band = ds.bands.get(1)
            assert.deepEqual(ds.rasterSize, { x: size, y: size })
            assert.throws(() => {
              band.pixels.read(0, 0, ds.rasterSize.x, ds.rasterSize.y, undefined,
                { data_type: gdal.GDT_Byte })
            }, /Failed constructing a TypedArray/)
          })
        })
        describe('w/data argument', () => {
          it('should put the data in the existing array', () => {
            const ds = gdal.open(
              'temp',
              'w',
              'MEM',
              256,
              256,
              1,
              gdal.GDT_Byte
            )
            const band = ds.bands.get(1)
            const data = new Uint8Array(new ArrayBuffer(20 * 30))
            data[15] = 31
            const result = band.pixels.read(0, 0, 20, 30, data)
            assert.equal(data, result)
            assert.equal(data[15], 0)
          })
          it('should create new array if null', () => {
            const ds = gdal.open(
              'temp',
              'w',
              'MEM',
              256,
              256,
              1,
              gdal.GDT_Byte
            )
            const band = ds.bands.get(1)
            const data = band.pixels.read(0, 0, 20, 30)
            assert.instanceOf(data, Uint8Array)
            assert.equal(data.length, 20 * 30)
          })
          it('should throw error if array is too small', () => {
            const ds = gdal.open(
              'temp',
              'w',
              'MEM',
              256,
              256,
              1,
              gdal.GDT_UInt16
            )
            const band = ds.bands.get(1)
            const data = new Uint8Array(new ArrayBuffer(20 * 30))
            assert.throws(() => {
              band.pixels.read(0, 0, 20, 31, data)
            })
          })
          it('should throw error if array is too small w/pixel_space', () => {
            const ds = gdal.open(
              'temp',
              'w',
              'MEM',
              984,
              804,
              1,
              gdal.GDT_UInt16
            )
            const band = ds.bands.get(1)
            const data = new Uint8Array(new ArrayBuffer(984 * 804))
            assert.throws(() => {
              band.pixels.read(0, 0, 984, 804, data, { pixel_space: 984 })
            })
          })
          it('should throw error if array is too small w/line_space', () => {
            const ds = gdal.open(
              'temp',
              'w',
              'MEM',
              984,
              804,
              1,
              gdal.GDT_UInt16
            )
            const band = ds.bands.get(1)
            const data = new Uint8Array(new ArrayBuffer(984 * 804))
            assert.throws(() => {
              band.pixels.read(0, 0, 984, 804, data, { line_space: 985 })
            })
          })
          it('should support negative line_space', () => {
            const w = 36,
              h = 48
            const ds = gdal.open(
              'temp',
              'w',
              'MEM',
              w,
              h,
              1,
              gdal.GDT_UInt16
            )
            const band = ds.bands.get(1)
            band.pixels.set(1, 1, 255)
            const data = new Uint8Array(new ArrayBuffer(w * h))
            band.pixels.read(0, 0, w, h, data, { line_space: -w, offset: (h-1)*w })
            assert.equal(data[w*h - 2*w + 1], 255)
          })
          it('should support negative pixel_space', () => {
            const w = 36,
              h = 48
            const ds = gdal.open(
              'temp',
              'w',
              'MEM',
              w,
              h,
              1,
              gdal.GDT_UInt16
            )
            const band = ds.bands.get(1)
            band.pixels.set(1, 1, 255)
            const data = new Uint8Array(new ArrayBuffer(w * h))
            band.pixels.read(0, 0, w, h, data, { pixel_space: -1, line_space: w, offset: w-1 })
            assert.equal(data[2*w - 2], 255)
          })
          it('should detect write before start w/negative line_space', () => {
            const w = 36,
              h = 48
            const ds = gdal.open(
              'temp',
              'w',
              'MEM',
              w,
              h,
              1,
              gdal.GDT_UInt16
            )
            const band = ds.bands.get(1)
            band.pixels.set(1, 1, 255)
            const data = new Uint8Array(new ArrayBuffer(w * h))
            assert.throws(() => {
              band.pixels.read(0, 0, w, h, data, { line_space: -w, offset: (h-1)*w - 1 })
            })
          })
          it('should detect write before start w/negative pixel_space', () => {
            const w = 36,
              h = 48
            const ds = gdal.open(
              'temp',
              'w',
              'MEM',
              w,
              h,
              1,
              gdal.GDT_UInt16
            )
            const band = ds.bands.get(1)
            band.pixels.set(1, 1, 255)
            const data = new Uint8Array(new ArrayBuffer(w * h))
            assert.throws(() => {
              band.pixels.read(0, 0, w, h, data, { pixel_space: -1, line_space: w, offset: w-1 - 1 })
            })
          })
          it('should automatically translate data to array data type', () => {
            const ds = gdal.open(
              'temp',
              'w',
              'MEM',
              256,
              256,
              1,
              gdal.GDT_Byte
            )
            const band = ds.bands.get(1)
            band.pixels.set(1, 1, 30)
            const data = new Float64Array(new ArrayBuffer(20 * 30 * 8))
            band.pixels.read(1, 1, 20, 30, data)
            assert.equal(data[0], 30)
          })
          it('should automatically translate data to specified data type', () => {
            const ds = gdal.open(
              'temp',
              'w',
              'MEM',
              256,
              256,
              1,
              gdal.GDT_Byte
            )
            const band = ds.bands.get(1)
            band.pixels.set(1, 1, 30)
            const data = band.pixels.read(1, 1, 20, 30, undefined, { data_type: gdal.GDT_Float64 })
            assert.instanceOf(data, Float64Array)
            assert.equal(data[0], 30)
          })
        })
        describe('w/options', () => {
          describe('"buffer_width", "buffer_height"', () => {
            it('should default to width, height when not present', () => {
              const ds = gdal.open(
                'temp',
                'w',
                'MEM',
                256,
                256,
                1,
                gdal.GDT_Byte
              )
              const band = ds.bands.get(1)
              const data = band.pixels.read(0, 0, 20, 30)
              assert.equal(data.length, 20 * 30)
            })
            it("should create new array with given dimensions if array isn't given", () => {
              const ds = gdal.open(
                'temp',
                'w',
                'MEM',
                256,
                256,
                1,
                gdal.GDT_Byte
              )
              const band = ds.bands.get(1)
              const data = band.pixels.read(0, 0, 20, 30, undefined, {
                buffer_width: 10,
                buffer_height: 15
              })
              assert.equal(data.length, 10 * 15)
            })
            it('should throw error if given array is smaller than given dimensions', () => {
              const ds = gdal.open(
                'temp',
                'w',
                'MEM',
                256,
                256,
                1,
                gdal.GDT_Byte
              )
              const band = ds.bands.get(1)
              const data = new Float64Array(new ArrayBuffer(8 * 10 * 14))
              assert.throws(() => {
                band.pixels.read(0, 0, 20, 30, data, {
                  buffer_width: 10,
                  buffer_height: 15
                })
              }, /Array length must be greater than.*/)
            })
          })
          describe('"type"', () => {
            it('should be ignored if typed array is given', () => {
              const ds = gdal.open(
                'temp',
                'w',
                'MEM',
                256,
                256,
                1,
                gdal.GDT_Byte
              )
              const band = ds.bands.get(1)
              const data = new Float64Array(new ArrayBuffer(20 * 30 * 8))
              const result = band.pixels.read(0, 0, 20, 30, data, {
                type: gdal.GDT_Byte
              })
              assert.instanceOf(result, Float64Array)
            })
            it('should create output array with given type', () => {
              const ds = gdal.open(
                'temp',
                'w',
                'MEM',
                256,
                256,
                1,
                gdal.GDT_Byte
              )
              const band = ds.bands.get(1)
              const data = band.pixels.read(0, 0, 20, 30, undefined, {
                type: gdal.GDT_Float64
              })
              assert.instanceOf(data, Float64Array)
            })
          })
          describe('"pixel_space", "line_space"', () => {
            it('should read data with space between values', () => {
              const w = 16,
                h = 16
              const ds = gdal.open('temp', 'w', 'MEM', w, h, 2, gdal.GDT_Byte)
              const red = ds.bands.get(1)
              const blue = ds.bands.get(2)
              red.fill(1)
              blue.fill(2)

              const interleaved = new Uint8Array(new ArrayBuffer(w * h * 2))

              const read_options = {
                buffer_width: w,
                buffer_height: h,
                type: gdal.GDT_Byte,
                pixel_space: 2,
                line_space: 2 * w
              }

              red.pixels.read(0, 0, w, h, interleaved, read_options)
              blue.pixels.read(
                0,
                0,
                w,
                h,
                interleaved.subarray(1),
                read_options
              )

              for (let y = 0; y < h; y++) {
                for (let x = 0; x < w; x++) {
                  const r = interleaved[x * 2 + 0 + y * w * 2]
                  const b = interleaved[x * 2 + 1 + y * w * 2]
                  assert.equal(r, 1)
                  assert.equal(b, 2)
                }
              }
            })
            it('should throw error if array is not long enough to store result', () => {
              const w = 16,
                h = 16
              const ds = gdal.open('temp', 'w', 'MEM', w, h, 2, gdal.GDT_Byte)
              const red = ds.bands.get(1)
              const blue = ds.bands.get(2)
              red.fill(1)
              blue.fill(2)

              const interleaved = new Uint8Array(new ArrayBuffer(w * h * 2))
              const read_options = {
                buffer_width: w,
                buffer_height: h,
                type: gdal.GDT_Byte,
                pixel_space: 2,
                line_space: 2 * w
              }

              red.pixels.read(0, 0, w, h, interleaved, read_options)
              assert.throws(() => {
                blue.pixels.read(
                  0,
                  0,
                  w,
                  h,
                  interleaved.subarray(2),
                  read_options
                )
              }, /Array length must be greater than.*/)
            })
          })
          it('should throw an error if region is out of bounds', () => {
            const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
            const band = ds.bands.get(1)
            assert.throws(() => {
              band.pixels.read(20, 20, 16, 16)
            })
          })
          it('should throw error if dataset already closed', () => {
            const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
            const band = ds.bands.get(1)
            ds.close()
            assert.throws(() => {
              band.pixels.read(0, 0, 16, 16)
            })
          })
        })
      })
      describe('write()', () => {
        it('should write data from TypedArray', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          let i
          const w = 16,
            h = 16

          const data = new Uint8Array(new ArrayBuffer(w * h))
          for (i = 0; i < w * h; i++) data[i] = i

          band.pixels.write(100, 120, w, h, data)

          const result = band.pixels.read(100, 120, w, h, data)
          for (i = 0; i < w * h; i++) {
            assert.equal(result[i], data[i])
          }
        })
        it('should throw error if array is too small', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          const w = 16,
            h = 16
          const data = new Uint8Array(new ArrayBuffer(w * h - 1))

          assert.throws(() => {
            band.pixels.write(100, 120, w, h, data)
          })
        })
        it('should throw error if array is too small w/pixel_space', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          const w = 16,
            h = 16
          const data = new Uint8Array(new ArrayBuffer(w * h))

          assert.throws(() => {
            band.pixels.write(0, 0, w, h, data, { pixel_space: 2 })
          })
        })
        it('should throw error if array is too small w/line_space', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          const w = 16,
            h = 16
          const data = new Uint8Array(new ArrayBuffer(w * h))

          assert.throws(() => {
            band.pixels.write(0, 0, w, h, data, { line_space: h + 1 })
          })
        })
        it('should automatically translate data to array data type', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          let i
          const w = 16,
            h = 16

          const data = new Float64Array(new ArrayBuffer(8 * w * h))
          for (i = 0; i < w * h; i++) data[i] = i + 0.33

          band.pixels.write(100, 120, w, h, data)

          const result = band.pixels.read(100, 120, w, h, data)
          for (i = 0; i < w * h; i++) {
            assert.equal(result[i], Math.floor(data[i]))
          }
        })
        it('should not fail when writing more than 4GB', function () {
          if (semver.lt(gdal.version, '3.6.0')) {
            this.skip()
          }
          const filename = `/vsimem/4G_write_test_${String(
            Math.random()
          ).substring(2)}.tiff`
          const size = 33000
          const gtiff = gdal.drivers.get('GTiff')
          const ds = gtiff.create(filename, size, size, 1, gdal.GDT_UInt32,
            { compress: 'packbits', blockxsize: 4096, blockysize: 4096 })
          const band = ds.bands.get(1)
          const data = new Uint32Array(size * size)
          band.pixels.write(0, 0, size, size, data)
          gdal.vsimem.release(filename)
        })
        describe('w/options', () => {
          describe('"buffer_width", "buffer_height"', () => {
            it('should throw error if given array is smaller than given dimensions', () => {
              const ds = gdal.open(
                'temp',
                'w',
                'MEM',
                256,
                256,
                1,
                gdal.GDT_Byte
              )
              const band = ds.bands.get(1)
              const data = new Float64Array(new ArrayBuffer(10 * 14 * 8))
              assert.throws(() => {
                band.pixels.write(0, 0, 20, 30, data, {
                  buffer_width: 10,
                  buffer_height: 15
                })
              })
            })
          })
          describe('"pixel_space", "line_space"', () => {
            it('should skip spaces in given data', () => {
              let i
              const w = 16,
                h = 16
              const ds = gdal.open('temp', 'w', 'MEM', w, h, 2, gdal.GDT_Byte)
              const red = ds.bands.get(1)
              const blue = ds.bands.get(2)

              const interleaved = new Uint8Array(new ArrayBuffer(w * h * 2))
              for (let y = 0; y < h; y++) {
                for (let x = 0; x < w; x++) {
                  interleaved[x * 2 + 0 + y * w * 2] = 1
                  interleaved[x * 2 + 1 + y * w * 2] = 2
                }
              }

              const write_options = {
                buffer_width: w,
                buffer_height: h,
                pixel_space: 2,
                line_space: 2 * w
              }

              red.pixels.write(0, 0, w, h, interleaved, write_options)
              blue.pixels.write(
                0,
                0,
                w,
                h,
                interleaved.subarray(1),
                write_options
              )

              let data
              data = red.pixels.read(0, 0, w, h)
              for (i = 0; i < data.length; i++) assert.equal(data[i], 1)
              data = blue.pixels.read(0, 0, w, h)
              for (i = 0; i < data.length; i++) assert.equal(data[i], 2)
            })
            it('should support negative pixel_space', () => {
              const w = 36,
                h = 48
              const ds = gdal.open('temp', 'w', 'MEM', w, h, 1, gdal.GDT_Byte)
              const data = new Uint8Array(new ArrayBuffer(w * h))
              data[0] = 255
              const band = ds.bands.get(1)
              band.pixels.write(0, 0, w, h, data, { line_space: w, pixel_space: -1, offset: w - 1 })
              assert.equal(band.pixels.get(w - 1, 0), 255)
            })
            it('should support negative line_space', () => {
              const w = 36,
                h = 48
              const ds = gdal.open('temp', 'w', 'MEM', w, h, 1, gdal.GDT_Byte)
              const data = new Uint8Array(new ArrayBuffer(w * h))
              data[0] = 255
              const band = ds.bands.get(1)
              band.pixels.write(0, 0, w, h, data, { line_space: -w, pixel_space: 1, offset: (h-1)*w })
              assert.equal(band.pixels.get(0, h - 1), 255)
            })
            it('should throw instead of reading before the start w/line-negative', () => {
              const w = 36,
                h = 48
              const ds = gdal.open('temp', 'w', 'MEM', w, h, 1, gdal.GDT_Byte)
              const data = new Uint8Array(new ArrayBuffer(w * h))
              data[0] = 255
              const band = ds.bands.get(1)
              assert.throws(() => {
                band.pixels.write(0, 0, w, h, data, { line_space: w, pixel_space: -1, offset: w - 1 - 1 })
              })
            })
            it('should throw instead of reading before the start w/row-negative', () => {
              const w = 36,
                h = 48
              const ds = gdal.open('temp', 'w', 'MEM', w, h, 1, gdal.GDT_Byte)
              const data = new Uint8Array(new ArrayBuffer(w * h))
              data[0] = 255
              const band = ds.bands.get(1)
              assert.throws(() => {
                band.pixels.write(0, 0, w, h, data, { line_space: -w, pixel_space: 1, offset: (h-1)*w - 1 })
              })
            })
            it('should throw error if array is not long enough', () => {
              const w = 16,
                h = 16
              const ds = gdal.open('temp', 'w', 'MEM', w, h, 2, gdal.GDT_Byte)
              const red = ds.bands.get(1)
              const blue = ds.bands.get(2)

              const interleaved = new Uint8Array(new ArrayBuffer(w * h * 2))

              const write_options = {
                buffer_width: w,
                buffer_height: h,
                pixel_space: 2,
                line_space: 2 * w
              }

              red.pixels.write(0, 0, w, h, interleaved, write_options)
              assert.throws(() => {
                blue.pixels.write(
                  0,
                  0,
                  w,
                  h,
                  interleaved.subarray(2),
                  write_options
                )
              })
            })
          })
          describe('"resampling"', () => {
            const w = 8,
              h = 8
            const stripes = new Uint8Array(new ArrayBuffer(w * h))

            let band_stripes: gdal.RasterBand, band_solid: gdal.RasterBand
            before(() => {
              for (let y = 0; y < h; y++) {
                for (let x = 0; x < w; x++) {
                  stripes[x + y * w] = (x % 2) ? 100 : 0
                }
              }
              const ds = gdal.open('temp', 'w', 'MEM', w, h, 2, gdal.GDT_Byte)
              band_stripes = ds.bands.get(1)
              band_solid = ds.bands.get(2)

              band_stripes.pixels.write(0, 0, w, h, stripes)
              band_solid.pixels.write(0, 0, w, h, stripes.subarray(1, 2), { buffer_width: 1, buffer_height: 1 })
            })
            it('should support default resampling', () => {
              let i

              let data
              data = band_stripes.pixels.read(0, 0, w, h)
              for (i = 0; i < data.length; i++) assert.equal(data[i], stripes[i])
              data = band_solid.pixels.read(0, 0, w, h)
              for (i = 0; i < data.length; i++) assert.equal(data[i], 100)
            })
            it('should support non-standard resampling', () => {
              let i

              let data
              data = band_stripes.pixels.read(0, 0, w, h, undefined, { buffer_width: w / 4, buffer_height: h / 4, resampling: gdal.GRA_Average })
              for (i = 0; i < data.length; i++) assert.equal(data[i], 50)
              data = band_stripes.pixels.read(0, 0, w, h, undefined, { buffer_width: w / 4, buffer_height: h / 4, resampling: gdal.GRA_Bilinear })
              for (i = 0; i < data.length; i++) assert.include([ 46, 54 ], data[i])
            })
          })
          describe('"progress_cb"', () => {
            let data: gdal.TypedArray, ds1: gdal.Dataset, ds2: gdal.Dataset
            it('should call the read() progress callback when one is provided', () => {
              ds1 = gdal.open(`${__dirname}/data/sample.tif`)
              const band = ds1.bands.get(1)
              let calls = 0
              let prevComplete = 0
              data = band.pixels.read(0, 0, ds1.rasterSize.x, ds1.rasterSize.y, undefined, {
                progress_cb: (complete): void => {
                  calls++
                  assert.isAbove(complete, prevComplete)
                  assert.isAtMost(complete, 1)
                  prevComplete = complete
                } })
              assert.isAtLeast(calls, 1)
            })
            it('should call the write() progress callback when one is provided', () => {
              const file = `/vsimem/write_progress_test.${String(
                Math.random()
              ).substring(2)}.tmp.tif`
              ds2 = gdal.open(file, 'w', 'GTiff', ds1.rasterSize.x, ds1.rasterSize.y, 1)
              const band = ds2.bands.get(1)
              let calls = 0
              let prevComplete = 0
              band.pixels.write(0, 0, ds1.rasterSize.x, ds1.rasterSize.y, data, {
                progress_cb: (complete): void => {
                  calls++
                  assert.isAbove(complete, prevComplete)
                  assert.isAtMost(complete, 1)
                  prevComplete = complete
                } })
              assert.isAtLeast(calls, 1)
            })
          })
        })
        it('should throw an error if region is out of bounds', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          const data = new Uint8Array(new ArrayBuffer(16 * 16))
          assert.throws(() => {
            band.pixels.write(20, 20, 16, 16, data)
          })
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          const data = new Uint8Array(new ArrayBuffer(16 * 16))
          assert.throws(() => {
            band.pixels.write(0, 0, 16, 16, data)
          })
        })
      })
      describe('readBlock()', () => {
        it('should return TypedArray', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)

          const data = band.pixels.readBlock(0, 0)
          assert.instanceOf(data, Uint8Array)
          assert.equal(data.length, band.blockSize.x * band.blockSize.y)
        })
        it('should throw error if offsets are out of range', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          assert.throws(() => {
            band.pixels.readBlock(-1, 0)
          })
        })
        describe('w/ data argument', () => {
          it('should read data into existing', () => {
            const ds = gdal.open(`${__dirname}/data/sample.tif`)
            const band = ds.bands.get(1)
            const data = new Uint8Array(
              new ArrayBuffer(band.blockSize.x * band.blockSize.y)
            )
            const result = band.pixels.readBlock(0, 0, data)
            assert.equal(result, data)
          })
          it('should throw error if given array is not big enough', () => {
            const ds = gdal.open(`${__dirname}/data/sample.tif`)
            const band = ds.bands.get(1)
            const data = new Uint8Array(
              new ArrayBuffer(band.blockSize.x * band.blockSize.y - 1)
            )
            assert.throws(() => {
              band.pixels.readBlock(0, 0, data)
            })
          })
          it('should throw error if given array is not the right type', () => {
            const ds = gdal.open(`${__dirname}/data/sample.tif`)
            const band = ds.bands.get(1)
            const data = new Float64Array(
              new ArrayBuffer(8 * band.blockSize.x * band.blockSize.y)
            )
            assert.throws(() => {
              band.pixels.readBlock(0, 0, data)
            })
          })
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            band.pixels.readBlock(0, 0)
          })
        })
      })
      describe('writeBlock()', () => {
        it('should write data from TypedArray', () => {
          let i
          const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)

          const length = band.blockSize.x * band.blockSize.y
          const data = new Uint8Array(new ArrayBuffer(length))
          for (i = 0; i < length; i++) data[i] = i

          band.pixels.writeBlock(0, 0, data)

          const result = band.pixels.readBlock(0, 0)
          for (i = 0; i < length; i++) {
            assert.equal(result[i], data[i])
          }
        })
        it('should throw error if offsets are out of range', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)

          const length = band.blockSize.x * band.blockSize.y
          const data = new Uint8Array(new ArrayBuffer(length))

          assert.throws(() => {
            band.pixels.writeBlock(0, 100, data)
          })
        })
        it('should throw error if given array is not big enough', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)

          const length = band.blockSize.x * band.blockSize.y - 1
          const data = new Uint8Array(new ArrayBuffer(length))

          assert.throws(() => {
            band.pixels.writeBlock(0, 0, data)
          })
        })
        it('should throw error if given array is not the right type', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)

          const length = band.blockSize.x * band.blockSize.y
          const data = new Float64Array(new ArrayBuffer(length * 8))

          assert.throws(() => {
            band.pixels.writeBlock(0, 0, data)
          })
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)

          const length = band.blockSize.x * band.blockSize.y
          const data = new Uint8Array(new ArrayBuffer(length))
          ds.close()
          assert.throws(() => {
            band.pixels.writeBlock(0, 0, data)
          })
        })
      })
      it('clampBlock()', () => {
        const ds = gdal.open(`${__dirname}/data/sample.tif`)
        const band = ds.bands.get(1)
        assert.deepEqual(band.blockSize, { x: 984, y: 8 })
        assert.deepEqual(band.pixels.clampBlock(0, 0), { x: 984, y: 8 })
        assert.deepEqual(band.pixels.clampBlock(0, 100), { x: 984, y: 4 })
      })
    })
    describe('flush()', () => {
      it('should flush the written data', () => {
        const file = `/vsimem/write_flush_test.${String(
          Math.random()
        ).substring(2)}.tmp.tif`
        const size = 64
        const ds = gdal.open(file, 'w', 'GTiff', size, size, 1, gdal.GDT_Byte)
        const band = ds.bands.get(1)
        let i

        const data = new Uint8Array(new ArrayBuffer(size * size))
        for (i = 0; i < size*size; i++) data[i] = i % 256

        band.pixels.write(0, 0, size, size, data)

        assert.throws(() => gdal.open(file))
        band.flush()
        const newDs = gdal.open(file)
        const result = newDs.bands.get(1).pixels.read(0, 0, size, size, data)
        for (i = 0; i < size * size; i++) {
          assert.equal(result[i], data[i])
        }
      })
    })
    describe('"overviews" property', () => {
      describe('getter', () => {
        it('should return overview collection', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 32, 32, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.instanceOf(band.overviews, gdal.RasterBandOverviews)
        })
      })
      describe('setter', () => {
        it('should throw an error', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 32, 32, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (band as any).overviews = null
          })
        })
      })
      describe('count()', () => {
        it('should return a number', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 32, 32, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          assert.equal(band.overviews.count(), 0)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 32, 32, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            band.overviews.count()
          })
        })
      })
      describe('get()', () => {
        it('should return a RasterBand', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
          const ds = gdal.open(tempFile, 'r+')
          const band = ds.bands.get(1)
          ds.buildOverviews('NEAREST', [ 2 ])
          assert.instanceOf(band.overviews.get(0), gdal.RasterBand)
          ds.close()
          gdal.vsimem.release(tempFile)
        })
        it('should throw error if dataset already closed', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
          const ds = gdal.open(tempFile, 'r+')
          const band = ds.bands.get(1)
          ds.buildOverviews('NEAREST', [ 2 ])
          ds.close()
          gdal.vsimem.release(tempFile)
          assert.throws(() => {
            band.overviews.get(0)
          })
        })
        it('should throw error if id out of range', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
          const ds = gdal.open(tempFile, 'r+')
          const band = ds.bands.get(1)
          ds.buildOverviews('NEAREST', [ 2 ])
          assert.throws(() => {
            band.overviews.get(2)
          })
          ds.close()
          gdal.vsimem.release(tempFile)
        })
      })
      describe('forEach()', () => {
        it('should pass each overview to the callback', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
          const ds = gdal.open(tempFile, 'r+')
          const band = ds.bands.get(1)
          ds.buildOverviews('NEAREST', [ 2, 4 ])
          const w = [] as number[]
          band.overviews.forEach((overview, i) => {
            assert.isNumber(i)
            w.push(overview.size.x)
          })
          assert.sameMembers(w, [ ds.rasterSize.x / 2, ds.rasterSize.x / 4 ])
          ds.close()
          gdal.vsimem.release(tempFile)
        })
        it('should throw error if dataset already closed', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
          const ds = gdal.open(tempFile, 'r+')
          const band = ds.bands.get(1)
          ds.buildOverviews('NEAREST', [ 2 ])
          ds.close()
          gdal.vsimem.release(tempFile)
          assert.throws(() => {
            band.overviews.forEach(() => undefined)
          })
        })
      })
      describe('@@iterator()', () => {
        it('should iterate through the overviews', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
          const ds = gdal.open(tempFile, 'r+')
          const band = ds.bands.get(1)
          ds.buildOverviews('NEAREST', [ 2, 4 ])
          const w = []
          for (const overview of band.overviews) {
            w.push(overview.size.x)
          }
          assert.sameMembers(w, [ ds.rasterSize.x / 2, ds.rasterSize.x / 4 ])
          ds.close()
          gdal.vsimem.release(tempFile)
        })
        it('should throw error if dataset already closed', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
          const ds = gdal.open(tempFile, 'r+')
          const band = ds.bands.get(1)
          ds.buildOverviews('NEAREST', [ 2 ])
          ds.close()
          gdal.vsimem.release(tempFile)
          assert.throws(() => {
            for (const overview of band.overviews) overview
          })
        })
      })
      describe('map()', () => {
        it('should operate normally', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
          const ds = gdal.open(tempFile, 'r+')
          const band = ds.bands.get(1)
          ds.buildOverviews('NEAREST', [ 2, 4 ])

          const result = band.overviews.map((overview, i) => {
            assert.isNumber(i)
            assert.isNumber(overview.size.x)
            return 'a'
          })
          assert.isArray(result)
          assert.lengthOf(result, band.overviews.count())
          assert.equal(result[0], 'a')
          ds.close()
          gdal.vsimem.release(tempFile)
        })
      })
    })
    describe('fill()', () => {
      it('should set all pixels to given value', () => {
        const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
        const band = ds.bands.get(1)
        band.fill(5)
        const data = band.pixels.read(0, 0, 16, 16)
        for (let i = 0; i < data.length; i++) {
          assert.equal(data[i], 5)
        }
      })
      it('should throw error if dataset already closed', () => {
        const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
        const band = ds.bands.get(1)
        ds.close()
        assert.throws(() => {
          band.fill(5)
        })
      })
    })
    describe('statistics', () => {
      const statsBand = () => {
        const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
        const band = ds.bands.get(1)
        band.fill(5)
        band.pixels.set(10, 10, 20)
        band.pixels.set(0, 0, 0)
        return band
      }
      describe('computeStatistics()', () => {
        it('should compute statistics', () => {
          const band = statsBand()
          const stats = band.computeStatistics(false)
          assert.equal(stats.min, 0)
          assert.equal(stats.max, 20)
          assert.closeTo(stats.mean, 5, 0.1)
          assert.closeTo(stats.std_dev, 1, 0.1)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            band.computeStatistics(false)
          })
        })
      })
      describe('setStatistics()', () => {
        it('should allow to manually set (false) statistics', () => {
          const band = statsBand()
          const stats = { min: -10, max: 30, mean: 15, std_dev: 2 }
          band.setStatistics(stats.min, stats.max, stats.mean, stats.std_dev)
          assert.deepEqual(band.getStatistics(false, true), stats)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          assert.throws(() => {
            band.setStatistics(0, 0, 0, 0)
          })
        })
      })
    })
    describe('getMetadata()', () => {
      it('should retrieve the band metadata', () => {
        const band = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`).bands.get(1)
        const meta = band.getMetadata()
        assert.equal(meta.LAYER_TYPE, 'athematic')
      })
    })
    describe('setMetadata()', () => {
      it('should set the metadata', () => {
        const band = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte).bands.get(1)
        assert.isTrue(band.setMetadata({ name: 'temporary' }))
        let metadata = band.getMetadata()
        assert.isObject(metadata)
        assert.equal(metadata.name, 'temporary')

        assert.isTrue(band.setMetadata([ 'name=temporary' ]))
        metadata = band.getMetadata()
        assert.isObject(metadata)
        assert.equal(metadata.name, 'temporary')
      })
      it('should throw on invalid arguments', () => {
        const band = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte).bands.get(1)
        assert.throws(() => {
          band.setMetadata(42 as unknown as string[])
        }, /Failed parsing/)
      })
    })
    describe('getMaskBand()', () => {
      it('should retrieve the band nodata mask', () => {
        const band = gdal.open(`${__dirname}/data/test_with_mask_1bit.tif`).bands.get(1)
        assert.equal(band.getMaskFlags(), 2)
        const mask = band.getMaskBand()
        assert.equal(mask.pixels.get(0, 0), 0)
        assert.equal(mask.pixels.get(10, 10), 255)
      })
    })
    describe('createMaskBand()', () => {
      it('should create the band nodata mask', () => {
        const band = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte).bands.get(1)
        assert.equal(band.getMaskFlags(), 1)
        band.createMaskBand(2)
        assert.equal(band.getMaskFlags(), 2)
        const mask = band.getMaskBand()
        assert.isNotNull(mask)
        assert.instanceOf(mask, gdal.RasterBand)
      })
    })
    describe('"categoryNames" property', () => {
      it('should allow setting and retrieving the category names', () => {
        const band = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte).bands.get(1)
        assert.isEmpty(band.categoryNames)
        const cats = [ 'dry', 'humid', 'wet', 'soaking' ]
        band.categoryNames = cats
        assert.deepEqual(band.categoryNames, cats)
      })
    })
    describe('"hasArbitraryOverviews" property', () => {
      it('should always return false', () => {
        const band = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte).bands.get(1)
        assert.equal(band.hasArbitraryOverviews, false)
      })
    })
  })
})
