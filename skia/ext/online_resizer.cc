// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "skia/ext/online_resizer.h"
#include "third_party/skia/include/core/SkColorPriv.h"
#include "third_party/skia/include/core/SkFixed.h"

namespace skia {

namespace {

int GetSourceBytesPerPixel(OnlineResizer::PixelFormatConversion conv) {
  switch (conv) {
    case OnlineResizer::CONVERSION_24_TO_32:
    case OnlineResizer::CONVERSION_24_TO_32_SWAP:
      return 3;
    case OnlineResizer::CONVERSION_32_TO_32:
    case OnlineResizer::CONVERSION_32_TO_32_SWAP:
    case OnlineResizer::CONVERSION_32_TO_32_PREMUL:
    case OnlineResizer::CONVERSION_32_TO_32_SWAP_PREMUL:
    case OnlineResizer::CONVERSION_CMYK_TO_RGBA32:
    case OnlineResizer::CONVERSION_CMYK_TO_BGRA32:
      return 4;
    default:
      SkASSERT(false);
  }
}

// If this level performs mipmapping then call this method to return
// optimal conversion format.
OnlineResizer::PixelFormatConversion GetThisLevelConversion(
    OnlineResizer::PixelFormatConversion conv) {
  switch (conv) {
    // If the input for this level is 24-bits then convert it to
    // 32-bits without swapping the channels.
    case OnlineResizer::CONVERSION_24_TO_32:
    case OnlineResizer::CONVERSION_24_TO_32_SWAP:
      return OnlineResizer::CONVERSION_24_TO_32;

    // If the input for this level is 32-bits then just scale and
    // don't sawp the channels or conversion.
    case OnlineResizer::CONVERSION_32_TO_32:
    case OnlineResizer::CONVERSION_32_TO_32_SWAP:
    case OnlineResizer::CONVERSION_32_TO_32_PREMUL:
    case OnlineResizer::CONVERSION_32_TO_32_SWAP_PREMUL:
    case OnlineResizer::CONVERSION_CMYK_TO_BGRA32:
    case OnlineResizer::CONVERSION_CMYK_TO_RGBA32:
      return OnlineResizer::CONVERSION_32_TO_32;
    default:
      SkASSERT(false);
  }
}

OnlineResizer::PixelFormatConversion GetNextLevelConversion(
    OnlineResizer::PixelFormatConversion conv) {
  switch (conv) {
    // Input for next level should be converted to 32-bits already.
    case OnlineResizer::CONVERSION_24_TO_32:
    case OnlineResizer::CONVERSION_32_TO_32:
      return OnlineResizer::CONVERSION_32_TO_32;

    // Since this level doesn't perform swapping, next level will
    // perform swapping.
    case OnlineResizer::CONVERSION_24_TO_32_SWAP:
    case OnlineResizer::CONVERSION_32_TO_32_SWAP:
      return OnlineResizer::CONVERSION_32_TO_32_SWAP;

    // Keep the covnersion for these types.
    case OnlineResizer::CONVERSION_32_TO_32_PREMUL:
    case OnlineResizer::CONVERSION_32_TO_32_SWAP_PREMUL:
    case OnlineResizer::CONVERSION_CMYK_TO_BGRA32:
    case OnlineResizer::CONVERSION_CMYK_TO_RGBA32:
      return conv;
    default:
      SkASSERT(false);
  }
}

#define MASK            0xFF00FF
#define LO_PAIR(x)      ((x) & MASK)
#define HI_PAIR(x)      (((x) >> 8) & MASK)
#define COMBINE(lo, hi) (((lo) & ~0xFF00) | (((hi) & ~0xFF00) << 8))
#define INTPL40(a, b)   (a)
#define INTPL31(a, b)   COMBINE((3 * LO_PAIR(a) + LO_PAIR(b)) >> 2,     \
                                (3 * HI_PAIR(a) + HI_PAIR(b)) >> 2)
// Note that this code rounds before adding and slight precision is lost.
#define INTPL22(a, b)   ((((a) >> 1) & 0x7f7f7f7f) + (((b) >> 1) & 0x7f7f7f7f))
#define INTPL13(a, b)   INTPL31(b, a)
#define FIRST_PIXEL(x, n) *(const uint32_t*)(x + first_pixel * n)
#define SECOND_PIXEL(x, n) *(const uint32_t*)(x + first_pixel * n + n)

// Conversion macros. They use the convention of convertion X and writing it to Y.
#define CONV_32_TO_32(X, Y) (Y) = (X)
#define CONV_32_TO_32_SWAP(X, Y)                                        \
  (Y) = SkPackARGB32NoCheck((X) & 0xff,                                 \
                            (X) >> 24,                                  \
                            ((X) >> 16) & 0xff,                         \
                            ((X) >> 8) & 0xff)
#define CONV_32_TO_32_PREMUL(X, Y)                                      \
  { uint32_t d = (X) >> 24;                                             \
    uint32_t ac = (((X) & MASK) * d) >> 8;                              \
    uint32_t b = (((X) >> 8) & 0xff) * d;                               \
    (Y) = (ac & MASK) | (b & ~MASK) | (d << 24);                        \
  }
#define CONV_32_TO_32_SWAP_PREMUL(X, Y)                                 \
  { uint32_t d = (X) >> 24;                                             \
    uint32_t ac = ((X) & MASK) * d;                                     \
    uint32_t b = (((X) >> 8) & 0xff) * d;                               \
    (Y) = (ac >> 24) | ((ac << 8) & MASK) | (b & ~MASK) | (d << 24);    \
  }
#define CONV_24_TO_32(X, Y) (Y) = ((X) | 0xff000000)
#define CONV_24_TO_32_SWAP(X, Y)                \
  (Y) = SkPackARGB32NoCheck(0xff,               \
                            (X) >> 24,          \
                            ((X) >> 16) & 0xff, \
                            ((X) >> 8) & 0xff)
#define CONV_CMYK_TO_RGBA32(X, Y)                       \
  { uint32_t d = (X) >> 24;                             \
    uint32_t ac = (((X) & MASK) * d) >> 8;              \
    uint32_t b = (((X) >> 8) & 0xff) * d;               \
    (Y) = (ac & MASK) | (b & ~MASK) | (0xff << 24);     \
  }
#define CONV_CMYK_TO_BGRA32(X, Y)                                       \
  { uint32_t d = (X) >> 24;                                             \
    uint32_t ac = ((X) & MASK) * d;                                     \
    uint32_t b = (((X) >> 8) & 0xff) * d;                               \
    (Y) = (ac >> 24) | ((ac << 8) & MASK) | (b & ~MASK) | (0xff << 24); \
  }

#ifdef _MSC_VER
#define restrict __restrict
#else
#define restrict __restrict__
#endif

// Template for defining a bilinear interpolation function.
// v - Vertical interpolation, either 40, 13, 22 or 31.
// c - Conversion, e.g. 32_TO_32, 32_TO_32_SWAP, etc.
// n - Number of bytes per source pixel.
#define BI_INTERPOLATE_FUNC_TEMPLATE(v, c, n)                           \
  void BiInterpolate##v##_##c(const uint8_t* restrict first_row,        \
                              const uint8_t* restrict second_row,       \
                              SkFixed source_x_step,                    \
                              uint8_t* restrict dest_row,               \
                              int dest_width) {                         \
    dest_width *= source_x_step;                                        \
    int last_second_pixel = -1;                                         \
    uint32_t rv;                                                        \
    for (SkFixed source_x = 0;                                          \
         source_x < dest_width;                                         \
         source_x += source_x_step) {                                   \
      int first_pixel = SkFixedFloor(source_x);                         \
      uint32_t lv = rv;                                                 \
      if (last_second_pixel != first_pixel)                             \
        lv = INTPL##v(FIRST_PIXEL(first_row, n),                        \
                      FIRST_PIXEL(second_row, n));                      \
      rv = INTPL##v(SECOND_PIXEL(first_row, n),                         \
                    SECOND_PIXEL(second_row, n));                       \
      uint32_t result;                                                  \
      int frac = (source_x >> 14) & 3;                                  \
      switch (frac) {                                                   \
        case 0: result = INTPL40(lv, rv); break;                        \
        case 1: result = INTPL31(lv, rv); break;                        \
        case 2: result = INTPL22(lv, rv); break;                        \
        case 3: result = INTPL13(lv, rv); break;                        \
      }                                                                 \
      CONV_##c(result, *(uint32_t*)dest_row);                           \
      last_second_pixel = first_pixel + 1;                              \
      dest_row += 4;                                                    \
    }                                                                   \
  }

// Declare bilinear interpolation conversion functions.
// c - Conversion, e.g. 32_TO_32, 32_TO_32_SWAP, etc.
// n - Number of bytes per source pixel.
#define DECLARE_BI_INTERPOLATE_FUNC(c, n)       \
  BI_INTERPOLATE_FUNC_TEMPLATE(40, c, n)        \
  BI_INTERPOLATE_FUNC_TEMPLATE(31, c, n)        \
  BI_INTERPOLATE_FUNC_TEMPLATE(22, c, n)        \
  BI_INTERPOLATE_FUNC_TEMPLATE(13, c, n)

// Define a list of bilinear conversion functions.
// c - Conversion, e.g. 32_TO_32, etc.
#define BI_INTERPOLATE_FUNC_LIST(c)                         \
  { BiInterpolate##40_##c, BiInterpolate##31_##c,           \
        BiInterpolate##22_##c, BiInterpolate##13_##c }

DECLARE_BI_INTERPOLATE_FUNC(32_TO_32, 4);
DECLARE_BI_INTERPOLATE_FUNC(32_TO_32_SWAP, 4);
DECLARE_BI_INTERPOLATE_FUNC(32_TO_32_PREMUL, 4);
DECLARE_BI_INTERPOLATE_FUNC(32_TO_32_SWAP_PREMUL, 4);
DECLARE_BI_INTERPOLATE_FUNC(24_TO_32, 3);
DECLARE_BI_INTERPOLATE_FUNC(24_TO_32_SWAP, 3);
DECLARE_BI_INTERPOLATE_FUNC(CMYK_TO_BGRA32, 4);
DECLARE_BI_INTERPOLATE_FUNC(CMYK_TO_RGBA32, 4);

typedef void (*BiInterpolateProc)(
    const uint8_t* restrict /* first_row */,
    const uint8_t* restrict /* second_row */,
    SkFixed /* source_x_step */,
    uint8_t* restrict /* dest_row */,
    int /* dest_width */);

// Define a map for getting the bilinear conversion function.
// This list *MUST* match the definition order of PixelFormatConversion.
static BiInterpolateProc
kBiInterpolateProcedures[skia::OnlineResizer::CONVERSION_LAST][4] =
{ BI_INTERPOLATE_FUNC_LIST(32_TO_32),
  BI_INTERPOLATE_FUNC_LIST(32_TO_32_SWAP),
  BI_INTERPOLATE_FUNC_LIST(32_TO_32_PREMUL),
  BI_INTERPOLATE_FUNC_LIST(32_TO_32_SWAP_PREMUL),
  BI_INTERPOLATE_FUNC_LIST(24_TO_32),
  BI_INTERPOLATE_FUNC_LIST(24_TO_32_SWAP),
  BI_INTERPOLATE_FUNC_LIST(CMYK_TO_BGRA32),
  BI_INTERPOLATE_FUNC_LIST(CMYK_TO_RGBA32)
};

// Extend one pixel at the end by the given amount of bytes.
inline void ExtendOnePixel(uint8_t* pixels, int scanline_bytes, int extend_bytes) {
  SkASSERT(scanline_bytes >= extend_bytes);
  for (int i = 0; i < extend_bytes; ++i)
    pixels[scanline_bytes + i] = pixels[scanline_bytes - extend_bytes + i];
}

}  // namespace

// static
OnlineResizer* OnlineResizer::Create(const SkISize& source_size,
                                     const SkISize& dest_size,
                                     PixelFormatConversion conversion) {
  if (source_size.isEmpty() || dest_size.isEmpty())
    return NULL;
  return new OnlineResizer(source_size, dest_size, conversion);
}

OnlineResizer::OnlineResizer(const SkISize& source_size,
                             const SkISize& dest_size,
                             PixelFormatConversion conversion)
    : source_size_(source_size),
      dest_size_(dest_size),
      format_conversion_(conversion),
      source_y_(0),
      source_bpp_(0),
      source_rows_saved_(0),
      internal_resizer_(NULL) {
  PrepareMipmapIfNeeded();
  PrepareFilters();
}

OnlineResizer::~OnlineResizer() {
  delete [] source_rows_[0];
  delete [] source_rows_[1];
  delete internal_resizer_;
}

bool OnlineResizer::SupplyScanline(void* scanline_pixels) {
  if (!scanline_pixels)
    return false;

  // Internal buffer is full.
  if (source_rows_saved_ > GetSecondRow())
    return false;

  uint8_t* row = source_rows_[source_rows_saved_ % 2];
  memcpy(row, scanline_pixels, source_row_bytes_);
  ++source_rows_saved_;

  // Extend the input by 1 pixel to avoid bounds checking.
  ExtendOnePixel(row, source_row_bytes_, source_bpp_);
  return true;
}

bool OnlineResizer::ResizeTo(void* dest_scanline) {
  // Pretend that we output successfully since parameters are invalid.
  if (!dest_scanline)
    return true;

  if (internal_resizer_)
    ResizeToNextLevel(dest_scanline);
  else
    ResizeToDestination(dest_scanline);
}

void OnlineResizer::PrepareMipmapIfNeeded() {
  // Perform mipmapping only if destination is less than half of the source
  // size. By doing mipmapping this resizer would only do a half scale and
  // use the internal resizer to scale further down.
  // TODO(hclam): Handle case where we can mipmap either horizontally or
  // vertically. Also avoid too many mipmap steps.
  bool mipmap = (dest_size_.width() < source_size_.width() / 2) &&
      (dest_size_.height() < source_size_.height() / 2);

  if (!mipmap)
    return;

  SkISize actual_dest_size = dest_size_;

  // Since this level of resizer performs mipmapping so size is halved.
  dest_size_.set(source_size_.width() / 2, source_size_.height() / 2);

  // Create the next level of resizer to scale to destination.
  internal_resizer_ = new OnlineResizer(
      dest_size_,
      actual_dest_size,
      GetNextLevelConversion(format_conversion_));

  // Alter conversion for this level.
  format_conversion_ = GetThisLevelConversion(format_conversion_);
}

void OnlineResizer::PrepareFilters() {
  source_bpp_ = GetSourceBytesPerPixel(format_conversion_);
  source_row_bytes_ = source_size_.width() * source_bpp_;

  if (dest_size_.height()) {
    source_y_step_ =
        SkIntToFixed(source_size_.height()) / dest_size_.height();
  }
  if (dest_size_.width()) {
    source_x_step_ =
        SkIntToFixed(source_size_.width()) / dest_size_.width();
  }

  // Internal buffer are padded to avoid bounds checking for
  // horizontal interpolation. Add extra 4 bytes so interpolation can
  // operation on 4 bytes each time safely.
  source_rows_[0] = new uint8_t[source_row_bytes_ + source_bpp_ + 4];
  source_rows_[1] = new uint8_t[source_row_bytes_ + source_bpp_ + 4];
}

bool OnlineResizer::ResizeToNextLevel(void* dest_scanline) {
  // First ask the internal scaler to output a line.
  bool dest_available = internal_resizer_->ResizeTo(dest_scanline);

  // We might be able to resize to next level's internal buffer.
  uint8_t* half_scanline = internal_resizer_->BeginResizeScanline();
  bool half_available = false;
  if (half_scanline) {
    half_available = ResizeToDestination(half_scanline);

    // If a half scanline is produced then need to wrap it up.
    if (half_available)
      internal_resizer_->EndResizeScanline();
  }

  // TODO(hclam): The following code is important, because it makes
  // sure SupplyScanline() will never be full. However doing this
  // will overwhelm the stack because each level calls never level
  // for two times. I have to decide whether to do it or not.
  // if (!dest_available && half_available)
  //   dest_available = internal_resizer_->ResizeTo(dest_scanline);
  return dest_available;
}

bool OnlineResizer::ResizeToDestination(void* dest_scanline) {
  int first_row = GetFirstRow();
  int second_row = GetSecondRow();

  // Not enough rows saved.
  if (source_rows_saved_ <= second_row)
    return false;

  // Interpolate two scanlines to the destination scanline.
  Interpolate((uint8_t*)source_rows_[first_row % 2],
              (uint8_t*)source_rows_[second_row % 2],
              (uint8_t*)dest_scanline);
  return true;
}

uint8_t* OnlineResizer::BeginResizeScanline() {
  // Internal buffer is full.
  if (source_rows_saved_ > GetSecondRow())
    return NULL;
  return source_rows_[source_rows_saved_ % 2];
}

void OnlineResizer::EndResizeScanline() {
  ExtendOnePixel(source_rows_[source_rows_saved_ % 2],
                 source_row_bytes_, source_bpp_);
  ++source_rows_saved_;
}

void OnlineResizer::Interpolate(const uint8_t* first_row,
                                const uint8_t* second_row,
                                uint8_t* dest_row) {
  SkASSERT(first_row);
  SkASSERT(second_row);

  int frac = (source_y_ >> 14) & 3;

  kBiInterpolateProcedures[format_conversion_][frac](
      first_row,
      second_row,
      source_x_step_,
      dest_row,
      dest_size_.width());
  source_y_ += source_y_step_;
}

int OnlineResizer::GetFirstRow() {
  return SkFixedFloor(source_y_);
}

int OnlineResizer::GetSecondRow() {
  return std::min(GetFirstRow() + 1,
                  static_cast<int>(source_size_.height() - 1));
}

}  // namespace skia
