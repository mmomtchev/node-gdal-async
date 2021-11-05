#include "gdal_fs.hpp"

namespace node_gdal {

/**
 * GDAL VSI layer file operations
 *
 * @class gdal.fs
 */

void VSI::Initialize(Local<Object> target) {
  Local<Object> fs = Nan::New<Object>();
  Nan::Set(target, Nan::New("fs").ToLocalChecked(), fs);
  Nan__SetAsyncableMethod(fs, "stat", stat);
}

/**
 * Get VSI file info
 *
 * @static
 * @method stat
 * @param {string} filename
 * @throws Error
 * @returns {gdal.VSIStat}
 */

/**
 * Get VSI file info.
 * {{{async}}}
 *
 * @static
 * @method statAsync
 * @param {string} filename
 * @throws Error
 * @param {callback<gdal.VSIStat>} [callback=undefined] {{{cb}}}
 * @returns {Promise<gdal.VSIStat>}
 */

GDAL_ASYNCABLE_DEFINE(VSI::stat) {
  std::string filename;

  NODE_ARG_STR(0, "filename", filename);

  GDALAsyncableJob<VSIStatBufL> job(0);
  job.main = [filename](const GDALExecutionProgress &) {
    VSIStatBufL stat;
    CPLErrorReset();
    int r = VSIStatL(filename.c_str(), &stat);
    if (r < 0) throw CPLGetLastErrorMsg();
    return stat;
  };

  job.rval = [](VSIStatBufL stat, GetFromPersistentFunc) {
    Nan::EscapableHandleScope scope;

    Local<Object> result = Nan::New<Object>();
    Nan::Set(result, Nan::New("dev").ToLocalChecked(), Nan::New<Integer>(static_cast<uint32_t>(stat.st_dev)));
    Nan::Set(result, Nan::New("mode").ToLocalChecked(), Nan::New<Integer>(stat.st_mode));
    Nan::Set(result, Nan::New("uid").ToLocalChecked(), Nan::New<Integer>(stat.st_uid));
    Nan::Set(result, Nan::New("gid").ToLocalChecked(), Nan::New<Integer>(stat.st_gid));
    Nan::Set(result, Nan::New("rdev").ToLocalChecked(), Nan::New<Integer>(static_cast<uint32_t>(stat.st_rdev)));
    Nan::Set(result, Nan::New("nlink").ToLocalChecked(), Nan::New<Integer>(static_cast<uint32_t>(stat.st_nlink)));

    Nan::Set(result, Nan::New("blksize").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_blksize));
    Nan::Set(result, Nan::New("ino").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_ino));
    Nan::Set(result, Nan::New("size").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_size));
    Nan::Set(result, Nan::New("blocks").ToLocalChecked(), v8::BigInt::New(v8::Isolate::GetCurrent(), stat.st_blocks));

    Nan::Set(result, Nan::New("atime").ToLocalChecked(), Nan::New<Date>(stat.st_atime * 1000).ToLocalChecked());
    Nan::Set(result, Nan::New("mtime").ToLocalChecked(), Nan::New<Date>(stat.st_mtime * 1000).ToLocalChecked());
    Nan::Set(result, Nan::New("ctime").ToLocalChecked(), Nan::New<Date>(stat.st_ctime * 1000).ToLocalChecked());

    return scope.Escape(result);
  };

  job.run(info, async, 1);
}
} // namespace node_gdal