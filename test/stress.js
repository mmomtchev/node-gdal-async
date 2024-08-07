const openDatasets = 10 // 10 raster + 10 vector
const parallelOps = 50
const probabilityToKeepDataset = 0.95
const magicOffset = 98500
const magicData = 75
const magicString = 'L\'Adour, de son embouchure au confluent avec le gave de Pau'

// Add tests here
const tests = [
  async (slot) => {
    const ds = await datasetsRaster[Math.floor(Math.random() * openDatasets)]
    const band = await ds.bands.getAsync(1)
    const rasterSize = await ds.rasterSizeAsync
    const data = await band.pixels.readAsync(0, 0, rasterSize.x, rasterSize.y, staticBuffers[slot])
    assert(data[magicOffset] === magicData)
  },
  () => {
    const ds1q = datasetsRaster[Math.floor(Math.random() * openDatasets)]
    const ds2q = datasetsRaster[Math.floor(Math.random() * openDatasets)]
    const ds3q = datasetsVector[Math.floor(Math.random() * openDatasets)]
    return Promise.all([ ds1q, ds2q, ds3q ])
      // This is a method that artificially stresses the locking
      .then(([ ds1, ds2, ds3 ]) => gdal._acquireLocksAsync(ds1, ds2, ds3))
  },
  () => {
    const dsq = datasetsVector[Math.floor(Math.random() * openDatasets)]
    return dsq.then((ds) => ds.layers.getAsync('sample_int64'))
      .then((l) => l.features.getAsync(0))
      .then((f) => assert(f.fields.toObject().texte_arre === magicString))
  }
  /*() => {
    // This should trigger the event loop warning
    const dsq = datasetsRaster[Math.floor(Math.random() * openDatasets)]
    return dsq.then((ds) => assert(ds.rasterSize.x == size.x))
  }*/
]

const gdal = require('..')
const path = require('path')
const os = require('os')
const assert = require('assert')


const testFileRaster = path.resolve(__dirname, 'data', 'sample.tif')
const testFileVector = path.resolve(__dirname, 'data', 'shp')

const datasetsRaster = new Array(openDatasets)
const datasetsVector = new Array(openDatasets)
const operations = new Array(parallelOps)

const size = { x: gdal.open(testFileRaster).rasterSize.x, y: gdal.open(testFileRaster).rasterSize.y }

// We want to stress node-gdal, not the GC
const staticBuffers = new Array(parallelOps).fill(new Uint8Array(size.x * size.y))

let opens = 0, ops = 0
let stop = false

try {
  gdal.startLogging(`${__dirname}/artifacts/stress.log`)
} catch (_e) {
  /* ignore */
}

async function cleanup() {
  console.log('Ctrl-C... exiting')
  stop = true
}
process.once('SIGHUP', cleanup)
process.once('SIGINT', cleanup)
process.on('unhandledRejection', (reason, promise) => {
  console.error('Unhandled Promise rejection', reason, promise)
  process.exit(1)
})

function operation(slot) {
  if (Math.random() > probabilityToKeepDataset) {
    opens += 2
    const ds = Math.floor(Math.random() * openDatasets)
    datasetsRaster[ds] = gdal.openAsync(testFileRaster)
    datasetsVector[ds] = gdal.openAsync(testFileVector)
  }

  ops++
  if (ops % 1000 == 0) process.stdout.write('.')
  const testFn = tests[Math.floor(Math.random() * tests.length)]
  operations[slot] = testFn(slot).then(() => {
    operations[slot] = undefined
  })
}

for (let i = 0; i < openDatasets; i++) {
  opens += 2
  datasetsRaster[i] = gdal.openAsync(testFileRaster)
  datasetsVector[i] = gdal.openAsync(testFileVector)
}
const cpuUserStart = os.cpus().map((cpu) => cpu.times.user).reduce((a, x) => a + x, 0)
const cpuIdleStart = os.cpus().map((cpu) => cpu.times.idle).reduce((a, x) => a + x, 0)

const timeStart = Date.now()
let timeEnd
if (!process.argv[2] || isNaN(parseInt(process.argv[2]))) {
  console.log('Hit Ctrl-C to stop')
} else {
  timeEnd = parseInt(process.argv[2]) * 1000 + timeStart
}

(async () => {
  try {
    while (!stop && (!timeEnd || Date.now() < timeEnd)) {
      let freeSlot
      do {
        freeSlot = operations.findIndex((op) => !op)
        if (freeSlot == -1) {
        // eslint-disable-next-line no-await-in-loop
          await Promise.race(operations.filter((op) => op))
        }
      } while (freeSlot == -1)

      operation(freeSlot)
    }

    await Promise.all(operations.filter((op) => op))
    console.log(`total opens: ${opens}, total ops: ${ops}, ops/s: ${(ops * 1000 / (Date.now() - timeStart)).toPrecision(3)}`)
    const cpuUserEnd = os.cpus().map((cpu) => cpu.times.user).reduce((a, x) => a + x, 0)
    const cpuIdleEnd = os.cpus().map((cpu) => cpu.times.idle).reduce((a, x) => a + x, 0)
    console.log('parallelization waste: ', ((cpuIdleEnd - cpuIdleStart) / (cpuUserEnd - cpuUserStart)).toPrecision(3))
  } catch (e) {
    console.error(e)
    process.exit(1)
  }
})()
