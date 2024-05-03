import * as gdal from 'gdal-async'
import { Transform, finished as _finished } from 'stream'
import { promisify } from 'util'
import * as chai from 'chai'
import * as path from 'path'
import * as semver from 'semver'
const assert: Chai.Assert = chai.assert
const finished = promisify(_finished)

type doneCb = (err?: unknown) => void

describe('gdal.RasterReadStream', () => {
  function noDataTest(done: doneCb, file: string, convert?: boolean) {
    const ds = gdal.open(path.resolve(__dirname, 'data', file))
    const band = ds.bands.get(1)
    const noData = ds.bands.get(1).noDataValue
    let test: (v: number) => boolean
    if (convert) test = isNaN
    else test = (v) => v === noData
    const rs = band.pixels.createReadStream({
      convertNoData: convert,
      type: Float64Array
    })
    rs.once('data', (chunk) => {
      try {
        assert.instanceOf(chunk, Float64Array)
        assert.isTrue(chunk.subarray(0, 50).every(test))
        done()
      } catch (e) {
        done(e)
      }
    })
  }

  function readTest(done: doneCb, file: string, blockOptimize: boolean) {
    const ds = gdal.open(path.resolve(__dirname, 'data', file))
    const band = ds.bands.get(1)
    const expected = band.pixels.read(0, 0, band.size.x, band.size.y)
    const type = gdal.fromDataType(band.dataType)
    const actual = new type(band.size.x * band.size.y)

    const rs = band.pixels.createReadStream({ blockOptimize })
    assert.instanceOf(rs, gdal.RasterReadStream)
    let length = 0
    rs.on('data', (chunk) => {
      try {
        assert.instanceOf(chunk, type)
        actual.set(chunk, length)
        length += chunk.length
      } catch (e) {
        rs.removeAllListeners('data')
        rs.removeAllListeners('end')
        done(e)
      }
    })
    rs.on('end', () => {
      try {
        assert.equal(length, band.size.x * band.size.y)
        assert.deepEqual(actual, expected)
        done()
      } catch (e) {
        done(e)
      }
    })
  }

  const inputFiles = [
    'gfs.t00z.alnsf.nc',
    'h5ex_d_gzip.h5',
    'sample.tif',
    'a39se10.jpg'
  ]
  if (semver.gte(gdal.version, '3.0.0')) {
    inputFiles.push('gfs.t00z.pgrb2b.4p.f000.grb2')
    inputFiles.push('France_120km_WRF_WAM_210525-00.grb')
  }

  it('should accept a raster band', (done) => readTest(done, 'sample.tif', true))
  it('should accept a raster band w/o blockOptimize', (done) => readTest(done, 'sample.tif', false))
  it('should accept a raster band w/Float', (done) => readTest(done, 'AROME_T2m_10.tiff', true))
  it('should accept a raster band w/Float w/o blockOptimize', (done) => readTest(done, 'AROME_T2m_10.tiff', false))
  it('should support on the fly conversion w/ noData', (done) => noDataTest(done, 'dem_azimuth50_pa.img', undefined))
  it('should support noData conversion', (done) => noDataTest(done, 'dem_azimuth50_pa.img', true))
  for (const file of inputFiles) {
    it(`should accept various formats (${file})`, (done) => readTest(done, file, true))
  }
})

describe('gdal.RasterWriteStream', () => {
  function writeTest(done: doneCb, w: number, h: number, len: number, blockSize: number, blockOptimize: boolean, convertNoData?: boolean) {
    const filename = `/vsimem/ds_ws_test.${String(
      Math.random()
    ).substring(2)}.tmp.tiff`
    const ds = gdal.open(filename, 'w', 'GTiff', w, h, 1, gdal.GDT_Float64, { BLOCKXSIZE: w, BLOCKYSIZE: blockSize })
    const band = ds.bands.get(1)
    if (convertNoData) band.noDataValue = 1e38
    const ws = band.pixels.createWriteStream({ blockOptimize, convertNoData })
    const pattern = new Float64Array(len)
    for (let i = 0; i < len; i++) {
      if (i % 10 == 0 && convertNoData) pattern[i] = NaN
      else pattern[i] = i
    }

    let written = 0

    // this is the callback for the final write
    function endCheck() {
      try {
        ds.flush()
        ds.close()
        const testDS = gdal.open(filename)
        const testData = testDS.bands.get(1).pixels.read(0, 0, w, h)
        if (convertNoData) {
          for (let i = 0; i < pattern.length; i++) if (isNaN(pattern[i])) pattern[i] = 1e38
        }
        for (let i = 0; i < w*h; i += len) {
          assert.deepEqual(pattern, testData.subarray(i, i + len))
        }
        testDS.close()
        gdal.vsimem.release(filename)
        done()
      } catch (e) {
        // mocha has a complicated relationship with async exceptions
        // we have to explicitly signal errors
        done(e)
      }
    }

    function write() {
      while (written < w*h) {
        written += pattern.length
        // write until finished or write returns false because the buffer is full
        if (!ws.write(pattern, written == w*h ? endCheck : undefined)) break
      }
    }

    // continue writing if we had to stop because the buffer was full
    ws.on('drain', write)

    // mocha can't catch this
    ws.on('error', (e) => {
      done(e)
    })

    // start writing
    write()
  }

  function writeTestOverflow(done: doneCb, w: number, h: number, len: number, blockSize: number, blockOptimize: boolean) {
    const doneInverted = (err: Error) => {
      if (!err) done('did not throw')
      else if (err.toString().match(/beyond the end/)) done()
      else done(err)
    }

    writeTest(doneInverted as doneCb, w, h, len, blockSize, blockOptimize, undefined)
  }

  it('should write a raster band in zero-copy mode w/ edge block',
    (done) => writeTest(done, 801, 600, 1602, 2, true, undefined))
  it('should write a raster band in zero-copy mode w/o edge block',
    (done) => writeTest(done, 801, 601, 1803, 3, true, undefined))
  it('should write a raster band in block consolidation mode w/ big chunks w/ edge block',
    (done) => writeTest(done, 801, 601, 1803, 2, true, undefined))
  it('should write a raster band in block consolidation mode w/ small chunks w/ edge block',
    (done) => writeTest(done, 801, 601, 267, 2, true, undefined))
  it('should write a raster band in block consolidation mode w/ big chunks w/o edge block',
    (done) => writeTest(done, 801, 601, 1803, 6, true, undefined))
  it('should write a raster band in block consolidation mode w/ small chunks w/o edge block',
    (done) => writeTest(done, 801, 601, 267, 6, true, undefined))
  it('should write a raster band in line mode w/ big chunks',
    (done) => writeTest(done, 801, 601, 1803, 2, false, undefined))
  it('should write a raster band in line mode w/ small chunks',
    (done) => writeTest(done, 801, 601, 267, 2, false, undefined))

  it('should throw on writing beyond the end in zero-copy mode w/ edge block',
    (done) => writeTestOverflow(done, 801, 601, 1804, 2, true))
  it('should throw on writing beyond the end in zero-copy mode w/o edge block',
    (done) => writeTestOverflow(done, 801, 601, 1804, 3, true))
  it('should throw on writing beyond the end in block consolidation mode w/ big chunks w/ edge block',
    (done) => writeTestOverflow(done, 801, 601, 1804, 2, true))
  it('should throw on writing beyond the end in block consolidation mode w/ small chunks w/ edge block',
    (done) => writeTestOverflow(done, 801, 601, 268, 2, true))
  it('should throw on writing beyond the end in block consolidation mode w/ big chunks w/o edge block',
    (done) => writeTestOverflow(done, 801, 601, 1804, 6, true))
  it('should throw on writing beyond the end in block consolidation mode w/ small chunks w/o edge block',
    (done) => writeTestOverflow(done, 801, 601, 268, 6, true))
  it('should throw on writing beyond the end in line mode w/ big chunks',
    (done) => writeTestOverflow(done, 801, 601, 1804, 2, false))
  it('should throw on writing beyond the end in line mode w/ small chunks',
    (done) => writeTestOverflow(done, 801, 601, 268, 2, false))

  it('should support noData conversion', (done) => writeTest(done, 801, 601, 1803, 2, true, true))

  it('should support an ill-behaved user application', (done) => {
    const filename = `/vsimem/ds_pressure_test.${String(
      Math.random()
    ).substring(2)}.tmp.tiff`
    const ds = gdal.open(filename, 'w', 'GTiff', 64, 128, 1, gdal.GDT_Float64)
    const band = ds.bands.get(1)
    const ws = band.pixels.createWriteStream()
    const data = new Float64Array(64)
    for (let i = 0; i < 64; i++) data[i] = i
    for (let i = 0; i < 127; i++) ws.write(data)
    ws.write(data, () => {
      try {
        ds.flush()
        ds.close()
        const testDS = gdal.open(filename)
        const testData = testDS.bands.get(1).pixels.read(0, 0, 64, 128)
        for (let i = 0; i < 128; i += 128) {
          assert.deepEqual(data, testData.subarray(i * 64, (i+1) * 64))
        }
        testDS.close()
        gdal.vsimem.release(filename)
        done()
      } catch (e) {
        done(e)
      }
    })
  })
})

describe('gdal.RasterReadStream + gdal.RasterWriteStream', () => {
  it('should support piping', () => {
    const dsIn = gdal.open(path.resolve(__dirname, 'data', 'AROME_T2m_10.tiff'))
    const filename = `/vsimem/ds_pipe_test.${String(
      Math.random()
    ).substring(2)}.tmp.tiff`
    const dsOut = gdal.open(filename, 'w', 'GTiff', dsIn.rasterSize.x, dsIn.rasterSize.y, 1, gdal.GDT_Float64)
    const bandIn = dsIn.bands.get(1)
    const bandOut = dsOut.bands.get(1)
    const rs = bandIn.pixels.createReadStream()
    const ws = bandOut.pixels.createWriteStream()

    rs.pipe(ws)
    return assert.isFulfilled(finished(ws).then(() => {
      dsOut.close()
      const dataOrig = bandIn.pixels.read(0, 0, bandIn.size.x, bandIn.size.y)

      const dsTest = gdal.open(filename)
      const dataTest = dsTest.bands.get(1).pixels.read(0, 0, bandIn.size.x, bandIn.size.y)
      assert.deepEqual(dataOrig, dataTest)
      dsTest.close()
      gdal.vsimem.release(filename)
    }))
  })
  it('should support transforms', () => {
    const dsIn = gdal.open(path.resolve(__dirname, 'data', 'AROME_T2m_10.tiff'))
    const filename = `/vsimem/ds_transform_test.${String(
      Math.random()
    ).substring(2)}.tmp.tiff`
    const dsOut = gdal.open(filename, 'w', 'GTiff', dsIn.rasterSize.x, dsIn.rasterSize.y, 1, gdal.GDT_Float64)
    const bandIn = dsIn.bands.get(1)
    const bandOut = dsOut.bands.get(1)
    const rs = bandIn.pixels.createReadStream()
    const ws = bandOut.pixels.createWriteStream()

    const scaleTransform = new Transform({
      objectMode: true,
      transform(chunk, _, cb) {
        for (let i = 0; i < chunk.length; i++) {
          chunk[i] = Math.min(chunk[i], 6000) / 250
        }
        cb(null, chunk)
      }
    })

    rs.pipe(scaleTransform).pipe(ws)
    return assert.isFulfilled(finished(ws).then(() => {
      dsOut.close()

      const dataOrig = bandIn.pixels.read(0, 0, bandIn.size.x, bandIn.size.y)
      for (let i = 0; i < dataOrig.length; i++) {
        dataOrig[i] = Math.min(dataOrig[i], 6000) / 250
      }

      const dsTest = gdal.open(filename)
      const dataTest = dsTest.bands.get(1).pixels.read(0, 0, bandIn.size.x, bandIn.size.y)
      assert.deepEqual(dataOrig, dataTest)
      dsTest.close()
      gdal.vsimem.release(filename)
    }))
  })
})

describe('gdal.RasterMuxStream', () => {
  function testMux(blockOptimize?: boolean) {
    const dsT2m = gdal.open(path.resolve(__dirname, 'data', 'AROME_T2m_10.tiff'))
    const dsD2m = gdal.open(path.resolve(__dirname, 'data', 'AROME_D2m_10.tiff'))

    const filename = `/vsimem/ds_mux_test.${String(
      Math.random()
    ).substring(2)}.tmp.tiff`
    const dsCloudBase = gdal.open(filename, 'w', 'GTiff', dsT2m.rasterSize.x, dsD2m.rasterSize.y, 1, gdal.GDT_Float64)

    const mux = new gdal.RasterMuxStream({
      T2m: dsT2m.bands.get(1).pixels.createReadStream(),
      D2m: dsD2m.bands.get(1).pixels.createReadStream()
    }, { blockOptimize })

    const ws = dsCloudBase.bands.get(1).pixels.createWriteStream()

    // Espy's estimation for cloud base height (lifted condensation level)
    // LCL = 125 * (T2m - Td2m)
    // where T2m is the temperature at 2m and Td2m is the dew point at 2m
    const fn = (t: number, td: number) => 125 * (t - td)
    const espyEstimation = new gdal.RasterTransform({ type: Float64Array, fn })

    mux.pipe(espyEstimation).pipe(ws)
    return assert.isFulfilled(finished(ws).then(() => {
      dsCloudBase.close()

      const dataOrigT2m = dsT2m.bands.get(1).pixels.read(0, 0, dsT2m.rasterSize.x, dsT2m.rasterSize.y)
      const dataOrigD2m = dsD2m.bands.get(1).pixels.read(0, 0, dsD2m.rasterSize.x, dsD2m.rasterSize.y)
      const dataCloudBase = new Float64Array(dsD2m.rasterSize.x * dsD2m.rasterSize.y)
      for (let i = 0; i < dataOrigT2m.length; i++) {
        dataCloudBase[i] = 125 * (dataOrigT2m[i] - dataOrigD2m[i])
      }

      const dsTest = gdal.open(filename)
      const dataTest = dsTest.bands.get(1).pixels.read(0, 0, dsTest.rasterSize.x, dsTest.rasterSize.y)
      assert.deepEqual(dataCloudBase, dataTest)
      dsTest.close()
      gdal.vsimem.release(filename)
    }))
  }

  it('should accept multiple inputs', () => testMux(undefined))
  it('should support different block sizes', () => testMux(false))
})
