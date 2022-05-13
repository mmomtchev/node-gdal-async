#include "gdal_fs.hpp"

namespace node_gdal {

/**
 * GDAL VSI layer file operations.
 *
 * @namespace fs
 */

void VSI::Initialize(Local<Object> target) {
  Local<Object> fs = Nan::New<Object>();
  Nan::Set(target, Nan::New("fs").ToLocalChecked(), fs);
  Nan__SetAsyncableMethod(fs, "stat", stat);
  Nan__SetAsyncableMethod(fs, "readDir", readDir);
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
      Nan::EscapableHandleScope scope;

      Local<Object> result = Nan::New<Object>();
      Nan::Set(result, Nan::New("dev").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_dev));
      Nan::Set(result, Nan::New("mode").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_mode));
      Nan::Set(result, Nan::New("nlink").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_nlink));
      Nan::Set(result, Nan::New("uid").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_uid));
      Nan::Set(result, Nan::New("gid").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_gid));
      Nan::Set(result, Nan::New("rdev").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_rdev));

#ifndef WIN32
      Nan::Set(
        result, Nan::New("blksize").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_blksize));
      Nan::Set(result, Nan::New("ino").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_ino));
      Nan::Set(result, Nan::New("size").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_size));
      Nan::Set(result, Nan::New("blocks").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_blocks));
#else
      Nan::Set(result, Nan::New("blksize").ToLocalChecked(), Nan::Undefined());
      Nan::Set(result, Nan::New("ino").ToLocalChecked(), Nan::Undefined());
      Nan::Set(result, Nan::New("size").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_size));
      Nan::Set(result, Nan::New("blocks").ToLocalChecked(), Nan::Undefined());
#endif

      Nan::Set(result, Nan::New("atime").ToLocalChecked(), Nan::New<Date>(stat.st_atime * 1000).ToLocalChecked());
      Nan::Set(result, Nan::New("mtime").ToLocalChecked(), Nan::New<Date>(stat.st_mtime * 1000).ToLocalChecked());
      Nan::Set(result, Nan::New("ctime").ToLocalChecked(), Nan::New<Date>(stat.st_ctime * 1000).ToLocalChecked());

      return scope.Escape(result);
    };
  } else {
    // Ahh, the joy of JavaScript number types
    // Which other language has floating-point file sizes
    // Anyway, 2^53 bytes ought to be enough for anybody
    job.rval = [](VSIStatBufL stat, const GetFromPersistentFunc &) {
      Nan::EscapableHandleScope scope;

      Local<Object> result = Nan::New<Object>();
      Nan::Set(result, Nan::New("dev").ToLocalChecked(), Nan::New<Integer>(static_cast<uint32_t>(stat.st_dev)));
      Nan::Set(result, Nan::New("mode").ToLocalChecked(), Nan::New<Integer>(stat.st_mode));
      Nan::Set(result, Nan::New("nlink").ToLocalChecked(), Nan::New<Integer>(static_cast<uint32_t>(stat.st_nlink)));
      Nan::Set(result, Nan::New("uid").ToLocalChecked(), Nan::New<Integer>(stat.st_uid));
      Nan::Set(result, Nan::New("gid").ToLocalChecked(), Nan::New<Integer>(stat.st_gid));
      Nan::Set(result, Nan::New("rdev").ToLocalChecked(), Nan::New<Integer>(static_cast<uint32_t>(stat.st_rdev)));

#ifndef WIN32
      Nan::Set(result, Nan::New("blksize").ToLocalChecked(), Nan::New<Number>(static_cast<double>(stat.st_blksize)));
      Nan::Set(result, Nan::New("ino").ToLocalChecked(), Nan::New<Number>(static_cast<double>(stat.st_ino)));
      Nan::Set(result, Nan::New("size").ToLocalChecked(), Nan::New<Number>(static_cast<double>(stat.st_size)));
      Nan::Set(result, Nan::New("blocks").ToLocalChecked(), Nan::New<Number>(static_cast<double>(stat.st_blocks)));
#else
      Nan::Set(result, Nan::New("blksize").ToLocalChecked(), Nan::Undefined());
      Nan::Set(result, Nan::New("ino").ToLocalChecked(), Nan::Undefined());
      Nan::Set(result, Nan::New("size").ToLocalChecked(), Nan::New<Number>(static_cast<double>(stat.st_size)));
      Nan::Set(result, Nan::New("blocks").ToLocalChecked(), Nan::Undefined());
#endif

      Nan::Set(result, Nan::New("atime").ToLocalChecked(), Nan::New<Date>(stat.st_atime * 1000).ToLocalChecked());
      Nan::Set(result, Nan::New("mtime").ToLocalChecked(), Nan::New<Date>(stat.st_mtime * 1000).ToLocalChecked());
      Nan::Set(result, Nan::New("ctime").ToLocalChecked(), Nan::New<Date>(stat.st_ctime * 1000).ToLocalChecked());

      return scope.Escape(result);
    };
  }

  job.run(info, async, 2);
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
    Nan::EscapableHandleScope scope;
    Local<Array> results = Nan::New<Array>();
    int i = 0;
    while (names[i] != nullptr) {
      Nan::Set(results, i, SafeString::New(names[i]));
      i++;
    }
    CSLDestroy(names);
    return scope.Escape(results);
  };
  job.run(info, async, 1);
}
} // namespace node_gdal
