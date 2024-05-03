import * as chaiAsPromised from 'chai-as-promised'
import * as chai from 'chai'
const assert: Chai.Assert = chai.assert
import * as path from 'path'
import * as gdal from 'gdal-async'
import * as semver from 'semver'

chai.use(chaiAsPromised)

describe('gdal', () => {
  if (!semver.gte(gdal.version, '3.1.0')) {
    return
  }

  let ds: gdal.Dataset
  beforeEach(() => {
    ds = gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr')
  })

  afterEach(() => {
    ds.close()
    // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
    global.gc!()
  })

  describe('gdal.Group', () => {
    it('should exist as "root" on datasets opened in MD mode', () => {
      assert.instanceOf(ds.root, gdal.Group)
    })

    it('should replace the "bands" property', () => {
      assert.isNull(ds.bands)
    })

    it('should have "description" property', () => {
      assert.equal(ds.root.description, '/')
    })

    describe('"groups" property', () => {
      it('should be an instance of gdal.GroupGroups', () => {
        assert.instanceOf(ds.root.groups, gdal.GroupGroups)
      })

      it('should have "names" property', () => {
        assert.deepEqual(ds.root.groups.names, [ ])
      })

      it('should have "count()" method', () => {
        assert.equal(ds.root.groups.count(), 0)
      })

      it('@@iterator()', () => {
        for (const i of ds.root.groups) {
          assert.instanceOf(i, gdal.Group)
        }
      })
    })

    describe('"arrays" property"', () => {
      it('should be an instance of gdal.GroupArrays', () => {
        assert.instanceOf(ds.root.arrays, gdal.GroupArrays)
      })

      it('should have "names" property', () => {
        assert.deepEqual(ds.root.arrays.names, [ 'time', 'lat', 'lon', 'alnsf' ])
      })

      it('should have "count()" method', () => {
        assert.equal(ds.root.arrays.count(), 4)
      })

      // These two must wait before executing the after hook which destroys the ds
      // (otherwise they become an excellent dangling pointer handling test)
      it('should have "countAsync()" method', () =>
        assert.eventually.equal(ds.root.arrays.countAsync(), 4)
      )
      it('should have "getAsync()" method', () =>
        assert.eventually.instanceOf(ds.root.arrays.getAsync('time'), gdal.MDArray)
      )

      it('@@iterator()', () => {
        let called = 0
        for (const i of ds.root.arrays) {
          called++
          assert.instanceOf(i, gdal.MDArray)
        }
        assert.isAbove(called, 0)
      })
    })

    describe('"dimensions" property', () => {
      it('should be an instance of gdal.GroupDimensions', () => {
        assert.instanceOf(ds.root.dimensions, gdal.GroupDimensions)
      })

      it('should have "names" property', () => {
        assert.deepEqual(ds.root.dimensions.names, [ 'lon', 'lat', 'time' ])
      })

      it('should have "count()" method', () => {
        assert.equal(ds.root.dimensions.count(), 3)
      })

      it('@@iterator()', () => {
        let called = 0
        for (const i of ds.root.dimensions) {
          called++
          assert.instanceOf(i, gdal.Dimension)
        }
        assert.isAbove(called, 0)
      })
    })

    describe('"attributes" property"', () => {
      it('should be an instance of gdal.GroupAttributes', () => {
        assert.instanceOf(ds.root.attributes, gdal.GroupAttributes)
      })

      it('should have "names" property', () => {
        assert.deepEqual(ds.root.attributes.names, [ 'grid', 'im', 'jm', 'nsoil', 'source', 'Conventions', 'GDAL', 'history' ])
      })

      it('should have "count()" method', () => {
        assert.equal(ds.root.attributes.count(), 8)
      })

      it('@@iterator()', () => {
        let called = 0
        for (const i of ds.root.attributes) {
          called++
          assert.instanceOf(i, gdal.Attribute)
        }
        assert.isAbove(called, 0)
      })
    })

  })

  describe('gdal.Dimension', () => {
    it('should be an instance of gdal.Dimensions', () => {
      assert.instanceOf(ds.root.dimensions.get('time'), gdal.Dimension)
    })

    it('should have "description" property', () => {
      assert.equal(ds.root.dimensions.get('time').description, '/time')
    })

    it('should have "type" property', () => {
      assert.equal(ds.root.dimensions.get('time').type, gdal.DIM_TEMPORAL)
      assert.equal(ds.root.dimensions.get('lat').type, gdal.DIM_HORIZONTAL_Y)
    })

    it('should have "size" property', () => {
      assert.equal(ds.root.dimensions.get('lat').size, 45)
    })

    it('should have "direction" property', () => {
      assert.equal(ds.root.dimensions.get('lat').direction, gdal.DIR_NORTH)
    })
  })

  describe('gdal.Attribute', () => {
    it('should be an instance of gdal.Attribute', () => {
      assert.instanceOf(ds.root.attributes.get('grid'), gdal.Attribute)
    })

    it('should have "dataType" property', () => {
      assert.equal(ds.root.attributes.get('grid').dataType, gdal.GEDTC_String)
      assert.equal(ds.root.attributes.get('im').dataType, gdal.GDT_Int32)
    })

    it('should have "value" property', () => {
      assert.equal(ds.root.attributes.get('grid').value, 'gaussian')
      assert.equal(ds.root.attributes.get('im').value, 3072)
    })
  })

  describe('gdal.MDArray', () => {
    let ds2: gdal.Dataset
    let mdarray: gdal.MDArray, mdarray2: gdal.MDArray
    beforeEach(() => {
      ds = gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr')
      mdarray = ds.root.arrays.get('alnsf')
      ds2 = gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.pgrb2b.4p.f000.grb2'), 'mr')
      mdarray2 = ds2.root.arrays.get('HGT_100-ISBL')
    })

    it('should be an instance of gdal.MDArray', () => {
      assert.instanceOf(mdarray, gdal.MDArray)
      assert.instanceOf(mdarray2, gdal.MDArray)
    })

    describe('"dimensions" property', () => {
      it('should be an instance of gdal.ArrayDimensions', () => {
        assert.instanceOf(mdarray.dimensions, gdal.ArrayDimensions)
        assert.instanceOf(mdarray2.dimensions, gdal.ArrayDimensions)
      })

      it('should have "names" property', () => {
        assert.deepEqual(mdarray.dimensions.names, [ 'time', 'lat', 'lon' ])
        assert.deepEqual(mdarray2.dimensions.names, [ 'Y', 'X' ])
      })

      it('should return dimensions', () => {
        assert.instanceOf(mdarray.dimensions.get('time'), gdal.Dimension)
        assert.equal(mdarray.dimensions.get('time').type, gdal.DIM_TEMPORAL)
        assert.instanceOf(mdarray2.dimensions.get('Y'), gdal.Dimension)
        assert.equal(mdarray2.dimensions.get('Y').type, gdal.DIM_HORIZONTAL_Y)
      })

      it('should have "count()" method', () => {
        assert.equal(mdarray.dimensions.count(), 3)
        assert.equal(mdarray2.dimensions.count(), 2)
      })

      it('@@iterator()', () => {
        let called = 0
        for (const i of mdarray.dimensions) {
          called++
          assert.instanceOf(i, gdal.Dimension)
        }
        assert.isAbove(called, 0)
      })
    })

    describe('"attributes" property', () => {
      it('should be an instance of gdal.ArrayAttributes', () => {
        assert.instanceOf(mdarray.attributes, gdal.ArrayAttributes)
        assert.instanceOf(mdarray2.attributes, gdal.ArrayAttributes)
      })

      it('should return attributes', () => {
        assert.instanceOf(mdarray.attributes.get('long_name'), gdal.Attribute)
        assert.equal(mdarray.attributes.get('long_name').value, 'mean nir albedo with strong cosz dependency')
        assert.instanceOf(mdarray2.attributes.get('long_name'), gdal.Attribute)
        assert.equal(mdarray2.attributes.get('long_name').value, 'Geopotential height [gpm]')
      })

      it('should have "count()" method', () => {
        assert.equal(mdarray.attributes.count(), 4)
        assert.equal(mdarray2.attributes.count(), 20)
      })

      it('@@iterator()', () => {
        let called = 0
        for (const i of mdarray.attributes) {
          called++
          assert.instanceOf(i, gdal.Attribute)
        }
        assert.isAbove(called, 0)
      })
    })

    describe('"length" property', () => {
      it('should return the total flattened length of the MDArray', () => {
        assert.equal(mdarray.length, 4050)
        assert.equal(mdarray2.length, 4050)
      })
    })

    it('should have "description" property', () => {
      assert.typeOf(mdarray.description, 'string')
      assert.equal(mdarray2.description, '/HGT_100-ISBL')
    })

    it('should have "srs" property', () => {
      assert.isNull(mdarray.srs)
      assert.instanceOf(mdarray2.srs, gdal.SpatialReference)
      assert.equal(mdarray2.srs.getAttrValue('SPHEROID', 1), '6371229')
    })

    it('should have "dataType" property', () => {
      assert.equal(mdarray.dataType, gdal.GDT_Float32)
      assert.equal(mdarray2.dataType, gdal.GDT_Float64)
    })

    it('should have "scale" property', () => {
      assert.equal(mdarray.scale, 1)
    })

    it('should have "offset" property', () => {
      assert.equal(mdarray.offset, 0)
    })

    it('should have "noDataValue" property', () => {
      assert.equal(mdarray.noDataValue, 9.969209968386869e+36)
      assert.isNull(mdarray2.noDataValue)
    })

    it('should have "unitType" property', () => {
      assert.equal(mdarray.unitType, '%')
      assert.equal(mdarray2.unitType, 'gpm')
    })

    describe('getView', () => {
      let view: gdal.MDArray

      before(() => {
        const ds = gdal.open(path.resolve(__dirname, 'data', 'gfs.t00z.alnsf.nc'), 'mr')
        const mdarray = ds.root.arrays.get('alnsf')
        view = mdarray.getView('[0]')
      })

      it('should return an instance of MDArray', () => {
        assert.instanceOf(view, gdal.MDArray)
      })

      it('should have the correct number of dimensions and elements', () => {
        assert.equal(view.dimensions.names.length, 2)
        assert.equal(view.length, 4050)
      })

      if (semver.gte(gdal.version, '3.3.0')) {
        it('should keep attributes of the parent', () => {
          assert.equal(view.attributes.names.length, 4)
        })
      }
    })

    describe('getMask', () => {
      it('should return an instance of MDArray with the same size as the parent MDArray', () => {
        const mask = mdarray.getMask()
        assert.instanceOf(mask, gdal.MDArray)
        assert.equal(mask.length, mdarray.length)
        assert.equal(mask.dataType, gdal.GDT_Byte)
        assert.deepEqual(mask.dimensions.names, mdarray.dimensions.names)
      })
    })

    describe('asDataset', () => {
      it('should return an instance of gdal.Dataset with the same size as the parent MDArray', () => {
        const ds = mdarray.asDataset('lon', 'lat')
        assert.instanceOf(ds, gdal.Dataset)
        assert.equal(ds.bands.count(), 1)
        const band = ds.bands.get(1)
        assert.equal(band.size.x, mdarray.dimensions.get('lon').size)
        assert.equal(band.size.y, mdarray.dimensions.get('lat').size)
        assert.equal(band.dataType, gdal.GDT_Float32)
        ds.close()
      })

      it('should read the same data as the parent MDArray', () => {
        const ds = mdarray.asDataset(2, 1)
        const data1 = mdarray.read({
          origin: [ 0, 7, 10 ],
          span: [ 1, 5, 5 ],
          data_type: gdal.GDT_Float32
        })
        const data2 = ds.bands.get(1).pixels.read(10, 7, 5, 5)
        assert.deepEqual(data1, data2)
        ds.close()
      })
    })

    describe('read', () => {
      it('should deduce the type when it is not given', () => {
        const data = mdarray.read({
          origin: [ 0, 0, 0 ],
          span: [ 1, 5, 4 ]
        })
        assert.instanceOf(data, Float32Array)
        assert.equal(data.length, 20)
      })

      it('should transform the data to the given type', () => {
        const data = mdarray.read({
          origin: [ 0, 0, 0 ],
          span: [ 1, 5, 5 ],
          data_type: gdal.GDT_Byte
        })
        assert.instanceOf(data, Uint8Array)
        assert.equal(data.length, 25)
      })

      it('should use a preallocated array', () => {
        const pre = new Uint32Array(25)
        const data = mdarray.read({
          origin: [ 0, 0, 0 ],
          span: [ 1, 5, 5 ],
          data: pre
        })
        assert.equal(data, pre)
        assert.equal(data.length, 25)
      })

      it('should support different strides when reading', () => {
        const data = mdarray.read({
          origin: [ 0, 0, 0 ],
          span: [ 1, 3, 5 ],
          stride: [ 1, 2, 12 ]
        })
        assert.equal(data.length, 4*12 + 2*2 + 1)
        for (let y = 0; y < 3; y++) {
          for (let x = 0; x < 12; x++) {
            assert.notEqual(data[y * 2 + x * 12], 0)
            if (y * 2 + x * 12 + 1 < data.length) {
              assert.equal(data[y * 2 + x * 12 + 1], 0)
            }
          }
        }
      })

      it('should support negative strides when reading', () => {
        const data = mdarray.read({
          origin: [ 0, 0, 0 ],
          span: [ 1, 3, 5 ],
          stride: [ -1, -1, -3 ],
          _offset: 14
        })
        assert.equal(data.length, 15)
        assert.closeTo(data[0], 0.10000000149011612, 1e-8)
      })

      it('should throw on out of bounds condition in the target array', () => {
        const pre = new Uint32Array(20)
        assert.throws(() => {
          mdarray.read({
            origin: [ 0, 0, 0 ],
            span: [ 1, 5, 5 ],
            data: pre
          })
        }, /Failed to allocate/)
      })

      it('should throw on out of bounds condition in the target array w/negative stride', () => {
        assert.throws(() => {
          mdarray.read({
            origin: [ 0, 0, 0 ],
            span: [ 1, 3, 5 ],
            stride: [ -1, -1, -3 ],
            _offset: 13
          })
        }, /before the start/)
      })

      it('should throw on out of bounds condition in the source MDArray', () => {
        assert.throws(() => {
          mdarray.read({
            origin: [ 0, 0, 0 ],
            span: [ 2, 5, 5 ]
          })
        }, /arrayStartIdx/)
      })

    })

    describe('readAsync', () => {
      it('should deduce the type when it is not given', () => {
        const data = mdarray.readAsync({
          origin: [ 0, 0, 0 ],
          span: [ 1, 5, 4 ]
        })
        return assert.isFulfilled(Promise.all([
          assert.eventually.instanceOf(data, Float32Array),
          assert.eventually.propertyVal(data, 'length', 20)
        ]))
      })

      it('should transform the data to the given type', () => {
        const data = mdarray.readAsync({
          origin: [ 0, 0, 0 ],
          span: [ 1, 5, 5 ],
          data_type: gdal.GDT_Byte
        })
        return assert.isFulfilled(Promise.all([
          assert.eventually.instanceOf(data, Uint8Array),
          assert.eventually.propertyVal(data, 'length', 25)
        ]))
      })

      it('should use a preallocated array', () => {
        const pre = new Uint32Array(25)
        const data = mdarray.readAsync({
          origin: [ 0, 0, 0 ],
          span: [ 1, 5, 5 ],
          data: pre
        })
        return assert.isFulfilled(Promise.all([
          assert.eventually.equal(data, pre),
          assert.eventually.propertyVal(data, 'length', 25)
        ]))
      })

      it('should support different strides when reading', () => {
        const q = mdarray.readAsync({
          origin: [ 0, 0, 0 ],
          span: [ 1, 3, 5 ],
          stride: [ 1, 2, 12 ]
        })
        return assert.isFulfilled(q.then((data) => {
          assert.equal(data.length, 4*12 + 2*2 + 1)
          for (let y = 0; y < 3; y++) {
            for (let x = 0; x < 12; x++) {
              assert.notEqual(data[y * 2 + x * 12], 0)
              if (y * 2 + x * 12 + 1 < data.length) {
                assert.equal(data[y * 2 + x * 12 + 1], 0)
              }
            }
          }
        }))
      })

      it('should throw on out of bounds condition in the target array', () => {
        const pre = new Uint32Array(20)
        assert.isRejected(mdarray.readAsync({
          origin: [ 0, 0, 0 ],
          span: [ 1, 5, 5 ],
          data: pre
        }), /Failed to allocate/)
      })

      it('should throw on out of bounds condition in the source MDArray', () => {
        assert.isRejected(mdarray.readAsync({
          origin: [ 0, 0, 0 ],
          span: [ 2, 5, 5 ]
        }), /arrayStartIdx/)
      })
    })
  })

  describe('gdal.RasterBand', () => {
    describe('asMDArray', () => {
      let ds: gdal.Dataset, band: gdal.RasterBand
      beforeEach(() => {
        ds = gdal.open(path.resolve(__dirname, 'data', 'sample.tif'))
        band = ds.bands.get(1)
      })

      it('should return an instance of gdal.MDArray with the same size as the parent RasterBand', () => {
        const mdarray = band.asMDArray()
        assert.instanceOf(band, gdal.RasterBand)
        assert.equal(mdarray.dimensions.names.length, 2)
        assert.equal(band.size.x, mdarray.dimensions.get('X').size)
        assert.equal(band.size.y, mdarray.dimensions.get('Y').size)
        assert.equal(band.dataType, mdarray.dataType)
        ds.close()
      })

      it('should read the same data as the parent RasterBand', () => {
        const mdarray = band.asMDArray()
        const data1 = mdarray.read({
          origin: [ 7, 10 ],
          span: [ 5, 5 ]
        })
        const data2 = band.pixels.read(10, 7, 5, 5)
        assert.deepEqual(data1, data2)
        ds.close()
      })
    })
  })
})
