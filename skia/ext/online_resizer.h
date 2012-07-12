// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a class that implements a online bilinear scaling with mipmap.
// The design goal of this class is to minimize the use of memory and
// achieve high performance while compromizing little quality.
//
// The biggest feature of this class is that it can perform online, i.e.
// input image is supplied line-by-line. Available output is then written
// to the target directly. This makes it particularly useful when
// used with image decoders. User can decode a scanline, supply to this
// class and scale to target size without writing to an intermediate
// image.
//
// Another feature of this class is that it performs color space
// conversion, channel switching and alpha premultiplication. This allows
// a very high speed down-scaling because these operations are applied
// at the end of scaling.
//
// In terms of implementation this class has a recursive structure, each
// OnlineResizer object can contain an internal resizer if it performs
// an mipmap down-scale operation. At each level of down-scaling, at
// most 2 rows of source image is kept internally. Which means the
// total memory used is no more than O(4 * source_width).
//
// Example usage:
//
// OnlineResizer resizer(SkISize::Make(kWidth, kHeight),
//                       SkISize::Make(kDestWidth, kDestHeight),
//                       OnlineResizer::CONVERSION_32_TO_32);
// for (int row = 0; row < kDestHeight; ++row) {
//   while (!resizer.ResizeTo(dest.GetNextRow())) {
//     if (resizer.SupplyScanline(source.GetNextRow()))
//       source.AdvanceRow();
//     dest.AdvanceRow();
//   }
// }

#ifndef SKIA_EXT_ONLINE_RESIZER_H_
#define SKIA_EXT_ONLINE_RESIZER_H_

#include "third_party/skia/include/core/SkFixed.h"
#include "third_party/skia/include/core/SkSize.h"
#include "third_party/skia/include/core/SkTypes.h"

namespace skia {

class SK_API OnlineResizer {
 public:
  enum PixelFormatConversion {
    // These are the basic conversions.
    CONVERSION_32_TO_32,
    CONVERSION_32_TO_32_SWAP,    // Swap first and third channel.
    CONVERSION_32_TO_32_PREMUL,  // 32-bits to 32-bits, pre-multiply alpha.
    CONVERSION_32_TO_32_SWAP_PREMUL,
                                 // Swap 1st, 3rd and pre-multiply alpha.
    CONVERSION_24_TO_32,         // 24-bits to 32-bits, order unchanged.
    CONVERSION_24_TO_32_SWAP,    // 24-bits to 32-bits, swap 1st and 3rd
                                 // component.
    CONVERSION_CMYK_TO_BGRA32,
    CONVERSION_CMYK_TO_RGBA32,

    CONVERSION_LAST,             // Place holder to count conversions.

    // These are just different names of the above conversions.
    CONVERSION_BGRA32_TO_BGRA32 = CONVERSION_32_TO_32,
    CONVERSION_BGRA32_TO_RGBA32 = CONVERSION_32_TO_32_SWAP,
    CONVERSION_RGBA32_TO_RGBA32 = CONVERSION_32_TO_32,
    CONVERSION_RGBA32_TO_BGRA32 = CONVERSION_32_TO_32_SWAP,
    CONVERSION_RGB24_TO_RGBA32 = CONVERSION_24_TO_32,
    CONVERSION_RGB24_TO_BGRA32 = CONVERSION_24_TO_32_SWAP,

    // TODO(hclam): The following are not implemented yet.
    CONVERSION_BGRA32_TO_BGRA32_PREMUL = CONVERSION_32_TO_32_PREMUL,
    CONVERSION_BGRA32_TO_RGBA32_PREMUL = CONVERSION_32_TO_32_SWAP_PREMUL,
    CONVERSION_RGBA32_TO_RGBA32_PREMUL = CONVERSION_32_TO_32_PREMUL,
    CONVERSION_RGBA32_TO_BGRA32_PREMUL = CONVERSION_32_TO_32_SWAP_PREMUL,
  };

  // Create an OnlineResizer.
  static OnlineResizer* Create(const SkISize& source_size,
                               const SkISize& dest_size,
                               PixelFormatConversion conversion);

  ~OnlineResizer();

  // Supply a scanline of the source image to this scaler, the scanline
  // is copied to an internal data structure.
  //
  // Scaling is not performed until ResizeTo() is called.
  //
  // Return true if the supplied scanline is saved.
  // Return false if the supplied scanline is rejected because internal buffer
  // is full.
  bool SupplyScanline(void* scanline_pixels);

  // Generate a destination scanline by using saved scanlines.
  //
  // If a destination scanline is written then return true.
  // Otherwise return false if there is no enough source scanlines to
  // perform scaling.
  bool ResizeTo(void* dest_scanline);

 private:
  OnlineResizer(const SkISize& source_size,
                const SkISize& dest_size,
                PixelFormatConversion conversion);

  // Helper methods to do initialization.
  void PrepareMipmapIfNeeded();
  void PrepareFilters();

  // Resize input to the next level of resizer.
  // Return true if output is generated.
  bool ResizeToNextLevel(void* dest_scanline);

  // Resize input to destination, call this if this is the last scale
  // level. Return true if output is generated.
  bool ResizeToDestination(void* dest_scanline);

  // Called by an upper level of resizer to obtain a buffer for writing
  // a halved scanline. Use this method instead of SupplyScanline() can
  // avoid memory copying.
  //
  // EndResizeScanline() must be called after this to complete extending
  // the scanline.
  //
  // Return NULL if there's no space in internal buffer.
  uint8_t* BeginResizeScanline();

  // Called by an upper level of resizer after a halved scanline has
  // been written to this object's internal buffer.
  void EndResizeScanline();

  // Perform bilinear interpolation of the two input rows.
  void Interpolate(const uint8_t* first_row,
                   const uint8_t* second_row,
                   uint8_t* dest_row);
  int GetFirstRow();
  int GetSecondRow();

  SkISize source_size_;
  SkISize dest_size_;
  PixelFormatConversion format_conversion_;

  // Sub-pixel step for Y direction in the source image.
  SkFixed source_y_step_;

  // Sub-pixel step for X direction in the source image.
  SkFixed source_x_step_;

  // Current row in the source image.
  SkFixed source_y_;

  // Number of bytes per pixel in the source.
  int source_bpp_;

  // Number of bytes of a source row.
  int source_row_bytes_;

  // Save at most two source rows.
  uint8_t* source_rows_[2];

  // Number of rows ever supplied to this class.
  int source_rows_saved_;

  // If there is an internal resizer then this object does a
  // half resize.
  OnlineResizer* internal_resizer_;
};

}  // namespace skia

#endif  // SKIA_EXT_ONLINE_RESIZER_H_
