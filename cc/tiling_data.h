// Copyright 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TILING_DATA_H_
#define CC_TILING_DATA_H_

#include "base/basictypes.h"
#include "base/logging.h"
#include "cc/cc_export.h"
#include "ui/gfx/size.h"

namespace gfx {
class Rect;
class Vector2d;
}

namespace cc {

class CC_EXPORT TilingData {
 public:
  TilingData(gfx::Size max_texture_size, gfx::Size total_size, bool has_border_texels);
  ~TilingData();

  gfx::Size total_size() const { return total_size_; }
  void SetTotalSize(const gfx::Size total_size);

  gfx::Size max_texture_size() const { return max_texture_size_; }
  void SetMaxTextureSize(gfx::Size max_texture_size);

  int border_texels() const { return border_texels_; }
  void SetHasBorderTexels(bool has_border_texels);

  bool has_empty_bounds() const { return !num_tiles_x_ || !num_tiles_y_; }
  int num_tiles_x() const { return num_tiles_x_; }
  int num_tiles_y() const { return num_tiles_y_; }
  int TileXIndexFromSrcCoord(int src_position) const;
  int TileYIndexFromSrcCoord(int src_position) const;

  gfx::Rect TileBounds(int i, int j) const;
  gfx::Rect TileBoundsWithBorder(int i, int j) const;
  int TilePositionX(int x_index) const;
  int TilePositionY(int y_index) const;
  int TileSizeX(int x_index) const;
  int TileSizeY(int y_index) const;

  // Difference between TileBound's and TileBoundWithBorder's origin().
  gfx::Vector2d TextureOffset(int x_index, int y_index) const;

 private:
  void AssertTile(int i, int j) const {
    DCHECK_GE(i,  0);
    DCHECK_LT(i, num_tiles_x_);
    DCHECK_GE(j, 0);
    DCHECK_LT(j, num_tiles_y_);
  }

  void RecomputeNumTiles();

  gfx::Size max_texture_size_;
  gfx::Size total_size_;
  // This value is always 0 or 1.
  int border_texels_;

  // These are computed values.
  int num_tiles_x_;
  int num_tiles_y_;
};

}  // namespace cc

#endif  // CC_TILING_DATA_H_
