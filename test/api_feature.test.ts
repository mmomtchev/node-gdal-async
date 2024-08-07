import * as gdal from 'gdal-async'
import { assert } from 'chai'
import * as path from 'path'

describe('gdal.Feature', () => {
  afterEach(global.gc!)

  let ds: gdal.Dataset, lyr: gdal.Layer, defn: gdal.FeatureDefn, fields: gdal.FieldDefn[]
  before(() => {
    ds = gdal.open('', 'w', 'Memory')
    lyr = ds.layers.create('', null, gdal.Point)
    fields = [
      new gdal.FieldDefn('id', gdal.OFTInteger),
      new gdal.FieldDefn('name', gdal.OFTString),
      new gdal.FieldDefn('value', gdal.OFTReal)
    ]
    defn = new gdal.FeatureDefn()
    defn.fields.add(fields)
    lyr.fields.add(fields)
  })

  describe('gdal.FieldDefn', () => {
    it('constructor should throw on invalid field types', () => {
      assert.throw(() => {
        new gdal.FieldDefn('invalid', 'invalid')
      }, /Unrecognized field type/)
    })
    it('"ignored" property', () => {
      const def = fields[0].ignored
      assert.isBoolean(def)
      fields[0].ignored = !def
      assert.strictEqual(fields[0].ignored, !def)
    })
    it('"justification" property', () => {
      const def = fields[0].justification
      assert.isUndefined(def)
      fields[0].justification = gdal.OJRight
      assert.strictEqual(fields[0].justification, gdal.OJRight)
      fields[0].justification = gdal.OJLeft
      assert.strictEqual(fields[0].justification, gdal.OJLeft)
      assert.throws(() => {
        fields[0].justification = 'invalid'
      }, /Unrecognized justification/)
      assert.strictEqual(fields[0].justification, gdal.OJLeft)
    })
    it('"width" property', () => {
      const def = fields[0].width
      assert.isNumber(def)
      fields[0].width = 10
      assert.strictEqual(fields[0].width, 10)
    })
    it('"precision" property', () => {
      const def = fields[0].precision
      assert.isNumber(def)
      fields[0].precision = 1
      assert.strictEqual(fields[0].precision, 1)
    })
    it('"name" property', () => {
      const def = fields[0].name
      assert.equal(def, 'id')
      fields[0].name = 'newname'
      assert.equal(fields[0].name, 'newname')
      fields[0].name = 'id'
    })
  })

  describe('gdal.FeatureDefn', () => {
    it('name should return a string', () => {
      assert.typeOf(defn.name, 'string')
    })

    it('geomType should return a Geometry type', () => {
      assert.strictEqual(defn.geomType, gdal.wkbUnknown)
    })

    it('geomIgnored should accept and return a Boolean', () => {
      assert.strictEqual(defn.geomIgnored, false)
      defn.geomIgnored = true
      assert.strictEqual(defn.geomIgnored, true)
    })

    it('styleIgnored should accept and return a Boolean', () => {
      assert.strictEqual(defn.styleIgnored, false)
      defn.styleIgnored = true
      assert.strictEqual(defn.styleIgnored, true)
    })
  })

  describe('gdal.FeatureDefnFields', () => {
    it('count() should return the field count', () => {
      assert.equal(defn.fields.count(), 3)
    })
    it('get() should accept an index', () => {
      assert.propertyVal(defn.fields.get(0), 'name', 'id')
    })
    it('get() should accept an id', () => {
      assert.propertyVal(defn.fields.get('id'), 'name', 'id')
    })
    it('get() should throw on invalid id', () => {
      assert.throws(() => {
        defn.fields.get('bogus')
      })
    })
    it('indexOf() should find a field by id', () => {
      assert.equal(defn.fields.indexOf('value'), 2)
    })
    it('remove() should remove a field by id', () => {
      const field = new gdal.FieldDefn('fourth', gdal.OFTInteger)
      defn.fields.add(field)
      assert.equal(defn.fields.count(), 4)
      defn.fields.remove('fourth')
      assert.equal(defn.fields.count(), 3)
    })
    it('remove() should remove a field by index', () => {
      const field = new gdal.FieldDefn('fourth', gdal.OFTInteger)
      defn.fields.add(field)
      assert.equal(defn.fields.count(), 4)
      defn.fields.remove(3)
      assert.equal(defn.fields.count(), 3)
    })
    it('reorder() should reorder the fields', () => {
      defn.fields.reorder([ 2, 0, 1 ])
      assert.equal(defn.fields.indexOf('id'), 1)
      defn.fields.reorder([ 1, 2, 0 ])
      assert.equal(defn.fields.indexOf('id'), 0)
    })
  })

  describe('constructor', () => {
    describe('w/Layer', () => {
      it('should create instance', () => {
        new gdal.Feature(lyr)
      })
      it('instance should use fields from LayerDefn', () => {
        const feature = new gdal.Feature(lyr)
        assert.deepEqual(feature.fields.getNames(), lyr.fields.getNames())
      })
      it('should throw error if layer is destroyed', () => {
        const ds = gdal.open('', 'w', 'Memory')
        const lyr = ds.layers.create('', null, gdal.Point)
        lyr.fields.add(fields)
        ds.close()
        assert.throws(() => {
          new gdal.Feature(lyr)
        })
      })
      it('should not throw error if layer is destroyed after feature is created', () => {
        const ds = gdal.open('', 'w', 'Memory')
        const lyr = ds.layers.create('', null, gdal.Point)
        lyr.fields.add(fields)
        const feature = new gdal.Feature(lyr)
        ds.close()
        assert.doesNotThrow(() => {
          feature.defn.fields.getNames()
        })
      })
    })
    describe('w/FeatureDefn', () => {
      it('should create instance', () => {
        new gdal.Feature(defn)
      })
      it('instance should use fields from FeatureDefn', () => {
        const feature = new gdal.Feature(defn)
        assert.deepEqual(feature.fields.getNames(), defn.fields.getNames())
      })
    })
  })

  describe('instance', () => {
    describe('destroy', () => {
      it('should destroy the feature', () => {
        const feature = new gdal.Feature(defn)
        feature.destroy()
        assert.throws(() => {
          feature.getGeometry()
        })
      })
    })
    describe('"fid" property', () => {
      describe('getter', () => {
        it('should return integer', () => {
          const feature = new gdal.Feature(defn)
          assert.equal(feature.fid, -1)
        })
        it('should throw on destroyed feature', () => {
          const feature = new gdal.Feature(defn)
          feature.destroy()
          assert.throws(() => {
            console.log(feature.fid)
          })
        })
      })
      describe('setter', () => {
        it('should set fid', () => {
          const feature = new gdal.Feature(defn)
          feature.fid = 5
          assert.equal(feature.fid, 5)
        })
      })
    })

    describe('"defn" property', () => {
      describe('getter', () => {
        it('should return FeatureDefn', () => {
          const feature = new gdal.Feature(defn)
          assert.instanceOf(feature.defn, gdal.FeatureDefn)
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          const feature = new gdal.Feature(defn)
          assert.throws(() => {
            // In TypeScript these exceptions require disabling the type checks
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (feature as any).defn = null
          })
        })
      })
    })

    describe('"fields" property', () => {
      describe('getter', () => {
        it('should return FeatureFields', () => {
          const feature = new gdal.Feature(defn)
          assert.instanceOf(feature.fields, gdal.FeatureFields)
        })
      })
      describe('setter', () => {
        it('should throw error', () => {
          const feature = new gdal.Feature(defn)
          assert.throws(() => {
            /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
            (feature as any).fields = null
          }, /fields is a read-only property/)
        })
      })
      describe('count()', () => {
        it('should return an integer', () => {
          const feature = new gdal.Feature(defn)
          assert.equal(feature.fields.count(), 3)
        })
      })
      describe('set()', () => {
        describe('w/id argument', () => {
          it('should properly set values', () => {
            const feature = new gdal.Feature(defn)

            feature.fields.set(0, 5)
            feature.fields.set(1, 'test')
            feature.fields.set(2, 3.14)

            assert.equal(feature.fields.get(0), 5)
            assert.equal(feature.fields.get(1), 'test')
            assert.closeTo(feature.fields.get(2), 3.14, 0.0001)
          })
          it('should unset field if value is null', () => {
            const feature = new gdal.Feature(defn)
            feature.fields.set(1, 'test')
            feature.fields.set(1, null)
            assert.isNull(feature.fields.get(1))
          })
          it('should throw an error if id is out of range', () => {
            const feature = new gdal.Feature(defn)
            assert.throws(() => {
              feature.fields.set(100, 'test')
            })
          })
          it('should throw an error if the value type is not supported', () => {
            const feature = new gdal.Feature(defn)
            assert.throws(() => {
              feature.fields.set(1, { invalid: true })
            }, /Unsupported type/)
          })
        })
        describe('w/name argument', () => {
          it('should properly set values', () => {
            const feature = new gdal.Feature(defn)

            feature.fields.set('id', 5)
            feature.fields.set('name', 'test')
            feature.fields.set('value', 3.14)

            assert.equal(feature.fields.get('id'), 5)
            assert.equal(feature.fields.get('name'), 'test')
            assert.closeTo(feature.fields.get('value'), 3.14, 0.0001)
          })
          it('should unset field if value is null', () => {
            const feature = new gdal.Feature(defn)
            feature.fields.set('name', 'test')
            feature.fields.set('name', null)
            assert.isNull(feature.fields.get('name'))
          })
          it('should throw an error if field name does not exist', () => {
            const feature = new gdal.Feature(defn)
            assert.throws(() => {
              feature.fields.set('bogus', 'test')
            })
          })
          it('should throw an error if the value type is not supported', () => {
            const feature = new gdal.Feature(defn)
            assert.throws(() => {
              feature.fields.set('name', { invalid: true })
            }, /Unsupported type/)
          })
        })
        describe('w/array argument', () => {
          it('should properly set all fields', () => {
            const feature = new gdal.Feature(defn)
            feature.fields.set([ 5, 'test', 3.14 ])
            assert.equal(feature.fields.get(0), 5)
            assert.equal(feature.fields.get(1), 'test')
            assert.closeTo(feature.fields.get(2), 3.14, 0.0001)
          })
          it('should throw an error if the value type is not supported', () => {
            const feature = new gdal.Feature(defn)
            assert.throws(() => {
              feature.fields.set([ { name: { invalid: true } } ])
            }, /Unsupported type/)
          })
        })
        describe('w/object argument', () => {
          it('should properly set all fields', () => {
            const feature = new gdal.Feature(defn)
            feature.fields.set({ id: 5, name: 'test', value: 3.14 })
            assert.equal(feature.fields.get(0), 5)
            assert.equal(feature.fields.get(1), 'test')
            assert.closeTo(feature.fields.get(2), 3.14, 0.0001)
          })
          it('should properly one field', () => {
            const feature = new gdal.Feature(defn)
            feature.fields.set({ name: 'test2' })
            assert.equal(feature.fields.get(1), 'test2')
          })
          it('should throw an error if the value type is not supported', () => {
            const feature = new gdal.Feature(defn)
            assert.throws(() => {
              feature.fields.set({ name: { invalid: true } })
            }, /Unsupported type/)
          })
          it('should throw an error if the arguments are invalid', () => {
            const feature = new gdal.Feature(defn)
            assert.throws(() => {
              /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
              (feature.fields as any).set(42)
            }, /expected an object/)
            assert.throws(() => {
              /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
              (feature.fields as any).set(0, 1, 2)
            }, /Invalid number of arguments/)
          })
        })
      })
      describe('get()', () => {
        describe('w/id argument', () => {
          it('should properly get values', () => {
            const feature = new gdal.Feature(defn)

            feature.fields.set(0, 5)
            feature.fields.set(1, 'test')
            feature.fields.set(2, 3.14)

            assert.equal(feature.fields.get(0), 5)
            assert.equal(feature.fields.get(1), 'test')
            assert.closeTo(feature.fields.get(2), 3.14, 0.0001)
          })
          it('should return unset fields as null', () => {
            const feature = new gdal.Feature(defn)

            assert.isNull(feature.fields.get(0))
            assert.isNull(feature.fields.get(1))
            assert.isNull(feature.fields.get(2))
          })
          it('should throw an error if id out of range', () => {
            const feature = new gdal.Feature(defn)
            assert.throws(() => {
              feature.fields.get(100)
            })
          })
          it('should throw an error with no arguments', () => {
            const feature = new gdal.Feature(defn)
            assert.throws(() => {
              /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
              (feature.fields as any).get()
            }, /Field index or name must be given/)
          })
        })
        describe('w/name argument', () => {
          it('should properly get/set values', () => {
            const feature = new gdal.Feature(defn)

            feature.fields.set('id', 5)
            feature.fields.set('name', 'test')
            feature.fields.set('value', 3.14)

            assert.equal(feature.fields.get('id'), 5)
            assert.equal(feature.fields.get('name'), 'test')
            assert.closeTo(feature.fields.get('value'), 3.14, 0.0001)
          })
          it('should return unset fields as null', () => {
            const feature = new gdal.Feature(defn)

            assert.isNull(feature.fields.get('id'))
            assert.isNull(feature.fields.get('name'))
            assert.isNull(feature.fields.get('value'))
          })
          it('should throw an error if field name doesnt exist', () => {
            const feature = new gdal.Feature(defn)
            assert.throws(() => {
              feature.fields.get('bogus')
            })
          })
        })
      })
      describe('toObject()', () => {
        it('should return the fields as a JSON object', () => {
          const feature = new gdal.Feature(defn)
          feature.fields.set([ 5, 'test', 3.14 ])
          const obj = feature.fields.toObject()
          assert.equal(obj.id, 5)
          assert.equal(obj.name, 'test')
          assert.closeTo(obj.value, 3.14, 0.0001)
        })
      })
      describe('toJSON()', () => {
        it('should return the fields as a stringified JSON object', () => {
          const feature = new gdal.Feature(defn)
          feature.fields.set([ 5, 'test', 3.14 ])
          const obj = JSON.parse(feature.fields.toJSON())
          assert.equal(obj.id, 5)
          assert.equal(obj.name, 'test')
          assert.closeTo(obj.value, 3.14, 0.0001)
        })
      })
      describe('toArray()', () => {
        it('should return an array of field values', () => {
          const feature = new gdal.Feature(defn)
          feature.fields.set([ 5, 'test', 3.14 ])
          const array = feature.fields.toArray()
          assert.equal(array[0], 5)
          assert.equal(array[1], 'test')
          assert.closeTo(array[2], 3.14, 0.0001)
        })
      })
      describe('forEach()', () => {
        it('should return pass each value and key to callback', () => {
          const feature = new gdal.Feature(defn)
          const expected_keys = [ 'id', 'name', 'value' ]
          const expected_values = [ 5, 'test', 3.14 ]
          const values = [] as (string|number)[]
          const keys = [] as (string|number)[]
          feature.fields.set(expected_values)
          feature.fields.forEach((value, key) => {
            values.push(value)
            keys.push(key)
          })
          assert.equal(keys.length, 3)
          assert.deepEqual(keys, expected_keys)
          assert.equal(values[0], expected_values[0])
          assert.equal(values[1], expected_values[1])
          assert.closeTo(values[2] as number, expected_values[2] as number, 0.0001)
        })
      })
      describe('getNames()', () => {
        it('should return an array of field names', () => {
          const feature = new gdal.Feature(defn)
          const expected_names = [ 'id', 'name', 'value' ]
          assert.deepEqual(feature.fields.getNames(), expected_names)
        })
      })
      describe('indexOf()', () => {
        it('should return index of field name', () => {
          const feature = new gdal.Feature(defn)
          assert.equal(feature.fields.indexOf('name'), 1)
        })
      })
      describe('reset()', () => {
        describe('w/no argument', () => {
          it('should reset all fields to null', () => {
            const feature = new gdal.Feature(defn)
            feature.fields.set([ 5, 'test', 3.14 ])
            feature.fields.reset()
            assert.isNull(feature.fields.get(0))
            assert.isNull(feature.fields.get(1))
            assert.isNull(feature.fields.get(2))
          })
        })
        describe('w/object argument', () => {
          it('should set fields from object and reset others', () => {
            const feature = new gdal.Feature(defn)
            feature.fields.set([ 5, 'test', 3.14 ])
            feature.fields.reset({ name: 'reset' })
            assert.isNull(feature.fields.get(0))
            assert.equal(feature.fields.get(1), 'reset')
            assert.isNull(feature.fields.get(2))
          })
          it('should throw an error if arguments are invalid', () => {
            const feature = new gdal.Feature(defn)
            assert.throws(() => {
              /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
              (feature.fields as any).reset('bogus')
            }, /fields must be an object/)
            assert.throws(() => {
              feature.fields.reset({ name: { invalid: true } })
            }, /Unsupported type of field value/)
          })
        })
      })
      describe('"feature" property', () => {
        it('should contain a reference to the parent feature', () => {
          const feature = new gdal.Feature(defn)
          assert.strictEqual(feature.fields.feature, feature)
        })
      })
    })

    describe('clone()', () => {
      it('should return new Feature', () => {
        const feature = new gdal.Feature(defn)
        const clone = feature.clone()
        assert.instanceOf(clone, gdal.Feature)
        assert.notEqual(clone, feature)
      })
    })
    describe('setGeometry()', () => {
      it('should set geometry', () => {
        const feature = new gdal.Feature(defn)
        feature.setGeometry(new gdal.Point(5, 10))
        const pt = feature.getGeometry() as gdal.Point
        assert.equal(pt.x, 5)
        assert.equal(pt.y, 10)
      })
      it('should clear geometry if null is passed', () => {
        const feature = new gdal.Feature(defn)
        feature.setGeometry(new gdal.Point(5, 10))
        feature.setGeometry(null)
        assert.isNull(feature.getGeometry())
      })
      it('should clear geometry if undefined is passed', () => {
        const feature = new gdal.Feature(defn)
        feature.setGeometry(new gdal.Point(5, 10))
        feature.setGeometry(undefined as unknown as null)
        assert.isNull(feature.getGeometry())
      })
      /*
			// http://gdal.org/1.11/ogr/classOGRFeature.html#af1181ade837a52129ea25b46dd50cf30
			// "checking for geometry type not yet implemented"
			it('should throw error if wrong geometry type', function(){
				var feature = new gdal.Feature(defn);
				assert.throws(function(){
					feature.setGeometry(new gdal.LineString());
				});
			});
			*/
    })
    describe('getGeometry()', () => {
      it('should get geometry', () => {
        const feature = new gdal.Feature(defn)
        feature.setGeometry(new gdal.Point(5, 10))
        const pt = feature.getGeometry() as gdal.Point
        assert.equal(pt.x, 5)
        assert.equal(pt.y, 10)
      })
      it('should return null if geometry is not set', () => {
        const feature = new gdal.Feature(defn)
        const geom = feature.getGeometry()
        assert.isNull(geom)
      })
      it('should throw on destroyed feature', () => {
        const feature = new gdal.Feature(defn)
        feature.destroy()
        assert.throws(() => {
          feature.getGeometry()
        }, /destroyed/)
      })
    })
    describe('setFrom()', () => {
      it('should set fields and geometry from other feature', () => {
        const feature1 = new gdal.Feature(defn)
        const feature2 = new gdal.Feature(defn)
        feature1.setGeometry(new gdal.Point(5, 10))
        feature1.fields.set([ 5, 'test', 3.14 ])
        feature2.setFrom(feature1)

        const pt = feature2.getGeometry() as gdal.Point
        assert.equal(pt.x, 5)
        assert.equal(pt.y, 10)
        assert.equal(feature2.fields.get(0), 5)
        assert.equal(feature2.fields.get(1), 'test')
        assert.closeTo(feature2.fields.get(2), 3.14, 0.0001)
      })
      it('should support reordering/ignoring fields when copying from another feature', () => {
        const fields = [
          new gdal.FieldDefn('id1', gdal.OFTInteger),
          new gdal.FieldDefn('id2', gdal.OFTInteger),
          new gdal.FieldDefn('id3', gdal.OFTInteger)
        ]
        const defn = new gdal.FeatureDefn()
        defn.fields.add(fields)

        const feature1 = new gdal.Feature(defn)
        const feature2 = new gdal.Feature(defn)
        feature1.setGeometry(new gdal.Point(5, 10))
        feature1.fields.set([ 0, 1, 2 ])
        feature2.setFrom(feature1, [ 0, -1, 1 ], false)

        const pt = feature2.getGeometry() as gdal.Point
        assert.equal(pt.x, 5)
        assert.equal(pt.y, 10)
        assert.equal(feature2.fields.get(0), 0)
        assert.equal(feature2.fields.get(1), 2)
        assert.isNull(feature2.fields.get(2))
      })
    })
    describe('FieldDefn list properties', () => {
      it('should parse list properties', () => {
        const ds = gdal.open(path.join(__dirname, 'data', 'complexfields.geo.json'))
        const layer = ds.layers.get(0)

        assert.deepEqual(layer.features.get(0).fields.get('OFTInteger64List'), [
          281474976710656,
          281474976710657
        ])
        assert.deepEqual(layer.features.get(0).fields.get('OFTIntegerList'), [ 1, 2, 3 ])
        assert.deepEqual(layer.features.get(0).fields.get('OFTRealList'), [ 1.2, 1.3, 1.4 ])
        assert.deepEqual(layer.features.get(0).fields.get('OFTStringList'), [ 'a', 'b', 'c' ])
      })
    })
  })
})
