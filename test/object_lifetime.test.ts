// These objects are not public
/* eslint-disable @typescript-eslint/no-explicit-any */
import * as gdal from 'gdal-async'
import { assert } from 'chai'
import * as path from 'path'

describe('object cache', () => {
  it('should return same object if pointer is same', () => {
    for (let i = 0; i < 10; i++) {
      (gdal as any).log(`Object Cache test run #${i}`)
      const ds = gdal.open('temp', 'w', 'MEM', 4, 4, 1)
      const band1 = ds.bands.get(1)
      const band2 = ds.bands.get(1)
      assert.instanceOf(band1, gdal.RasterBand)
      assert.equal(band1, band2)
      assert.equal(band1.size.x, 4)
      assert.equal(band2.size.x, 4)
          global.gc!()
    }
  })
})
describe('object lifetimes', () => {
  it('datasets should stay alive until all bands go out of scope', () => {
    let ds: gdal.Dataset | null = gdal.open('temp', 'w', 'MEM', 4, 4, 1)
    let band: gdal.RasterBand | null = ds.bands.get(1)

    const ds_uid = (ds as any)._uid
    const band_uid = (band as any)._uid

    ds = null
      global.gc!()

      assert.isTrue((gdal as any)._isAlive(ds_uid))
      assert.isTrue((gdal as any)._isAlive(band_uid))

      band = null
      global.gc!()

      assert.isFalse((gdal as any)._isAlive(ds_uid))
      assert.isFalse((gdal as any)._isAlive(band_uid))
  })
  it('bands should immediately be garbage collected as they go out of scope', () => {
    const ds = (gdal as any).open('temp', 'w', 'MEM', 4, 4, 1)
    let band = ds.bands.get(1)

    const ds_uid = ds._uid
    const band_uid = band._uid

    band = null
      global.gc!()

      assert.isTrue((gdal as any)._isAlive(ds_uid))
      assert.isFalse((gdal as any)._isAlive(band_uid))
  })
  it('datasets should stay alive until all layers go out of scope', () => {
    let ds: gdal.Dataset | null = gdal.open(path.join(__dirname, 'data/shp/sample.shp'))
    let layer: gdal.Layer | null = ds.layers.get(0)

    const ds_uid = (ds as any)._uid
    const layer_uid = (layer as any)._uid

    ds = null
      global.gc!()

      assert.isTrue((gdal as any)._isAlive(ds_uid))
      assert.isTrue((gdal as any)._isAlive(layer_uid))

      layer = null
      global.gc!()

      assert.isFalse((gdal as any)._isAlive(ds_uid))
      assert.isFalse((gdal as any)._isAlive(layer_uid))
  })
  it('layers should immediately be garbage collected as they go out of scope', () => {
    const ds: gdal.Dataset | null = gdal.open(path.join(__dirname, 'data/shp/sample.shp'))
    let layer: gdal.Layer | null = ds.layers.get(0)

    const ds_uid = (ds as any)._uid
    const layer_uid = (layer as any)._uid

    layer = null
      global.gc!()

      assert.isTrue((gdal as any)._isAlive(ds_uid))
      assert.isFalse((gdal as any)._isAlive(layer_uid))
  })
})
