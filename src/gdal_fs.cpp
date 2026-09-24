#include "gdal_fs.hpp"

namespace node_gdal {

/**
 * GDAL VSI layer file operations.
 *
 * @namespace fs
 */

/**
 * Clears the /vsicurl/ and related network caches.
 *
 * @static
 * @method clearCurlCache
 * @memberof fs
 */
NAPI_METHOD(VSI::clearCurlCache) {
  VSICurlClearCache();
  return node_gdal::napi_env().Undefined();
}

void VSI::Initialize(Napi::Object target) {
  Napi::Env env = target.Env();

  Napi::Object fs = Napi::Object::New(env);
  target.Set("fs", fs);
  GDAL_SetAsyncableMethod(env, fs, "stat", stat);
  GDAL_SetAsyncableMethod(env, fs, "readDir", readDir);
  GDAL_SetMethod(env, fs, "clearCurlCache", clearCurlCache);
}

/**
 * @typedef {object} VSIStat
 * @memberof fs
 * @property {number} dev
 * @property {number} mode
 * @property {number} nlink
 * @property {number} uid
 * @property {number} gid
 * @property {number} rdev
 * @property {number} blksize
 * @property {number} ino
 * @property {number} size
 * @property {number} blocks
 * @property {Date} atime
 * @property {Date} mtime
 * @property {Date} ctime
 */

/**
 * @typedef {object} VSIStat64
 * @memberof fs
 * @property {BigInt} dev
 * @property {BigInt} mode
 * @property {BigInt} nlink
 * @property {BigInt} uid
 * @property {BigInt} gid
 * @property {BigInt} rdev
 * @property {BigInt} blksize
 * @property {BigInt} ino
 * @property {BigInt} size
 * @property {BigInt} blocks
 * @property {Date} atime
 * @property {Date} mtime
 * @property {Date} ctime
 */

/**
 * Get VSI file info.
 *
 * @example
 *
 * const gdalStats = gdal.fs.stat('/vsis3/noaa-gfs-bdp-pds/gfs.20210918/06/atmos/gfs.t06z.pgrb2.0p25.f010')
 * if ((gdalStats.mode & fs.constants.S_IFREG) === fs.constants.S_IFREG) console.log('is regular file')
 *
 * // convert to Node.js fs.Stats
 * const fsStats = new (Function.prototype.bind.apply(fs.Stats, [null, ...Object.keys(s).map(k => s[k])]))
 * if (fsStats.isFile) console.log('is regular file')
 *
 * @static
 * @method stat
 * @memberof fs
 * @param {string} filename
 * @param {false} [bigint=false] Return BigInt numbers. JavaScript numbers are safe for integers up to 2^53.
 * @throws {Error}
 * @returns {VSIStat}
 */

/**
 * Get VSI file info.
 *
 * @static
 * @method stat
 * @memberof fs
 * @param {string} filename
 * @param {true} True Return BigInt numbers. JavaScript numbers are safe for integers up to 2^53.
 * @throws {Error}
 * @returns {VSIStat64}
 */

/**
 * Get VSI file info.
 * @async
 *
 * @static
 * @method statAsync
 * @memberof fs
 * @param {string} filename
 * @param {false} [bigint=false] Return BigInt numbers. JavaScript numbers are safe for integers up to 2^53.
 * @throws {Error}
 * @param {callback<VSIStat>} [callback=undefined]
 * @returns {Promise<VSIStat>}
 */

/**
 * Get VSI file info.
 * @async
 *
 * @static
 * @method statAsync
 * @memberof fs
 * @param {string} filename
 * @param {true} True Return BigInt numbers. JavaScript numbers are safe for integers up to 2^53.
 * @throws {Error}
 * @param {callback<VSIStat>} [callback=undefined]
 * @returns {Promise<VSIStat>}
 */

GDAL_ASYNCABLE_DEFINE(VSI::stat) {
  std::string filename;
  bool bigint = false;

  NODE_ARG_STR(0, "filename", filename);
  NODE_ARG_BOOL_OPT(1, "bigint", bigint);

  GDALAsyncableJob<VSIStatBufL> job(0);
  job.main = [filename](const GDALExecutionProgress &) {
    VSIStatBufL stat;
    CPLErrorReset();
    int r = VSIStatL(filename.c_str(), &stat);
    if (r < 0) throw CPLGetLastErrorMsg();
    return stat;
  };

  if (bigint) {
    job.rval = [](VSIStatBufL stat, const GetFromPersistentFunc &) {

      Napi::Object result = Napi::Object::New(node_gdal::napi_env());
      result.Set( Napi::String::New(node_gdal::napi_env(), "dev"), Napi::BigInt::New(node_gdal::napi_env(), static_cast<int64_t>(stat.st_dev)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "mode"), Napi::BigInt::New(node_gdal::napi_env(), static_cast<int64_t>(stat.st_mode)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "nlink"), Napi::BigInt::New(node_gdal::napi_env(), static_cast<int64_t>(stat.st_nlink)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "uid"), Napi::BigInt::New(node_gdal::napi_env(), static_cast<int64_t>(stat.st_uid)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "gid"), Napi::BigInt::New(node_gdal::napi_env(), static_cast<int64_t>(stat.st_gid)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "rdev"), Napi::BigInt::New(node_gdal::napi_env(), static_cast<int64_t>(stat.st_rdev)));

#ifndef WIN32
      result.Set( Napi::String::New(node_gdal::napi_env(), "blksize"), Napi::BigInt::New(node_gdal::napi_env(), static_cast<int64_t>(stat.st_blksize)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "ino"), Napi::BigInt::New(node_gdal::napi_env(), static_cast<int64_t>(stat.st_ino)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "size"), Napi::BigInt::New(node_gdal::napi_env(), static_cast<int64_t>(stat.st_size)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "blocks"), Napi::BigInt::New(node_gdal::napi_env(), static_cast<int64_t>(stat.st_blocks)));
#else
      result.Set( Napi::String::New(node_gdal::napi_env(), "blksize"), node_gdal::napi_env().Undefined());
      result.Set( Napi::String::New(node_gdal::napi_env(), "ino"), node_gdal::napi_env().Undefined());
      result.Set( Napi::String::New(node_gdal::napi_env(), "size"), Napi::BigInt::New(node_gdal::napi_env(), static_cast<int64_t>(stat.st_size)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "blocks"), node_gdal::napi_env().Undefined());
#endif

      result.Set( Napi::String::New(node_gdal::napi_env(), "atime"), Napi::Date::New(node_gdal::napi_env(), stat.st_atime * 1000));
      result.Set( Napi::String::New(node_gdal::napi_env(), "mtime"), Napi::Date::New(node_gdal::napi_env(), stat.st_mtime * 1000));
      result.Set( Napi::String::New(node_gdal::napi_env(), "ctime"), Napi::Date::New(node_gdal::napi_env(), stat.st_ctime * 1000));

      return result;
    };
  } else {
    // Ahh, the joy of JavaScript number types
    // Which other language has floating-point file sizes
    // Anyway, 2^53 bytes ought to be enough for anybody
    job.rval = [](VSIStatBufL stat, const GetFromPersistentFunc &) {

      Napi::Object result = Napi::Object::New(node_gdal::napi_env());
      result.Set( Napi::String::New(node_gdal::napi_env(), "dev"), Napi::Number::New(node_gdal::napi_env(), static_cast<uint32_t>(stat.st_dev)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "mode"), Napi::Number::New(node_gdal::napi_env(), stat.st_mode));
      result.Set( Napi::String::New(node_gdal::napi_env(), "nlink"), Napi::Number::New(node_gdal::napi_env(), static_cast<uint32_t>(stat.st_nlink)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "uid"), Napi::Number::New(node_gdal::napi_env(), stat.st_uid));
      result.Set( Napi::String::New(node_gdal::napi_env(), "gid"), Napi::Number::New(node_gdal::napi_env(), stat.st_gid));
      result.Set( Napi::String::New(node_gdal::napi_env(), "rdev"), Napi::Number::New(node_gdal::napi_env(), static_cast<uint32_t>(stat.st_rdev)));

#ifndef WIN32
      result.Set( Napi::String::New(node_gdal::napi_env(), "blksize"), Napi::Number::New(node_gdal::napi_env(), static_cast<double>(stat.st_blksize)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "ino"), Napi::Number::New(node_gdal::napi_env(), static_cast<double>(stat.st_ino)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "size"), Napi::Number::New(node_gdal::napi_env(), static_cast<double>(stat.st_size)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "blocks"), Napi::Number::New(node_gdal::napi_env(), static_cast<double>(stat.st_blocks)));
#else
      result.Set( Napi::String::New(node_gdal::napi_env(), "blksize"), node_gdal::napi_env().Undefined());
      result.Set( Napi::String::New(node_gdal::napi_env(), "ino"), node_gdal::napi_env().Undefined());
      result.Set( Napi::String::New(node_gdal::napi_env(), "size"), Napi::Number::New(node_gdal::napi_env(), static_cast<double>(stat.st_size)));
      result.Set( Napi::String::New(node_gdal::napi_env(), "blocks"), node_gdal::napi_env().Undefined());
#endif

      result.Set( Napi::String::New(node_gdal::napi_env(), "atime"), Napi::Date::New(node_gdal::napi_env(), stat.st_atime * 1000));
      result.Set( Napi::String::New(node_gdal::napi_env(), "mtime"), Napi::Date::New(node_gdal::napi_env(), stat.st_mtime * 1000));
      result.Set( Napi::String::New(node_gdal::napi_env(), "ctime"), Napi::Date::New(node_gdal::napi_env(), stat.st_ctime * 1000));

      return result;
    };
  }

  return job.run(info, async, 2);
}

/**
 * Read file names in a directory.
 *
 * @static
 * @method readDir
 * @memberof fs
 * @param {string} directory
 * @throws {Error}
 * @returns {string[]}
 */

/**
 * Read file names in a directory.
 * @async
 *
 * @static
 * @method readDirAsync
 * @memberof fs
 * @param {string} directory
 * @throws {Error}
 * @param {callback<string[]>} [callback=undefined]
 * @returns {Promise<string[]>}
 */
GDAL_ASYNCABLE_DEFINE(VSI::readDir) {
  std::string directory;

  NODE_ARG_STR(0, "directory", directory);

  GDALAsyncableJob<char **> job(0);
  job.main = [directory](const GDALExecutionProgress &) {
    CPLErrorReset();

    char **names = VSIReadDir(directory.c_str());
    if (names == nullptr) throw CPLGetLastErrorMsg();

    return names;
  };

  job.rval = [](char **names, const GetFromPersistentFunc &) {
    Napi::Array results = Napi::Array::New(node_gdal::napi_env());
    int i = 0;
    while (names[i] != nullptr) {
      results.Set( i, SafeString::New(names[i]));
      i++;
    }
    CSLDestroy(names);
    return results;
  };
  return job.run(info, async, 1);
}
} // namespace node_gdal
