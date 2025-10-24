#include "node_gdal.h"
#include "gdal_dataset.hpp"
#include "gdal_rasterband.hpp"
#include "gdal_algebra.hpp"
#include <string>
#include <cmath>

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 12)

namespace node_gdal {

/**
 * GDAL RasterBand Algebra operations
 *
 * @namespace algebra
 */

namespace Algebra {

void Initialize(Local<Object> target) {
  Local<Object> algebra = Nan::New<Object>();
  Nan::Set(target, Nan::New("algebra").ToLocalChecked(), algebra);

  Nan__SetAsyncableMethod(algebra, "abs", abs);
  Nan__SetAsyncableMethod(algebra, "sqrt", sqrt);
  Nan__SetAsyncableMethod(algebra, "log", log);
  Nan__SetAsyncableMethod(algebra, "log10", log10);
  Nan__SetAsyncableMethod(algebra, "not", gdal_not);

  Nan__SetAsyncableMethod(algebra, "add", add);
  Nan__SetAsyncableMethod(algebra, "sub", sub);
  Nan__SetAsyncableMethod(algebra, "mul", mul);
  Nan__SetAsyncableMethod(algebra, "div", div);
  Nan__SetAsyncableMethod(algebra, "pow", pow);
  Nan__SetAsyncableMethod(algebra, "lt", lt);
  Nan__SetAsyncableMethod(algebra, "lte", lte);
  Nan__SetAsyncableMethod(algebra, "gt", gt);
  Nan__SetAsyncableMethod(algebra, "gte", gte);
  Nan__SetAsyncableMethod(algebra, "eq", eq);
  Nan__SetAsyncableMethod(algebra, "notEq", notEq);
  Nan__SetAsyncableMethod(algebra, "and", gdal_and);
  Nan__SetAsyncableMethod(algebra, "or", gdal_or);

  Nan__SetAsyncableMethod(algebra, "ifThenElse", ifThenElse);

  Nan__SetAsyncableMethod(algebra, "asType", asType);
}

#define NODE_ALGEBRA_ARG(num, var)                                                                                     \
  if (info.Length() < num + 1) {                                                                                       \
    Nan::ThrowError("Two arguments must be given");                                                                    \
    return;                                                                                                            \
  }                                                                                                                    \
  if (info[num]->IsNumber()) {                                                                                         \
    var##Number = Nan::To<double>(info[num]).ToChecked();                                                              \
    var##Band = nullptr;                                                                                               \
  } else if (info[num]->IsObject() && Nan::New(RasterBand::constructor)->HasInstance(info[num])) {                     \
    var##Number = NAN;                                                                                                 \
    var##Band = Nan::ObjectWrap::Unwrap<RasterBand>(info[num].As<Object>());                                           \
  } else {                                                                                                             \
    Nan::ThrowError("Argument must be either a number or a RasterBand");                                               \
    return;                                                                                                            \
  }

#define GDAL_ALGEBRA_UNARY_OP(NAME, OPERATOR)                                                                          \
  GDAL_ASYNCABLE_DEFINE(NAME) {                                                                                        \
    RasterBand *arg;                                                                                                   \
                                                                                                                       \
    NODE_ARG_WRAPPED(0, "Argument", RasterBand, arg);                                                                  \
                                                                                                                       \
    GDALAsyncableJob<GDALRasterBand *> job(arg->parent_uid);                                                           \
    job.persist(arg->handle());                                                                                        \
                                                                                                                       \
    GDALRasterBand *raw = arg->get();                                                                                  \
    job.main = [raw](const GDALExecutionProgress &) { return new GDALComputedRasterBand(OPERATOR(*raw)); };            \
    job.rval = [](GDALRasterBand *r, const GetFromPersistentFunc &) {                                                  \
      /* Create an anonymous object cache entry, it will be immediately */                                             \
      /* referenced as RasterBand.ds. This is a GDALComputedDataset that */                                            \
      /* should not be closed. */                                                                                      \
      Dataset::New(r->GetDataset(), nullptr, false);                                                                   \
      return RasterBand::New(r, r->GetDataset());                                                                      \
    };                                                                                                                 \
    job.run(info, async, 1);                                                                                           \
  }

#define GDAL_ALGEBRA_BINARY_OP(NAME, OPERATOR)                                                                         \
  GDAL_ASYNCABLE_DEFINE(NAME) {                                                                                        \
    RasterBand *arg1Band, *arg2Band;                                                                                   \
    double arg1Number, arg2Number;                                                                                     \
                                                                                                                       \
    NODE_ALGEBRA_ARG(0, arg1);                                                                                         \
    NODE_ALGEBRA_ARG(1, arg2);                                                                                         \
                                                                                                                       \
    std::vector<long> ds_uids = {};                                                                                    \
    if (arg1Band) ds_uids.push_back(arg1Band->parent_uid);                                                             \
    if (arg2Band) ds_uids.push_back(arg2Band->parent_uid);                                                             \
    GDALAsyncableJob<GDALRasterBand *> job(ds_uids);                                                                   \
    if (arg1Band) job.persist(arg1Band->handle());                                                                     \
    if (arg2Band) job.persist(arg2Band->handle());                                                                     \
                                                                                                                       \
    if (arg1Band && arg2Band) {                                                                                        \
      GDALRasterBand *arg1 = arg1Band->get();                                                                          \
      GDALRasterBand *arg2 = arg2Band->get();                                                                          \
      job.main = [arg1, arg2](const GDALExecutionProgress &) {                                                         \
        return new GDALComputedRasterBand(OPERATOR(*arg1, *arg2));                                                     \
      };                                                                                                               \
    } else if (arg1Band) {                                                                                             \
      GDALRasterBand *arg1 = arg1Band->get();                                                                          \
      double arg2 = arg2Number;                                                                                        \
      job.main = [arg1, arg2](const GDALExecutionProgress &) {                                                         \
        return new GDALComputedRasterBand(OPERATOR(*arg1, arg2));                                                      \
      };                                                                                                               \
    } else if (arg2Band) {                                                                                             \
      double arg1 = arg1Number;                                                                                        \
      GDALRasterBand *arg2 = arg2Band->get();                                                                          \
      job.main = [arg1, arg2](const GDALExecutionProgress &) {                                                         \
        return new GDALComputedRasterBand(OPERATOR(arg1, *arg2));                                                      \
      };                                                                                                               \
    } else {                                                                                                           \
      Nan::ThrowError("At least one RasterBand must be given");                                                        \
      return;                                                                                                          \
    }                                                                                                                  \
                                                                                                                       \
    job.rval = [](GDALRasterBand *r, const GetFromPersistentFunc &) {                                                  \
      /* Create an anonymous object cache entry, it will be immediately */                                             \
      /* referenced as RasterBand.ds. This is a GDALComputedDataset that */                                            \
      /* should not be closed. */                                                                                      \
      Dataset::New(r->GetDataset(), nullptr, false);                                                                   \
      return RasterBand::New(r, r->GetDataset());                                                                      \
    };                                                                                                                 \
    job.run(info, async, 2);                                                                                           \
  }

/**
 * Create a RasterBand that is the absolute value of the argument.
 *
 * @throws {Error}
 * @method abs
 * @static
 * @memberof algebra
 * @param {RasterBand} arg Aargument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the absolute value of the argument.
 * @async
 *
 * @throws {Error}
 * @method absAsync
 * @static
 * @memberof algebra
 * @param {RasterBand} arg Argument.
 * @return {Promise<RasterBand>}
 */
GDAL_ALGEBRA_UNARY_OP(abs, gdal::abs)

/**
 * Create a RasterBand that is the square root of the argument.
 *
 * @throws {Error}
 * @method sqrt
 * @static
 * @memberof algebra
 * @param {RasterBand} arg Aargument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the square root of the argument.
 * @async
 *
 * @throws {Error}
 * @method sqrtAsync
 * @static
 * @memberof algebra
 * @param {RasterBand} arg Argument.
 * @return {Promise<RasterBand>}
 */
GDAL_ALGEBRA_UNARY_OP(sqrt, gdal::sqrt)

/**
 * Create a RasterBand that is the natural logarithm of the argument.
 *
 * @throws {Error}
 * @method log
 * @static
 * @memberof algebra
 * @param {RasterBand} arg Aargument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the natural logarithm of the argument.
 * @async
 *
 * @throws {Error}
 * @method logAsync
 * @static
 * @memberof algebra
 * @param {RasterBand} arg Argument.
 * @return {Promise<RasterBand>}
 */
GDAL_ALGEBRA_UNARY_OP(log, gdal::log)

/**
 * Create a RasterBand that is the logarithm base 10 of the argument.
 *
 * @throws {Error}
 * @method log10
 * @static
 * @memberof algebra
 * @param {RasterBand} arg Aargument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the logarithm base 10 of the argument.
 * @async
 *
 * @throws {Error}
 * @method log10Async
 * @static
 * @memberof algebra
 * @param {RasterBand} arg Argument.
 * @return {Promise<RasterBand>}
 */
GDAL_ALGEBRA_UNARY_OP(log10, gdal::log10)

/**
 * Create a RasterBand that is the logical not of the argument.
 *
 * @throws {Error}
 * @method not
 * @static
 * @memberof algebra
 * @param {RasterBand} arg Aargument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the logical not of the argument.
 * @async
 *
 * @throws {Error}
 * @method notAsync
 * @static
 * @memberof algebra
 * @param {RasterBand} arg Argument.
 * @return {Promise<RasterBand>}
 */
#define OP(x) (!(x))
GDAL_ALGEBRA_UNARY_OP(gdal_not, OP)
#undef OP

/**
 * Create a RasterBand that is the sum of the arguments.
 *
 * @throws {Error}
 * @method add
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the sum of the arguments.
 * @async
 *
 * @throws {Error}
 * @method addAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
#define OP(a, b) ((a) + (b))
GDAL_ALGEBRA_BINARY_OP(add, OP)
#undef OP

/**
 * Create a RasterBand that is the difference of the arguments.
 *
 * @throws {Error}
 * @method sub
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the difference of the arguments.
 * @async
 *
 * @throws {Error}
 * @method subAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
#define OP(a, b) ((a) - (b))
GDAL_ALGEBRA_BINARY_OP(sub, OP)
#undef OP

/**
 * Create a RasterBand that is the product of the arguments.
 *
 * @throws {Error}
 * @method mul
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the multiply of the arguments.
 * @async
 *
 * @throws {Error}
 * @method mul
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
#define OP(a, b) ((a) * (b))
GDAL_ALGEBRA_BINARY_OP(mul, OP)
#undef OP

/**
 * Create a RasterBand that is the division of the arguments.
 *
 * @throws {Error}
 * @method div
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the division of the arguments.
 * @async
 *
 * @throws {Error}
 * @method divAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
#define OP(a, b) ((a) / (b))
GDAL_ALGEBRA_BINARY_OP(div, OP)
#undef OP

/**
 * Create a RasterBand that is the rising to the power of the arguments.
 *
 * @throws {Error}
 * @method pow
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the rising to the power of the arguments.
 * @async
 *
 * @throws {Error}
 * @method powAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
GDAL_ALGEBRA_BINARY_OP(pow, gdal::pow)

/**
 * Create a RasterBand that is the result of the comparison of the arguments.
 *
 * @throws {Error}
 * @method lt
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the result of the comparison of the arguments.
 * @async
 *
 * @throws {Error}
 * @method ltAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
#define OP(a, b) ((a) < (b))
GDAL_ALGEBRA_BINARY_OP(lt, OP)
#undef OP

/**
 * Create a RasterBand that is the result of the comparison of the arguments.
 *
 * @throws {Error}
 * @method lte
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the result of the comparison of the arguments.
 * @async
 *
 * @throws {Error}
 * @method lteAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
#define OP(a, b) ((a) <= (b))
GDAL_ALGEBRA_BINARY_OP(lte, OP)
#undef OP

/**
 * Create a RasterBand that is the result of the comparison of the arguments.
 *
 * @throws {Error}
 * @method gt
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the result of the comparison of the arguments.
 * @async
 *
 * @throws {Error}
 * @method gtAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
#define OP(a, b) ((a) > (b))
GDAL_ALGEBRA_BINARY_OP(gt, OP)
#undef OP

/**
 * Create a RasterBand that is the result of the comparison of the arguments.
 *
 * @throws {Error}
 * @method gte
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the result of the comparison of the arguments.
 * @async
 *
 * @throws {Error}
 * @method gteAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
#define OP(a, b) ((a) >= (b))
GDAL_ALGEBRA_BINARY_OP(gte, OP)
#undef OP

/**
 * Create a RasterBand that is the result of the comparison of the arguments.
 *
 * @throws {Error}
 * @method eq
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the result of the comparison of the arguments.
 * @async
 *
 * @throws {Error}
 * @method eqAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
#define OP(a, b) ((a) == (b))
GDAL_ALGEBRA_BINARY_OP(eq, OP)
#undef OP

/**
 * Create a RasterBand that is the result of the comparison of the arguments.
 *
 * @throws {Error}
 * @method notEq
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the result of the comparison of the arguments.
 * @async
 *
 * @throws {Error}
 * @method notEqAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
#define OP(a, b) ((a) != (b))
GDAL_ALGEBRA_BINARY_OP(notEq, OP)
#undef OP

/**
 * Create a RasterBand that is the result of the logical AND of the arguments.
 *
 * @throws {Error}
 * @method and
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the result of the logical AND of the arguments.
 * @async
 *
 * @throws {Error}
 * @method andAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
#define OP(a, b) ((a) && (b))
GDAL_ALGEBRA_BINARY_OP(gdal_and, OP)
#undef OP

/**
 * Create a RasterBand that is the result of the logical OR of the arguments.
 *
 * @throws {Error}
 * @method or
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the result of the logical OR of the arguments.
 * @async
 *
 * @throws {Error}
 * @method orAsync
 * @static
 * @memberof algebra
 * @param {RasterBand | number} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @return {Promise<RasterBand>}
 */
#define OP(a, b) ((a) || (b))
GDAL_ALGEBRA_BINARY_OP(gdal_or, OP)
#undef OP

/**
 * Create a RasterBand that is the result of the ternary operator of the arguments.
 *
 * @throws {Error}
 * @method ifThenElse
 * @static
 * @memberof algebra
 * @param {RasterBand} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @param {RasterBand | number} arg3 Third argument.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the result of the ternary operator of the arguments.
 * @async
 *
 * @throws {Error}
 * @method ifThenElseAsync
 * @static
 * @memberof algebra
 * @param {RasterBand} arg1 First argument.
 * @param {RasterBand | number} arg2 Second argument.
 * @param {RasterBand | number} arg3 Third argument.
 * @return {Promise<RasterBand>}
 */
GDAL_ASYNCABLE_DEFINE(ifThenElse) {
  RasterBand *arg1Band, *arg2Band, *arg3Band;
  double arg2Number, arg3Number;

  NODE_ARG_WRAPPED(0, "First argument", RasterBand, arg1Band);
  NODE_ALGEBRA_ARG(1, arg2);
  NODE_ALGEBRA_ARG(2, arg3);

  std::vector<long> ds_uids = {arg1Band->parent_uid};
  if (arg2Band) ds_uids.push_back(arg2Band->parent_uid);
  if (arg3Band) ds_uids.push_back(arg3Band->parent_uid);
  GDALAsyncableJob<GDALRasterBand *> job(ds_uids);
  job.persist(arg1Band->handle());
  if (arg2Band) job.persist(arg2Band->handle());
  if (arg3Band) job.persist(arg3Band->handle());

  GDALRasterBand *arg1 = arg1Band->get();
  if (arg2Band && arg3Band) {
    GDALRasterBand *arg2 = arg2Band->get();
    GDALRasterBand *arg3 = arg3Band->get();
    job.main = [arg1, arg2, arg3](const GDALExecutionProgress &) {
      return new GDALComputedRasterBand(gdal::IfThenElse(*arg1, *arg2, *arg3));
    };
  } else if (arg2Band) {
    GDALRasterBand *arg2 = arg2Band->get();
    double arg3 = arg3Number;
    job.main = [arg1, arg2, arg3](const GDALExecutionProgress &) {
      return new GDALComputedRasterBand(gdal::IfThenElse(*arg1, *arg2, arg3));
    };
  } else if (arg3Band) {
    double arg2 = arg2Number;
    GDALRasterBand *arg3 = arg3Band->get();
    job.main = [arg1, arg2, arg3](const GDALExecutionProgress &) {
      return new GDALComputedRasterBand(gdal::IfThenElse(*arg1, arg2, *arg3));
    };
  } else {
    double arg2 = arg2Number;
    double arg3 = arg3Number;
    job.main = [arg1, arg2, arg3](const GDALExecutionProgress &) {
      return new GDALComputedRasterBand(gdal::IfThenElse(*arg1, arg2, arg3));
    };
  }
  job.rval = [](GDALRasterBand *r, const GetFromPersistentFunc &) {
    Dataset::New(r->GetDataset(), nullptr, false);
    return RasterBand::New(r, r->GetDataset());
  };
  job.run(info, async, 3);
}

/**
 * Create a RasterBand with data converted to a new type.
 *
 * @throws {Error}
 * @method asType
 * @static
 * @memberof algebra
 * @param {RasterBand} arg Argument.
 * @param {string} type GDAL data type
 * @return {RasterBand}
 */

/**
 * Create a RasterBand with data converted to a new type.
 * @async
 *
 * @throws {Error}
 * @method asTypeAsync
 * @static
 * @memberof algebra
 * @param {RasterBand} arg Argument.
 * @param {string} type GDAL data type
 * @return {Promise<RasterBand>}
 */
GDAL_ASYNCABLE_DEFINE(asType) {
  RasterBand *argBand;
  std::string type_name;
  GDALDataType type;

  NODE_ARG_WRAPPED(0, "Argument", RasterBand, argBand);

  NODE_ARG_STR(1, "Data Type", type_name);
  type = GDALGetDataTypeByName(type_name.c_str());
  if (type == GDT_Unknown) {
    Nan::ThrowError("Invalid data type");
    return;
  }

  GDALAsyncableJob<GDALRasterBand *> job(argBand->parent_uid);
  job.persist(argBand->handle());
  GDALRasterBand *arg = argBand->get();

  job.main = [arg, type](const GDALExecutionProgress &) { return new GDALComputedRasterBand(arg->AsType(type)); };

  job.rval = [](GDALRasterBand *r, const GetFromPersistentFunc &) {
    Dataset::New(r->GetDataset(), nullptr, false);
    return RasterBand::New(r, r->GetDataset());
  };
  job.run(info, async, 2);
}

} // namespace Algebra
} // namespace node_gdal

#endif
