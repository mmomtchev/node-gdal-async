import * as gdal from 'gdal-async'
import * as chaiAsPromised from 'chai-as-promised'
import * as chai from 'chai'
const assert = chai.assert
chai.use(chaiAsPromised)
import * as path from 'path'
import * as semver from 'semver'

describe('gdal_utils', () => {
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  describe('translate', () => {
    it('should be equivalent to gdal_translate', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'multiband.tif'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tif`
      const out = gdal.translate(tmpFile, ds, [ '-b', '1' ])
      assert.equal(out.bands.count(), 1)
      out.close()
      gdal.vsimem.release(tmpFile)
    })
    it('should be callable w/o options', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'multiband.tif'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tif`
      const out = gdal.translate(tmpFile, ds)
      assert.equal(out.bands.count(), 3)
      out.close()
      gdal.vsimem.release(tmpFile)
    })
    it('should support progress callbacks', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'multiband.tif'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tif`
      let calls = 0
      const out = gdal.translate(tmpFile, ds, [ '-b', '1' ], { progress_cb: () => calls++ })
      assert.isAbove(calls, 0)
      out.close()
      gdal.vsimem.release(tmpFile)
    })
    it('should throw on unrecognized options', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'multiband.tif'))
      ds.close()
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tif`
      assert.throws(() => {
        gdal.translate(tmpFile, ds, [ '-nosuchoption' ])
      })
    })
    it('should throw if the Dataset is closed', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'multiband.tif'))
      ds.close()
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tif`
      assert.throws(() => {
        gdal.translate(tmpFile, ds)
      })
    })
    it('should throw if the Dataset object is of wrong type', () => {
      assert.throws(() => {
        gdal.translate('a', {} as gdal.Dataset)
      })
    })
  })

  describe('translateAsync', () => {
    it('should be equivalent to gdal_translate', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'multiband.tif'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tif`
      return assert.isFulfilled(gdal.translateAsync(tmpFile, ds, [ '-b', '1' ])
        .then((out) => {
          assert.equal(out.bands.count(), 1)
          out.close()
          gdal.vsimem.release(tmpFile)
        })
      )
    })
    it('should be callable w/o options', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'multiband.tif'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tif`
      return assert.isFulfilled(gdal.translateAsync(tmpFile, ds,)
        .then((out) => {
          assert.equal(out.bands.count(), 3)
          out.close()
          gdal.vsimem.release(tmpFile)
        })
      )
    })
    it('should reject when the dataset is already closed', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'multiband.tif'))
      ds.close()
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tif`
      return assert.isRejected(gdal.translateAsync(tmpFile, ds), /already been destroyed/)
    })
    it('should reject on error', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'multiband.tif'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tif`
      return assert.isRejected(gdal.translateAsync(tmpFile, ds, [ '-of', 'nosuchformat' ]), /not recognised/)
    })
  })

  describe('vectorTranslate', () => {
    it('should accept a destination filename', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`

      const out = gdal.vectorTranslate(tmpFile, ds, [ '-f', 'GPKG' ])

      assert.equal(out.driver.description, 'GPKG')
      assert.equal(out.layers.get(0).features.first().fields.get('kind'), 'county')
      out.close()
      gdal.vsimem.release(tmpFile)
    })
    it('should support progress callbacks', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`
      const tmpDS = gdal.open(tmpFile, 'w', 'GPKG')

      let calls = 0
      const out = gdal.vectorTranslate(tmpDS, ds, [ '-f', 'GPKG' ], { progress_cb: () => calls++ })
      assert.isAbove(calls, 0)

      out.close()
      gdal.vsimem.release(tmpFile)
    })
    it('should accept a destination dataset', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`
      const tmpDS = gdal.open(tmpFile, 'w', 'GPKG')

      const out = gdal.vectorTranslate(tmpDS, ds, [ '-f', 'GPKG' ])
      assert.strictEqual(out, tmpDS)

      assert.equal(out.layers.get(0).features.first().fields.get('kind'), 'county')
      assert.equal(out.driver.description, 'GPKG')
      out.close()
      gdal.vsimem.release(tmpFile)
    })
    it('should throw on error', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`

      assert.throws(() => {
        gdal.vectorTranslate(tmpFile, ds, [ '-f', 'nosuchformat' ])
      }, /nosuchformat/)
    })
    it('should throw on unrecognized options', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`

      assert.throws(() => {
        gdal.vectorTranslate(tmpFile, ds, [ '-nosuchoption' ])
      }, /nosuchoption/)
    })
  })

  describe('vectorTranslateAsync', () => {
    if (semver.gte(gdal.version, '3.0.0')) {
      it('should accept a destination filename', () => {
        const ds = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
        const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`

        return assert.isFulfilled(gdal.vectorTranslateAsync(tmpFile, ds, [ '-f', 'GPKG' ])
          .then((out) => {
            assert.equal(out.driver.description, 'GPKG')
            assert.equal(out.layers.get(0).features.first().fields.get('kind'), 'county')
            out.close()
            gdal.vsimem.release(tmpFile)
          })
        )
      })
    }
    it('should accept a destination dataset', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`
      const tmpDS = gdal.open(tmpFile, 'w', 'GPKG')

      return assert.isFulfilled(gdal.vectorTranslateAsync(tmpDS, ds, [ '-f', 'GPKG' ])
        .then((out) => {
          assert.equal(out.layers.get(0).features.first().fields.get('kind'), 'county')
          assert.strictEqual(out, tmpDS)
          out.close()
          gdal.vsimem.release(tmpFile)
        })
      )
    })
    it('should reject on error', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`

      return assert.isRejected(gdal.vectorTranslateAsync(tmpFile, ds, [ '-f', 'nosuchformat' ]), /nosuchformat/)
    })
    it('should reject when the dataset is already destroyed', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`
      ds.close()

      return assert.isRejected(gdal.vectorTranslateAsync(tmpFile, ds), /already been destroyed/)
    })
  })

  describe('info', () => {
    it('should be equivalent to gdalinfo', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))
      const out = JSON.parse(gdal.info(ds, [ '-json' ]))
      assert.equal(out.bands[0].type, 'Byte')
    })
    it('should be callable w/o options', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))
      assert.isString(gdal.info(ds))
    })
    it('should throw on error', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))
      ds.close()
      assert.throws(() => {
        gdal.info(ds)
      }, /already been destroyed/)
    })
    it('should throw on unrecognized options', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))
      assert.throws(() => {
        gdal.info(ds, [ '-nosuchoption' ])
      }, /nosuchoption/)
    })
  })

  describe('infoAsync', () => {
    it('should be equivalent to gdalinfo', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))
      return assert.isFulfilled(gdal.infoAsync(ds, [ '-json' ])
        .then((out) => JSON.parse(out))
        .then((json) => {
          assert.equal(json.bands[0].type, 'Byte')
        })
      )
    })
    it('should reject on error', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))
      ds.close()
      return assert.isRejected(gdal.infoAsync(ds), /already been destroyed/)
    })
  })

  describe('warp', () => {
    it('should be equivalent to gdalwarp', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))

      let out = gdal.warp(tmpFile, null, [ ds ], [ '-t_srs', 'epsg:3587' ])

      assert.isTrue(out.srs?.isSame(gdal.SpatialReference.fromEPSG(3587)))
      out.close()

      out = gdal.open(tmpFile)
      assert.isTrue(out.srs?.isSame(gdal.SpatialReference.fromEPSG(3587)))
      out.close()

      gdal.vsimem.release(tmpFile)
    })
    it('should accept a target dataset', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`
      const src_ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))
      const dst_ds = gdal.open(tmpFile, 'w', 'GTiff', src_ds.rasterSize.x, src_ds.rasterSize.y, 1, gdal.GDT_CFloat64)
      dst_ds.srs = gdal.SpatialReference.fromEPSG(3587)
      dst_ds.geoTransform = [
        3968863.6398134492,
        7.510187636375825,
        0,
        469467.09900118795,
        0,
        -7.510187636375825
      ]

      const out = gdal.warp(null, dst_ds, [ src_ds ])
      assert.strictEqual(dst_ds, out)
      assert.isTrue(dst_ds.srs.isSame(gdal.SpatialReference.fromEPSG(3587)))
      out.close()

      gdal.vsimem.release(tmpFile)
    })
    it('should support progress callbacks', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))

      let calls = 0
      const out = gdal.warp(tmpFile, null, [ ds ], [ '-t_srs', 'epsg:3587' ], { progress_cb: () => calls++ })
      assert.isAbove(calls, 0)

      out.close()
      gdal.vsimem.release(tmpFile)
    })
    it('should throw on unrecognized options', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))

      assert.throws(() => {
        gdal.warp(tmpFile, null, [ ds ], [ '-nosuchoption' ])
      }, /nosuchoption/)
    })
    it('should throw without destination', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))
      assert.throws(() => {
        gdal.warp(null, null, [ ds ], [ '-t_srs', 'epsg:3587' ])
      }, /must be given/)
    })
    it('should throw without src datasets', () => {
      assert.throws(() => {
        gdal.warp('/vsimem/out.tif', null, [])
      }, /at least one/)
    })
    it('should throw if the source has been closed', () => {
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))
      ds.close()
      assert.throws(() => {
        gdal.warp('/vsimem/out.tif', null, [ ds ])
      }, /already been destroyed/)
    })
  })

  describe('warpAsync', () => {
    it('should be equivalent to gdalwarp', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))

      const out = gdal.warpAsync(tmpFile, null, [ ds ], [ '-t_srs', 'epsg:3587' ])

      return assert.isFulfilled(out.then((out_ds) => {
        assert.isTrue(out_ds.srs?.isSame(gdal.SpatialReference.fromEPSG(3587)))
        out_ds.close()

        out_ds = gdal.open(tmpFile)
        assert.isTrue(out_ds.srs?.isSame(gdal.SpatialReference.fromEPSG(3587)))
        out_ds.close()

        gdal.vsimem.release(tmpFile)
      }))
    })
    it('should reject on error', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.gpkg`
      const ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))
      ds.close()
      return assert.isRejected(gdal.warpAsync(tmpFile, null, [ ds ], [ '-t_srs', 'epsg:3587' ]))
    })
  })

  describe('buildVRT', () => {
    it('should be equivalent to gdalbuildvrt', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.vrt`
      const t = path.resolve(__dirname, 'data', 'AROME_T2m_10.tiff')
      const td = path.resolve(__dirname, 'data', 'AROME_D2m_10.tiff')

      let out_ds = gdal.buildVRT(tmpFile, [ t, td ])

      assert.equal(out_ds.bands.count(), 1)
      out_ds.close()

      out_ds = gdal.open(tmpFile)
      assert.equal(out_ds.bands.count(), 1)
      out_ds.close()

      gdal.vsimem.release(tmpFile)
    })

    it('should accept open Datasets', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.vrt`
      const t = gdal.open(path.resolve(__dirname, 'data', 'AROME_T2m_10.tiff'))
      const td = gdal.open(path.resolve(__dirname, 'data', 'AROME_D2m_10.tiff'))

      let out_ds = gdal.buildVRT(tmpFile, [ t, td ])

      assert.equal(out_ds.bands.count(), 1)
      assert.deepEqual(out_ds.rasterSize, t.rasterSize)
      assert.deepEqual(out_ds.geoTransform, t.geoTransform)
      out_ds.close()

      out_ds = gdal.open(tmpFile)
      assert.equal(out_ds.bands.count(), 1)
      assert.deepEqual(out_ds.rasterSize, t.rasterSize)
      assert.deepEqual(out_ds.geoTransform, t.geoTransform)
      out_ds.close()

      gdal.vsimem.release(tmpFile)
    })

    it('should accept gdalbuildvrt command-line options', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.vrt`
      const t = path.resolve(__dirname, 'data', 'AROME_T2m_10.tiff')
      const td = path.resolve(__dirname, 'data', 'AROME_D2m_10.tiff')

      let out_ds = gdal.buildVRT(tmpFile, [ t, td ], [ '-separate' ])

      assert.equal(out_ds.bands.count(), 2)
      out_ds.close()

      out_ds = gdal.open(tmpFile)
      assert.equal(out_ds.bands.count(), 2)
      out_ds.close()

      gdal.vsimem.release(tmpFile)
    })

    it('should call a progress callback', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.vrt`
      const t = path.resolve(__dirname, 'data', 'AROME_T2m_10.tiff')
      const td = path.resolve(__dirname, 'data', 'AROME_D2m_10.tiff')

      let called = 0
      const out_ds = gdal.buildVRT(tmpFile, [ t, td ], [], { progress_cb: () => {
        called++
      } })
      assert.equal(out_ds.bands.count(), 1)
      assert.isAbove(called, 0)
      out_ds.close()

      gdal.vsimem.release(tmpFile)
    })

    it('should throw when mixing input types', () => {
      assert.throws(() => {
        gdal.buildVRT('file', [ 'a', {} as string ])
      }, /must have the same type/)
      assert.throws(() => {
        gdal.buildVRT('file', [
          gdal.open(path.resolve(__dirname, 'data', 'sample.tif')),
          'a' as unknown as gdal.Dataset
        ])
      }, /Object must be a Dataset object/)
    })
  })

  describe('buildVRTAsync', () => {
    it('should be equivalent to gdalbuildvrt', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.vrt`
      const t = path.resolve(__dirname, 'data', 'AROME_T2m_10.tiff')
      const td = path.resolve(__dirname, 'data', 'AROME_D2m_10.tiff')

      const out = gdal.buildVRTAsync(tmpFile, [ t, td ])

      return assert.isFulfilled(out.then((out_ds) => {
        assert.equal(out_ds.bands.count(), 1)
        out_ds.close()

        out_ds = gdal.open(tmpFile)
        assert.equal(out_ds.bands.count(), 1)
        out_ds.close()

        gdal.vsimem.release(tmpFile)
      }))
    })
    it('should reject on error', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.vrt`
      return assert.isRejected(gdal.buildVRTAsync(tmpFile, [ path.resolve(__dirname, 'data', 'nofile') ]),
        /nofile/)
    })
  })

  describe('rasterize', () => {
    it('should be equivalent to gdal_rasterize', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tiff`
      const dst = gdal.open(tmpFile, 'w', 'GTiff', 256, 256, 1, gdal.GDT_Float32)

      let out_ds = gdal.rasterize(dst, gdal.open(path.resolve(__dirname, 'data', 'park.geo.json')))

      assert.equal(out_ds.bands.count(), 1)
      out_ds.close()

      out_ds = gdal.open(tmpFile)
      assert.equal(out_ds.bands.count(), 1)
      out_ds.close()

      gdal.vsimem.release(tmpFile)
    })
    it('should accept open Dataset', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tiff`
      let out_ds = gdal.rasterize(tmpFile, gdal.open(path.resolve(__dirname, 'data', 'park.geo.json')),
        [ '-ts', '256', '256' ])

      assert.equal(out_ds.bands.count(), 1)
      assert.deepEqual(out_ds.rasterSize, { x: 256, y: 256 })
      out_ds.close()

      out_ds = gdal.open(tmpFile)
      assert.equal(out_ds.bands.count(), 1)
      assert.deepEqual(out_ds.rasterSize, { x: 256, y: 256 })
      out_ds.close()

      gdal.vsimem.release(tmpFile)
    })
    it('should call a progress callback', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tiff`
      let called = 0
      const out_ds = gdal.rasterize(tmpFile,
        gdal.open(path.resolve(__dirname, 'data', 'park.geo.json')),
        [ '-ts', '256', '256' ],
        { progress_cb: () => {
          called++
        } })

      assert.isAbove(called, 0)

      out_ds.close()
      gdal.vsimem.release(tmpFile)
    })
    it('should reject on error', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.vrt`
      const src = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
      src.close()
      assert.throws(() => {
        gdal.rasterize(tmpFile, src)
      }, /Dataset object has already been destroyed/)
    })
  })

  describe('rasterizeAsync', () => {
    it('should be equivalent to gdal_rasterize', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.tiff`
      const dst = gdal.open(tmpFile, 'w', 'GTiff', 256, 256, 1, gdal.GDT_Float32)

      const out = gdal.rasterizeAsync(dst, gdal.open(path.resolve(__dirname, 'data', 'park.geo.json')))

      return assert.isFulfilled(out.then((out_ds) => {
        assert.equal(out_ds.bands.count(), 1)
        out_ds.close()

        out_ds = gdal.open(tmpFile)
        assert.equal(out_ds.bands.count(), 1)
        out_ds.close()

        gdal.vsimem.release(tmpFile)
      }))
    })
    it('should reject on error', () => {
      const tmpFile = `/vsimem/${String(Math.random()).substring(2)}.vrt`
      const src = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
      src.close()
      return assert.isRejected(gdal.rasterizeAsync(tmpFile, src),
        /Dataset object has already been destroyed/)
    })
  })

  describe('calcAsync', () => {
    it('should perform the given calculation', async () => {
      const tempFile = `/vsimem/cloudbase_${String(Math.random()).substring(2)}.tiff`
      const T2m = await gdal.openAsync(path.resolve(__dirname, 'data','AROME_T2m_10.tiff'))
      const D2m = await gdal.openAsync(path.resolve(__dirname, 'data','AROME_D2m_10.tiff'))
      const size = await T2m.rasterSizeAsync
      const cloudBase = await gdal.openAsync(tempFile,
        'w', 'GTiff', size.x, size.y, 1, gdal.GDT_Float64)

      // Espy's estimation for cloud base height
      const espyFn = (t: number, td: number) => 125 * (t - td);

      // Underground clouds are very rare
      (await cloudBase.bands.getAsync(1)).noDataValue = -1e38

      let done = 0
      await gdal.calcAsync({
        t: await T2m.bands.getAsync(1),
        td: await D2m.bands.getAsync(1)
      }, await cloudBase.bands.getAsync(1), espyFn, {
        convertNoData: true,
        progress_cb: (complete) => {
          assert.isAbove(complete, done)
          done = complete
        }
      })
      assert.closeTo(done, 1, 0.1)

      const t2mData = await (await T2m.bands.getAsync(1)).pixels.readAsync(0, 0, size.x, size.y)
      const d2mData = await (await D2m.bands.getAsync(1)).pixels.readAsync(0, 0, size.x, size.y)
      const cbData = await (await cloudBase.bands.getAsync(1)).pixels.readAsync(0, 0, size.x, size.y)

      // check every 1000th element, there are lots of them
      for (let i = 0; i < cbData.length; i+=1000) {
        assert.closeTo(cbData[i], espyFn(t2mData[i], d2mData[i]), 1e-6)
      }
      cloudBase.close()
      gdal.vsimem.release(tempFile)
    })

    it('should reject on exception in the supplied function', () => {
      const tempFile = `/vsimem/invalid_calc_${String(Math.random()).substring(2)}.tiff`
      const T2m = gdal.open(path.resolve(__dirname, 'data','AROME_T2m_10.tiff'))
      const D2m = gdal.open(path.resolve(__dirname, 'data','AROME_D2m_10.tiff'))
      const size = T2m.rasterSize
      const throwFn = () => {
        throw new Error('propagate this')
      }
      return assert.isRejected(
        gdal.calcAsync({
          A: T2m.bands.get(1),
          B: D2m.bands.get(1)
        },
        gdal.open(tempFile, 'w', 'GTiff', size.x, size.y, 1, gdal.GDT_Float64).bands.get(1),
        throwFn),
        /propagate this/
      )
    })

    it('should reject when the file is corrupted', function () {
      const tempFile = `/vsimem/invalid_calc_${String(Math.random()).substring(2)}.tiff`
      let T2m, D2m
      try {
        T2m = gdal.open(path.resolve(__dirname, 'data','AROME_T2m_10.tiff'))
        D2m = gdal.open(path.resolve(__dirname, 'data','truncated.tiff'))
      } catch (e) {
        // Older GDAL versions cannot open the truncated file, so this is
        // considered a successful test too
        this.skip()
      }
      const size = T2m.rasterSize
      const espyFn = (t: number, td: number) => 125 * (t - td)
      return assert.isRejected(
        gdal.calcAsync({
          A: T2m.bands.get(1),
          B: D2m.bands.get(1)
        },
        gdal.open(tempFile, 'w', 'GTiff', size.x, size.y, 1, gdal.GDT_Float64).bands.get(1),
        espyFn),
        /Cannot read/
      )
    })

    it('should reject on progress exception', () => {
      const tempFile = `/vsimem/invalid_calc_${String(Math.random()).substring(2)}.tiff`
      const T2m = gdal.open(path.resolve(__dirname, 'data','AROME_T2m_10.tiff'))
      const D2m = gdal.open(path.resolve(__dirname, 'data','AROME_D2m_10.tiff'))
      const size = T2m.rasterSize
      const espyFn = (t: number, td: number) => 125 * (t - td)
      return assert.isRejected(
        gdal.calcAsync({
          A: T2m.bands.get(1),
          B: D2m.bands.get(1)
        },
        gdal.open(tempFile, 'w', 'GTiff', size.x, size.y, 1, gdal.GDT_Float64).bands.get(1),
        espyFn,
        { progress_cb: () => {
          throw new Error('progress error')
        } }
        ),
        /progress error/
      )
    })

    it('should reject when raster sizes do not match', () => {
      const tempFile = `/vsimem/invalid_calc_${String(Math.random()).substring(2)}.tiff`
      const espyFn = (t: number, td: number) => 125 * (t - td)
      return assert.isRejected(
        gdal.calcAsync({
          A: gdal.open(path.resolve(__dirname, 'data','AROME_T2m_10.tiff')).bands.get(1),
          B: gdal.open(path.resolve(__dirname, 'data','sample.tif')).bands.get(1)
        },
        gdal.open(tempFile, 'w', 'GTiff', 128, 128, 1, gdal.GDT_Float64).bands.get(1),
        espyFn),
        /dimensions must match/
      )
    })

    it('should support ignoring NoData values', async () => {
      const tempFile = `/vsimem/calc_nodata1_${String(Math.random()).substring(2)}.tiff`
      const dem = await gdal.openAsync(path.resolve(__dirname, 'data', 'dem_azimuth50_pa.img'))
      const size = await dem.rasterSizeAsync
      const output = await gdal.openAsync(tempFile,
        'w', 'GTiff', size.x, size.y, 1, gdal.GDT_Float64)

      const fn = (dem: number) => (dem + 1);

      (await output.bands.getAsync(1)).noDataValue = -100

      await gdal.calcAsync({
        dem: await dem.bands.getAsync(1)
      }, await output.bands.getAsync(1), fn, { convertNoData: false })

      assert.equal(output.bands.get(1).pixels.get(0, 0), 1)
      output.close()
      gdal.vsimem.release(tempFile)
    })

    it('should support converting NoData values', async () => {
      const tempFile = `/vsimem/calc_nodata2_${String(Math.random()).substring(2)}.tiff`
      const dem = await gdal.openAsync(path.resolve(__dirname, 'data', 'dem_azimuth50_pa.img'))
      const size = await dem.rasterSizeAsync
      const output = await gdal.openAsync(tempFile,
        'w', 'GTiff', size.x, size.y, 1, gdal.GDT_Float64)

      const fn = (dem: number) => (dem + 1);

      (await output.bands.getAsync(1)).noDataValue = -100

      await gdal.calcAsync({
        dem: await dem.bands.getAsync(1)
      }, await output.bands.getAsync(1), fn, { convertNoData: true, convertInput: true })

      assert.equal(output.bands.get(1).pixels.get(0, 0), -100)
      output.close()
      gdal.vsimem.release(tempFile)
    })
  })
})
