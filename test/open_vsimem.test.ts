import * as gdal from 'gdal-async'
import * as path from 'path'
import * as fs from 'fs'
import * as chai from 'chai'
import * as chaiAsPromised from 'chai-as-promised'
const assert: Chai.Assert = chai.assert
chai.use(chaiAsPromised)

describe('Open', () => {
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  describe('vsimem/open', () => {
    let filename, ds: gdal.Dataset, buffer: Buffer
    /* eslint-disable-next-line @typescript-eslint/no-explicit-any */

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/park.geo.json')
      buffer = fs.readFileSync(filename)
      ds = gdal.open(buffer)
    })
    it('should be able to read band count', () => {
      assert.equal(ds.layers.count(), 1)
    })
    it('should keep the buffer in the dataset', () => {
      // eslint-disable-next-line @typescript-eslint/no-explicit-any
      assert.instanceOf((ds as any).buffer, Buffer)
      // eslint-disable-next-line @typescript-eslint/no-explicit-any
      assert.equal((ds as any).buffer, buffer)
    })
    it('should throw on an empty buffer', () => {
      const buffer2 = Buffer.alloc(0)
      assert.throws(() => gdal.open(buffer2))
    })
    it('should throw on an invalid buffer', () => {
      const buffer2 = Buffer.alloc(1024)
      assert.throws(() => gdal.open(buffer2))
    })
    it('should be shareable across datasets', () => {
      const ds2 = gdal.open(buffer)
      /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
      assert.equal((ds2 as any).buffer, (ds as any).buffer)
      /* eslint-disable-next-line @typescript-eslint/no-explicit-any */
      ds2.close()
    })
    it('layer should have all fields defined', () => {
      const layer = ds.layers.get(0)
      assert.equal(layer.fields.count(), 3)
      assert.deepEqual(layer.fields.getNames(), [
        'kind',
        'name',
        'state'
      ])
    })
  })
  describe('vsimem/openAsync', () => {
    let filename, ds: Promise<gdal.Dataset>, buffer: Buffer
    after(() => ds.then((r) => {
      r.close()
      // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
      global.gc!()
    }))
    // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
    afterEach(global.gc!)

    it('should not throw', () => {
      filename = path.join(__dirname, 'data/park.geo.json')
      buffer = fs.readFileSync(filename)
      ds = gdal.openAsync(buffer)
    })
    it('should be able to read band count', () =>
      assert.eventually.equal(ds.then((ds) => ds.layers.count()), 1)
    )
    it('should keep the buffer in the dataset', () =>
      // eslint-disable-next-line @typescript-eslint/no-explicit-any
      Promise.all([ assert.eventually.instanceOf(ds.then((ds) => (ds as any).buffer), Buffer),
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        assert.eventually.equal(ds.then((ds) => (ds as any).buffer), buffer)
      ])
    )
    it('should throw on an empty buffer', () => {
      const buffer2 = Buffer.alloc(0)
      return assert.isRejected(gdal.openAsync(buffer2))
    })
    it('should throw on an invalid buffer', () => {
      const buffer2 = Buffer.alloc(1024)
      return assert.isRejected(gdal.openAsync(buffer2))
    })
  })
})

describe('gdal.vsimem', () => {
  // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
  afterEach(global.gc!)

  describe('set()', () => {
    it('should create a vsimem file from a Buffer', () => {
      const buffer_in = fs.readFileSync(path.join(__dirname, 'data/park.geo.json'))
      gdal.vsimem.set(buffer_in, '/vsimem/park.geo.json')
      const ds = gdal.open('/vsimem/park.geo.json')
      ds.close()
      const buffer_out = gdal.vsimem.release('/vsimem/park.geo.json')
      assert.strictEqual(buffer_in, buffer_out)
    })
    it('should throw if the buffer is not a Buffer', () => {
      assert.throws(() => {
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        gdal.vsimem.set(({}) as any, '/vsimem/park.geo.json')
      })
    })
  })

  describe('copy()', () => {
    it('should create a vsimem file from a Buffer', () => {
      const buffer_in = fs.readFileSync(path.join(__dirname, 'data/park.geo.json'))
      gdal.vsimem.copy(buffer_in, '/vsimem/park.geo.json')
      const ds = gdal.open('/vsimem/park.geo.json')
      ds.close()
      const buffer_out = gdal.vsimem.release('/vsimem/park.geo.json')
      assert.deepEqual(buffer_in, buffer_out)
    })
    it('should throw if the buffer is not a Buffer', () => {
      assert.throws(() => {
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        gdal.vsimem.copy(({}) as any, '/vsimem/park.geo.json')
      })
    })
  })

  describe('release()', () => {
    it('should allow to retrieve the contents from a vsimem', () => {
      const size = 64
      const ds = gdal.open('/vsimem/temp1.tiff', 'w', 'GTiff', size, size, 1, gdal.GDT_Byte)
      const data = new Uint8Array(size * size)
      for (let i = 0; i < data.length; i ++) data[i] = Math.round(Math.random() * 256)
      ds.bands.get(1).pixels.write(0, 0, ds.rasterSize.x, ds.rasterSize.y, data)
      ds.close()
      const buffer = gdal.vsimem.release('/vsimem/temp1.tiff')
      assert.instanceOf(buffer, Buffer)

      const tmpName = path.join(__dirname, 'data', 'temp', `temp_vsimem_${Date.now()}.tiff`)
      fs.writeFileSync(tmpName, buffer)

      const tmp = gdal.open(tmpName)
      assert.instanceOf(tmp, gdal.Dataset)
      assert.deepEqual(tmp.rasterSize, { x: size, y: size })
      assert.deepEqual(tmp.bands.get(1).pixels.read(0, 0, size, size), data)

      tmp.close()
      fs.unlinkSync(tmpName)
    })
    it('should throw if the file is not a vsimem file', () => {
      assert.throws(() => {
        gdal.vsimem.release('park.geo.json')
      })
    })
  })
})
