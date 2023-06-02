import * as gdal from 'gdal-async'
import * as path from 'path'
import { assert } from 'chai'
import * as semver from 'semver'

describe('Open', () => {
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  describe('OpenFileGDB', () => {
    let filename, ds: gdal.Dataset

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/testopenfilegdb.gdb.zip')
      ds = gdal.open(filename)
    })
    it('should be able to read layer count', () => {
      assert.equal(ds.layers.count(), 37)
    })

    describe('layer', () => {
      let layer: gdal.Layer
      it('should exist', () => {
        layer = ds.layers.get(0)
        assert.ok(layer)
        assert.instanceOf(layer, gdal.Layer)
      })
      it('should have all fields defined', () => {
        assert.equal(layer.fields.count(), 12)
        assert.deepEqual(layer.fields.getNames(), [
          'id', 'str', 'smallint', 'int', 'float', 'real',
          'adate', 'guid', 'xml', 'binary', 'nullint', 'binary2'
        ])
      })
      describe('features', () => {
        it('should be readable', () => {
          assert.equal(layer.features.count(), 6)
          const feature = layer.features.first()
          const fields = feature.fields.toObject()

          assert.deepInclude(fields,
            {
              id: 1,
              str: 'foo_é',
              smallint: -13,
              int: 123,
              float: 1.5,
              real: 4.56,
              guid: '{12345678-9ABC-DEF0-1234-567890ABCDEF}',
              xml: '<foo></foo>',
              nullint: 0
            })
          assert.deepEqual(fields.adate, { year: 2013, month: 12, day: 26, hour: 12, minute: 34, second: 56 })
          assert.instanceOf(fields.binary, Buffer)
          assert.instanceOf(fields.binary2, Buffer)
        })
      })
    })
  })
})

describe('Create', () => {
  it('OpenFileGDB', function () {
    if (semver.gte(gdal.version, '3.6.0')) {
      const ds1 = gdal.drivers.get('OpenFileGDB').create('/vsimem/openfilegdb_create1_test.gdb', 0, 0, 0)
      assert.instanceOf(ds1, gdal.Dataset)
      const ds2 = gdal.drivers.get('OpenFileGDB').create('/vsimem/openfilegdb_create2_test.gdb')
      assert.instanceOf(ds2, gdal.Dataset)
    } else {
      this.skip()
    }
  })
})
