// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PICTURE_LAYER_TILING_SET_H_
#define CC_PICTURE_LAYER_TILING_SET_H_

#include "cc/picture_layer_tiling.h"
#include "cc/region.h"
#include "cc/scoped_ptr_vector.h"
#include "ui/gfx/size.h"

namespace cc {

class CC_EXPORT PictureLayerTilingSet {
 public:
  PictureLayerTilingSet(PictureLayerTilingClient* client);
  ~PictureLayerTilingSet();

  // Shallow copies all data (except client) from other.
  void CloneAll(
     const PictureLayerTilingSet& other,
     const Region& invalidation);
  void Clone(const PictureLayerTiling* tiling, const Region& invalidation);

  // TODO(enne): Remove this once syncing happens to the pending tree.
  void Invalidate(const Region& invalidation);

  void SetLayerBounds(gfx::Size layer_bounds);
  gfx::Size LayerBounds() const;

  const PictureLayerTiling* AddTiling(
      float contents_scale,
      gfx::Size tile_size);
  size_t num_tilings() const { return tilings_.size(); }

  void Reset();

  void UpdateTilePriorities(const gfx::Size& device_viewport,
                            float layer_content_scale_x,
                            float layer_content_scale_y,
                            const gfx::Transform& last_screen_transform,
                            const gfx::Transform& current_screen_transform,
                            double time_delta);

  // For a given rect, iterates through tiles that can fill it.  If no
  // set of tiles with resources can fill the rect, then it will iterate
  // through null tiles with valid geometry_rect() until the rect is full.
  // If all tiles have resources, the union of all geometry_rects will
  // exactly fill rect with no overlap.
  class CC_EXPORT Iterator {
   public:
    Iterator(PictureLayerTilingSet* set, float contents_scale, gfx::Rect rect);
    ~Iterator();

    // Visible rect (no borders), always in the space of rect,
    // regardless of the relative contents scale of the tiling.
    gfx::Rect geometry_rect() const;
    // Texture rect (in texels) for geometry_rect
    gfx::RectF texture_rect() const;
    // Texture size in texels
    gfx::Size texture_size() const;

    Tile* operator->() const;
    Tile* operator*() const;

    Iterator& operator++();
    operator bool() const;

   private:
    PictureLayerTilingSet* set_;
    float contents_scale_;
    PictureLayerTiling::Iterator tiling_iter_;
    int current_tiling_;

    Region current_region_;
    Region missing_region_;
    Region::Iterator region_iter_;
  };

 private:
  PictureLayerTilingClient* client_;
  gfx::Size layer_bounds_;
  ScopedPtrVector<PictureLayerTiling> tilings_;
  Region invalidation_;

  friend class Iterator;
};

}  // namespace cc

#endif  // CC_PICTURE_LAYER_TILING_SET_H_
