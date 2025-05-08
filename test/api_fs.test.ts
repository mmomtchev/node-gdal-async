import * as gdal from 'gdal-async'
import * as fs from 'fs'
import * as path from 'path'
import { assert } from 'chai'

describe('gdal.fs', () => {
  describe('stat()', () => {
    it('should return information on file system objects', () => {
      const stat = gdal.fs.stat(path.resolve(__dirname, 'data', 'sample.tif'))
      assert.equal(stat.size, 794079)
      assert.isTrue((stat.mode & fs.constants.S_IFREG) === fs.constants.S_IFREG)
    })
    it('should support BigInt results', () => {
      const stat = gdal.fs.stat(path.resolve(__dirname, 'data', 'sample.tif'), true)
      assert.equal(stat.size, BigInt(794079))
    })
    it('should throw on non-existent files', () => {
      assert.throws(() => {
        gdal.fs.stat(path.resolve(__dirname, 'data', 'sample2.tif'))
      })
    })
  })
  describe('statAsync()', () => {
    it('should return information on file system objects', () => {
      const stat = gdal.fs.statAsync(path.resolve(__dirname, 'data', 'sample.tif'))
      return assert.eventually.propertyVal(stat, 'size', 794079)
    })
    it('should support BigInt results', () => {
      const stat = gdal.fs.statAsync(path.resolve(__dirname, 'data', 'sample.tif'), true)
      return assert.eventually.propertyVal(stat, 'size', BigInt(794079))
    })
    it('should reject on non-existent files', () =>
      assert.isRejected(gdal.fs.statAsync(path.resolve(__dirname, 'data', 'sample2.tif'))))
  })
  describe('readDir()', () => {
    it('should return all files in a directory', () => {
      const list = gdal.fs.readDir(path.resolve(__dirname, 'data'))
      assert.include(list, 'sample.tif')
    })
    it('should throw on non-existent directories', () => {
      assert.throws(() => {
        gdal.fs.readDir(path.resolve(__dirname, 'data2'))
      })
    })
  })
  describe('readDirAsync()', () => {
    it('should return all files in a directory', () => {
      const list = gdal.fs.readDirAsync(path.resolve(__dirname, 'data'))
      return assert.eventually.include(list, 'sample.tif')
    })
    it('should throw on non-existent directories', () =>
      assert.isRejected(gdal.fs.readDirAsync(path.resolve(__dirname, 'data2')))
    )
  })
})
