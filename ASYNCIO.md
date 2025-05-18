# Notes on async I/O parallelism and the event loop

Common pitfalls to avoid when writing server code

## Mixing of synchronous and asynchronous operations (or why does gdal-async complain about blocking the event loop)

*If you are not writing server code that must remain responsive at all times and should never block the event loop for more than a few hundred microseconds, you can safely ignore this warning. In this case you can simply set `gdal.eventLoopWarning = false`.*

Consider the following `async` code:
```js
const ds = await gdal.openAsync('4bands.tif')
const band1 = await ds.bands.getAsync(1)
const band2 = await ds.bands.getAsync(2)
const data1 = band1.pixels.readAsync(0, 0, ds.rasterSize.x, ds.rasterSize.y)
const data2 = band2.pixels.readAsync(0, 0, ds.rasterSize.x, ds.rasterSize.y)
await Promise.all([ data1, data2 ])
```

As this code uses only async operations, one could expect that it will never block the event loop. In fact, it is a very dangerous code to put in a multi-user server as depending on the exact order of execution, the first asynchronous read could lock the dataset before that second one has had the chance to start. In this case, accessing `ds.rasterSize` will be a blocking operation that will have to wait for the first read to complete. As this is a synchronous getter, this will have the effect of completely blocking the event loop until the first read is finished.

In all other cases, accessing a synchronous getter or setter is an instantaneous operation that can be safely used in server code.

Version 3.3.2 introduces async getters that solve this problem in an elegant way:

```js
const ds = await gdal.openAsync('4bands.tif')
const band1 = await ds.bands.getAsync(1)
const band2 = await ds.bands.getAsync(2)
const data1 = band2.pixels.readAsync(0, 0, (await ds.rasterSizeAsync).x, (await ds.rasterSizeAsync).y)
const data2 = band2.pixels.readAsync(0, 0, (await ds.rasterSizeAsync).x, (await ds.rasterSizeAsync).y)
await Promise.all([ data1, data2 ])
```

**As a general rule, never access synchronous getters or setters on a Dataset after starting any I/O operation on that same Dataset. Retrieve all the needed values beforehand or use an async getter whenever one is available.**

## Worker thread starvation

Prior to 3.3, all async I/O was deferred to `Nan::AsyncWorker` which in turn scheduled the I/O work through `libuv`.
Internally, `libuv` uses a thread pool to avoid a costly thread setup and teardown for small jobs. The default size of this pool in Node.js is 4 threads. It can be adjusted by setting the environment variable `UV_THREADPOOL_SIZE`. It is a good idea to raise that value, especially on hosts equipped with more than 4 cores and when doing CPU-bound work such as GDAL.

Consider now the following code:
```js
const ds1 = gdal.open('4bands1.tif')
const data1 = [] // Promise<TypedArray>[]
const width1 = ds1.rasterSize.x
const height1 = ds1.rasterSize.y
for (let band = 1; band <= 4; band++)
  data1[band - 1] = ds1.bands.get(band).pixels.readAsync(0, 0, width1, height1)

const ds2 = gdal.open('4bands2.tif')
const data2 = []  // Promise<TypedArray>[]
const width2 = ds2.rasterSize.x
const height2 = ds2.rasterSize.y
for (let band = 1; band <= 4; band++)
  data2[band - 1] = ds2.bands.get(band).pixels.readAsync(0, 0, width2, height2)

await Promise.all([Promise.all(data1), Promise.all(data2)])
```

It launches 8 parallel operations: 4 reads for one band on 2 datasets of 4 bands each. One could expect that with 4 threads in the pool, this one will be at least partially parallelized. Alas, this is not really the case.

The first loop will schedule 4 jobs on the Node.js event loop. libuv will place them in 4 different threads - allocating all slots. As GDAL does not support multiple concurrent operations on a single Dataset handle - as reading with multiple threads from the same file handle will hardly achieve anything in most cases - these 4 threads will compete for a single mutex. One of them will acquire it, leaving the other 3 threads sleeping while occupying a slot on the thread pool. This is a classical example of *thread starvation*. None of the 4 jobs, scheduled by the second loop, will be able to run as there won't be any free slots left.

As a note, I/O is, most of the time, limited by the I/O bandwidth of the host and performing more than one read or write in parallel won't always result in higher performance. There are two notable exceptions to this rule: network I/O and I/O of very complex (highly compressed) data formats and/or very high speed devices (SSD).

### Solution 1: Increase the thread pool size

The first and easiest solution is to simply raise the value of `UV_THREADPOOL_SIZE`. It is suboptimal - as it launches more threads than needed - and it works only up to a certain point, ie number of threads.

### Solution 2: Manual I/O scheduling

Taking care to never launch more than operation on the same Dataset in parallel is probably the best solution, but it makes parallel reading much more complex and impractical:
```js
const ds1 = gdal.open('4bands1.tif')
const data1 = [] // Promise<TypedArray>[]
data1[0] = Promise.resolve(null)
const width1 = ds1.rasterSize.x
const height1 = ds1.rasterSize.y
for (let band = 1; band <= 4; band++)
  data1[band] = data1[band - 1].then(() =>
    ds1.bands.get(band).pixels.readAsync(0, 0, width1, height1))

const ds2 = gdal.open('4bands2.tif')
const data2 = []  // Promise<TypedArray>[]
data2[0] = Promise.resolve(null)
const width2 = ds2.rasterSize.x
const height2 = ds2.rasterSize.y
for (let band = 1; band <= 4; band++)
  data2[band] = data2[band - 1].then(() =>
    ds2.bands.get(band).pixels.readAsync(0, 0, width2, height2))

await Promise.all(data1[4], data2[4])
```

Here we are chaining all the Promises one on another. Now the read of the second band of the first dataset won't launch until the first one has completed - ensuring that there will be enough free slots on the thread pool for the jobs of the second loop to run.

## SQL layers

SQL layers present a unique challenge when implementing asynchronous bindings - they require holding a lock over the parent Dataset in order to destroy them. This means that if a Dataset with multiple layers has an asynchronous operation running on one of them and the GC decides it is time to reclaim the SQL results layer - there will be only one solution - to completely block the Node.js process until that background operation finishes.

Alas, there are no simple solutions for this issue. `gdal-async`prints a warning to stderr when this happens.

## RFC101 thread-safe datasets with GDAL >= 3.10

GDAL 3.10 introduces a major performance improvement when accessing raster datasets in read-only *threadsafe* mode.

If a raster datasets is opened in threadsafe mode, by specifying the `t` flag:

```js
const ds = gdal.open(`${__dirname}/data/sample.tif`, 'rt')
assert(ds.threadSafe)
```

then all asynchronous operations can run in parallel and can be freely mixed with synchronous operations without ever blocking the event loop.

## `LIBERTIFF` driver with GDAL >= 3.11

The new `LIBERTIFF` driver introduced in GDAL 3.11 is always read-only and thread-safe:

```js
const driver = gdal.drivers.get('LIBERTIFF')
ds = driver.open(path.join(__dirname, 'data/sample.tif'))
assert.strictEqual(ds.driver.description, 'LIBERTIFF')
assert.strictEqual(ds.threadSafe, true)
```

For more information refer to the GDAL [documentation](https://gdal.org/en/stable/drivers/raster/libertiff.html#libertiff-geotiff-file-format).
