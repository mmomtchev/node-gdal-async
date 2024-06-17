import * as chaiAsPromised from 'chai-as-promised'
import * as chai from 'chai'
const assert: Chai.Assert = chai.assert
import * as gdal from 'gdal-async'
import * as semver from 'semver'

chai.use(chaiAsPromised)

describe('gdal.RasterBandAsync', () => {
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  it('should not be instantiable', () => {
    assert.throws(() => {
      new gdal.RasterBand()
    })
  })
  describe('instance', () => {
    describe('"descriptionAsync" property', () => {
      describe('getter', () => {
        it('should return string', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          return assert.eventually.equal(ds.bands.get(1).descriptionAsync, 'hshade17')
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.descriptionAsync)
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          gdal.openAsync(`${__dirname}/data/dem_azimuth50_pa.img`,
            (e, ds) => {
              const band = ds.bands.get(1)
              assert.throws(() => {
                // eslint-disable-next-line @typescript-eslint/no-explicit-any
                (band as any).description = 'test'
              })
            })
        })
      })
    })
    describe('"readOnlyAsync" property', () => {
      describe('getter', () => {
        it('should return true on readOnly dataset', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          return assert.eventually.isTrue(ds.bands.get(1).readOnlyAsync)
        })
      })
    })
    describe('"pixels" property', () => {
      describe('getAsync()', () => {
        it('should return a number', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          return assert.eventually.equal(band.pixels.getAsync(200, 300), 10)
        })
        it('should throw an error if x,y is out of bounds', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          return assert.isRejected(band.pixels.getAsync(-1, -1))
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.pixels.getAsync(200, 300))
        })
      })
      describe('set()', () => {
        it('should set the pixel to the value', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          return assert.isFulfilled(band.pixels.setAsync(10, 20, 30).then(() => {
            assert.equal(band.pixels.get(10, 20), 30)
            band.pixels.set(10, 20, 33.6)
            assert.equal(band.pixels.get(10, 20), 34)
          }))
        })
        it('should throw an error if x,y is out of bounds', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          return assert.isRejected(band.pixels.setAsync(-1, -1, 20))
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.pixels.setAsync(10, 20, 30))
        })
      })
      describe('readAsync() w/cb', () => {
        it('should not crash if the dataset is immediately closed', () => {
          // This test has good chances of triggering the event loop warning
          // eslint-disable-next-line @typescript-eslint/no-explicit-any
          (gdal as any).eventLoopWarning = false
          for (let i = 0; i < 20; i++) {
            const ds = gdal.open(`${__dirname}/data/sample.tif`)
            const band = ds.bands.get(1)
            const w = 20
            const h = 30
            // This is an intentional race condition, two things can happen here:
            // a) the async op starts before the dataset is closed and ds.close has to block the event loop
            // b) the dataset is closed before the read starts, readAsync will report an error through the empty callback
            band.pixels.readAsync(190, 290, w, h, undefined, undefined, () => undefined)
            band.pixels.readAsync(190, 290, w, h, undefined, undefined, () => undefined)
            ds.close()
          }
          // eslint-disable-next-line @typescript-eslint/no-explicit-any
          (gdal as any).eventLoopWarning = true
        })
        it('should return a TypedArray', () => {
          gdal.openAsync(`${__dirname}/data/sample.tif`, (e, ds) => {
            const band = ds.bands.get(1)
            const w = 20
            const h = 30
            band.pixels.readAsync(190, 290, w, h, undefined, undefined, (e, data) => {
              assert.instanceOf(data, Uint8Array)
              assert.equal(data.length, w * h)
              assert.equal(data[10 * 20 + 10], 10)
            })
          })
        })
      })
      describe('readAsync() w/Promise', () => {
        it('should return a TypedArray', () => {
          const ds = gdal.openAsync(`${__dirname}/data/sample.tif`)
          return assert.isFulfilled(ds.then((r) => {
            const band = r.bands.get(1)
            const w = 20
            const h = 30
            const p = band.pixels.readAsync(190, 290, w, h)
            return assert.isFulfilled(p.then((data) => {
              assert.instanceOf(data, Uint8Array)
              assert.equal(data.length, w * h)
              assert.equal(data[10 * 20 + 10], 10)
            }))
          }))
        })
        describe('w/data argument', () => {
          it('should put the data in the existing array', () => {
            const ds = gdal.openAsync('temp',
              'w',
              'MEM',
              256,
              256,
              1,
              gdal.GDT_Byte)
            return assert.isFulfilled(ds.then((r) => {
              const band = r.bands.get(1)
              const data = new Uint8Array(new ArrayBuffer(20 * 30))
              data[15] = 31
              const result = band.pixels.readAsync(0, 0, 20, 30, data)
              return assert.isFulfilled(Promise.all([ assert.eventually.equal(result, data),
                assert.isFulfilled(result),
                result.then((data) => {
                  assert.equal(data[15], 0)
                })
              ]))
            }))
          })
          it('should create new array if null', () => {
            const ds = gdal.openAsync(
              'temp',
              'w',
              'MEM',
              256,
              256,
              1,
              gdal.GDT_Byte)
            return assert.isFulfilled(ds.then((r) => {
              const band = r.bands.get(1)
              const p = band.pixels.readAsync(0, 0, 20, 30)
              return assert.isFulfilled(p.then((data) => {
                assert.instanceOf(data, Uint8Array)
                assert.equal(data.length, 20 * 30)
              }))
            }))
          })
          it('should throw error if array is too small', () => {
            const ds = gdal.open('temp',
              'w',
              'MEM',
              256,
              256,
              1,
              gdal.GDT_Byte)
            const band = ds.bands.get(1)
            const data = new Uint8Array(new ArrayBuffer(20 * 30))
            return assert.isRejected(band.pixels.readAsync(0, 0, 20, 31, data))
          })
          it('should automatically translate data to array data type', () => {
            const ds = gdal.open('temp',
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
            const p = band.pixels.readAsync(1, 1, 20, 30, data)
            return assert.eventually.equal(p.then(() => data[0]), 30)
          })
        })
        describe('w/options', () => {
          describe('"buffer_width", "buffer_height"', () => {
            it('should default to width, height when not present', () => {
              const ds = gdal.open(`${__dirname}/data/sample.tif`)
              const band = ds.bands.get(1)
              const data = band.pixels.readAsync(0, 0, 20, 30)
              return assert.eventually.propertyVal(data, 'length', 20 * 30)
            })
            it("should create new array with given dimensions if array isn't given", () => {
              const ds = gdal.open(`${__dirname}/data/sample.tif`)
              const band = ds.bands.get(1)
              const data = band.pixels.readAsync(0, 0, 20, 30, undefined, {
                buffer_width: 10,
                buffer_height: 15
              })
              return assert.eventually.propertyVal(data, 'length', 10 * 15)
            })
            it('should throw error if given array is smaller than given dimensions', () => {
              const ds = gdal.open(`${__dirname}/data/sample.tif`)
              const band = ds.bands.get(1)
              const data = new Float64Array(new ArrayBuffer(8 * 10 * 14))
              return assert.isRejected(band.pixels.readAsync(0, 0, 20, 30, data, {
                buffer_width: 10,
                buffer_height: 15
              }), /Array length must be greater than.*/)
            })
          })
          describe('"type"', () => {
            it('should be ignored if typed array is given', () =>
              assert.isFulfilled(gdal.openAsync(`${__dirname}/data/sample.tif`).then((ds) => {
                const band = ds.bands.get(1)
                const data = new Float64Array(new ArrayBuffer(20 * 30 * 8))
                const result = band.pixels.read(0, 0, 20, 30, data, {
                  type: gdal.GDT_Byte
                })
                assert.instanceOf(result, Float64Array)
              })))
            it('should create output array with given type', () =>
              assert.isFulfilled(gdal.openAsync(`${__dirname}/data/sample.tif`).then((ds) => {
                const band = ds.bands.get(1)
                const data = band.pixels.read(0, 0, 20, 30, undefined, {
                  type: gdal.GDT_Float64
                })
                assert.instanceOf(data, Float64Array)
              })))
          })
          describe('"pixel_space", "line_space"', () => {
            it('should read data with space between values', () => {
              const w = 16,
                h = 16
              return assert.isFulfilled(gdal.openAsync('temp', 'w', 'MEM', w, h, 2, gdal.GDT_Byte).then((ds) => {
                const red = ds.bands.get(1)
                const blue = ds.bands.get(2)
                const rq = red.fillAsync(1)
                const bq = blue.fillAsync(2)

                const interleaved = new Uint8Array(new ArrayBuffer(w * h * 2))

                const read_options = {
                  buffer_width: w,
                  buffer_height: h,
                  type: gdal.GDT_Byte,
                  pixel_space: 2,
                  line_space: 2 * w
                }

                return assert.isFulfilled(Promise.all([ rq, bq ])
                  .then(() => red.pixels.readAsync(0, 0, w, h, interleaved, read_options)
                    .then((interleaved) => blue.pixels.readAsync(
                      0,
                      0,
                      w,
                      h,
                      interleaved.subarray(1),
                      read_options
                    ).then(() => {
                      for (let y = 0; y < h; y++) {
                        for (let x = 0; x < w; x++) {
                          const r = interleaved[x * 2 + 0 + y * w * 2]
                          const b = interleaved[x * 2 + 1 + y * w * 2]
                          assert.equal(r, 1)
                          assert.equal(b, 2)
                        }
                      }
                    }))
                  ))
              }))
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

              const q1 = band_stripes.pixels.readAsync(0, 0, w, h, undefined, { buffer_width: w, buffer_height: h })
              const q2 = band_solid.pixels.readAsync(0, 0, w, h, undefined, { buffer_width: w, buffer_height: h })
              return assert.isFulfilled(Promise.all([ q1, q2 ]).then((data) => {
                const [ data1, data2 ] = data
                assert.equal(data1.length, w*h)
                for (i = 0; i < data1.length; i++) assert.equal(data1[i], stripes[i])
                for (i = 0; i < data2.length; i++) assert.equal(data2[i], 100)
              }))
            })
            it('should support non-standard resampling', () => {
              let i

              const q1 = band_stripes.pixels.readAsync(0, 0, w, h, undefined, { buffer_width: w / 4, buffer_height: h / 4, resampling: gdal.GRA_Average })
              const q2 = band_stripes.pixels.readAsync(0, 0, w, h, undefined, { buffer_width: w / 4, buffer_height: h / 4, resampling: gdal.GRA_Bilinear })
              return assert.isFulfilled(Promise.all([ q1, q2 ]).then((data) => {
                const [ data1, data2 ] = data
                assert.equal(data1.length, w*h / 16)
                for (i = 0; i < data1.length; i++) assert.equal(data1[i], 50)
                for (i = 0; i < data2.length; i++) assert.include([ 46, 54 ], data2[i])
              }))
            })
          })
          describe('"progress_cb" w/Net', () => {
            // When running the full test suite, all test files will be cached
            // In order to not create a flaky test and to make sure that the progress callback
            // has always a chance to run at least once, we must use a very slow datasource
            // and even in this we should be ready to retry if the host is very busy
            // or it has somehow cached the resource
            it('should call the progress callback when one is provided', function () {
              this.retries(10)

              const ds = gdal.open(`/vsicurl/https://s3.amazonaws.com/elevation-tiles-prod/geotiff/1/1/1.tif`)
              const band = ds.bands.get(1)

              let calls = 0
              let prevComplete = 0
              const q = band.pixels.readAsync(0, 0, ds.rasterSize.x, ds.rasterSize.y, undefined, { progress_cb: (complete): void => {
                calls++
                assert.isAbove(complete, prevComplete)
                assert.isAtMost(complete, 1)
                prevComplete = complete
              } })
              return assert.isFulfilled(q.then(() => {
                assert.isAtLeast(calls, 1)
                ds.close()
              }))
            })
          })
          it('should throw error if array is not long enough to store result', () => {
            const w = 16,
              h = 16
            return assert.isFulfilled(gdal.openAsync('temp', 'w', 'MEM', w, h, 2, gdal.GDT_Byte).then((ds) => {
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

              return red.pixels.readAsync(0, 0, w, h, interleaved, read_options)
                .then((interleaved) => assert.isRejected(blue.pixels.readAsync(
                  0,
                  0,
                  w,
                  h,
                  interleaved.subarray(2),
                  read_options
                )))
            }))
          })
          it('should throw an error if region is out of bounds', () => {
            const ds = gdal.openAsync(`${__dirname}/data/sample.tif`)
            return assert.isFulfilled(ds.then((r) => {
              const band = r.bands.get(1)
              return assert.isRejected(band.pixels.readAsync(2000, 2000, 16, 16))
            }))
          })
          it('should throw error if dataset already closed', () => {
            const ds = gdal.openAsync(`${__dirname}/data/sample.tif`)
            return assert.isFulfilled(ds.then((r) => {
              const band = r.bands.get(1)
              r.close()
              return assert.isRejected(band.pixels.readAsync(0, 0, 16, 16))
            }))
          })
        })
        describe('readBlockAsync()', () => {
          it('should return TypedArray', () => {
            const ds = gdal.open(`${__dirname}/data/sample.tif`)
            const band = ds.bands.get(1)

            const dataq = band.pixels.readBlockAsync(0, 0)
            return assert.isFulfilled(dataq.then((data) => {
              assert.instanceOf(data, Uint8Array),
              assert.equal(data.length, band.blockSize.x * band.blockSize.y)
            }))
          })
          it('should throw error if offsets are out of range', () => {
            const ds = gdal.open(`${__dirname}/data/sample.tif`)
            const band = ds.bands.get(1)
            return assert.isRejected(band.pixels.readBlockAsync(-1, 0))
          })
          describe('w/ data argument', () => {
            it('should read data into existing', () => {
              const ds = gdal.open(`${__dirname}/data/sample.tif`)
              const band = ds.bands.get(1)
              const data = new Uint8Array(
                new ArrayBuffer(band.blockSize.x * band.blockSize.y)
              )
              const result = band.pixels.readBlockAsync(0, 0, data)
              return assert.eventually.equal(result, data)
            })
            it('should throw error if given array is not big enough', () => {
              const ds = gdal.open(`${__dirname}/data/sample.tif`)
              const band = ds.bands.get(1)
              const data = new Uint8Array(
                new ArrayBuffer(band.blockSize.x * band.blockSize.y - 1)
              )
              return assert.isRejected(band.pixels.readBlockAsync(0, 0, data))
            })
            it('should throw error if given array is not the right type', () => {
              const ds = gdal.open(`${__dirname}/data/sample.tif`)
              const band = ds.bands.get(1)
              const data = new Float64Array(
                new ArrayBuffer(8 * band.blockSize.x * band.blockSize.y)
              )
              return assert.isRejected(band.pixels.readBlockAsync(0, 0, data))
            })
          })
          it('should throw error if dataset already closed', () => {
            const ds = gdal.open(`${__dirname}/data/sample.tif`)
            const band = ds.bands.get(1)
            ds.close()
            return assert.isRejected(band.pixels.readBlockAsync(0, 0))
          })
        })
        describe('writeBlockAsync()', () => {
          it('should write data from TypedArray', () => {
            let i
            const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
            const band = ds.bands.get(1)

            const length = band.blockSize.x * band.blockSize.y
            const data = new Uint8Array(new ArrayBuffer(length))
            for (i = 0; i < length; i++) data[i] = i

            const p = band.pixels.writeBlockAsync(0, 0, data)
            return assert.isFulfilled(p.then(() => {
              const result = band.pixels.readBlock(0, 0)
              for (i = 0; i < length; i++) {
                assert.equal(result[i], data[i])
              }
            }))
          })
          it('should throw error if offsets are out of range', () => {
            const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
            const band = ds.bands.get(1)

            const length = band.blockSize.x * band.blockSize.y
            const data = new Uint8Array(new ArrayBuffer(length))

            return assert.isRejected(band.pixels.writeBlockAsync(0, 100, data))
          })
          it('should throw error if given array is not big enough', () => {
            const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
            const band = ds.bands.get(1)

            const length = band.blockSize.x * band.blockSize.y - 1
            const data = new Uint8Array(new ArrayBuffer(length))

            return assert.isRejected(band.pixels.writeBlockAsync(0, 0, data))
          })
          it('should throw error if given array is not the right type', () => {
            const ds = gdal.open(`${__dirname}/data/sample.tif`)
            const band = ds.bands.get(1)

            const length = band.blockSize.x * band.blockSize.y
            const data = new Float64Array(new ArrayBuffer(length * 8))

            return assert.isRejected(band.pixels.writeBlockAsync(0, 0, data))
          })
          it('should throw error if dataset already closed', () => {
            const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
            const band = ds.bands.get(1)

            const length = band.blockSize.x * band.blockSize.y
            const data = new Uint8Array(new ArrayBuffer(length))
            ds.close()
            return assert.isRejected(band.pixels.writeBlockAsync(0, 0, data))
          })
        })
        it('clampBlockAsync()', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          assert.deepEqual(band.blockSize, { x: 984, y: 8 })
          return assert.isFulfilled(Promise.all([
            band.pixels.clampBlockAsync(0, 0),
            band.pixels.clampBlockAsync(0, 100)
          ]).then(([ size1, size2 ]) => {
            assert.deepEqual(size1, { x: 984, y: 8 })
            assert.deepEqual(size2, { x: 984, y: 4 })
          }))
        })
      })
    })
    describe('flushAsync()', () => {
      it('should flush the written data', () => {
        const file = `/vsimem/write_flushAsync_test.${String(
          Math.random()
        ).substring(2)}.tmp.tif`
        const size = 64
        const ds = gdal.open(file, 'w', 'GTiff', size, size, 1, gdal.GDT_Byte)
        const band = ds.bands.get(1)
        let i

        const data = new Uint8Array(new ArrayBuffer(size * size))
        for (i = 0; i < size*size; i++) data[i] = i % 256

        band.pixels.write(0, 0, size, size, data)

        return assert.isFulfilled(band.flushAsync().then(() => {
          const newDs = gdal.open(file)
          const result = newDs.bands.get(1).pixels.read(0, 0, size, size, data)
          for (i = 0; i < size * size; i++) {
            assert.equal(result[i], data[i])
          }
        }))
      })
    })
    describe('fillAsync()', () => {
      it('should set all pixels to given value', () => {
        const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
        const band = ds.bands.get(1)
        const q = band.fillAsync(5)
        return assert.isFulfilled(q.then(() => {
          const data = band.pixels.read(0, 0, 16, 16)
          for (let i = 0; i < data.length; i++) {
            assert.equal(data[i], 5)
          }
        }))
      })
      it('should reject if dataset already closed', () => {
        const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
        const band = ds.bands.get(1)
        ds.close()
        return assert.isRejected(band.fillAsync(5))
      })
    })
    describe('"idAsync" property', () => {
      describe('getter', () => {
        it('should return number', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          return assert.eventually.equal(band.idAsync, 1)
        })
        it('should reject if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.idAsync)
        })
      })
    })
    describe('"sizeAsync" property', () => {
      describe('getter', () => {
        it('should return object', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 128, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          return assert.eventually.deepEqual(band.sizeAsync, { x: 128, y: 256 })
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.sizeAsync)
        })
      })
    })
    describe('"blockSizeAsync" property', () => {
      describe('getter', () => {
        it('should return object', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          return assert.eventually.deepEqual(band.blockSizeAsync, { x: 984, y: 8 })
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.blockSizeAsync)
        })
      })
    })
    describe('"unitTypeAsync" property', () => {
      describe('getter', () => {
        it('should return string', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          return assert.eventually.isString(band.unitTypeAsync)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.unitTypeAsync)
        })
      })
    })
    describe('"dataTypeAsync" property', () => {
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
          return assert.eventually.equal(band.dataTypeAsync, gdal.GDT_Float64)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.dataTypeAsync)
        })
      })
    })
    describe('"minimumAsync" property', () => {
      describe('getter', () => {
        it('should return number', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          const band = ds.bands.get(1)
          return assert.eventually.equal(band.minimumAsync, 177)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.minimumAsync)
        })
      })
    })
    describe('"maximumAsync" property', () => {
      describe('getter', () => {
        it('should return number', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          const band = ds.bands.get(1)
          return assert.eventually.equal(band.maximumAsync, 182)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.maximumAsync)
        })
      })
    })
    describe('"offsetAsync" property', () => {
      describe('getter', () => {
        it('should return number', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          if (semver.gt(gdal.version, '3.4.999')) {
            return assert.eventually.isNull(band.offsetAsync)
          }
          return assert.eventually.equal(band.offsetAsync, 0)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.offsetAsync)
        })
      })
    })
    describe('"scaleAsync" property', () => {
      describe('getter', () => {
        it('should return number', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          if (semver.gt(gdal.version, '3.4.999')) {
            return assert.eventually.isNull(band.scaleAsync)
          }
          return assert.eventually.equal(band.scaleAsync, 1)
        })
        it('should throw error if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.scaleAsync)
        })
      })
    })
    describe('"noDataValueAsync" property', () => {
      describe('getter', () => {
        it('should return number', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          const band = ds.bands.get(1)
          return assert.eventually.equal(band.noDataValueAsync, 0)
        })
        it('should return null if not set', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          return assert.eventually.isNull(band.noDataValueAsync)
        })
        it('should reject if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.noDataValueAsync)
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
      describe('computeStatisticsAsync()', () => {
        it('should compute statistics', () => {
          const band = statsBand()
          const statsq = band.computeStatisticsAsync(false)
          return assert.isFulfilled(statsq.then((stats) => {
            assert.equal(stats.min, 0)
            assert.equal(stats.max, 20)
            assert.closeTo(stats.mean, 5, 0.1)
            assert.closeTo(stats.std_dev, 1, 0.1)
          }))
        })
        it('should reject if dataset already closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.computeStatisticsAsync(false))
        })
      })
    })
    describe('getMetadataAsync()', () => {
      it('should return object', () => {
        const band = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`).bands.get(1)
        const meta = band.getMetadataAsync()
        return Promise.all([
          assert.eventually.isObject(meta),
          assert.eventually.propertyVal(meta, 'LAYER_TYPE', 'athematic')
        ])
      })
    })
    describe('setMetadataAsync()', () => {
      it('should set the metadata', () => {
        const band = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte).bands.get(1)
        return assert.isFulfilled(band.setMetadataAsync({ name: 'temporary' }).then((r) => {
          assert.isTrue(r)
          const metadata = band.getMetadata()
          assert.isObject(metadata)
          assert.equal(metadata.name, 'temporary')
        }))
      })
    })
    describe('"colorInterpretationAsync" property', () => {
      describe('getter', () => {
        it('should return colorInterpretation', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          return assert.eventually.equal(band.colorInterpretationAsync, gdal.GCI_GrayIndex)
        })
        it('should reject if dataset already closed', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.colorInterpretationAsync, /already been destroyed/)
        })
      })
    })
    describe('"colorTableAsync" property', () => {
      describe('getter', () => {
        it('should return a read-only colorTable', () => {
          const ds = gdal.open(`${__dirname}/data/CM13ct.png`)
          const band = ds.bands.get(1)
          return assert.isFulfilled(Promise.all([
            assert.eventually.instanceOf(band.colorTableAsync, gdal.ColorTable),
            assert.eventually.deepEqual(band.colorTableAsync.then((c) => c?.get(1)),
              { c1: 7, c2: 8, c3: 45, c4: 255 })
          ]))
        })
        it('should reject if dataset already closed', () => {
          const ds = gdal.open(`${__dirname}/data/CM13ct.png`)
          const band = ds.bands.get(1)
          ds.close()
          return assert.isRejected(band.colorTableAsync, /already been destroyed/)
        })
      })
    })
    describe('"categoryNamesAsync" property', () => {
      it('should allow setting and retrieving the category names', () => {
        const band = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte).bands.get(1)
        const cats = [ 'dry', 'humid', 'wet', 'soaking' ]
        band.categoryNames = cats
        return assert.eventually.deepEqual(band.categoryNamesAsync, cats)
      })
    })
    describe('"hasArbitraryOverviewsAsync" property', () => {
      it('should always return false', () => {
        const band = gdal.open('temp', 'w', 'MEM', 16, 16, 1, gdal.GDT_Byte).bands.get(1)
        return assert.eventually.equal(band.hasArbitraryOverviewsAsync, false)
      })
    })
  })
})
