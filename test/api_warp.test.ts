import * as gdal from 'gdal-async'
import { assert } from 'chai'
import * as semver from 'semver'

describe('gdal', () => {
  afterEach(() => void global.gc!())

  describe('suggestedWarpOutput()', () => {
    let src: gdal.Dataset
    beforeEach(() => {
      src = gdal.open(`${__dirname}/data/sample.tif`)
    })
    afterEach(() => {
      src.close()
    })
    it('should return object with suggested output dimensions', () => {
      // src properties
      const w = src.rasterSize.x
      const h = src.rasterSize.y
      const gt = src.geoTransform

      // warp options
      const s_srs = src.srs
      if (s_srs === null) throw new TypeError('No SRS')
      const t_srs = gdal.SpatialReference.fromProj4('+init=epsg:4326')
      const tx = new gdal.CoordinateTransformation(s_srs, t_srs)

      // compute output extent
      if (gt === null) throw new TypeError('No GeoTransform')
      const ul = tx.transformPoint(gt[0], gt[3])
      const ur = tx.transformPoint(gt[0] + gt[1] * w, gt[3])
      const lr = tx.transformPoint(gt[0] + gt[1] * w, gt[3] + gt[5] * h)
      const ll = tx.transformPoint(gt[0], gt[3] + gt[5] * h)

      const extent = new gdal.Polygon()
      const ring = new gdal.LinearRing()
      ring.points.add([ ul, ur, lr, ll, ul ])
      extent.rings.add(ring)
      const envelope = extent.getEnvelope()

      // compute pixel resolution in target srs (function assumes square pixels)

      const s_diagonal = new gdal.LineString()
      s_diagonal.points.add(gt[0], gt[3])
      s_diagonal.points.add(gt[0] + gt[1] * w, gt[3] + gt[5] * h)
      const t_diagonal = s_diagonal.clone() as gdal.LineString
      t_diagonal.transform(tx)

      const pixels_along_diagonal = Math.sqrt(w * w + h * h)
      // var sr = s_diagonal.getLength() / pixels_along_diagonal;
      const tr = t_diagonal.getLength() / pixels_along_diagonal

      // compute expected size / geotransform with computed resolution

      const expected = {
        geoTransform: [ envelope.minX, tr, gt[2], envelope.maxY, gt[4], -tr ],
        rasterSize: {
          x: Math.ceil(Math.max(envelope.maxX - envelope.minX) / tr),
          y: Math.ceil(Math.max(envelope.maxY - envelope.minY) / tr)
        }
      }

      const output = gdal.suggestedWarpOutput({
        src: src,
        s_srs: s_srs,
        t_srs: t_srs
      })

      assert.closeTo(output.rasterSize.x, expected.rasterSize.x, 1)
      assert.closeTo(output.rasterSize.y, expected.rasterSize.y, 1)
      assert.closeTo(output.geoTransform[0], expected.geoTransform[0], 0.001)
      assert.closeTo(output.geoTransform[1], expected.geoTransform[1], 0.001)
      assert.closeTo(output.geoTransform[2], expected.geoTransform[2], 0.001)
      assert.closeTo(output.geoTransform[3], expected.geoTransform[3], 0.001)
      assert.closeTo(output.geoTransform[4], expected.geoTransform[4], 0.001)
      assert.closeTo(output.geoTransform[5], expected.geoTransform[5], 0.001)
    })
  })
  describe('reprojectImage()', () => {
    let src: gdal.Dataset
    beforeEach(() => {
      src = gdal.open(`${__dirname}/data/sample.tif`)
    })
    afterEach(() => {
      try {
        src.close()
      } catch (_err) {
        /* ignore */
      }
    })
    it('should write reprojected image into dst dataset', () => {
      if (semver.satisfies(gdal.version, '^2.2.0')) {
        /* GDALReprojectImage with 1 band with different source and target number
         * is bugged on GDAL 2.2.x
         * https://github.com/OSGeo/gdal/commit/7bb8d37f1bfbb9c7bbcbe3050df55ef227bbc260#diff-3e9f012797a2e1a7d138afea7e967499799c6a65281b9769b5f56377119c919c
         */
        return
      }

      let x, y

      /*
       * expected result is the same (but not necessarily exact) as the result of:
       *
       *  gdalwarp
       * -tr 0.0005 0.0005
       * -t_srs EPSG:4326
       * -r bilinear
       * -cutline ./test/data/cutline.shp
       * -dstalpha
       * -of GTiff
       * ./test/data/sample.tif ./test/data/sample_warped.tif
       */

      const expected = gdal.open(`${__dirname}/data/sample_warped.tif`)
      const cutline_ds = gdal.open(`${__dirname}/data/cutline.shp`)

      // src properties
      let w = src.rasterSize.x
      let h = src.rasterSize.y
      const gt = src.geoTransform

      // warp options
      const s_srs = src.srs
      const t_srs = gdal.SpatialReference.fromProj4('+init=epsg:4326')
      const tr = { x: 0.0005, y: 0.0005 } // target resolution
      if (t_srs === null || s_srs === null) throw new TypeError('No SRS')
      const tx = new gdal.CoordinateTransformation(s_srs, t_srs)
      const cutline = cutline_ds.layers
        .get(0)
        .features.get(0)
        .getGeometry()

      // transform cutline to source dataset px/line coordinates
      const geotransformer = new gdal.CoordinateTransformation(t_srs, src)
      cutline.transform(geotransformer)

      // compute output geotransform / dimensions
      if (gt === null) throw new TypeError('No GeoTransform')
      const ul = tx.transformPoint(gt[0], gt[3])
      const ur = tx.transformPoint(gt[0] + gt[1] * w, gt[3])
      const lr = tx.transformPoint(gt[0] + gt[1] * w, gt[3] + gt[5] * h)
      const ll = tx.transformPoint(gt[0], gt[3] + gt[5] * h)

      const extent = new gdal.Polygon()
      const ring = new gdal.LinearRing()
      ring.points.add([ ul, ur, lr, ll, ul ])
      extent.rings.add(ring)
      const envelope = extent.getEnvelope()

      const tw = Math.ceil(Math.max(envelope.maxX - envelope.minX) / tr.x)
      const th = Math.ceil(Math.max(envelope.maxY - envelope.minY) / tr.y)

      const dst = gdal.open('temp', 'w', 'MEM', tw, th, 2, gdal.GDT_Int16)
      dst.srs = t_srs
      dst.geoTransform = [ envelope.minX, tr.x, gt[2], envelope.maxY, gt[4], -tr.y ]

      // warp
      gdal.reprojectImage({
        src: src,
        dst: dst,
        s_srs: s_srs,
        t_srs: t_srs,
        resampling: gdal.GRA_Bilinear,
        cutline: cutline,
        dstAlphaBand: 1,
        blend: 0,
        srcBands: [ 1 ],
        dstBands: [ 2 ]
      })

      // compare with result of gdalwarp

      // gdalwarp might pick the output size slightly differently (+/- 1 px)
      assert.closeTo(dst.rasterSize.x, expected.rasterSize.x, 1)
      assert.closeTo(dst.rasterSize.y, expected.rasterSize.y, 1)
      w = Math.min(dst.rasterSize.x, expected.rasterSize.x)
      h = Math.min(dst.rasterSize.y, expected.rasterSize.y)

      // check data band
      const expected_pixels = expected.bands.get(1).pixels
      const actual_pixels = dst.bands.get(2).pixels
      let error = 0
      let n = 0
      for (x = 0; x < w; x += 10) {
        for (y = 0; y < h; y += 10) {
          error += Math.abs(
            actual_pixels.get(x, y) - expected_pixels.get(x, y)
          )
          n++
        }
      }
      const avgerror = error / n
      assert.isBelow(avgerror, 0.5, 'minimal error in pixel data')

      // check alpha band - skipped for now (breaks on shared MacOS builds on travis)
      /*
      expected_pixels = expected.bands.get(2).pixels;
      actual_pixels = dst.bands.get(1).pixels;
      error = 0;
      for (x = 0; x < w; x += 10) {
        for (y = 0; y < h; y += 10) {
          error += Math.abs(actual_pixels.get(x, y) - expected_pixels.get(x, y));
        }
      }
      avgerror = error / n;
      assert.isBelow(avgerror, 0.5, 'minimal error in alpha band pixel data');
      */

      dst.close()
      cutline_ds.close()
      expected.close()
    })
    it('should use approx transformer if maxError is given', () => {
      const options = {
        src: src,
        s_srs: src.srs,
        t_srs: gdal.SpatialReference.fromEPSG(4326)
      } as gdal.ReprojectOptions
      const info = gdal.suggestedWarpOutput(options)

      // use lower than suggested resolution (faster)
      info.rasterSize.x /= 4
      info.rasterSize.y /= 4
      info.geoTransform[1] *= 4
      info.geoTransform[5] *= 4

      // compute exact version
      options.dst = gdal.open(
        'temp',
        'w',
        'MEM',
        info.rasterSize.x,
        info.rasterSize.y,
        1,
        gdal.GDT_Byte
      )
      options.dst.geoTransform = info.geoTransform

      gdal.reprojectImage(options)

      const exact_checksum = gdal.checksumImage(options.dst.bands.get(1))

      // compute approximate version
      options.dst = gdal.open(
        'temp',
        'w',
        'MEM',
        info.rasterSize.x,
        info.rasterSize.y,
        1,
        gdal.GDT_Byte
      )
      options.dst.geoTransform = info.geoTransform
      options.maxError = 4

      gdal.reprojectImage(options)

      const approx_checksum = gdal.checksumImage(options.dst.bands.get(1))

      assert.notEqual(approx_checksum, exact_checksum)
    })
    it('should produce same result using multi option', () => {
      const options = {
        src: src,
        s_srs: src.srs,
        t_srs: gdal.SpatialReference.fromEPSG(4326)
      } as gdal.ReprojectOptions
      const info = gdal.suggestedWarpOutput(options)

      // use lower than suggested resolution (faster)
      info.rasterSize.x /= 4
      info.rasterSize.y /= 4
      info.geoTransform[1] *= 4
      info.geoTransform[5] *= 4

      options.dst = gdal.open(
        'temp',
        'w',
        'MEM',
        info.rasterSize.x,
        info.rasterSize.y,
        1,
        gdal.GDT_Byte
      )
      options.dst.geoTransform = info.geoTransform

      gdal.reprojectImage(options)

      const expected_checksum = gdal.checksumImage(options.dst.bands.get(1))

      options.dst = gdal.open(
        'temp',
        'w',
        'MEM',
        info.rasterSize.x,
        info.rasterSize.y,
        1,
        gdal.GDT_Byte
      )
      options.dst.geoTransform = info.geoTransform
      options.multi = true

      gdal.reprojectImage(options)

      const result_checksum = gdal.checksumImage(options.dst.bands.get(1))

      assert.equal(result_checksum, expected_checksum)
    })
    describe('argument errors', () => {
      let warpOptions, reprojectOptions: gdal.ReprojectOptions

      beforeEach(() => {
        warpOptions = {
          src: src,
          s_srs: src.srs,
          t_srs: gdal.SpatialReference.fromEPSG(4326)
        } as gdal.WarpOptions
        const info = gdal.suggestedWarpOutput(warpOptions)

        reprojectOptions = {
          ...warpOptions,
          dst: gdal.open(
            'temp',
            'w',
            'MEM',
            info.rasterSize.x,
            info.rasterSize.y,
            1,
            gdal.GDT_Byte
          )
        }
        reprojectOptions.dst.geoTransform = info.geoTransform
      })

      it('should throw if cutline is wrong geometry type', () => {
        if (semver.satisfies(gdal.version, '^2.2.0')) {
          /* GDALReprojectImage with 1 band with different source and target number
           * is bugged on GDAL 2.2.x
           * https://github.com/OSGeo/gdal/commit/7bb8d37f1bfbb9c7bbcbe3050df55ef227bbc260#diff-3e9f012797a2e1a7d138afea7e967499799c6a65281b9769b5f56377119c919c
           */
          return
        }
        reprojectOptions.cutline = new gdal.LineString()

        assert.throws(() => {
          gdal.reprojectImage(reprojectOptions)
        })
      })
      it('should throw if src dataset has been closed', () => {
        reprojectOptions.src.close()

        assert.throws(() => {
          gdal.reprojectImage(reprojectOptions)
        }, 'src dataset already closed')
      })
      it('should throw if dst dataset has been closed', () => {
        reprojectOptions.dst.close()

        assert.throws(() => {
          gdal.reprojectImage(reprojectOptions)
        }, 'dst dataset already closed')
      })
      it('should throw if dst dataset isnt a raster', () => {
        reprojectOptions.dst = gdal.open('temp', 'w', 'Memory')

        assert.throws(() => {
          gdal.reprojectImage(reprojectOptions)
        }, /must be a raster dataset|There is no affine transformation and no GCPs/)
      })
      it('should throw if src dataset isnt a raster', () => {
        reprojectOptions.src = gdal.open('temp_src', 'w', 'Memory')

        assert.throws(() => {
          gdal.reprojectImage(reprojectOptions)
        }, /must be a raster dataset|There is no affine transformation and no GCPs/)
      })
      it('should throw if srcBands option is provided but dstBands isnt', () => {
        reprojectOptions.srcBands = [ 1 ]

        assert.throws(() => {
          gdal.reprojectImage(reprojectOptions)
        }, 'dstBands must be provided if srcBands option is used')
      })
      it('should throw if dstBands option is provided but srcBands isnt', () => {
        reprojectOptions.dstBands = [ 1 ]

        assert.throws(() => {
          gdal.reprojectImage(reprojectOptions)
        }, 'srcBands must be provided if dstBands option is used')
      })
      it('should throw if srcBands option is invalid', () => {
        reprojectOptions.srcBands = [ 3 ]
        reprojectOptions.dstBands = [ 1 ]

        assert.throws(() => {
          gdal.reprojectImage(reprojectOptions)
        }, 'out of range for dataset')
      })
      it('should throw if dstBands option is invalid', () => {
        reprojectOptions.srcBands = [ 1 ]
        reprojectOptions.dstBands = [ 3 ]

        assert.throws(() => {
          gdal.reprojectImage(reprojectOptions)
        }, 'out of range for dataset')
      })
      it('should throw if dstAlphaBand is invalid', () => {
        reprojectOptions.dstAlphaBand = 5

        assert.throws(() => {
          gdal.reprojectImage(reprojectOptions)
        }, 'out of range for dataset')
      })
      it('should throw if memoryLimit is invalid', () => {
        reprojectOptions.memoryLimit = 1

        assert.throws(() => {
          gdal.reprojectImage(reprojectOptions)
        }, 'dfWarpMemoryLimit=1 is unreasonably small')
      })

      it('should use additional stringlist options', () => {
        reprojectOptions.options = { INIT_DEST: 123 }

        gdal.reprojectImage(reprojectOptions)

        const value = reprojectOptions.dst.bands.get(1).pixels.get(0, 0)

        assert.equal(value, 123)
      })

      it('should call a "progress_cb" when provided', () => {
        let calls = 0
        reprojectOptions.progress_cb = () => {
          calls++
        }

        gdal.reprojectImage(reprojectOptions)
        assert.isAbove(calls, 0)
      })

      it("should throw error if GDAL can't create transformer", () => {
        src = gdal.open(`${__dirname}/data/unsupported-srs.tif`)

        if (src.srs === null) throw new TypeError('No SRS')

        const options = {
          src: src,
          s_srs: src.srs,
          t_srs: gdal.SpatialReference.fromEPSG(3857)
        }

        assert.throws(() => {
          gdal.suggestedWarpOutput(options)
          // gdal has a different error message between versions
        }, /(Cannot find coordinate operations from)|(Mercator_1SP)/)
      })
    })
  })
})
