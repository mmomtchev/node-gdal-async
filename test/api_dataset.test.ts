import * as gdal from 'gdal-async'
import * as path from 'path'
import * as fs from 'fs'
import * as cp from 'child_process'
import { assert } from 'chai'
import * as fileUtils from './utils/file'
import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as semver from 'semver'
chai.use(chaiAsPromised)

const NAD83_WKT =
  'PROJCS["NAD_1983_UTM_Zone_10N",' +
  'GEOGCS["GCS_North_American_1983",' +
  'DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137,298.257222101]],' +
  'PRIMEM["Greenwich",0],UNIT["Degree",0.0174532925199433]],' +
  'PROJECTION["Transverse_Mercator"],PARAMETER["False_Easting",500000.0],' +
  'PARAMETER["False_Northing",0.0],PARAMETER["Central_Meridian",-123.0],' +
  'PARAMETER["Scale_Factor",0.9996],PARAMETER["Latitude_of_Origin",0.0],' +
  'UNIT["Meter",1.0]]'

describe('gdal.Dataset', () => {
  afterEach(global.gc!)

  let ds: gdal.Dataset
  before(() => {
    ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
  })
  it('should be exposed', () => {
    assert.ok(gdal.Dataset)
  })
  it('should not be instantiable', () => {
    assert.throws(() => {
      new gdal.Dataset()
    }, /Cannot create dataset directly/)
  })

  describe('instance', () => {

    it('properly close open Datasets when exiting the process', () => {
      const tempFile = path.join(__dirname, 'data', 'temp', 'destructor_close.tiff').replace(/\\/g, '/')
      const gdalJS = fs.existsSync('./lib/gdal.js') ? './lib/gdal.js' : 'gdal-async'
      const command =
          `"const gdal = require('${gdalJS}'); gdal.open('${tempFile}', 'w', 'GTiff', 100, 100, 1, gdal.GDT_Float64);"`
      let execPath = process.execPath
      if (process.platform === 'win32') {
        // quotes to avoid errors like ''C:\Program' is not recognized as an internal or external command'
        execPath = `"${execPath}"`
      }
      try {
        cp.execSync(`${execPath} ${[ '-e', command ].join(' ')}`)
      } catch (e) {
        // This test will return an error when built with ASAN
        console.warn(e)
      }
      const ds = gdal.open(tempFile)
      assert.instanceOf(ds.bands.get(1).pixels.read(0, 0, 100, 100), Float64Array)
      ds.close()
      fs.unlinkSync(tempFile)
    })

    describe('"bands" property', () => {
      it('should exist', () => {
        assert.instanceOf(ds.bands, gdal.DatasetBands)
      })
      describe('count()', () => {
        it('should return number', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          assert.equal(ds.bands.count(), 1)
        })
        it('should be 0 for vector datasets', () => {
          const arr = []
          for (let i = 0; i < 10000; i++) arr.push(i)
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          assert.equal(ds.bands.count(), 0)
        })
        it('should throw if dataset is closed', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          ds.close()
          assert.throws(() => {
            ds.bands.count()
          })
        })
      })
      describe('countAsync()', () => {
        it('should return number', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          assert.becomes(ds.bands.countAsync(), 1)
        })
        it('should be 0 for vector datasets', () => {
          const arr = []
          for (let i = 0; i < 10000; i++) arr.push(i)
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          assert.becomes(ds.bands.countAsync(), 0)
        })
        it('should throw if dataset is closed', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          ds.close()
          return assert.isRejected(ds.bands.countAsync())
        })
      })
      describe('get()', () => {
        it('should return RasterBand', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          assert.instanceOf(ds.bands.get(1), gdal.RasterBand)
        })
        it('should return null if band id is out of range', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          assert.throws(() => {
            ds.bands.get(0)
          })
        })
        it('should throw if dataset is closed', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          ds.close()
          assert.throws(() => {
            ds.bands.get(1)
          })
        })
      })
      describe('getAsync()', () => {
        it('should return RasterBand', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const p = ds.bands.getAsync(1)
          return assert.eventually.instanceOf(p, gdal.RasterBand)
        })
        it('should reject if band id is out of range', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          return assert.isRejected(ds.bands.getAsync(0))
        })
        it('should throw if dataset is closed', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          ds.close()
          return assert.isRejected(ds.bands.getAsync(1))
        })
      })
      describe('forEach()', () => {
        it('should call callback for each RasterBand', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const expected_ids = [ 1 ] as (number|null)[]
          const ids = [] as (number|null)[]
          ds.bands.forEach((band, i) => {
            assert.isNumber(i)
            assert.isTrue(i > 0)
            assert.instanceOf(band, gdal.RasterBand)
            ids.push(band.id)
          })
          assert.deepEqual(ids, expected_ids)
        })
        it('should throw if dataset is closed', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          ds.close()
          assert.throws(() => {
            ds.bands.forEach(() => undefined)
          })
        })
      })
      describe('@@iterator()', () => {
        it('should iterate over all values', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const expected_ids = [ 1 ]
          const ids = []
          for (const band of ds.bands) {
            assert.instanceOf(band, gdal.RasterBand)
            ids.push(band.id)
          }
          assert.deepEqual(ids, expected_ids)
        })
        it('should throw if dataset is closed', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          ds.close()
          assert.throws(() => {
            for (const band of ds.bands) void band
          })
        })
      })
      describe('map()', () => {
        it('should operate normally', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const result = ds.bands.map((band, i) => {
            assert.isNumber(i)
            assert.instanceOf(band, gdal.RasterBand)
            return 'a'
          })

          assert.isArray(result)
          assert.equal(result[0], 'a')
          assert.equal(result.length, ds.bands.count())
        })
      })
      describe('create()', () => {
        it('should create a new RasterBand', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.create(gdal.GDT_Byte)
          assert.instanceOf(band, gdal.RasterBand)
          assert.equal(ds.bands.count(), 2)
        })
        it('should throw if the dataset has been closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          ds.close()
          assert.throws(() => {
            ds.bands.create(gdal.GDT_Byte)
          }, /Dataset object has already been destroyed/)
        })
        it('should throw if the arguments are invalid', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          assert.throws(() => {
            // In TypeScript these exceptions require disabling the type checks
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (ds.bands as any).create(42)
          }, /data type must be string/)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (ds.bands as any).create()
          }, /data type argument needed/)
        })
        it('should throw if the options cannot be parsed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            ds.bands.create(gdal.GDT_Byte, 'invalid=true' as any)
          }, /String list must be an array or object/)
        })
      })
      describe('createAsync()', () => {
        it('should create a new RasterBand', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          const band = ds.bands.createAsync(gdal.GDT_Byte)
          return assert.eventually.instanceOf(band, gdal.RasterBand)
        })
        it('should throw if the dataset has been closed', () => {
          const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
          ds.close()
          const band = ds.bands.createAsync(gdal.GDT_Byte)
          return assert.isRejected(band, /Dataset object has already been destroyed/)
        })
      })
      describe('getEnvelope()', () => {
        it('should return the envelope', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const extent = ds.bands.getEnvelope()
          assert.instanceOf(extent, gdal.Envelope)
          assert.closeTo(extent.minX, -1134675.2952829634, 1e-6)
          assert.closeTo(extent.minY, 2479678.7999914493, 1e-6)
          assert.closeTo(extent.maxX, -1127293.2565036996, 1e-6)
          assert.closeTo(extent.maxY, 2485710.4658232867, 1e-6)
        })
      })
    })
    describe('"layers" property', () => {
      it('should exist', () => {
        assert.instanceOf(ds.layers, gdal.DatasetLayers)
      })
      describe('count()', () => {
        it('should return number', () => {
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          assert.equal(ds.layers.count(), 1)
        })
        it('should be 0 for raster datasets', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          assert.equal(ds.layers.count(), 0)
        })
        it('should throw if dataset is closed', () => {
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          ds.close()
          assert.throws(() => {
            ds.layers.count()
          })
        })
      })
      describe('countAsync()', () => {
        it('should return number', () => {
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          return assert.eventually.equal(ds.layers.countAsync(), 1)
        })
        it('should be 0 for raster datasets', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          return assert.eventually.equal(ds.layers.countAsync(), 0)
        })
        it('should throw if dataset is closed', () => {
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          ds.close()
          return assert.isRejected(ds.layers.countAsync())
        })
      })
      describe('get()', () => {
        describe('w/id argument', () => {
          it('should return Layer', () => {
            const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
            assert.instanceOf(ds.layers.get(0), gdal.Layer)
          })
          it('should throw if layer id is out of range', () => {
            const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
            assert.throws(() => {
              ds.layers.get(5)
            })
          })
          it('should throw if dataset is closed', () => {
            const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
            ds.close()
            assert.throws(() => {
              ds.layers.get(0)
            })
          })
        })
        describe('w/name argument', () => {
          it('should return Layer', () => {
            const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
            assert.instanceOf(ds.layers.get('sample'), gdal.Layer)
          })
          it('should throw if layer name doesnt exist', () => {
            const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
            assert.throws(() => {
              ds.layers.get('bogus')
            })
          })
          it('should throw if dataset is closed', () => {
            const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
            ds.close()
            assert.throws(() => {
              ds.layers.get('sample')
            })
          })
        })
      })
      describe('getAsync()', () => {
        describe('w/id argument', () => {
          it('should return Layer', () => {
            const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
            return assert.eventually.instanceOf(ds.layers.getAsync(0), gdal.Layer)
          })
        })
        describe('w/name argument', () => {
          it('should return Layer', () => {
            const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
            return assert.eventually.instanceOf(ds.layers.getAsync('sample'), gdal.Layer)
          })
        })
      })
      describe('forEach()', () => {
        it('should call callback for each Layer', () => {
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          const expected_names = [ 'sample' ]
          const names = [] as string[]
          ds.layers.forEach((layer, i) => {
            assert.isNumber(i)
            assert.instanceOf(layer, gdal.Layer)
            names.push(layer.name)
          })
          assert.deepEqual(names, expected_names)
        })
        it('should throw if dataset is closed', () => {
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          ds.close()
          assert.throws(() => {
            ds.layers.forEach(() => undefined)
          })
        })
      })
      describe('map()', () => {
        it('should operate normally', () => {
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          const result = ds.layers.map((layer, i) => {
            assert.isNumber(i)
            assert.instanceOf(layer, gdal.Layer)
            return 'a'
          })

          assert.isArray(result)
          assert.equal(result[0], 'a')
          assert.equal(result.length, ds.layers.count())
        })
      })
      describe('create()', () => {
        it('should return Layer', () => {
          const file = `/vsimem/ds_layer_test.${String(
            Math.random()
          ).substring(2)}.tmp.shp`
          const ds = gdal.open(file, 'w', 'ESRI Shapefile')
          const srs = gdal.SpatialReference.fromEPSG(4326)
          const lyr = ds.layers.create('layer_name', srs, gdal.wkbPoint)
          assert.instanceOf(lyr, gdal.Layer)
          assert.equal(lyr.geomType, gdal.wkbPoint)
        })
        it('should set spatial reference of created layer', () => {
          const file = `/vsimem/ds_layer_test.${String(
            Math.random()
          ).substring(2)}.tmp.shp`
          const ds = gdal.open(file, 'w', 'ESRI Shapefile')
          const srs = gdal.SpatialReference.fromEPSG(4326)
          const lyr = ds.layers.create('layer_name', srs, gdal.wkbPoint)
          assert.instanceOf(lyr.srs, gdal.SpatialReference)
        })
        it('should accept null for srs', () => {
          const file = `/vsimem/ds_layer_test.${String(
            Math.random()
          ).substring(2)}.tmp.shp`
          const ds = gdal.open(file, 'w', 'ESRI Shapefile')
          const lyr = ds.layers.create('layer_name', null, gdal.wkbPoint)
          assert.instanceOf(lyr, gdal.Layer)
        })
        it('should accept Geometry constructor for geom_type', () => {
          const file = `/vsimem/ds_layer_test.${String(
            Math.random()
          ).substring(2)}.tmp.shp`
          const ds = gdal.open(file, 'w', 'ESRI Shapefile')
          const lyr = ds.layers.create('layer_name', null, gdal.Point)
          assert.instanceOf(lyr, gdal.Layer)
          assert.equal(lyr.geomType, gdal.wkbPoint)
        })
        it('should accept 2.5D Types for geom_type', () => {
          let file = `/vsimem/ds_layer_test.${String(
            Math.random()
          ).substring(2)}.tmp.shp`
          let ds = gdal.open(file, 'w', 'ESRI Shapefile')
          let lyr = ds.layers.create('layer_name', null, gdal.wkbPoint25D)
          assert.instanceOf(lyr, gdal.Layer)
          assert.equal(lyr.geomType, gdal.wkbPoint25D)

          file = `/vsimem/ds_layer_test.${String(
            Math.random()
          ).substring(2)}.tmp.shp`
          ds = gdal.open(file, 'w', 'ESRI Shapefile')
          lyr = ds.layers.create(
            'layer_name',
            null,
            gdal.wkbPoint | gdal.wkb25DBit
          )
          assert.instanceOf(lyr, gdal.Layer)
          assert.equal(lyr.geomType, gdal.wkbPoint25D)
        })
        it('should throw if bad geometry type is given', () => {
          const file = `/vsimem/ds_layer_test.${String(
            Math.random()
          ).substring(2)}.tmp.shp`
          const ds = gdal.open(file, 'w', 'ESRI Shapefile')
          assert.throws(() => {
            ds.layers.create('layer_name', null, console.log)
          })
          assert.throws(() => {
            ds.layers.create('layer_name', null, 16819189)
          })
        })
        it('should error if dataset doesnt support creating layers', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/park.geo.json`)
          ds = gdal.open(tempFile, 'r')
          assert.throws(() => {
            try {
              ds.layers.create('layer_name', null, gdal.wkbPoint)
            } catch (e) {
              ds.close()
              gdal.vsimem.release(tempFile)
              throw e
            }
          })
        })
        it('should accept layer creation options', () => {
          const basename = `/vsimem/ds_layer_test.${String(
            Math.random()
          ).substring(2)}`
          const file = `${basename}.dbf`
          const ds = gdal.open(file, 'w', 'ESRI Shapefile')
          const lyr = ds.layers.create('layer_name', null, null, [ 'SHPT=NULL' ])
          assert.instanceOf(lyr, gdal.Layer)
          ds.close()
          // check if .dbf file was created
          gdal.fs.stat(file)
          // make sure that .shp file wasnt created
          assert.throws(() => {
            gdal.fs.stat(`${basename}.shp`)
          })
          gdal.drivers.get('ESRI Shapefile').deleteDataset(file)
        })
        it('should throw if dataset is closed', () => {
          const file = `/vsimem/ds_layer_test.${String(
            Math.random()
          ).substring(2)}.tmp.shp`
          const ds = gdal.open(file, 'w', 'ESRI Shapefile')
          ds.close()
          assert.throws(() => {
            ds.layers.create('layer_name', null, gdal.wkbPoint)
          })
        })
      })
      describe('createAsync()', () => {
        it('should return Layer', () => {
          const file = `/vsimem/ds_layer_test.${String(
            Math.random()
          ).substring(2)}.tmp.shp`
          const ds = gdal.open(file, 'w', 'ESRI Shapefile')
          const srs = gdal.SpatialReference.fromEPSG(4326)
          const lyr = ds.layers.createAsync('layer_name', srs, gdal.wkbPoint)
          return assert.isFulfilled(Promise.all([ assert.eventually.instanceOf(lyr, gdal.Layer),
            assert.eventually.propertyVal(lyr, 'geomType', gdal.wkbPoint) ]))
        })
      })
    })
    describe('"srs" property', () => {
      describe('getter', () => {
        it('should return SpatialReference', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          assert.ok(ds?.srs?.toWKT()?.indexOf('PROJCS["WGS_1984_Albers"') as number > -1)
        })
        it("should return null when dataset doesn't have projection", () => {
          let ds
          ds = gdal.open(`${__dirname}/data/blank.jpg`)
          assert.isNull(ds.srs)

          ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          assert.isNull(ds.srs)
        })
        it('should throw if dataset is already closed', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          ds.close()
          assert.throws(() => {
            console.log(ds.srs)
          })
        })
      })
      describe('getter/Async', () => {
        it('should return SpatialReference', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          return assert.eventually.isTrue(ds.srsAsync.then((srs) => srs?.toWKT()?.indexOf('PROJCS["WGS_1984_Albers"') as number > -1))
        })
        it("should return null when dataset doesn't have projection", () => {
          const ds = gdal.open(`${__dirname}/data/blank.jpg`)
          return assert.eventually.isNull(ds.srsAsync)
        })
        it('should reject if dataset is already closed', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          ds.close()
          return assert.isRejected(ds.srsAsync)
        })
      })
      describe('setter', () => {
        it('should throw when not an SpatialReference object', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            ds.srs = '`1`inoinawfawfian!@121' as any
          }, /srs must be SpatialReference object/)
        })
        it('should set projection', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/dem_azimuth50_pa.img`)
          const ds = gdal.open(tempFile)
          const expected = [
            'PROJCS["NAD83 / UTM zone 10N",GEOGCS["NAD83",DATUM["North_American_Datum_1983",SPHEROID["GRS 1980",6378137,298.257222101,AUTHORITY["EPSG","7019"]],AUTHORITY["EPSG","6269"]],PRIMEM["Greenwich",0],UNIT["Degree",0.0174532925199433]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",0],PARAMETER["central_meridian",-123],PARAMETER["scale_factor",0.9996],PARAMETER["false_easting",500000],PARAMETER["false_northing",0],UNIT["metre",1,AUTHORITY["EPSG","9001"]],AXIS["Easting",EAST],AXIS["Northing",NORTH]]', // new gdal
            'PROJCS["NAD_1983_UTM_Zone_10N",GEOGCS["GCS_North_American_1983",DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137,298.257222101]],PRIMEM["Greenwich",0],UNIT["Degree",0.0174532925199433]],PROJECTION["Transverse_Mercator"],PARAMETER["False_Easting",500000.0],PARAMETER["False_Northing",0.0],PARAMETER["Central_Meridian",-123.0],PARAMETER["Scale_Factor",0.9996],PARAMETER["Latitude_of_Origin",0.0],UNIT["Meter",1.0]]' // old gdal
          ]
          ds.srs = gdal.SpatialReference.fromWKT(NAD83_WKT)
          assert.include(expected, ds.srs.toWKT())
          ds.close()
          gdal.vsimem.release(tempFile)
        })
        it('should clear projection', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/dem_azimuth50_pa.img`)
          const ds = gdal.open(tempFile)
          assert.isNotNull(ds.srs)
          ds.srs = null
          assert.isNull(ds.srs)
          ds.close()
          gdal.vsimem.release(tempFile)
        })
        it('should throw error if dataset doesnt support setting srs', () => {
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          assert.throws(() => {
            ds.srs = gdal.SpatialReference.fromWKT(NAD83_WKT)
          })
        })
        it('should throw if dataset is already closed', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/dem_azimuth50_pa.img`)
          const ds = gdal.open(tempFile)
          ds.close()

          assert.throws(() => {
            try {
              ds.srs = gdal.SpatialReference.fromWKT(NAD83_WKT)
            } catch (e) {
              ds.close()
              gdal.vsimem.release(tempFile)
              throw e
            }
          })
        })
      })
    })

    describe('"rasterSize" property', () => {
      describe('getter', () => {
        it('should return dataset dimensions', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          assert.deepEqual(ds.rasterSize, {
            x: 495,
            y: 286
          })
        })
        it('should return null if dataset isnt a raster', () => {
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          assert.isNull(ds.rasterSize)
        })
        it('should throw if dataset is already closed', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          ds.close()
          assert.throws(() => {
            console.log(ds.rasterSize)
          }, /already been destroyed/)
        })
      })
      describe('setter', () => {
        it('should throw', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (ds as any).rasterSize = { x: 0, y: 0 }
          }, /rasterSize is a read-only property/)
        })
      })
      describe('getter/Async', () => {
        it('should return dataset dimensions', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          return assert.eventually.deepEqual(ds.rasterSizeAsync, {
            x: 495,
            y: 286
          })
        })
        it('should reject if dataset is already closed', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          ds.close()
          return assert.isRejected(ds.rasterSizeAsync, /already been destroyed/)
        })
      })
    })

    describe('"driver" property', () => {
      describe('getter', () => {
        it('should return Driver instance', () => {
          let ds
          ds = gdal.open(`${__dirname}/data/sample.tif`)
          assert.instanceOf(ds.driver, gdal.Driver)
          assert.equal(ds.driver.description, 'GTiff')

          ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          assert.instanceOf(ds.driver, gdal.Driver)
          assert.equal(ds.driver.description, 'ESRI Shapefile')
        })
        it('should throw if dataset is already closed', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          ds.close()
          assert.throws(() => {
            console.log(ds.driver)
          })
        })
      })
      describe('setter', () => {
        it('should throw', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (ds as any).driver = null
          })
        })
      })
    })
    describe('"description" property', () => {
      it('should return the description field', () => {
        const ds = gdal.open(`${__dirname}/data/sample.tif`)
        assert.strictEqual(ds.description, `${__dirname}/data/sample.tif`)
      })
      it('should throw if dataset is already closed', () => {
        const ds = gdal.open(`${__dirname}/data/sample.tif`)
        ds.close()
        assert.throws(() => {
          console.log(ds.description)
        })
      })
    })
    describe('"geoTransform" property', () => {
      describe('getter', () => {
        it('should return array', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const expected_geotransform = [
            -1134675.2952829634,
            7.502071930146189,
            0,
            2485710.4658232867,
            0,
            -7.502071930145942
          ]

          const actual_geotransform = ds.geoTransform
          assert.isNotNull(ds.geoTransform)
          if (actual_geotransform === null) return // TS type guard
          const delta = 0.00001
          assert.closeTo(
            actual_geotransform[0],
            expected_geotransform[0],
            delta
          )
          assert.closeTo(
            actual_geotransform[1],
            expected_geotransform[1],
            delta
          )
          assert.closeTo(
            actual_geotransform[2],
            expected_geotransform[2],
            delta
          )
          assert.closeTo(
            actual_geotransform[3],
            expected_geotransform[3],
            delta
          )
          assert.closeTo(
            actual_geotransform[4],
            expected_geotransform[4],
            delta
          )
          assert.closeTo(
            actual_geotransform[5],
            expected_geotransform[5],
            delta
          )
        })
        it('should return null if dataset doesnt have geotransform', () => {
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          assert.isNull(ds.geoTransform)
        })
        it('should throw if dataset is already closed', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          ds.close()
          assert.throws(() => {
            console.log(ds.geoTransform)
          })
        })
      })
      describe('getter/Async', () => {
        it('should return array', () => {
          const ds = gdal.open(`${__dirname}/data/sample.tif`)
          const expected_geotransform = [
            -1134675.2952829634,
            7.502071930146189,
            0,
            2485710.4658232867,
            0,
            -7.502071930145942
          ]

          const p = ds.geoTransformAsync
          return assert.isFulfilled(p.then((actual_geotransform) => {
            const delta = 0.00001
            assert.isNotNull(ds.geoTransform)
            if (actual_geotransform === null) return // TS type guard
            assert.closeTo(
              actual_geotransform[0],
              expected_geotransform[0],
              delta
            )
            assert.closeTo(
              actual_geotransform[1],
              expected_geotransform[1],
              delta
            )
            assert.closeTo(
              actual_geotransform[2],
              expected_geotransform[2],
              delta
            )
            assert.closeTo(
              actual_geotransform[3],
              expected_geotransform[3],
              delta
            )
            assert.closeTo(
              actual_geotransform[4],
              expected_geotransform[4],
              delta
            )
            assert.closeTo(
              actual_geotransform[5],
              expected_geotransform[5],
              delta
            )
          }))
        })
        it('should return null if dataset doesnt have geotransform', () => {
          const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
          return assert.eventually.isNull(ds.geoTransformAsync)
        })
        it('should reject if dataset is already closed', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          ds.close()
          return assert.isRejected(ds.geoTransformAsync)
        })
      })
      describe('setter', () => {
        it('should set geotransform', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.vrt`)
          const ds = gdal.open(tempFile)

          const transform = [ 0, 2, 0, 0, 0, 2 ]
          ds.geoTransform = transform
          assert.deepEqual(ds.geoTransform, transform)
          ds.close()
          gdal.vsimem.release(tempFile)
        })
        it('should throw if dataset doesnt support setting geotransform', () => {
          const transform = [ 0, 2, 0, 0, 0, 2 ]

          const tempFile = fileUtils.clone(`${__dirname}/data/park.geo.json`)
          const ds = gdal.open(tempFile)
          assert.throws(() => {
            ds.geoTransform = transform
          }, /not supported for this dataset/)
          ds.close()
          gdal.vsimem.release(tempFile)
        })
        it('should throw if dataset is already closed', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.vrt`)
          const ds = gdal.open(tempFile)
          ds.close()
          gdal.vsimem.release(tempFile)

          const transform = [ 0, 2, 0, 0, 0, 2 ]
          assert.throws(() => {
            ds.geoTransform = transform
          })
        })
        it('should throw if geotransform is invalid', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.vrt`)
          const ds = gdal.open(tempFile)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            ds.geoTransform = [ 0, 1, 'bad_value' as any, 0, 0, 1 ]
          })
          assert.throws(() => {
            ds.geoTransform = [ 0, 1 ]
          })
          ds.close()
          gdal.vsimem.release(tempFile)
        })
        it('should throw if geotransform is not an array', () => {
          const ds = gdal.open(`${__dirname}/data/dem_azimuth50_pa.img`)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            ds.geoTransform = '42' as any
          })
        })
      })
    })
    describe('executeSQL()', () => {
      it('should return Layer', () => {
        const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
        const result_set = ds.executeSQL('SELECT name FROM sample')

        assert.instanceOf(result_set, gdal.Layer)
        assert.deepEqual(result_set.fields.getNames(), [ 'name' ])
      })
      it('should destroy result set when dataset is closed', () => {
        const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
        const result_set = ds.executeSQL('SELECT name FROM sample')
        ds.close()
        assert.throws(() => {
          result_set.fields.getNames()
        })
      })
      it('should throw if dataset already closed', () => {
        const ds = gdal.open(`${__dirname}/data/sample.vrt`)
        ds.close()
        assert.throws(() => {
          ds.executeSQL('SELECT name FROM sample')
        })
      })
    })
    describe('executeSQLAsync()', () => {
      it('should return Layer', () => {
        const ds = gdal.open(`${__dirname}/data/shp/sample.shp`)
        const result_set = ds.executeSQLAsync('SELECT name FROM sample')

        return assert.isFulfilled(Promise.all([
          assert.eventually.instanceOf(result_set, gdal.Layer),
          assert.eventually.deepEqual(result_set.then((r) => r.fields.getNames()), [ 'name' ]) ]))
      })
      it('should reject if dataset already closed', () => {
        const ds = gdal.open(`${__dirname}/data/sample.vrt`)
        ds.close()
        return assert.isRejected(ds.executeSQLAsync('SELECT name FROM sample'))
      })
    })
    describe('getFileList()', () => {
      it('should return list of filenames', () => {
        const ds = gdal.open(path.join(__dirname, 'data', 'sample.vrt'))
        const expected_filenames = [
          ds.description,
          path.join(__dirname, 'data', 'sample.tif')
        ]
        assert.deepEqual(ds.getFileList(), expected_filenames)
      })
      it('should throw if dataset already closed', () => {
        const ds = gdal.open(`${__dirname}/data/sample.vrt`)
        ds.close()
        assert.throws(() => {
          ds.getFileList()
        })
      })
    })
    describe('flush()', () => {
      it('should return without error', () => {
        const ds = gdal.open(path.join(__dirname, 'data', 'sample.vrt'))
        ds.flush()
      })
      it('should throw if dataset already closed', () => {
        const ds = gdal.open(path.join(__dirname, 'data', 'sample.vrt'))
        ds.close()
        assert.throws(() => {
          ds.flush()
        })
      })
    })
    describe('flushAsync()', () => {
      it('should fulfill without error', () => {
        const ds = gdal.open(path.join(__dirname, 'data', 'sample.vrt'))
        return assert.isFulfilled(ds.flushAsync())
      })
      it('should throw if dataset already closed', () => {
        const ds = gdal.open(path.join(__dirname, 'data', 'sample.vrt'))
        ds.close()
        return assert.isRejected(ds.flushAsync())
      })
    })
    describe('getMetadata()', () => {
      it('should return object', () => {
        const ds = gdal.open(`${__dirname}/data/sample.tif`)
        const metadata = ds.getMetadata()
        assert.isObject(metadata)
        assert.equal(metadata.AREA_OR_POINT, 'Area')
      })
      it('should throw if dataset already closed', () => {
        const ds = gdal.open(`${__dirname}/data/sample.tif`)
        ds.close()
        assert.throws(() => {
          ds.getMetadata()
        })
      })
    })
    describe('getMetadataAsync()', () => {
      it('should return object', () => {
        const ds = gdal.open(`${__dirname}/data/sample.tif`)
        const metadata = ds.getMetadataAsync()
        return Promise.all([
          assert.eventually.isObject(metadata),
          assert.eventually.propertyVal(metadata, 'AREA_OR_POINT', 'Area')
        ])
      })
      it('should reject if dataset already closed', () => {
        const ds = gdal.open(`${__dirname}/data/sample.tif`)
        ds.close()
        return assert.isRejected(ds.getMetadataAsync())
      })
    })
    describe('setMetadata()', () => {
      it('should set the metadata', () => {
        const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
        assert.isTrue(ds.setMetadata({ name: 'temporary' }))
        let metadata = ds.getMetadata()
        assert.isObject(metadata)
        assert.equal(metadata.name, 'temporary')

        assert.isTrue(ds.setMetadata([ 'name=temporary' ]))
        metadata = ds.getMetadata()
        assert.isObject(metadata)
        assert.equal(metadata.name, 'temporary')
      })
      it('should throw if dataset already closed', () => {
        const ds = gdal.open(`${__dirname}/data/sample.tif`)
        ds.close()
        assert.throws(() => {
          ds.setMetadata({})
        }, /destroyed/)
      })
      it('should throw on invalid arguments', () => {
        const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
        assert.throws(() => {
          ds.setMetadata(42 as unknown as string[])
        }, /Failed parsing/)
      })
    })
    describe('setMetadataAsync()', () => {
      it('should set the metadata', () => {
        const ds = gdal.open('temp', 'w', 'MEM', 256, 256, 1, gdal.GDT_Byte)
        return assert.isFulfilled(ds.setMetadataAsync({ name: 'temporary' }).then(() => {
          const metadata = ds.getMetadata()
          assert.isObject(metadata)
          assert.equal(metadata.name, 'temporary')
        }))
      })
      it('should reject if dataset already closed', () => {
        const ds = gdal.open(`${__dirname}/data/sample.tif`)
        ds.close()
        return assert.isRejected(ds.setMetadataAsync({}))
      })
    })
    describe('buildOverviews()', () => {
      it('should generate overviews for all bands', () => {
        const tempFile = fileUtils.clone(`${__dirname}/data/multiband.tif`)
        const ds = gdal.open(tempFile, 'r+')
        const expected_w = [
          ds.rasterSize.x / 2,
          ds.rasterSize.x / 4,
          ds.rasterSize.x / 8
        ]
        ds.buildOverviews('NEAREST', [ 2, 4, 8 ])
        ds.bands.forEach((band) => {
          const w = [] as number[]
          assert.equal(band.overviews.count(), 3)
          band.overviews.forEach((overview) => {
            w.push(overview.size.x)
          })
          assert.sameMembers(w, expected_w)
        })
        ds.close()
        gdal.vsimem.release(tempFile)
      })
      it('should not fail hard if invalid overview is given', () => {
        // 1.11 introduced an error for this, but 1.10 and lower
        // fail silently - so really all we can do is make sure
        // nothing fatal (segfault, etc) happens
        const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
        const ds = gdal.open(tempFile, 'r+')
        try {
          ds.buildOverviews('NEAREST', [ 2, 4, -3 ])
        } catch (_e) {
          /* ignore (see above) */
        }
        ds.close()
        gdal.vsimem.release(tempFile)
      })
      it('should throw if overview is not a number', () => {
        const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
        const ds = gdal.open(tempFile, 'r+')
        assert.throws(() => {
          /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
          ds.buildOverviews('NEAREST', [ 2, 4, {} as any ])
        })
        ds.close()
        gdal.vsimem.release(tempFile)
      })
      describe('w/bands argument', () => {
        before(() => gdal.config.set('USE_RRD', 'YES'))
        it('should generate overviews only for the given bands', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/multiband.tif`)
          const ds = gdal.open(tempFile, 'r+')
          ds.buildOverviews('NEAREST', [ 2, 4, 8 ], [ 1 ])
          assert.equal(ds.bands.get(1).overviews.count(), 3)
          ds.close()
          gdal.vsimem.release(tempFile)
        })
        it('should throw if invalid band given', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
          const ds = gdal.open(tempFile, 'r+')
          assert.throws(() => {
            ds.buildOverviews('NEAREST', [ 2, 4, 8 ], [ 4 ])
          })
          ds.close()
          gdal.vsimem.release(tempFile)
        })
        it('should throw if band id is not a number', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
          const ds = gdal.open(tempFile, 'r+')
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            ds.buildOverviews('NEAREST', [ 2, 4, 8 ], [ {} as any ])
          })
          ds.close()
          gdal.vsimem.release(tempFile)
        })
      })
      it('should throw if dataset already closed', () => {
        const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
        const ds = gdal.open(tempFile, 'r+')
        ds.close()
        gdal.vsimem.release(tempFile)
        assert.throws(() => {
          ds.buildOverviews('NEAREST', [ 2, 4, 8 ])
        })
      })
      describe('w/progress_cb option', () => {
        before(() => gdal.config.set('USE_RRD', 'YES'))
        it('should invoke the progress callback', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/multiband.tif`)
          const ds = gdal.open(tempFile, 'r+')
          let calls = 0
          ds.buildOverviews('NEAREST', [ 2, 4, 8 ], [ 1 ], { progress_cb: (complete) => {
            calls++
            if (semver.gte(gdal.version, '3.2.0')) {
              // GDAL < 3.0.0 will sometimes report weird values
              assert.isAtLeast(complete, 0)
              assert.isAtMost(complete, 1)
            }
          } })
          assert.isAbove(calls, 0)
          assert.equal(ds.bands.get(1).overviews.count(), 3)
          ds.close()
          gdal.vsimem.release(tempFile)
        })
      })
    })
    describe('buildOverviewsAsync()', () => {
      it('should generate overviews for all bands', () => {
        const tempFile = fileUtils.clone(`${__dirname}/data/multiband.tif`)
        const ds = gdal.open(tempFile, 'r+')
        const expected_w = [
          ds.rasterSize.x / 2,
          ds.rasterSize.x / 4,
          ds.rasterSize.x / 8
        ]
        return assert.isFulfilled(ds.buildOverviewsAsync('NEAREST', [ 2, 4, 8 ]).then(() => {
          ds.bands.forEach((band) => {
            const w = [] as number[]
            assert.equal(band.overviews.count(), 3)
            band.overviews.forEach((overview) => {
              w.push(overview.size.x)
            })
            assert.sameMembers(w, expected_w)
          })
          ds.close()
          gdal.vsimem.release(tempFile)
        }))
      })
      it('should throw if overview is not a number', () => {
        const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
        const ds = gdal.open(tempFile, 'r+')
        /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
        return assert.isRejected(ds.buildOverviewsAsync('NEAREST', [ 2, 4, {} as any ])).then(() => {
          ds.close()
          gdal.vsimem.release(tempFile)
        })
      })
      describe('w/bands argument', () => {
        it('should generate overviews only for the given bands', () => {
          gdal.config.set('USE_RRD', 'YES')
          const tempFile = fileUtils.clone(`${__dirname}/data/multiband.tif`)
          const ds = gdal.open(tempFile, 'r+')
          ds.buildOverviews('NEAREST', [ 2, 4, 8 ], [ 1 ])
          assert.equal(ds.bands.get(1).overviews.count(), 3)
          ds.close()
          gdal.vsimem.release(tempFile)
        })
        it('should throw if invalid band given', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
          const ds = gdal.open(tempFile, 'r+')
          return assert.isRejected(ds.buildOverviewsAsync('NEAREST', [ 2, 4, 8 ], [ 4 ])).then(() => {
            ds.close()
            gdal.vsimem.release(tempFile)
          })
        })
        it('should throw if band id is not a number', () => {
          const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
          const ds = gdal.open(tempFile, 'r+')
          /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
          return assert.isRejected(ds.buildOverviewsAsync('NEAREST', [ 2, 4, 8 ], [ {} as any ])).then(() => {
            ds.close()
            gdal.vsimem.release(tempFile)
          })
        })
      })
      it('should throw if dataset already closed', () => {
        const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
        const ds = gdal.open(tempFile, 'r+')
        ds.close()
        gdal.vsimem.release(tempFile)
        return assert.isRejected(ds.buildOverviewsAsync('NEAREST', [ 2, 4, 8 ]))
      })
    })
  })
  describe('setGCPs()', () => {
    it('should update gcps', () => {
      const driver = gdal.drivers.get('MEM')
      const outputFilename = '' // __dirname + '/data/12_791_1476.tif';
      const ds = driver.createCopy(
        outputFilename,
        gdal.open(`${__dirname}/data/12_791_1476.jpg`)
      )
      const srs = gdal.SpatialReference.fromEPSG(4326)
      ds.srs = srs
      const bounds = {
        minX: -110.478515625,
        maxX: -110.390625,
        minY: 44.77793589631623,
        maxY: 44.84029065139799
      }
      const expectedGCPs = [
        {
          dfGCPPixel: 0,
          dfGCPLine: 0,
          dfGCPX: bounds.minX,
          dfGCPY: bounds.maxY,
          dfGCPZ: 0
        },
        {
          dfGCPPixel: 255,
          dfGCPLine: 0,
          dfGCPX: bounds.maxX,
          dfGCPY: bounds.maxY,
          dfGCPZ: 0
        },
        {
          dfGCPPixel: 255,
          dfGCPLine: 255,
          dfGCPX: bounds.maxX,
          dfGCPY: bounds.minY,
          dfGCPZ: 0
        },
        {
          dfGCPPixel: 0,
          dfGCPLine: 255,
          dfGCPX: bounds.minX,
          dfGCPY: bounds.minY,
          dfGCPZ: 0
        }
      ]

      ds.setGCPs(expectedGCPs, srs.toWKT())
      const actualGCPs = ds.getGCPs()

      expectedGCPs.forEach((expectedGCP, i) => {
        const actualGCP = actualGCPs[i]
        const delta = 0.00001
        assert.closeTo(actualGCP.dfGCPLine, expectedGCP.dfGCPLine, delta)
        assert.closeTo(actualGCP.dfGCPPixel, expectedGCP.dfGCPPixel, delta)
        assert.closeTo(actualGCP.dfGCPX, expectedGCP.dfGCPX, delta)
        assert.closeTo(actualGCP.dfGCPY, expectedGCP.dfGCPY, delta)
        assert.closeTo(actualGCP.dfGCPZ, expectedGCP.dfGCPZ, delta)
      })
      assert.strictEqual(ds.getGCPProjection(), srs.toWKT())

      ds.close()
    })
  })
  describe('testCapability()', () => {
    it("should return false when layer doesn't support capability", () => {
      const ds = gdal.open(`${__dirname}/data/sample.tif`)
      assert.isFalse(ds.testCapability(gdal.ODrCCreateDataSource))
      assert.isFalse(ds.testCapability(gdal.ODrCDeleteDataSource))
      assert.isFalse(ds.testCapability(gdal.ODsCCreateGeomFieldAfterCreateLayer))
      assert.isFalse(ds.testCapability(gdal.ODsCCreateLayer))
      assert.isFalse(ds.testCapability(gdal.ODsCDeleteLayer))
    })
    it('should return true when layer does support capability', () => {
      const file = `/vsimem/ds_layer_test.${String(
        Math.random()
      ).substring(2)}.tmp.shp`
      const ds = gdal.open(file, 'w', 'ESRI Shapefile')
      assert.isFalse(ds.testCapability(gdal.ODrCCreateDataSource))
      assert.isFalse(ds.testCapability(gdal.ODrCDeleteDataSource))
      assert.isFalse(ds.testCapability(gdal.ODsCCreateGeomFieldAfterCreateLayer))
      assert.isTrue(ds.testCapability(gdal.ODsCCreateLayer))
      assert.isTrue(ds.testCapability(gdal.ODsCDeleteLayer))
    })
    it('should throw error if dataset is destroyed', () => {
      const ds = gdal.open(`${__dirname}/data/sample.tif`)
      ds.close()
      assert.throws(() => {
        ds.testCapability(gdal.ODrCCreateDataSource)
      }, /already been destroyed/)
    })
  })
})
