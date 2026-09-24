#include "node_gdal.h"
#include "gdal_dataset.hpp"
#include "gdal_rasterband.hpp"
#include "gdal_algebra.hpp"
#include <string>
#include <cmath>
#include <memory>

#if GDAL_VERSION_MAJOR > 3 || (GDAL_VERSION_MAJOR == 3 && GDAL_VERSION_MINOR >= 12)

namespace node_gdal {

/**
 * GDAL RasterBand Algebra operations
 *
 * @namespace algebra
 */

namespace Algebra {

void Initialize(Napi::Object target) {
  Napi::Env env = target.Env();
  Napi::Object algebra = Napi::Object::New(env);
  target.Set("algebra", algebra);

  // Unary ops
  GDAL_SetAsyncableMethod(env, algebra, "abs", abs);
  GDAL_SetAsyncableMethod(env, algebra, "sqrt", sqrt);
  GDAL_SetAsyncableMethod(env, algebra, "log", log);
  GDAL_SetAsyncableMethod(env, algebra, "log10", log10);
  GDAL_SetAsyncableMethod(env, algebra, "not", gdal_not);

  // Binary ops
  GDAL_SetAsyncableMethod(env, algebra, "add", add);
  GDAL_SetAsyncableMethod(env, algebra, "sub", sub);
  GDAL_SetAsyncableMethod(env, algebra, "mul", mul);
  GDAL_SetAsyncableMethod(env, algebra, "div", div);
  GDAL_SetAsyncableMethod(env, algebra, "pow", pow);
  GDAL_SetAsyncableMethod(env, algebra, "lt", lt);
  GDAL_SetAsyncableMethod(env, algebra, "lte", lte);
  GDAL_SetAsyncableMethod(env, algebra, "gt", gt);
  GDAL_SetAsyncableMethod(env, algebra, "gte", gte);
  GDAL_SetAsyncableMethod(env, algebra, "eq", eq);
  GDAL_SetAsyncableMethod(env, algebra, "notEq", notEq);
  GDAL_SetAsyncableMethod(env, algebra, "and", gdal_and);
  GDAL_SetAsyncableMethod(env, algebra, "or", gdal_or);

  // Variadic ops
  GDAL_SetAsyncableMethod(env, algebra, "min", gdal_min);
  GDAL_SetAsyncableMethod(env, algebra, "max", gdal_max);
  GDAL_SetAsyncableMethod(env, algebra, "mean", gdal_mean);

  // Ternary op
  GDAL_SetAsyncableMethod(env, algebra, "ifThenElse", ifThenElse);

  // Special case
  GDAL_SetAsyncableMethod(env, algebra, "asType", asType);
}

#define NODE_ALGEBRA_ARG(num, var)                                                                                     \
  if (info.Length() < num + 1) {                                                                                       \
    Napi::Error::New(node_gdal::napi_env(), "Two arguments must be given").ThrowAsJavaScriptException();                                                                    \
    return node_gdal::napi_env().Undefined();                                                                                                            \
  }                                                                                                                    \
  if (info[num].IsNumber()) {                                                                                         \
    var##_number = info[num].As<Napi::Number>().DoubleValue();                                                             \
    var##_band = nullptr;                                                                                              \
  } else if (info[num].IsObject() && info[num].As<Napi::Object>().InstanceOf(RasterBand::constructor.Value())) {                     \
    var##_number = NAN;                                                                                                \
    var##_band = node_gdal::UnwrapWrapped<RasterBand>(info[num].As<Napi::Object>());                                          \
  } else {                                                                                                             \
    Napi::Error::New(node_gdal::napi_env(), "Argument must be either a number or a RasterBand").ThrowAsJavaScriptException();                                               \
    return node_gdal::napi_env().Undefined();                                                                          \
  }

#define GDAL_ALGEBRA_UNARY_OP(NAME, OPERATOR)                                                                          \
  GDAL_ASYNCABLE_DEFINE(NAME) {                                                                                        \
    RasterBand *arg;                                                                                                   \
                                                                                                                       \
    NODE_ARG_WRAPPED(0, "Argument", RasterBand, arg);                                                                  \
                                                                                                                       \
    GDALAsyncableJob<GDALRasterBand *> job(arg->parent_uid);                                                           \
    job.persist(arg->Value());                                                                                        \
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
    return job.run(info, async, 1);                                                                                           \
  }

#define GDAL_ALGEBRA_BINARY_OP(NAME, OPERATOR)                                                                         \
  GDAL_ASYNCABLE_DEFINE(NAME) {                                                                                        \
    RasterBand *arg1_band, *arg2_band;                                                                                 \
    double arg1_number, arg2_number;                                                                                   \
                                                                                                                       \
    NODE_ALGEBRA_ARG(0, arg1);                                                                                         \
    NODE_ALGEBRA_ARG(1, arg2);                                                                                         \
                                                                                                                       \
    std::vector<long> ds_uids = {};                                                                                    \
    if (arg1_band) ds_uids.push_back(arg1_band->parent_uid);                                                           \
    if (arg2_band) ds_uids.push_back(arg2_band->parent_uid);                                                           \
    GDALAsyncableJob<GDALRasterBand *> job(ds_uids);                                                                   \
    if (arg1_band) job.persist(arg1_band->Value());                                                                   \
    if (arg2_band) job.persist(arg2_band->Value());                                                                   \
                                                                                                                       \
    if (arg1_band && arg2_band) {                                                                                      \
      GDALRasterBand *arg1 = arg1_band->get();                                                                         \
      GDALRasterBand *arg2 = arg2_band->get();                                                                         \
      job.main = [arg1, arg2](const GDALExecutionProgress &) {                                                         \
        return new GDALComputedRasterBand(OPERATOR(*arg1, *arg2));                                                     \
      };                                                                                                               \
    } else if (arg1_band) {                                                                                            \
      GDALRasterBand *arg1 = arg1_band->get();                                                                         \
      double arg2 = arg2_number;                                                                                       \
      job.main = [arg1, arg2](const GDALExecutionProgress &) {                                                         \
        return new GDALComputedRasterBand(OPERATOR(*arg1, arg2));                                                      \
      };                                                                                                               \
    } else if (arg2_band) {                                                                                            \
      double arg1 = arg1_number;                                                                                       \
      GDALRasterBand *arg2 = arg2_band->get();                                                                         \
      job.main = [arg1, arg2](const GDALExecutionProgress &) {                                                         \
        return new GDALComputedRasterBand(OPERATOR(arg1, *arg2));                                                      \
      };                                                                                                               \
    } else {                                                                                                           \
      Napi::Error::New(node_gdal::napi_env(), "At least one RasterBand must be given").ThrowAsJavaScriptException();                                                        \
      return node_gdal::napi_env().Undefined();                                                                        \
    }                                                                                                                  \
                                                                                                                       \
    job.rval = [](GDALRasterBand *r, const GetFromPersistentFunc &) {                                                  \
      /* Create an anonymous object cache entry, it will be immediately */                                             \
      /* referenced as RasterBand.ds. This is a GDALComputedDataset that */                                            \
      /* should not be closed. */                                                                                      \
      Dataset::New(r->GetDataset(), nullptr, false);                                                                   \
      return RasterBand::New(r, r->GetDataset());                                                                      \
    };                                                                                                                 \
    return job.run(info, async, 2);                                                                                           \
  }

#define GDAL_ALGEBRA_VARIADIC_BANDONLY_OP(NAME, OPERATOR)                                                              \
  GDAL_ASYNCABLE_DEFINE(NAME) {                                                                                        \
    std::vector<RasterBand *> args_band;                                                                               \
    std::vector<long> uids;                                                                                            \
                                                                                                                       \
    for (int i = 0; i < info.Length(); i++) {                                                                          \
      if (info[i].IsFunction()) break;                                                                                \
      RasterBand *arg_band;                                                                                            \
      NODE_ARG_WRAPPED(i, "Argument", RasterBand, arg_band);                                                           \
      args_band.push_back(arg_band);                                                                                   \
      uids.push_back(arg_band->parent_uid);                                                                            \
    }                                                                                                                  \
    if (args_band.size() < 2) {                                                                                        \
      Napi::Error::New(node_gdal::napi_env(), "At least two arguments must be given").ThrowAsJavaScriptException();                                                         \
      return node_gdal::napi_env().Undefined();                                                                        \
    }                                                                                                                  \
    GDALAsyncableJob<GDALRasterBand *> job(uids);                                                                      \
    GDALRasterBandH *handles = new GDALRasterBandH[args_band.size()];                                                  \
    size_t i = 0;                                                                                                      \
    for (auto arg : args_band) {                                                                                       \
      job.persist(arg->Value());                                                                                      \
      handles[i++] = GDALRasterBand::ToHandle(arg->get());                                                             \
    }                                                                                                                  \
    auto args = std::shared_ptr<GDALRasterBandH[]>{handles};                                                           \
                                                                                                                       \
    job.main = [args, i](const GDALExecutionProgress &) {                                                              \
      return GDALRasterBand::FromHandle(OPERATOR(i, args.get()));                                                      \
    };                                                                                                                 \
                                                                                                                       \
    job.rval = [](GDALRasterBand *r, const GetFromPersistentFunc &) {                                                  \
      /* Create an anonymous object cache entry, it will be immediately */                                             \
      /* referenced as RasterBand.ds. This is a GDALComputedDataset that */                                            \
      /* should not be closed. */                                                                                      \
      Dataset::New(r->GetDataset(), nullptr, false);                                                                   \
      return RasterBand::New(r, r->GetDataset());                                                                      \
    };                                                                                                                 \
    return job.run(info, async, i);                                                                                           \
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
 * @param {RasterBand} arg Argument.
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
 * @method mulAsync
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
 * Create a RasterBand that contains the lesser values of each band.
 *
 * @throws {Error}
 * @method min
 * @static
 * @memberof algebra
 * @param {RasterBand[]} ...args Arguments.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that contains the lesser values of each band.
 * @async
 *
 * @throws {Error}
 * @method minAsync
 * @static
 * @memberof algebra
 * @param {RasterBand[]} ...args Arguments.
 * @return {Promise<RasterBand>}
 */
GDAL_ALGEBRA_VARIADIC_BANDONLY_OP(gdal_min, GDALMinimumOfNBands)

/**
 * Create a RasterBand that contains the bigger values of each band.
 *
 * @throws {Error}
 * @method max
 * @static
 * @memberof algebra
 * @param {RasterBand[]} ...args Arguments.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that contains the bigger values of each band.
 * @async
 *
 * @throws {Error}
 * @method maxAsync
 * @static
 * @memberof algebra
 * @param {RasterBand[]} ...args Arguments.
 * @return {Promise<RasterBand>}
 */
GDAL_ALGEBRA_VARIADIC_BANDONLY_OP(gdal_max, GDALMaximumOfNBands)

/**
 * Create a RasterBand that is the average of each bands.
 *
 * @throws {Error}
 * @method mean
 * @static
 * @memberof algebra
 * @param {RasterBand[]} ...args Arguments.
 * @return {RasterBand}
 */

/**
 * Create a RasterBand that is the average of each bands.
 * @async
 *
 * @throws {Error}
 * @method meanAsync
 * @static
 * @memberof algebra
 * @param {RasterBand[]} ...args Arguments.
 * @return {Promise<RasterBand>}
 */
GDAL_ALGEBRA_VARIADIC_BANDONLY_OP(gdal_mean, GDALMeanOfNBands)

/**
 * Create a RasterBand that is the result of the ternary operator of the arguments.
 *
 * @throws {Error}
 * @method ifThenElse
 * @static
 * @memberof algebra
 * @param {RasterBand} arg1 Condition.
 * @param {RasterBand | number} arg2 If value.
 * @param {RasterBand | number} arg3 Else value.
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
 * @param {RasterBand} arg1 Condition.
 * @param {RasterBand | number} arg2 If value.
 * @param {RasterBand | number} arg3 Else value.
 * @return {Promise<RasterBand>}
 */
GDAL_ASYNCABLE_DEFINE(ifThenElse) {
  RasterBand *arg1_band, *arg2_band, *arg3_band;
  double arg2_number, arg3_number;

  NODE_ARG_WRAPPED(0, "First argument", RasterBand, arg1_band);
  NODE_ALGEBRA_ARG(1, arg2);
  NODE_ALGEBRA_ARG(2, arg3);

  std::vector<long> ds_uids = {arg1_band->parent_uid};
  if (arg2_band) ds_uids.push_back(arg2_band->parent_uid);
  if (arg3_band) ds_uids.push_back(arg3_band->parent_uid);
  GDALAsyncableJob<GDALRasterBand *> job(ds_uids);
  job.persist(arg1_band->Value());
  if (arg2_band) job.persist(arg2_band->Value());
  if (arg3_band) job.persist(arg3_band->Value());

  GDALRasterBand *arg1 = arg1_band->get();
  if (arg2_band && arg3_band) {
    GDALRasterBand *arg2 = arg2_band->get();
    GDALRasterBand *arg3 = arg3_band->get();
    job.main = [arg1, arg2, arg3](const GDALExecutionProgress &) {
      return new GDALComputedRasterBand(gdal::IfThenElse(*arg1, *arg2, *arg3));
    };
  } else if (arg2_band) {
    GDALRasterBand *arg2 = arg2_band->get();
    double arg3 = arg3_number;
    job.main = [arg1, arg2, arg3](const GDALExecutionProgress &) {
      return new GDALComputedRasterBand(gdal::IfThenElse(*arg1, *arg2, arg3));
    };
  } else if (arg3_band) {
    double arg2 = arg2_number;
    GDALRasterBand *arg3 = arg3_band->get();
    job.main = [arg1, arg2, arg3](const GDALExecutionProgress &) {
      return new GDALComputedRasterBand(gdal::IfThenElse(*arg1, arg2, *arg3));
    };
  } else {
    double arg2 = arg2_number;
    double arg3 = arg3_number;
    job.main = [arg1, arg2, arg3](const GDALExecutionProgress &) {
      return new GDALComputedRasterBand(gdal::IfThenElse(*arg1, arg2, arg3));
    };
  }
  job.rval = [](GDALRasterBand *r, const GetFromPersistentFunc &) {
    Dataset::New(r->GetDataset(), nullptr, false);
    return RasterBand::New(r, r->GetDataset());
  };
  return job.run(info, async, 3);
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
  RasterBand *arg_band;
  std::string type_name;
  GDALDataType type;

  NODE_ARG_WRAPPED(0, "Argument", RasterBand, arg_band);

  NODE_ARG_STR(1, "Data Type", type_name);
  type = GDALGetDataTypeByName(type_name.c_str());
  if (type == GDT_Unknown) {
    Napi::Error::New(node_gdal::napi_env(), "Invalid data type").ThrowAsJavaScriptException();
    return node_gdal::napi_env().Undefined();
  }

  GDALAsyncableJob<GDALRasterBand *> job(arg_band->parent_uid);
  job.persist(arg_band->Value());
  GDALRasterBand *arg = arg_band->get();

  job.main = [arg, type](const GDALExecutionProgress &) { return new GDALComputedRasterBand(arg->AsType(type)); };

  job.rval = [](GDALRasterBand *r, const GetFromPersistentFunc &) {
    Dataset::New(r->GetDataset(), nullptr, false);
    return RasterBand::New(r, r->GetDataset());
  };
  return job.run(info, async, 2);
}

} // namespace Algebra
} // namespace node_gdal

#endif
