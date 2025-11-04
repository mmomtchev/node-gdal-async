# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [3.11.5] 2025-11-04

### Added
 - GDAL 3.11.5

## [3.11.4] 2025-09-12

 - Fix [mmomtchev/node-gdal-async#250](https://github.com/mmomtchev/node-gdal-async/issues/250), fix a leak of `Buffer` objects after using `vsimem.set` and `vsimem.release`

### Added
 - GDAL 3.11.4
 - [mmomtchev/node-gdal-async#233](https://github.com/mmomtchev/node-gdal-async/issues/233): Support SQLite SQL dialect
 - [mmomtchev/node-gdal-async#243](https://github.com/mmomtchev/node-gdal-async/pull/243), support for `Feature.{get|set}StyleString`

### Changed
 - [mmomtchev/node-gdal-async#242](https://github.com/mmomtchev/node-gdal-async/pull/242), TypeScript types correctly reflect that some functions can return `null` geometries

## [3.11.3] 2025-07-13

### Added
 - GDAL 3.11.3

### Changed
 - Fix [mmomtchev/node-gdal-async#221](https://github.com/mmomtchev/node-gdal-async/issues/221):
  * Fix RAM size memory reporting on Linux needed `GDAL_CACHEMAX` with a percentage
  * Fail graciously if the user specifies a GDAL data type not supported by the current version
  * Do not throw an error if GDAL returns a warning in `gdal.checksumImage`

# [3.11.0] 2025-05-10

### Added
 - GDAL 3.11.0
 - Node.js 24 binaries
 - Add `libaec` and AEC encoding support for GRIB2 files - including those provided by Météo France
 - Add the new `LIBERTIFF` driver
 - Initial `Float16` support through the [`@petamoriken/float16`](https://www.npmjs.com/package/@petamoriken/float16) polyfill with GDAL >= 3.11
 - `Int64` and `UInt64` support through `BigInt64Array` and `BigUint64Array` with GDAL >= 3.5
 - Native `ExprTk` support with GDAL 3.11, the old JS `ExprTk` support via the `gdal-exprtk` plugin is still available but won't be maintained anymore

### Changed
 - All `read` and `write` functions now use generics in TypeScript, deduction is automatic in many cases, but if TypeScript cannot deduce the type, it should be specified manually: `const data: Uint8Array = band.pixels.read(0, 0, w, h)` should become `const data = band.pixels.read<Uint8Array>(0, 0, w, h)`, pure JavaScript is not affected
 - Fix the `gdal.info()` example
 - Fix [mmomtchev/node-gdal-async#204](https://github.com/mmomtchev/node-gdal-async/issues/204), define `HAVE_TIFF` for other drivers using built-in TIFF support
 - Partial solution for [mmomtchev/node-gdal-async#217](https://github.com/mmomtchev/node-gdal-async/issues/217)

### Removed
 - `node-gdal-async` now requires ES2020 support
 - Drop all obsolete in GDAL 3.11 drivers, including `SDTS`

## [3.10.2] 2025-03-09

### Added
 - GDAL 3.10.2

## [3.10.1] 2025-01-14

### Added
 - GDAL 3.10.1

### Changed
 - Fix 3.10 package installation

# [3.10.0] 2025-01-13

### Added
 - GDAL 3.10.0
 - GEOS 3.13.0
 - PROJ 9.5.1
 - Apple ARM support on macOS 15 and later
 - Node.js 23 support (requires Ubuntu 20.04/later or Debian 11 Bullseye/later)
 - Implement RFC101 support, see [`ASYNCIO.md`](https://github.com/mmomtchev/node-gdal-async/blob/main/ASYNCIO.md) for more information

### Removed
 - Drop Node.js 16 support
 - Drop macOS 12 support

## [3.9.2] 2024-08-18

### Added
 - GDAL 3.9.2

## [3.9.0] 2024-06-24

### Added
 - GDAL 3.9.0
 - Node.js 22 support

### Changed
 - All shared library symbols are now hidden on Linux, allowing to load the binary addon in a process that has loaded a different version of GDAL (on Windows this has always been possible and on maOS, while possible in theory, this particular linking mode is not supported by `node-gyp`)

### Removed
 - Drop macOS 11 support
 - Drop Node.js 21 support
 - Mark Node.js 16 as obsolete with a warning

## [3.8.5] 2024-04-09

### Added
 - GDAL 3.8.5


## [3.8.4] 2024-02-19

### Added
 - GDAL 3.8.4


## [3.8.3] 2024-01-09

### Added
 - GDAL 3.8.3


## [3.8.2] 2024-01-08

### Added
 - GDAL 3.8.2

## [3.8.1] 2023-12-12

### Added
 - GDAL 3.8.1

### Changed
 - Fix [mmomtchev/node-gdal-async#123](https://github.com/mmomtchev/node-gdal-async/issues/123), `H5version.h` is ignored by `.gitignore` and missing from the NPM tarball

# [3.8.0] 2023-12-11

### Added
 - GDAL 3.8.0 with new `JSONFG`, `PMTiles` and `S102` drivers
 - PROJ 9.3.0
 - GEOS 3.12.1
 - HDF5 1.14.3
 - NetCDF 4.9.2
 - Node.js 21 support

### Changed
 - Upgrade the main test platform from Ubuntu 20.04 to Ubuntu 22.04

### Removed
 - Drop Node.js 14 support

## [3.7.3] 2023-11-28

### Added
 - GDAL 3.7.3

## [3.7.2] 2023-09-21

### Added
 - GDAL 3.7.2

## [3.7.1] 2023-07-30

### Added
 - GDAL 3.7.1

### Changed
 - Fix [#84](https://github.com/mmomtchev/node-gdal-async/issues/84), cannot create an `OpenFileGDB` with default options
 - Fix [#90](https://github.com/mmomtchev/node-gdal-async/issues/90), remove the old 2GB/4GB limit and allocate `TypedArray`s up to the limit allowed by Node.js and fail gracefully on overflow instead of segfaulting
 - Fix [#92](https://github.com/mmomtchev/node-gdal-async/issues/92), align `gdal.ReadOptions` with the documentation and all other structures by calling the type `data_type` while still accepting the old `type`
 - Merge [#94](https://github.com/mmomtchev/node-gdal-async/pull/94), handle `null` as a field value in OGR
 - Fix [#96](https://github.com/mmomtchev/node-gdal-async/issues/96), support PNG compression in GRIB files

# [3.7.0] 2023-05-26

### Added
 - GDAL 3.7.0

## [3.6.4] 2023-05-01

### Added
 - GDAL 3.6.4
 - Node.js 20.x support and precompiled binaries

## [3.6.3] 2023-03-20

### Added
 - GDAL 3.6.3

### Changed
 - Fix [#66](https://github.com/mmomtchev/node-gdal-async/issues/66), include `yatag` as a production dependency
 - Fix [#36](https://github.com/mmomtchev/node-gdal-async/issues/36), find a way around `jest` *resetting* modules between tests

## [3.6.2] 2023-01-09

### Added
 - GDAL 3.6.2
 - `gdal.dem{Async}`, library version of `gdaldem`

### Changed
 - Fix [#56](https://github.com/mmomtchev/node-gdal-async/issues/56), propagate input errors in `calcAsync` and `RasterMuxStream`

## [3.6.1] 2022-12-21

### Added
 - GDAL 3.6.1
 - Fix [#53](https://github.com/mmomtchev/node-gdal-async/issues/53), rebuilding from source fails

# [3.6.0] 2022-12-21
### Added
 - GDAL 3.6.0
 - Node.js 19.x
 - Drop support for glibc < 2.28 distributions (not supported by recent Node.js versions)
 - Drop support for all Amazon Linux 2, replacing it with Amazon Linux 2022
 - Drop support for Ubuntu 16.04 and 18.04
 - Ubuntu 22.04

## [3.5.3] 2022-11-02
### Added
 - GDAL 3.5.3

### Changed
 - Fix #43, wrong TypeScript type for `data_type` in `gdal.Driver.create`

## [3.5.2] 2022-09-23

### Added
 - GDAL 3.5.2

### Changed
 - Fix #38, integrate the fix for GDAL[#5028] (https://github.com/OSGeo/gdal/issues/5028)

## [3.5.1] 2022-07-09

### Added
 - GDAL 3.5.1
 - Add Electron support

# [3.5.0] 2022-05-18

### Added
 - GDAL 3.5.0
 - PROJ 9.0.0
 - NetCDF 4.8.1
 - GEOS 3.10.2
 - Node 18 support
 - API documentation now uses `documentation.js` with `documentation-polyglot`
 - Add `gdal.addPixelFunc` for registering a native code pixel function
 - Add `gdal.toPixelFunc` and `gdal.createPixelFunc` allowing to use a JavaScript function as a pixel function
 - `gdal.calcAsync` can now convert input data to the output type before applying the user function
 - `gdal.calcAsync` can now call a progress callback
 - Add `gdal.buildVRT` and `gdal.rasterize`, library versions of the GDAL CLI tools
 - Add `gdal.wrapVRT` allowing wrapping a regular Dataset inside a VRT Dataset
 - Support using driver-specific open options in `gdal.Driver.open`

### Changed
 - All `gdal.Driver` methods now throw an exception if the parsing of the driver-specific options fails

## [3.4.3] 2021-05-04

### Added
 - Bundled GDAL 3.4.3

## [3.4.2] 2021-03-15

### Added
 - Bundled GDAL 3.4.2

### Changed
 - Fix #27, rebuilding by `npm --build-from-source` fails
 - Fix #25, always close open Datasets when the process exits
 - `gdal.calcAsync` now correctly returns a rejected Promise instead of triggering an uncatchable error when handling an exception in the user-supplied transformation function
 - Fix #26, some methods do not check the passed object class

## [3.4.1] 2021-01-05

### Added
 - Bundled GDAL 3.4.1
 - Bundled OpenJPEG and enabled the JPEG2000 driver

### Changed
 - Fix #19, benchmarks do not execute
 - Fix #20, do not block the event loop in `calcAsync`
 - Fix #21, `gdal.vsimem.copy` doesn't properly deallocate the returned `Buffer` on Windows
 - Remove the documentation reference to the non-existing `copy` argument of `vsimem.set`, use `vsimem.copy` instead
 - Fix a memory leak when throwing an exception in `gdal.Geometry.exportToWKB{Async}`

# [3.4.0] 2021-11-08

### Added
 - Bundled GDAL 3.4.0
 - PROJ 8.2.0
 - GEOS 3.10.0
 - libcurl 7.79.1
 - hdf5 1.12.1
 - Node 17 support and binary packages
 - Streams-based API for reading raster data as a stream of pixels
 - Asynchronous iterators for all collections that have asynchronous getters
 - `gdal.info{Async}`, `gdal.translate{Async}`,`gdal.vectorTranslate{Async}` and `gdal.warp{Async}` - library versions of `gdalinfo`, `gdal_translate`, `ogr2ogr` and `gdalwarp`
 - `gdal.calcAsync`, an alternative implementation of `gdal_calc.py`
 - `gdal.ColorTable` implementing `GDALColorTable` and `gdal.RasterBand.colorTable{Async}`
 - All `gdal.RasterBand` getters now have an async version
 - `gdal.RasterBandPixels.clampBlock` for handling partial edge blocks
 - `gdal.RasterBand.colorInterpretationAsync`
 - `gdal.Dataset.getMetadataAsync` and `gdal.RasterBand.getMetadataAsync`
 - `gdal.Dataset.setMetadata{Async}` and `gdal.RasterBand.setMetadata{Async}`
 - `gdal.SpatialReference.fromURLAsync`, `gdal.SpatialReference.fromCRSURLAsync` and `gdal.SpatialReference.fromUserInputAsync`
 - `gdal.RasterBandOverviews.countAsync`, `gdal.RasterBandOverviews.getAsync` and `gdal.RasterBandOverviews.getBySampleCountAsync`
 - `gdal.fromDataType` and `gdal.toDataType` converters between `TypedArray` constructor and GDAL `dataType`
 - TypeScript bindings now officially support `strict: true`
 - `gdal.vsimem.copy` allowing to copy a `Buffer` to GDAL's own memory heap to create an in-memory file extendable by GDAL
 - `gdal.fs.stat{Async}` and `gdal.fs.readDir{Async}` allowing for querying GDAL VSI filesystem objects

### Changed
 - Requires GDAL >= 2.1 if rebuilding against a system-installed GDAL library
 - (***BREAKING***) Switch the progress callback of `gdal.Driver.createCopy{Async}` from a raw argument to an argument inside an object to differentiate it from a result callback when some options are omitted in async mode
 - (***BREAKING***) `gdal.RasterBand.noDataValue` now returns `NaN` if the original value was `NaN` instead of null; when setting it to `null` it correctly clears the `NoDataValue` instead of setting it to `NaN`; one should still exercise care when using `NaN` as a `NoDataValue` - since even if the binary representation is defined by IEEE 754, different formats, compilers or architectures may exhibit different behavior
 - Fix #16, missing locking for GDAL operations involving multiple datasets when an event loop warning is emitted
 - Enable lazy-binding on macOS, meaning that Release builds on all platforms use lazy-binding now
 - `gdal.Driver.create{Async}` now throw if they fail to parse the options instead of returning `undefined`
 - `gdal.DatasetBands.create{Async}` and `gdal.SpatialReference.fromESRI` now have correct TypeScript signatures, all options lists are supported either in `{object}` or in `string[]` form
 - `gdal.checksumImage{Async}` now throw/reject on error instead of returning 0 if the GDAL method returned an error, but the GDAL method still returns 0 on some errors and does take into account fractional numbers < 1.0
 - Fix a memory leak in `gdal.Dataset.srsAsync`
 - Fix a memory leak in the exception-handling path of the progress callbacks
 - Fix #13, now two different warnings are emitted whether a dataset is closed by `gdal.Dataset.close` or it is destroyed by the GC - the former is indicative of a bug in the user application, while the latter is indicative of a bug in `gdal-async` itself
 - Correctly identify all async getters as being read-only in TypeScript
 - TypeScript signature and documentation entry for `gdal.LayerFields.fromObject`, it did not exist before
 - Correct TypeScript signatures for all `gdal.FeatureFields` iterators, they now return `any` objects instead of a `gdal.fieldValue`
 - Correct TypeScript signature for `gdal.Dataset.srs{Async}` - now it can be `null`
 - Correct TypeScript signature for `gdal.Dataset.geoTransform{Async}` - now it can be `null`
 - Correct TypeScript signatures for `gdal.RasterBand.id{Async}`, `gdal.RasterBand.minimum{Async}`, `gdal.RasterBand.maximum{Async}`, `gdal.RasterBand.offset{Async}`, `gdal.RasterBand.scale{Async}` - now they can be `null`
 - Correct TypeScript signature for `gdal.DatasetLayers.create{Async}` - `srs` and `geometry` are optional and default to `null`
 - Correct TypeScript signature for `gdal.DatasetLayers.copy{Async}`, the first argument is of type `gdal.Layer`
 - Correct TypeScript signature for `gdal.Layer.setSpatialFilter`, it now has two separate signatures, one that accepts a `gdal.Geometry|null` and another that accepts `number, number, number, number`
 - Correct TypeScript signature for `gdal.Layer.setAttributeFilter`, it now accepts `null` as argument
 - Correct TypeScript signature for `gdal.Feature.setGeometry`, it now accepts `null` as argument
 - Correct TypeScript signature for `gdal.Geometry.srs`, now it can be `null`
 - Correct TypeScript signatures for `gdal.SpatialReference.getAuthority{Code,Name}`, now they accept `null` as argument
 - Correct TypeScript signatures for `gdal.config.set` and `gdal.config.get`, now they can accept/return `null`
 - Correct numerous TypeScript signatures for callback functions to match the return type of the function
 - Do not produce persistent temporary files in the unit tests

## [3.3.4] 2021-09-28

### Added
 - Add support for Amazon Linux and restore Ubuntu 16.04 support (glibc 2.23)
 - On Linux, build GDAL with `ENABLE_UFFD` for virtual I/O support on drivers that allow it (currently only NetCDF) to allow accessing NetCDF subdatasets with remote virtual filesystems
 - Add a new npm script - `npm run prune` - not to be confused with `npm prune` - which allows to keep only the module runtime, significantly reducing the size of the installed package

### Changed
 - Fix `gdal.RasterBand.flushAsync()`
 - Implement the previously non-existing `strict` argument of `gdal.Driver.createCopy{Async}()` and move it after `options`
 - Implement progress callbacks in `gdal.Driver.createCopy{Async}()`
 - `gdal.Driver.createCopy{Async}()` now always throws an `Error` / rejects the Promise if the operation failed instead of returning a null object in some cases
 - Fix #14, correctly handle exceptions in progress callbacks and return them to the calling user code
 - Add a previously missing TypeScript signature for `gdal.CoordinateTransformation.transformPoint(point: xyz)`
 - Fix the documentation / TypeScript signature for `gdal.Geometry.segmentize` - it returns `undefined`
 - Removed all source tarballs of the bundled dependencies from the published package to reduce its size

## [3.3.3] 2021-09-07

### Added
 - GDAL 3.3.2

### Changed
 - Fix #9, do not enumerate async getters
 - Fix #12, build-from-source is broken when building through npm
 - Compatibility with `rollup-plugin-natives`
 - Fix #6, `gdal.vsimem.release` is now always safe to use

## [3.3.2] 2021-07-08

### Added
 - GDAL 3.3.1
 - Support async getters
   - Add `gdal.Dataset.rasterSizeAsync`
   - Add `gdal.Dataset.srsAsync`
   - Add `gdal.Dataset.geoTransformAsync`
 - Add `gdal.DatasetBands.getEnvelope()`
 - Add `gdal.vsimem.set` and `gdal.vsimem.release` allowing to load and unload files between a `Buffer` and GDAL's in-memory `vsimem` file system
 - Add `gdal.Geometry.makeValid{Async}()`

### Changed
 - Correctly measure the wall time for the event loop warnings
 - Fix a number of memory leaks in `gdal.SpatialReference.clone()`, `gdal.SpatialReference.cloneGeogCS()` and when constructing a `gdal.SpatialReference` with a `wkt` argument
 - Remove a wasteful reallocation in `gdal.Dataset.root`

## [3.3.1] 2021-06-17

### Changed
 - `gdal.DatasetLayers.get{Async}()` now throws an `Error` if an invalid layer is requested instead of returning a null object
 - `gdal.FeatureDefnFields.get{Async}()` now throws an `Error` if an invalid field definition is requested instead of returning a null object
 - `gdal.FeatureFields.get{Async}()` now throws an `Error` if an invalid field is requested instead of returning a null object
    - it can still return `null` if the field exists in the definition but it is not set in the feature
 - `gdal.GDALDrivers.get()` now throws an `Error` if an invalid driver is requested instead of returning a null object
 - `gdal.GeometryCollectionChildren.get()` now throws an `Error` if an invalid sub-geometry is requested instead of returning a null object
 - `gdal.LayerFeatures.get{Async}()` now throws an `Error` if an invalid feature is requested instead of returning a null object
 - `gdal.LayerFields.get{Async}()` now throws an `Error` if an invalid field is requested instead of returning a null object
 - `gdal.LineStringPoints.get()` now throws an `Error` if an invalid point is requested instead of returning a null object
 - `gdal.PolygonRings.get()` now throws an `Error` if an invalid ring is requested instead of returning a null object
 - All `gdal.Geometry` methods that return a `gdal.Geometry` throw an `Error` on error instead of returning a null object
 - Fix #6, a case of sleeping in the GC while destroying the sibling of an object with an active asynchronous operation
 - Fix a potential race condition in the object store
 - Fix several potential race conditions in `gdal.RasterBand`
 - Correctly throw an `Error` with an empty message if a GDAL method fails without setting the error message instead of the last previously encountered error

# [3.3.0] 2021-06-15

### Added
  - GDAL 3.3.0
  - Read-only support of the new Multidimensional Raster Data Model introduced in GDAL 3.1, requires GDAL >= 3.1
  - NetCDF, GRIB, HDF5, Carto, FlatGeobuf, WMS, WMTS and WCS drivers
  - Support generalized strides in `gdal.RasterBandPixels.{write|read}()`
  - Add `gdal.RasterBandPixels.{set|get}Async()`
  - Add `gdal.RasterBand.computeStatisticsAsync`
  - Add an explicit stderr warning when the GC has to active sleep on an async operation and solve all (known) occurrences
  - Add an explicit stderr warning when user code is calling a synchronous method while an asynchronous operation is running in the background

### Changed
 - Vastly improved locking mechanism allowing better parallelization
 - Linux release builds have lazy binding enabled
 - `gdal.DatasetBands.get{Async}()` now throws an `Error` if an invalid band is requested instead of returning a null object
 - Completely reworked persistent object storage
 - Thread-safe `gdal.RasterBand.fillAsync`
 - Fix a number of cases where an unreferenced `gdal.Dataset` was not protected from the GC during an asynchronous operation

### Removed
 - Drop support for linking against a system-installed GDAL 1.x library
 - Drop support for Ubuntu 16.04
 - Drop support for Node.js 10.x and 15.x
 - Drop support for Python 2
 - Drop support for the drivers removed in GDAL 3.3: AeronavFAA, BNA, E00Grid, HTF, OpenAir, SEGUKOOA, SEGY, SUA and XPlane

## [3.2.3] 2021-06-03

### Added
 - Add async versions of `gdal.Layer.flush`, `gdal.RasterBand.flush` and `gdal.RasterBand.fill`

### Changed
 - Fix #1, locking on some `gdal.Layer` operations: `flush`, `getExtent`, `testCapability`, `setSpatialFilter`, `getSpatialFilter` and most scalar getters

## [3.2.2] 2021-05-25

### Changed
 - Fix a failing unit test on GDAL 1.x (no effect on the bindings)
 - Fix another randomly failing unit test (no effect on the bindings)

## [3.2.1] 2021-05-24

### Changed
 - Fix a number of memory leaks (exception throwing paths in all `gdal.SpatialReference.from*`, `gdal.DatasetBands.create`)

# [3.2.0] 2021-05-21

### Added
 - GDAL 3.2.3
 - Built-in vsicurl and major cloud services (Amazon, Google, Microsoft and Alibaba) support on all platforms
 - Support async_hooks
 - Support progress callbacks
 - Support different algorithms when resampling on-the-fly
 - Add `gdal.Geometry.fromGeoJsonBuffer(Async)()`, a completely parallelizable alternative to `gdal.Geometry.fromGeoJson()` for importing large GeoJSON geometries without blocking the event loop
 - Add `gdal.LayerFeatures.setAsync()`

### Changed
 - `gdal.Geometry.fromGeoJson` now throws an `Error` on error instead of simply returning a null geometry

   Previously all `gdal.Geometry.from*` functions except `fromGeoJson` threw undocumented errors, now all of them, including `fromGeoJson` throw documented errors

## [3.1.1] 2021-05-14

### Changed
  - Fully unit-tested TypeScript definitions with many method signatures fixed
  - Properties without setters are now correctly identified as being read-only in the TypeScript bindings
  - Exported all previously internal `_options` types used for methods such as `RasterBand.read` or `reprojectImage`

## [3.1.0] 2021-05-11

### Added
  - Add full TypeScript support

  - Complete reworking of the Geometry code adding support for CircularString, CompoundCurve and MultiCurve
  - Add JS iterators to all collections
  - Enable MapInfo TAB and MIF/MID
  - Fixed a typo in `gdal.SpatialReference`, `isVectical()` method should be `isVertical()`, the misspelled method will be kept until the next major release
  - Expanded unit testing, code coverage is 81% up from 70%

### Changed
  - Process the unit testing suite with ASAN and fix all memory leaks

### Added
  - Support Node.js 16
  - Merge `node-gdal-next@2.8.0`

## Changed
  - Build GDAL against the internal zlib in Node.js to avoid a conflict causing data corruption
  - Enforce code coverage checks and re-enable all disabled unit tests

## [3.0.1] 2021-04-09

### Changed

  - Follow the Node.js callback error convention, returning `null` instead of `undefined` for the `error` argument on success and returning an `Error` object instead of a string on error
  - Lots of minor documentation fixes

# [3.0.0] 2021-03-26

First release of `node-gdal-async`, forked from `node-gdal-next@2.6.2`

### Added
  - **Full support for asynchronous operations**
  - GDAL 3.2.2
  - vsimem support
  - MVT writing
  - SQLite integration

### Changed
  - Packages built through Github Actions
  - Packages published through Github Packages
  - Numerous small bugfixes
