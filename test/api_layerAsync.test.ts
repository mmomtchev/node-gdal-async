import * as chaiAsPromised from 'chai-as-promised'
import * as chai from 'chai'
const assert: Chai.Assert = chai.assert
import * as gdal from 'gdal-async'
import * as fileUtils from './utils/file'
import * as semver from 'semver'

chai.use(chaiAsPromised)

describe('gdal.LayerAsync', () => {
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  describe('instance', () => {
    const cleanupWrite = (ds: gdal.Dataset, file: string) => {
      try {
        ds.close()
      } catch (e) {
        /* ignore */
      }
      try {
        const driver = gdal.drivers.get('ESRI Shapefile')
        driver.deleteDataset(file)
      } catch (e) {
        /* ignore */
      }
    }

    const cleanupRead = (ds: gdal.Dataset, dir: string) => {
      try {
        ds.close()
      } catch (e) {
        /* ignore */
      }
      try {
        fileUtils.deleteRecursiveVSIMEM(dir)
      } catch (e) {
        /* ignore */
      }
    }


    type prepareCb = (ds: gdal.Dataset, l: gdal.Layer, file: string) => void
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    const prepare_dataset_layer_test = function (mode: string, _arg2: Record<string, unknown> | prepareCb, _arg3?: prepareCb) {
      let ds: gdal.Dataset, layer: gdal.Layer, options, callback: prepareCb,
        err, file: string, dir: string | null, driver: gdal.Driver

      if (arguments.length === 2) {
        options = {}
        // eslint-disable-next-line prefer-rest-params
        callback = arguments[1]
      } else {
        // eslint-disable-next-line prefer-rest-params
        options = arguments[1] || {}
        // eslint-disable-next-line prefer-rest-params
        callback = arguments[2]
      }

      dir = null
      // set dataset / layer
      if (mode === 'r') {
        dir = fileUtils.cloneDir(`${__dirname}/data/shp`)
        file = `${dir}/sample.shp`
        ds = gdal.open(file)
        layer = ds.layers.get(0)
      } else {
        driver = gdal.drivers.get('ESRI Shapefile')
        file = `/vsimem/layer_test.${String(
          Math.random()
        ).substring(2)}.tmp.shp`
        ds = driver.create(file)
        layer = ds.layers.create('layer_test', null, gdal.Point)
      }

      let r
      // run test and then teardown
      try {
        r = callback(ds, layer, file)
      } catch (e) {
        err = e
      }

      // teardown
      if (options.autoclose !== false) {
        if (file && mode === 'w') cleanupWrite(ds, file)
        if (dir) cleanupRead(ds, dir)
      }

      if (err) throw err
      return r
    }

    describe('"ds" property', () => {
      describe('getter', () => {
        it('should return Dataset', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.instanceOf(layer.ds, gdal.Dataset)
            assert.equal(layer.ds, dataset)
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              console.log(layer.ds.srs)
            }, /already been destroyed/)
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.throws(() => {
              // eslint-disable-next-line @typescript-eslint/no-explicit-any
              (layer as any).ds = null
            }, /ds is a read-only property/)
          })
        })
      })
    })

    describe('"srs" property', () => {
      describe('getter', () => {
        it('should return SpatialReference', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            // EPSG:4269 - exact WKT can vary when using shared GDAL / Proj4 library
            const expectedWKT = [
              'GEOGCS["NAD83",DATUM["North_American_Datum_1983",SPHEROID["GRS 1980",6378137,298.257222101,AUTHORITY["EPSG","7019"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY["EPSG","6269"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.0174532925199433,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4269"]]',
              'GEOGCS["GCS_North_American_1983",DATUM["North_American_Datum_1983",SPHEROID["GRS_1980",6378137,298.257222101]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]]',
              'GEOGCS["GCS_North_American_1983",DATUM["North_American_Datum_1983",SPHEROID["GRS_1980",6378137,298.257222101]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295],AUTHORITY["EPSG","4269"]]',
              'GEOGCS["NAD83",DATUM["North_American_Datum_1983",SPHEROID["GRS 1980",6378137,298.257222101,AUTHORITY["EPSG","7019"]],AUTHORITY["EPSG","6269"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.0174532925199433,AUTHORITY["EPSG","9122"]],AXIS["Latitude",NORTH],AXIS["Longitude",EAST],AUTHORITY["EPSG","4269"]]'
            ]
            assert.include(expectedWKT, layer.srs.toWKT())
          })
        })
        it('should return the same SpatialReference object', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            const srs1 = layer.srs
            const srs2 = layer.srs
            assert.equal(srs1, srs2)
          })
        })
        // NOTE: geojson has a default projection: EPSG 4326
        // it('should return null when dataset doesn\'t have projection', function() {
        // 	var ds = gdal.open(__dirname + "/data/park.geo.json");
        // 	var layer = ds.layers.get(0);
        // 	assert.isNull(layer.srs);
        // });
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              console.log(layer.srs)
            }, /already been destroyed/)
          })
        })
        describe('result', () => {
          it('should not be destroyed when dataset is destroyed', () => {
            prepare_dataset_layer_test('r', (dataset, layer) => {
              const srs = layer.srs
              dataset.close()
              assert.doesNotThrow(() => {
                assert.ok(srs.toWKT())
              })
            })
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.throws(() => {
              // eslint-disable-next-line @typescript-eslint/no-explicit-any
              (layer.srs as any) = 'ESPG:4326'
            }, /srs is a read-only property/)
          })
        })
      })
    })

    describe('"name" property', () => {
      describe('getter', () => {
        it('should return string', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.equal(layer.name, 'sample')
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              console.log(layer.name)
            })
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.throws(() => {
              // eslint-disable-next-line @typescript-eslint/no-explicit-any
              (layer as any).name = null
            }, /name is a read-only property/)
          })
        })
      })
    })

    describe('"geomType" property', () => {
      describe('getter', () => {
        it('should return wkbGeometryType', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.equal(layer.geomType, gdal.wkbPolygon)
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              console.log(layer.geomType)
            }, /already been destroyed/)
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.throws(() => {
              // eslint-disable-next-line @typescript-eslint/no-explicit-any
              (layer as any).geomType = null
            }, /geomType is a read-only property/)
          })
        })
      })
    })

    describe('testCapability()', () => {
      it("should return false when layer doesn't support capability", () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          assert.isFalse(layer.testCapability(gdal.OLCCreateField))
        })
      })
      it('should return true when layer does support capability', () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          assert.isTrue(layer.testCapability(gdal.OLCRandomRead))
        })
      })
      it('should throw error if dataset is destroyed', () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          dataset.close()
          assert.throws(() => {
            layer.testCapability(gdal.OLCCreateField)
          }, /already been destroyed/)
        })
      })
    })

    describe('copyAsync()', () => {
      it('should copy a layer/Async', () =>
        prepare_dataset_layer_test('w', { autoclose: false }, (dataset, layer, file) => {
          const r = dataset.layers.copyAsync(layer, 'newlayer')
          return assert.isFulfilled(Promise.all([
            assert.isFulfilled(r),
            assert.eventually.instanceOf(r, gdal.Layer),
            assert.eventually.propertyVal(r, 'name', 'newlayer')
          ])).then(() => cleanupWrite(dataset, file))
        })
      )
      it('should gracefully handle closing the dataset before the operation has started', () =>
        prepare_dataset_layer_test('w', (dataset, layer) =>
          assert.isRejected(new Promise((resolve, reject) =>
            process.nextTick(() => {
              try {
                dataset.layers.copyAsync(layer, 'newlayer', undefined, (e, r) => {
                  if (e) reject(e)
                  resolve(r)
                })
              } catch (e) {
                reject(e)
              }
            })))
        )
      )
    })

    describe('removeAsync()', () => {
      it('should remove a layer/Async', () =>
        prepare_dataset_layer_test('w', { autoclose: false }, (dataset, _, file) => {
          const layers = dataset.layers.count()
          const r = dataset.layers.removeAsync(0)
          return assert.isFulfilled(Promise.all([
            assert.isFulfilled(r),
            assert.eventually.equal(r.then(() => dataset.layers.count()), layers - 1)
          ])).then(() => cleanupWrite(dataset, file))
        })
      )
    })

    describe('getExtent()', () => {
      it('should return Envelope', () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          const actual_envelope = layer.getExtent()
          const expected_envelope = {
            minX: -111.05687488399991,
            minY: 40.99549316200006,
            maxX: -104.05224885499985,
            maxY: 45.00589722600017
          }

          assert.instanceOf(actual_envelope, gdal.Envelope)
          assert.closeTo(actual_envelope.minX, expected_envelope.minX, 0.00001)
          assert.closeTo(actual_envelope.minY, expected_envelope.minY, 0.00001)
          assert.closeTo(actual_envelope.maxX, expected_envelope.maxX, 0.00001)
          assert.closeTo(actual_envelope.maxY, expected_envelope.maxY, 0.00001)
        })
      })
      it("should throw error if force flag is false and layer doesn't have extent already computed", () => {
        // No longer true in GDAL 3.9
        if (semver.lt(gdal.version, '3.9.0')) {
          const dataset = gdal.open(`${__dirname}/data/park.geo.json`)
          const layer = dataset.layers.get(0)
          assert.throws(() => {
            layer.getExtent(false)
          }, "Can't get layer extent without computing it")
        }
      })
      it('should throw error if dataset is destroyed', () =>
        prepare_dataset_layer_test('r', (dataset, layer) => {
          dataset.close()
          assert.throws(() => {
            layer.getExtent()
          }, /already been destroyed/)
        })
      )
    })

    describe('setSpatialFilter()', () => {
      it('should accept 4 numbers', () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          const count_before = layer.features.count()
          layer.setSpatialFilter(-111, 41, -104, 43)
          const count_after = layer.features.count()

          assert.isTrue(
            count_after < count_before,
            'feature count has decreased'
          )
        })
      })
      it('should accept Geometry', () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          const count_before = layer.features.count()
          const filter = new gdal.Polygon()
          const ring = new gdal.LinearRing()
          ring.points.add(-111, 41)
          ring.points.add(-104, 41)
          ring.points.add(-104, 43)
          ring.points.add(-111, 43)
          ring.points.add(-111, 41)
          filter.rings.add(ring)
          layer.setSpatialFilter(filter)
          const count_after = layer.features.count()

          assert.isTrue(
            count_after < count_before,
            'feature count has decreased'
          )
        })
      })
      it('should clear the spatial filter if passed null', () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          const count_before = layer.features.count()
          layer.setSpatialFilter(-111, 41, -104, 43)
          layer.setSpatialFilter(null)
          const count_after = layer.features.count()

          assert.equal(count_before, count_after)
        })
      })
      it('should throw error if dataset is destroyed', () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          dataset.close()
          assert.throws(() => {
            layer.setSpatialFilter(-111, 41, -104, 43)
          }, /already been destroyed/)
        })
      })
    })

    describe('getSpatialFilter()', () => {
      it('should return Geometry', () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          const filter = new gdal.Polygon()
          const ring = new gdal.LinearRing()
          ring.points.add(-111, 41)
          ring.points.add(-104, 41)
          ring.points.add(-104, 43)
          ring.points.add(-111, 43)
          ring.points.add(-111, 41)
          filter.rings.add(ring)
          layer.setSpatialFilter(filter)

          const result = layer.getSpatialFilter()
          assert.instanceOf(result, gdal.Polygon)
        })
      })
      it('should throw error if dataset is destroyed', () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          dataset.close()
          assert.throws(() => {
            layer.getSpatialFilter()
          }, /already been destroyed/)
        })
      })
    })

    describe('setAttributeFilter()', () => {
      it('should filter layer by expression', () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          const count_before = layer.features.count()
          layer.setAttributeFilter("name = 'Park'")
          const count_after = layer.features.count()

          assert.isTrue(
            count_after < count_before,
            'feature count has decreased'
          )
        })
      })
      it('should clear the attribute filter if passed null', () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          const count_before = layer.features.count()
          layer.setAttributeFilter("name = 'Park'")
          layer.setAttributeFilter(null)
          const count_after = layer.features.count()

          assert.equal(count_before, count_after)
        })
      })
      it('should throw error if dataset is destroyed', () => {
        prepare_dataset_layer_test('r', (dataset, layer) => {
          dataset.close()
          assert.throws(() => {
            layer.setAttributeFilter("name = 'Park'")
          }, /already been destroyed/)
        })
      })
    })

    describe('"features" property', () => {
      describe('getter', () => {
        it('should return LayerFeatures', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.instanceOf(layer.features, gdal.LayerFeatures)
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.throws(() => {
              // eslint-disable-next-line @typescript-eslint/no-explicit-any
              (layer as any).features = null
            }, /features is a read-only property/)
          })
        })
      })
      describe('countAsync() w/cb', () => {
        it('should return an integer', (done) =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer, file) => {
            layer.features.countAsync(undefined, (e, r) => {
              assert.equal(r, 23)
              cleanupWrite(dataset, file)
              done()
            })
          })
        )
        it('should throw error if dataset is destroyed', () =>
          prepare_dataset_layer_test('r', (dataset, layer) => {
            dataset.close()
            assert.throws(() => layer.features.countAsync(undefined, (e) => {
              assert.instanceOf(e, Error)
            }), /already destroyed/)
          })
        )
      })
      describe('countAsync() w/Promise', () => {
        it('should return an integer', () =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer, file) =>
            assert.becomes(layer.features.countAsync(), 23)
              .then(() => cleanupWrite(dataset, file))
          )
        )
        it('should throw error if dataset is destroyed', () =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer, file) => {
            dataset.close()
            return assert.isRejected(layer.features.countAsync(), /already destroyed/)
              .then(() => cleanupWrite(dataset, file))
          })
        )
      })
      describe('getAsync()', () => {
        it('should return a Feature', () =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer, file) => {
            const feature = layer.features.getAsync(0)
            return assert.eventually.instanceOf(feature, gdal.Feature)
              .then(() => cleanupWrite(dataset, file))
          })
        )
        it("should reject if index doesn't exist", () =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer, file) => {
            const feature = layer.features.getAsync(99)
            return assert.isRejected(feature)
              .then(() => cleanupWrite(dataset, file))
          })
        )
        it('should throw error if dataset is destroyed', () =>
          prepare_dataset_layer_test('r', (dataset, layer) => {
            dataset.close()
            const feature = layer.features.getAsync(0)
            return assert.isRejected(feature, /already destroyed/)
          })
        )
      })
      describe('nextAsync()', () => {
        it('should return a Feature and increment the iterator', () =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer, file) => {
            const f1 = layer.features.nextAsync()
            const f2 = layer.features.nextAsync()
            return assert.isFulfilled(Promise.all([ assert.eventually.instanceOf(f1, gdal.Feature),
              assert.eventually.instanceOf(f2, gdal.Feature),
              assert.eventually.notEqual(f1, f2)
            ])).then(() => cleanupWrite(dataset, file))
          })
        )
        it('should return null after last feature', () =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer, file) => {
            const count = layer.features.count()
            const p = []
            for (let i = 0; i < count; i++) {
              p.push(layer.features.nextAsync())
            }
            return assert.eventually.isNull(Promise.all(p).then(() => layer.features.nextAsync()))
              .then(() => cleanupWrite(dataset, file))
          })
        )
        it('should throw error if dataset is destroyed', () =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer, file) => {
            dataset.close()
            return assert.isRejected(layer.features.nextAsync(), /already destroyed/)
              .then(() => cleanupWrite(dataset, file))
          })
        )
      })
      describe('firstAsync()', () => {
        it('should return a Feature and reset the iterator', () =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer, file) => {
            const f = layer.features.nextAsync().then(() => layer.features.firstAsync())
            return assert.isFulfilled(Promise.all([ assert.eventually.instanceOf(f, gdal.Feature),
              assert.eventually.propertyVal(f, 'fid', 0)
            ])).then(() => cleanupWrite(dataset, file))
          })
        )
        it('should throw error if dataset is destroyed', () =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer, file) => {
            dataset.close()
            return assert.isRejected(layer.features.firstAsync(), /already destroyed/)
              .then(() => cleanupWrite(dataset, file))
          })
        )
      })
      describe('forEach()', () => {
        it('should pass each feature to the callback', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            let count = 0
            layer.features.forEach((feature, i) => {
              assert.isNumber(i)
              assert.instanceOf(feature, gdal.Feature)
              count++
            })
            assert.equal(count, layer.features.count())
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              layer.features.forEach(() => undefined)
            }, /already destroyed/)
          })
        })
      })
      describe('map()', () => {
        it('should operate normally', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            const result = layer.features.map((feature, i) => {
              assert.isNumber(i)
              assert.instanceOf(feature, gdal.Feature)
              return 'a'
            })

            assert.isArray(result)
            assert.equal(result[0], 'a')
            assert.equal(result.length, layer.features.count())
          })
        })
      })
      describe('addAsync()', () => {
        it('should add Feature to layer', () =>
          prepare_dataset_layer_test('w', { autoclose: false }, (dataset, layer, file) => {
            const f = layer.features.addAsync(new gdal.Feature(layer))
            return assert.eventually.equal(f.then(() => layer.features.count()), 1)
              .then(() => cleanupWrite(dataset, file))
          })
        )
        it('should throw error if layer doesnt support creating features', () =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer, file) => {
            assert.isRejected(layer.features.addAsync(new gdal.Feature(layer)), /read-only/)
              .then(() => cleanupWrite(dataset, file))
          })
        )
        it('should throw error if dataset is destroyed', () =>
          prepare_dataset_layer_test('w', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              const feature = new gdal.Feature(layer)
              layer.features.addAsync(feature)
            }, /already destroyed/)
          })
        )
      })

      describe('setAsync()', () => {
        let f0: gdal.Feature, f1: gdal.Feature, f1_new: gdal.Feature
        let layer: gdal.Layer, dataset: gdal.Dataset, file: string
        beforeEach(() => {
          prepare_dataset_layer_test('w', { autoclose: false }, (ds, lyr, f) => {
            layer = lyr
            dataset = ds
            file = f

            layer.fields.add(new gdal.FieldDefn('status', gdal.OFTString))

            f0 = new gdal.Feature(layer)
            f1 = new gdal.Feature(layer)
            f1_new = new gdal.Feature(layer)

            f0.fields.set('status', 'unchanged')
            f1.fields.set('status', 'unchanged')
            f1_new.fields.set('status', 'changed')

            layer.features.add(f0)
            layer.features.add(f1)
          })
        })
        afterEach(() => cleanupWrite(dataset, file))

        describe('w/feature argument', () => {
          describe('w/fid,feature arguments', () => {
            it('should replace existing feature', () => {
              assert.equal(
                layer.features.get(1).fields.get('status'),
                'unchanged'
              )
              const q = layer.features.setAsync(1, f1_new)
              return assert.eventually.equal(q.then(() => layer.features.get(1).fields.get('status')), 'changed')
            })
          })
          it('should reject if layer doesnt support changing features', () =>
            prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer) =>
              assert.isRejected(layer.features.setAsync(1, new gdal.Feature(layer)), /read-only/)
            ))
        })
        it('should reject if dataset is destroyed', () =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer: gdal.Layer) => {
            const newFeature = new gdal.Feature(layer)
            dataset.close()
            return assert.isRejected(layer.features.setAsync(1, newFeature), /already destroyed/)
          }))
      })

      describe('removeAsync()', () => {
        it('should make the feature at fid null', () =>
          prepare_dataset_layer_test('w', { autoclose: false }, (dataset, layer) => {
            layer.features.add(new gdal.Feature(layer))
            layer.features.add(new gdal.Feature(layer))

            assert.instanceOf(layer.features.get(1), gdal.Feature)
            return assert.isRejected(layer.features.removeAsync(1).then(() => layer.features.get(1)))
          })
        )
        it('should throw error if driver doesnt support deleting features', () =>
          prepare_dataset_layer_test('r', { autoclose: false }, (dataset, layer) => {
            assert.throws(() => {
              layer.features.remove(1)
            }, /read-only/)
          })
        )
        it('should throw error if dataset is destroyed', () =>
          prepare_dataset_layer_test('w', { autoclose: false }, (dataset, layer) => {
            dataset.close()
            return assert.isRejected(layer.features.removeAsync(1), /already destroyed/)
          })
        )
      })
    })

    describe('"fields" property', () => {
      describe('getter', () => {
        it('should return LayerFields', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.instanceOf(layer.fields, gdal.LayerFields)
          })
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          prepare_dataset_layer_test('w', (dataset, layer) => {
            assert.throws(() => {
              // eslint-disable-next-line @typescript-eslint/no-explicit-any
              (layer as any).fields = null
            }, /fields is a read-only property/)
          })
        })
      })
      describe('count()', () => {
        it('should return an integer', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.equal(layer.fields.count(), 8)
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              layer.fields.count()
            }, /already destroyed/)
          })
        })
      })
      describe('get()', () => {
        describe('w/id argument', () => {
          it('should return a FieldDefn', () => {
            prepare_dataset_layer_test('r', (dataset, layer) => {
              const field = layer.fields.get(4)
              assert.instanceOf(field, gdal.FieldDefn)
              assert.equal(field.name, 'fips_num')
            })
          })
        })
        describe('w/name argument', () => {
          it('should return a FieldDefn', () => {
            prepare_dataset_layer_test('r', (dataset, layer) => {
              const field = layer.fields.get('fips_num')
              assert.instanceOf(field, gdal.FieldDefn)
              assert.equal(field.name, 'fips_num')
            })
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              layer.fields.get(4)
            }, /already destroyed/)
          })
        })
      })
      describe('forEach()', () => {
        it('should return pass each FieldDefn to callback', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            const expected_names = [
              'path',
              'name',
              'type',
              'long_name',
              'fips_num',
              'fips',
              'state_fips',
              'state_abbr'
            ]
            let count = 0
            layer.fields.forEach((field, i) => {
              assert.isNumber(i)
              assert.instanceOf(field, gdal.FieldDefn)
              assert.equal(expected_names[i], field.name)
              count++
            })
            assert.equal(layer.fields.count(), count)
            assert.deepEqual(layer.fields.getNames(), expected_names)
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              layer.fields.forEach(() => undefined)
            }, /already destroyed/)
          })
        })
      })
      describe('map()', () => {
        it('should operate normally', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            const result = layer.fields.map((field, i) => {
              assert.isNumber(i)
              assert.instanceOf(field, gdal.FieldDefn)
              return 'a'
            })

            assert.isArray(result)
            assert.equal(result[0], 'a')
            assert.equal(result.length, layer.fields.count())
          })
        })
      })
      describe('getNames()', () => {
        it('should return an array of field names', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            const expected_names = [
              'path',
              'name',
              'type',
              'long_name',
              'fips_num',
              'fips',
              'state_fips',
              'state_abbr'
            ]
            assert.deepEqual(layer.fields.getNames(), expected_names)
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              layer.fields.getNames()
            }, /already destroyed/)
          })
        })
      })
      describe('indexOf()', () => {
        it('should return index of field name', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            const field_name = layer.fields.get(4).name
            assert.equal(layer.fields.indexOf(field_name), 4)
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              layer.fields.indexOf('fips_num')
            }, /already destroyed/)
          })
        })
      })
      describe('add()', () => {
        describe('w/FieldDefn argument', () => {
          it('should add FieldDefn to layer definition', () => {
            prepare_dataset_layer_test('w', (dataset, layer) => {
              const f0 = new gdal.FieldDefn('field0', gdal.OFTString)
              const f1 = new gdal.FieldDefn('field1', gdal.OFTInteger)
              const f2 = new gdal.FieldDefn('field2', gdal.OFTReal)
              layer.fields.add(f0)
              layer.fields.add(f1)
              layer.fields.add(f2)
              assert.equal(layer.fields.count(), 3)
              assert.equal(layer.fields.get(0).name, 'field0')
              assert.equal(layer.fields.get(1).name, 'field1')
              assert.equal(layer.fields.get(2).name, 'field2')
            })
          })
          it('should throw an error if approx flag is false and layer doesnt support field as it is', () => {
            prepare_dataset_layer_test('w', (dataset, layer) => {
              assert.throws(() => {
                layer.fields.add(
                  new gdal.FieldDefn(
                    'some_long_name_over_10_chars',
                    gdal.OFTString
                  ),
                  false
                )
              }, /Failed to add/)
            })
          })
        })
        describe('w/FieldDefn array argument', () => {
          it('should add FieldDefns to layer definition', () => {
            prepare_dataset_layer_test('w', (dataset, layer) => {
              const fields = [
                new gdal.FieldDefn('field0', gdal.OFTString),
                new gdal.FieldDefn('field1', gdal.OFTInteger),
                new gdal.FieldDefn('field2', gdal.OFTReal)
              ]
              layer.fields.add(fields)
              assert.equal(layer.fields.count(), 3)
              assert.equal(layer.fields.get(0).name, 'field0')
              assert.equal(layer.fields.get(1).name, 'field1')
              assert.equal(layer.fields.get(2).name, 'field2')
            })
          })
          it('should throw an error if approx flag is false and layer doesnt support field as it is', () => {
            prepare_dataset_layer_test('w', (dataset, layer) => {
              assert.throws(() => {
                layer.fields.add(
                  [
                    new gdal.FieldDefn(
                      'some_long_name_over_10_chars',
                      gdal.OFTString
                    )
                  ],
                  false
                )
              }, /Failed to add/)
            })
          })
        })
        it('should throw an error if layer doesnt support adding fields', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.throws(() => {
              layer.fields.add(new gdal.FieldDefn('field0', gdal.OFTString))
            }, /read-only/)
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('w', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              layer.fields.add(new gdal.FieldDefn('field0', gdal.OFTString))
            }, /already destroyed/)
          })
        })
      })
      describe('fromObject()', () => {
        it('should make fields from object keys/values', () => {
          prepare_dataset_layer_test('w', (dataset, layer) => {
            const sample_fields = {
              id: 1,
              name: 'some_name',
              value: 3.1415,
              flag: true
            }
            layer.fields.fromObject(sample_fields)
            const f0 = layer.fields.get(0)
            const f1 = layer.fields.get(1)
            const f2 = layer.fields.get(2)
            const f3 = layer.fields.get(3)
            assert.equal(f0.name, 'id')
            assert.equal(f1.name, 'name')
            assert.equal(f2.name, 'value')
            assert.equal(f3.name, 'flag')
            assert.equal(f0.type, gdal.OFTInteger)
            assert.equal(f1.type, gdal.OFTString)
            assert.equal(f2.type, gdal.OFTReal)
            assert.equal(f3.type, gdal.OFTInteger)
          })
        })
        it("should throw error if field name isn't supported", () => {
          prepare_dataset_layer_test('w', (dataset, layer) => {
            assert.throws(() => {
              layer.fields.fromObject({ some_really_long_name: 'test' })
            }, /Failed to add/)
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('w', (dataset, layer) => {
            dataset.close()
            assert.throws(() => {
              layer.fields.fromObject({ name: 'test' })
            }, /already destroyed/)
          })
        })
      })
      describe('remove()', () => {
        describe('w/id argument', () => {
          it('should remove FieldDefn from layer definition', () => {
            prepare_dataset_layer_test('w', (dataset, layer) => {
              layer.fields.add(new gdal.FieldDefn('field0', gdal.OFTString))
              layer.fields.add(new gdal.FieldDefn('field1', gdal.OFTString))
              assert.equal(layer.fields.count(), 2)

              layer.fields.remove(0)
              assert.equal(layer.fields.count(), 1)
              assert.equal(layer.fields.get(0).name, 'field1')
            })
          })
        })
        describe('w/name argument', () => {
          it('should remove FieldDefn from layer definition', () => {
            prepare_dataset_layer_test('w', (dataset, layer) => {
              layer.fields.add(new gdal.FieldDefn('field0', gdal.OFTString))
              layer.fields.add(new gdal.FieldDefn('field1', gdal.OFTString))
              assert.equal(layer.fields.count(), 2)

              layer.fields.remove('field0')
              assert.equal(layer.fields.count(), 1)
              assert.equal(layer.fields.get(0).name, 'field1')
            })
          })
        })
        it('should throw error if layer doesnt support removing fields', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.throws(() => {
              layer.fields.remove(0)
            }, /read-only/)
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('w', (dataset, layer) => {
            layer.fields.add(new gdal.FieldDefn('field0', gdal.OFTString))
            dataset.close()
            assert.throws(() => {
              layer.fields.remove(0)
            }, /already destroyed/)
          })
        })
      })
      describe('reorder()', () => {
        it('should reorder fields', () => {
          prepare_dataset_layer_test('w', (dataset, layer) => {
            layer.fields.add(new gdal.FieldDefn('field0', gdal.OFTString))
            layer.fields.add(new gdal.FieldDefn('field1', gdal.OFTString))
            layer.fields.add(new gdal.FieldDefn('field2', gdal.OFTString))

            layer.fields.reorder([ 2, 0, 1 ])
            const f0 = layer.fields.get(0)
            const f1 = layer.fields.get(1)
            const f2 = layer.fields.get(2)
            assert.equal(f0.name, 'field2')
            assert.equal(f1.name, 'field0')
            assert.equal(f2.name, 'field1')
          })
        })
        it('should throw an error if layer doesnt support reordering fields', () => {
          prepare_dataset_layer_test('r', (dataset, layer) => {
            assert.throws(() => {
              layer.fields.reorder([ 2, 0, 1, 3, 4, 5, 6, 7 ])
            }, /read-only/)
          })
        })
        it('should throw error if dataset is destroyed', () => {
          prepare_dataset_layer_test('w', (dataset, layer) => {
            layer.fields.add(new gdal.FieldDefn('field0', gdal.OFTString))
            layer.fields.add(new gdal.FieldDefn('field1', gdal.OFTString))
            layer.fields.add(new gdal.FieldDefn('field2', gdal.OFTString))
            dataset.close()
            assert.throws(() => {
              layer.fields.reorder([ 2, 0, 1 ])
            }, /already destroyed/)
          })
        })
      })
    })
  })
})
