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
  explicit PictureLayerTilingSet(PictureLayerTilingClient* client);
  ~PictureLayerTilingSet();

  void SetClient(PictureLayerTilingClient* client);

  // Shallow copies all data (except client and bounds from other).
  void CloneAll(
     const PictureLayerTilingSet& other,
     const Region& invalidation);
  void Clone(const PictureLayerTiling* tiling, const Region& invalidation);

  void SetLayerBounds(gfx::Size layer_bounds);
  gfx::Size LayerBounds() const;

  PictureLayerTiling* AddTiling(float contents_scale);
  size_t num_tilings() const { return tilings_.size(); }
  PictureLayerTiling* tiling_at(size_t idx) { return tilings_[idx]; }
  const PictureLayerTiling* tiling_at(size_t idx) const {
    return tilings_[idx];
  }

  // Remove all tilings.
  void RemoveAllTilings();

  // Remove one tiling.
  void Remove(PictureLayerTiling* tiling);

  // Remove all tiles; keep all tilings.
  void RemoveAllTiles();

  // For all tilings, create any tile that intersects |layer_rect|.
  void CreateTilesFromLayerRect(gfx::Rect layer_rect);

  void UpdateTilePriorities(
      WhichTree tree,
      gfx::Size device_viewport,
      gfx::Rect viewport_in_content_space,
      gfx::Size last_layer_bounds,
      gfx::Size current_layer_bounds,
      gfx::Size last_layer_content_bounds,
      gfx::Size current_layer_content_bounds,
      float last_layer_contents_scale,
      float current_layer_contents_scale,
      const gfx::Transform& last_screen_transform,
      const gfx::Transform& current_screen_transform,
      int current_source_frame_number,
      double current_frame_time);

  void DidBecomeActive();

  // For a given rect, iterates through tiles that can fill it.  If no
  // set of tiles with resources can fill the rect, then it will iterate
  // through null tiles with valid geometry_rect() until the rect is full.
  // If all tiles have resources, the union of all geometry_rects will
  // exactly fill rect with no overlap.
  class CC_EXPORT Iterator {
   public:
    Iterator(const PictureLayerTilingSet* set,
      float contents_scale,
      gfx::Rect content_rect,
      float ideal_contents_scale,
      PictureLayerTiling::LayerDeviceAlignment layerDeviceAlignment);
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

    PictureLayerTiling* CurrentTiling();

   private:
    int NextTiling() const;

    const PictureLayerTilingSet* set_;
    float contents_scale_;
    float ideal_contents_scale_;
    PictureLayerTiling::LayerDeviceAlignment layer_device_alignment_;
    PictureLayerTiling::Iterator tiling_iter_;
    int current_tiling_;
    int ideal_tiling_;

    Region current_region_;
    Region missing_region_;
    Region::Iterator region_iter_;
  };

 private:
  PictureLayerTilingClient* client_;
  gfx::Size layer_bounds_;
  ScopedPtrVector<PictureLayerTiling> tilings_;

  friend class Iterator;
};

}  // namespace cc

#endif  // CC_PICTURE_LAYER_TILING_SET_H_
