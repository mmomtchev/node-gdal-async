import * as gdal from 'gdal-async'
import * as path from 'path'
import { assert } from 'chai'
import * as fileUtils from './utils/file'
import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
import * as semver from 'semver'
chai.use(chaiAsPromised)

if (Symbol.asyncIterator) {
  describe('gdal.Dataset', () => {
    describe('instance', () => {
      describe('"bands" propertry', () => {
        describe('@@asyncIterator()', () => {
          it('should support iterating over the values', async () => {
            const ds = gdal.open(path.resolve(__dirname, 'data', 'multiband.tif'))
            let count = 0
            for await (const band of ds.bands) {
              assert.instanceOf(band, gdal.RasterBand)
              count++
            }
            assert.equal(count, ds.bands.count())
          })
          it('should throw error if dataset is destroyed', () => {
            const ds = gdal.open(path.resolve(__dirname, 'data', 'multiband.tif'))
            ds.close()
            return assert.isRejected((async () => {
              for await (const band of ds.bands) void band
            })(), /already been destroyed/)
          })
        })
      })
    })
  })
  describe('gdal.Layer', () => {
    describe('instance', () => {
      describe('"features" propertry', () => {
        describe('@@asyncIterator()', () => {
          it('should support iterating over the values', async () => {
            const ds = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
            const layer = ds.layers.get(0)
            let count = 0
            for await (const feature of layer.features) {
              assert.instanceOf(feature, gdal.Feature)
              count++
            }
            assert.equal(count, layer.features.count())
          })
          it('should throw error if dataset is destroyed', () => {
            const ds = gdal.open(path.resolve(__dirname, 'data', 'park.geo.json'))
            const layer = ds.layers.get(0)
            ds.close()
            return assert.isRejected((async () => {
              for (const l of layer.features) void l
            })(), /already destroyed/)
          })
        })
      })
    })
  })
  describe('gdal.RasterBand', () => {
    describe('"overviews" property', () => {
      describe('@@asyncIterator()', () => {
        if (!semver.satisfies(gdal.version, '^3.1.4')) {
          // Work around https://github.com/OSGeo/gdal/issues/3746
          // on Fedora 33
          it('should iterate through the overviews', async () => {
            const tempFile = fileUtils.clone(`${__dirname}/data/sample.tif`)
            const ds = gdal.open(tempFile, 'r+')
            const band = ds.bands.get(1)
            ds.buildOverviews('NEAREST', [ 2, 4 ])
            const w = []
            for await (const overview of band.overviews) {
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
            return assert.isRejected((async () => {
              for await (const overview of band.overviews) void overview
            })(), /already been destroyed/)
          })
        }
      })
    })
  })
  if (semver.gte(gdal.version, '3.1.0')) {
    const tests = {
      Group: { groups: gdal.Group, arrays: gdal.MDArray, dimensions: gdal.Dimension, attributes: gdal.Attribute },
      MDArray: { dimensions: gdal.Dimension, attributes: gdal.Attribute }
    } as Record<string, Record<string, new () => gdal.Attribute | gdal.Dimension | gdal.MDArray | gdal.Group>>
    for (const tested of Object.keys(tests) as ('Group' | 'MDArray')[]) {
      describe(`gdal.${tested}`, () => {
        for (const prop of Object.keys(tests[tested]) as ('groups' | 'arrays' | 'dimensions' | 'attributes')[]) {
          describe(`"${prop}" property`, () => {
            describe('@@asyncIterator()', () => {
              it('should iterate over the values', () => {
                const ds = gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr')
                for (const i of ds.root[prop]) {
                  assert.instanceOf(i, tests[tested][prop])
                }
              })
              it('should reject if the dataset is closed', () => {
                const ds = gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr')
                ds.close()
                return assert.isRejected((async () => {
                  for await (const i of ds.root[prop]) void i
                })(), /already been destroyed/)
              })
            })
          })
        }
      })
    }
  }
}
