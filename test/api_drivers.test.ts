import * as chaiAsPromised from 'chai-as-promised'
import * as chai from 'chai'
const assert = chai.assert
import * as gdal from 'gdal-async'

chai.use(chaiAsPromised)

describe('gdal.drivers', () => {
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  describe('count()', () => {
    it('should return the number of drivers', () => {
      assert.isNumber(gdal.drivers.count())
    })
  })

  describe('getNames()', () => {
    it('should return array of strings', () => {
      const result = gdal.drivers.getNames()
      assert.isArray(result)
      result.forEach((driver) => {
        assert.isString(driver)
      })
    })
    it('should return array the same length as driver count', () => {
      const count = gdal.drivers.count()
      assert.lengthOf(gdal.drivers.getNames(), count)
    })
  })

  describe('get()', () => {
    it('should support numeric argument', () => {
      assert.instanceOf(gdal.drivers.get(0), gdal.Driver)
    })
    it('should support string argument', () => {
      assert.instanceOf(gdal.drivers.get('GTiff'), gdal.Driver)
    })
    it('should throw when not found', () => {
      assert.throws(() => {
        gdal.drivers.get('awfag')
      })
    })

    type driverMeta = Record<'DMD_LONGNAME' | 'DMD_MIMETYPE' | 'DMD_EXTENSION' | 'DCAP_CREATE', string | undefined>
    // raster drivers
    const expected = {
      GTiff: {
        DMD_LONGNAME: 'GeoTIFF',
        DMD_MIMETYPE: 'image/tiff',
        DMD_EXTENSION: 'tif',
        DCAP_CREATE: 'YES'
      },
      'VRT:raster': {
        DMD_LONGNAME: 'Virtual Raster',
        DMD_MIMETYPE: undefined,
        DMD_EXTENSION: 'vrt',
        DCAP_CREATE: 'YES'
      },
      MEM: {
        DMD_LONGNAME: 'In Memory Raster',
        DMD_MIMETYPE: undefined,
        DMD_EXTENSION: undefined,
        DCAP_CREATE: 'YES'
      },
      PNG: {
        DMD_LONGNAME: 'Portable Network Graphics',
        DMD_MIMETYPE: 'image/png',
        DMD_EXTENSION: 'png',
        DCAP_CREATE: undefined
      },
      JPEG: {
        DMD_LONGNAME: 'JPEG JFIF',
        DMD_MIMETYPE: 'image/jpeg',
        DMD_EXTENSION: 'jpg',
        DCAP_CREATE: undefined
      }
    } as Record<string, driverMeta>

    Object.keys(expected).forEach((o) => {
      it(`should support "${o}" driver`, () => {
        const driver = gdal.drivers.get(o)
        assert.ok(driver)

        const metadata = driver.getMetadata() as driverMeta
        const expected_meta = expected[o]
        assert.equal(expected_meta.DMD_LONGNAME, metadata.DMD_LONGNAME)
        assert.equal(expected_meta.DMD_MIMETYPE, metadata.DMD_MIMETYPE)
        assert.equal(expected_meta.DMD_EXTENSION, metadata.DMD_EXTENSION)
        assert.equal(expected_meta.DCAP_CREATE, metadata.DCAP_CREATE)
      })
    })

    // vector drivers
    const drivers = [ 'GEOJSON', 'VRT:vector', 'ESRI Shapefile' ]

    drivers.forEach((o) => {
      it(`should support "${o}" (ogr) driver`, () => {
        assert.ok(gdal.drivers.get(o))
      })
    })
  })

  describe('forEach()', () => {
    it('should iterate through all Driver objects', () => {
      const n = gdal.drivers.count()
      let count = 0
      gdal.drivers.forEach((driver, i) => {
        assert.isNumber(i)
        assert.instanceOf(driver, gdal.Driver)
        assert.equal(driver, gdal.drivers.get(i))
        count++
      })
      assert.equal(count, n)
    })
    it('should stop when false is returned from callback', () => {
      let i = 0
      gdal.drivers.forEach(() => {
        if (++i === 1) return false
      })
      assert.equal(i, 1)
    })
    it('should always start from beginning', () => {
      const n = gdal.drivers.count()
      let i = 0,
        j = 0
      gdal.drivers.forEach(() => {
        if (i++ === 1) return false
      })
      gdal.drivers.forEach(() => {
        j++
      })
      assert.equal(j, n)
    })
  })
  describe('@@iterator()', () => {
    it('should iterate through all Driver objects', () => {
      const n = gdal.drivers.count()
      let count = 0
      for (const driver of gdal.drivers) {
        assert.instanceOf(driver, gdal.Driver)
        assert.equal(driver, gdal.drivers.get(count))
        count++
      }
      assert.equal(count, n)
    })
  })

  describe('map()', () => {
    it('should operate normally', () => {
      const result = gdal.drivers.map((driver, i) => {
        assert.isNumber(i)
        assert.instanceOf(driver, gdal.Driver)
        assert.equal(driver, gdal.drivers.get(i))
        return 'a'
      })

      assert.isArray(result)
      assert.equal(result[0], 'a')
      assert.equal(result.length, gdal.drivers.count())
    })
  })

  describe('open()', () => {
    it('should operate normally', () => {
      const driver = gdal.drivers.get('GTiff')
      const ds = driver.open(`${__dirname}/data/sample.tif`)
      assert.instanceOf(ds, gdal.Dataset)
      assert.isNotNull(ds.srs)
      assert.isNotNull(ds.geoTransform)
      assert.strictEqual(ds.driver, driver)
    })

    it('should support passing driver-specific open options', () => {
      const driver = gdal.drivers.get('GTiff')
      const ds1 = driver.open(`${__dirname}/data/sample.tif`, 'r', { GEOREF_SOURCES: 'NONE' })
      assert.instanceOf(ds1, gdal.Dataset)
      assert.isNull(ds1.srs)
      assert.isNull(ds1.geoTransform)
      assert.strictEqual(ds1.driver, driver)

      const ds2 = driver.open(`${__dirname}/data/sample.tif`, 'r', [ 'GEOREF_SOURCES=NONE' ])
      assert.instanceOf(ds2, gdal.Dataset)
      assert.isNull(ds2.srs)
      assert.isNull(ds2.geoTransform)
      assert.strictEqual(ds2.driver, driver)
    })

    it('should throw if the driver-specific options are invalid', () => {
      assert.throws(() => {
        const driver = gdal.drivers.get('GTiff')
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        driver.open(`${__dirname}/data/sample.tif`, 'r', 'GEOREF_SOURCES=NONE' as any)
      }, /Failed parsing options/)
    })
  })

  describe('create()', () => {
    it('should operate normally', () => {
      const driver = gdal.drivers.get('GTiff')
      const outputFilename = '/vsimem/gdal_driver_create.tiff'
      const ds = driver.create(outputFilename, 50, 50, 1, gdal.GDT_Byte)
      assert.equal(ds.driver.description, 'GTiff')
      ds.close()
      assert.equal(gdal.open('/vsimem/gdal_driver_create.tiff').rasterSize.x, 50)
    })
  })

  describe('createCopy()', () => {
    it('should operate normally', () => {
      const driver = gdal.drivers.get('MEM')
      const outputFilename = '' // __dirname + '/data/12_791_1476.tif';
      const ds = driver.createCopy(
        outputFilename,
        gdal.open(`${__dirname}/data/12_791_1476.jpg`)
      )
      assert.equal(ds.driver.description, 'MEM')
    })

    it('should support progress callbacks', () => {
      const driver = gdal.drivers.get('MEM')
      const outputFilename = ''
      let calls = 0
      const ds = driver.createCopy(
        outputFilename,
        gdal.open(`${__dirname}/data/12_791_1476.jpg`),
        {},
        false,
        { progress_cb: (): void => {
          calls++
        } }
      )
      assert.equal(ds.driver.description, 'MEM')
      assert.isAbove(calls, 0)
    })

    it('should throw on error', () => {
      const driver = gdal.drivers.get('GTiff')
      const outputFilename = '' // __dirname + '/data/12_791_1476.tif';
      assert.throws(() => {
        driver.createCopy(
          outputFilename,
          gdal.open(`${__dirname}/data/12_791_1476.jpg`)
        )
      }, /No such file/)
    })

    it('should throw on invalid options', () => {
      const driver = gdal.drivers.get('GTiff')
      const outputFilename = '' // __dirname + '/data/12_791_1476.tif';
      assert.throws(() => {
        driver.createCopy(
          outputFilename,
          gdal.open(`${__dirname}/data/12_791_1476.jpg`),
          // eslint-disable-next-line @typescript-eslint/no-explicit-any
          'option=invalid' as any
        )
      }, /Failed parsing options/)
    })
  })

  describe('createCopyAsync', () => {
    it('should operate normally', () => {
      const driver = gdal.drivers.get('MEM')
      const outputFilename = '' // __dirname + '/data/12_791_1476.tif';
      const p = driver.createCopyAsync(
        outputFilename,
        gdal.open(`${__dirname}/data/12_791_1476.jpg`)
      )
      return assert.eventually.equal(p.then((r) => r.driver.description), 'MEM')
    })
  })
})
