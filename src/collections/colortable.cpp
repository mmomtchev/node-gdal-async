#include <memory>
#include "colortable.hpp"
#include "../gdal_common.hpp"
#include "../gdal_dataset.hpp"
#include "../gdal_rasterband.hpp"
#include "../utils/string_list.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> ColorTable::constructor;

void ColorTable::Initialize(Local<Object> target) {
  Nan::HandleScope scope;

  Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(ColorTable::New);
  lcons->InstanceTemplate()->SetInternalFieldCount(1);
  lcons->SetClassName(Nan::New("ColorTable").ToLocalChecked());

  Nan::SetPrototypeMethod(lcons, "toString", toString);
  Nan::SetPrototypeMethod(lcons, "isSame", isSame);
  Nan::SetPrototypeMethod(lcons, "clone", clone);
  Nan::SetPrototypeMethod(lcons, "count", count);
  Nan::SetPrototypeMethod(lcons, "get", get);
  Nan::SetPrototypeMethod(lcons, "set", set);
  Nan::SetPrototypeMethod(lcons, "ramp", ramp);
  ATTR(lcons, "interpretation", interpretationGetter, READ_ONLY_SETTER);

  ATTR_DONT_ENUM(lcons, "band", bandGetter, READ_ONLY_SETTER);

  Nan::Set(target, Nan::New("ColorTable").ToLocalChecked(), Nan::GetFunction(lcons).ToLocalChecked());

  constructor.Reset(lcons);
}

ColorTable::ColorTable(GDALColorTable *raw, long parent_uid) : Nan::ObjectWrap(), parent_uid(parent_uid), this_(raw) {
}

ColorTable::~ColorTable() {
  dispose();
}

void ColorTable::dispose() {
  if (this_) {
    LOG("Disposing ColorTable [%p]", this_);
    object_store.dispose(uid, false);
    if (parent_uid == 0 && this_ != nullptr) delete this_;
    this_ = nullptr;
  }
}
/**
 * An encapsulation of a {@link RasterBand}
 * color table.
 *
 * @example
 * var colorTable = band.colorTable;
 *
 * band.colorTable = new gdal.ColorTable(gdal.GPI_RGB);
 *
 * @class ColorTable
 * @param {string} interpretation palette interpretation
 */
NAN_METHOD(ColorTable::New) {
  ColorTable *f;

  if (!info.IsConstructCall()) {
    Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
    return;
  }
  if (info[0]->IsExternal()) {
    Local<External> ext = info[0].As<External>();
    void *ptr = ext->Value();
    f = static_cast<ColorTable *>(ptr);
  } else {
    std::string pi;
    NODE_ARG_STR(0, "palette interpretation", pi);
    GDALPaletteInterp gpi;
    if (pi == "Gray")
      gpi = GPI_Gray;
    else if (pi == "RGB")
      gpi = GPI_RGB;
    else if (pi == "CMYK")
      gpi = GPI_CMYK;
    else if (pi == "HLS")
      gpi = GPI_HLS;
    else {
      Nan::ThrowRangeError("Invalid palette interpretation");
      return;
    }
    f = new ColorTable(new GDALColorTable(gpi), 0);
  }

  f->Wrap(info.This());
  f->uid = object_store.add(f->get(), f->persistent(), f->parent_uid);
  info.GetReturnValue().Set(info.This());
  return;
}

/*
 * Create a color table owned by a gdal.RasterBand
 */
Local<Value> ColorTable::New(GDALColorTable *raw, Local<Value> parent) {
  Nan::EscapableHandleScope scope;

  if (!raw) { return scope.Escape(Nan::Null()); }
  if (object_store.has(raw)) { return scope.Escape(object_store.get(raw)); }

  RasterBand *band = Nan::ObjectWrap::Unwrap<RasterBand>(parent.As<Object>());

  ColorTable *wrapped = new ColorTable(raw, band->parent_uid);

  v8::Local<v8::Value> ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(ColorTable::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();

  Nan::SetPrivate(obj, Nan::New("parent_").ToLocalChecked(), parent);

  return scope.Escape(obj);
}

/*
 * Create a standalone color table.
 */
Local<Value> ColorTable::New(GDALColorTable *raw) {
  Nan::EscapableHandleScope scope;

  if (!raw) { return scope.Escape(Nan::Null()); }
  if (object_store.has(raw)) { return scope.Escape(object_store.get(raw)); }

  ColorTable *wrapped = new ColorTable(raw, 0);

  v8::Local<v8::Value> ext = Nan::New<External>(wrapped);
  v8::Local<v8::Object> obj =
    Nan::NewInstance(Nan::GetFunction(Nan::New(ColorTable::constructor)).ToLocalChecked(), 1, &ext).ToLocalChecked();

  return scope.Escape(obj);
}

NAN_METHOD(ColorTable::toString) {
  info.GetReturnValue().Set(Nan::New("ColorTable").ToLocalChecked());
}

/**
 * @typedef {object} Color
 * @memberof ColorTable
 * @property {number} c1
 * @property {number} c2
 * @property {number} c3
 * @property {number} c4
 */

/**
 * Clones the instance.
 * The newly created ColorTable is not owned by any RasterBand.
 *
 * @method clone
 * @instance
 * @memberof ColorTable
 * @return {ColorTable}
 */
NAN_METHOD(ColorTable::clone) {
  ColorTable *ct = Nan::ObjectWrap::Unwrap<ColorTable>(info.This());
  info.GetReturnValue().Set(ColorTable::New(ct->this_->Clone()));
}

/**
 * Compares two ColorTable objects for equality.
 *
 * @method isSame
 * @instance
 * @memberof ColorTable
 * @param {ColorTable} other
 * @throws {Error}
 * @return {boolean}
 */
NAN_METHOD(ColorTable::isSame) {

  NODE_UNWRAP_CHECK(ColorTable, info.This(), self);
  GDAL_RAW_CHECK(GDALColorTable *, self, raw);

  ColorTable *other;
  NODE_ARG_WRAPPED(0, "other", ColorTable, other);
  GDAL_RAW_CHECK(GDALColorTable *, other, raw_other);

  CPLErrorReset();
  info.GetReturnValue().Set(Nan::New<Boolean>(raw->IsSame(raw_other)));
}

/**
 * Returns the color with the given ID.
 *
 * @method get
 * @instance
 * @memberof ColorTable
 * @param {number} index
 * @throws {Error}
 * @return {Color}
 */
NAN_METHOD(ColorTable::get) {

  int index;
  NODE_ARG_INT(0, "index", index);

  NODE_UNWRAP_CHECK(ColorTable, info.This(), self);
  GDAL_RAW_CHECK(GDALColorTable *, self, raw);

  MaybeLocal<Value> parentMaybe = Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked());
  if (!parentMaybe.IsEmpty() && !parentMaybe.ToLocalChecked()->IsNullOrUndefined()) {
    Local<Object> parent = parentMaybe.ToLocalChecked().As<Object>();
    NODE_UNWRAP_CHECK(RasterBand, parent, band);
  }

  CPLErrorReset();
  const GDALColorEntry *color = raw->GetColorEntry(index);
  if (color == nullptr) {
    NODE_THROW_LAST_CPLERR;
    return;
  }

  Local<Object> result = Nan::New<Object>();
  Nan::Set(result, Nan::New("c1").ToLocalChecked(), Nan::New<Number>(color->c1));
  Nan::Set(result, Nan::New("c2").ToLocalChecked(), Nan::New<Number>(color->c2));
  Nan::Set(result, Nan::New("c3").ToLocalChecked(), Nan::New<Number>(color->c3));
  Nan::Set(result, Nan::New("c4").ToLocalChecked(), Nan::New<Number>(color->c4));
  info.GetReturnValue().Set(result);
}

#define NODE_COLOR_FROM_OBJ(obj, color)                                                                                \
  NODE_INT_FROM_OBJ(obj, "c1", color.c1);                                                                              \
  NODE_INT_FROM_OBJ(obj, "c2", color.c2);                                                                              \
  NODE_INT_FROM_OBJ(obj, "c3", color.c3);                                                                              \
  NODE_INT_FROM_OBJ(obj, "c4", color.c4);

/**
 * Sets the color entry with the given ID.
 *
 * @method set
 * @instance
 * @memberof ColorTable
 * @throws {Error}
 * @param {number} index
 * @param {Color} color
 * @return {void}
 */

NAN_METHOD(ColorTable::set) {

  int index;
  NODE_ARG_INT(0, "index", index);

  Local<Object> color_obj;
  NODE_ARG_OBJECT(1, "color", color_obj);

  NODE_UNWRAP_CHECK(ColorTable, info.This(), self);
  GDAL_RAW_CHECK(GDALColorTable *, self, raw);

  MaybeLocal<Value> parentMaybe = Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked());
  if (!parentMaybe.IsEmpty() && !parentMaybe.ToLocalChecked()->IsNullOrUndefined()) {
    Nan::ThrowError("RasterBand color tables are read-only, create a new one to modify it");
    return;
  }

  GDALColorEntry color;
  NODE_COLOR_FROM_OBJ(color_obj, color);

  CPLErrorReset();
  raw->SetColorEntry(index, &color);
}

/**
 * Creates a color ramp from one color entry to another.
 *
 * @method ramp
 * @instance
 * @memberof ColorTable
 * @throws {Error}
 * @param {number} start_index
 * @param {Color} start_color
 * @param {number} end_index
 * @param {Color} end_color
 * @return {number} total number of color entries
 */

NAN_METHOD(ColorTable::ramp) {

  int start_index, end_index;
  NODE_ARG_INT(0, "start_index", start_index);
  NODE_ARG_INT(2, "end_index", end_index);
  if (start_index < 0 || end_index < 0 || end_index < start_index) {
    Nan::ThrowRangeError("Invalid color interval");
    return;
  }

  Local<Object> start_color_obj, end_color_obj;
  NODE_ARG_OBJECT(1, "start_color", start_color_obj);
  NODE_ARG_OBJECT(3, "start_color", end_color_obj);

  NODE_UNWRAP_CHECK(ColorTable, info.This(), self);
  GDAL_RAW_CHECK(GDALColorTable *, self, raw);

  GDALColorEntry start_color, end_color;
  NODE_COLOR_FROM_OBJ(start_color_obj, start_color);
  NODE_COLOR_FROM_OBJ(end_color_obj, end_color);

  MaybeLocal<Value> parentMaybe = Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked());
  if (!parentMaybe.IsEmpty() && !parentMaybe.ToLocalChecked()->IsNullOrUndefined()) {
    Nan::ThrowError("RasterBand color tables are read-only, create a new one to modify it");
    return;
  }

  int r = raw->CreateColorRamp(start_index, &start_color, end_index, &end_color);
  if (r == -1) {
    NODE_THROW_LAST_CPLERR;
    return;
  }

  info.GetReturnValue().Set(Nan::New<Number>(r));
}

/**
 * Returns the number of color entries.
 *
 * @method count
 * @instance
 * @memberof ColorTable
 * @return {number}
 */
NAN_METHOD(ColorTable::count) {

  NODE_UNWRAP_CHECK(ColorTable, info.This(), self);
  GDAL_RAW_CHECK(GDALColorTable *, self, raw);

  MaybeLocal<Value> parentMaybe = Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked());
  if (!parentMaybe.IsEmpty() && !parentMaybe.ToLocalChecked()->IsNullOrUndefined()) {
    Local<Object> parent = parentMaybe.ToLocalChecked().As<Object>();
    NODE_UNWRAP_CHECK(RasterBand, parent, band);
  }

  CPLErrorReset();
  info.GetReturnValue().Set(raw->GetColorEntryCount());
}

/**
 * Color interpretation of the palette.
 *
 * @readonly
 * @kind member
 * @name interpretation
 * @instance
 * @memberof ColorTable
 * @type {string}
 */
NAN_GETTER(ColorTable::interpretationGetter) {

  NODE_UNWRAP_CHECK(ColorTable, info.This(), self);
  GDAL_RAW_CHECK(GDALColorTable *, self, raw);

  MaybeLocal<Value> parentMaybe = Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked());
  if (!parentMaybe.IsEmpty() && !parentMaybe.ToLocalChecked()->IsNullOrUndefined()) {
    Local<Object> parent = parentMaybe.ToLocalChecked().As<Object>();
    NODE_UNWRAP_CHECK(RasterBand, parent, band);
  }

  CPLErrorReset();
  GDALPaletteInterp interp = raw->GetPaletteInterpretation();
  std::string r;
  switch (interp) {
    case GPI_Gray: r = "Gray"; break;
    case GPI_RGB: r = "RGB"; break;
    case GPI_CMYK: r = "CMYK"; break;
    case GPI_HLS: r = "HLS"; break;
    default: r = "invalid"; break;
  }
  info.GetReturnValue().Set(SafeString::New(r.c_str()));
}

/**
 * Returns the parent band.
 *
 * @readonly
 * @kind member
 * @name band
 * @instance
 * @memberof ColorTable
 * @type {RasterBand|undefined}
 */
NAN_GETTER(ColorTable::bandGetter) {
  MaybeLocal<Value> parentMaybe = Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked());
  if (!parentMaybe.IsEmpty() && !parentMaybe.ToLocalChecked()->IsNullOrUndefined()) {
    info.GetReturnValue().Set(parentMaybe.ToLocalChecked());
  }
}

} // namespace node_gdal
