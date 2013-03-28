// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TILE_H_
#define CC_TILE_H_

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "cc/layer_tree_host_impl.h"
#include "cc/picture_pile_impl.h"
#include "cc/resource_provider.h"
#include "cc/tile_manager.h"
#include "cc/tile_priority.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

namespace cc {

class Tile;

class CC_EXPORT Tile : public base::RefCounted<Tile> {
 public:
  Tile(TileManager* tile_manager,
       PicturePileImpl* picture_pile,
       gfx::Size tile_size,
       GLenum format,
       gfx::Rect content_rect,
       float contents_scale);

  PicturePileImpl* picture_pile() {
    return picture_pile_.get();
  }

  const TilePriority& priority(WhichTree tree) const {
    return priority_[tree];
  }

  TilePriority combined_priority() const {
    return TilePriority(priority_[ACTIVE_TREE],
                        priority_[PENDING_TREE]);
  }

  void set_priority(WhichTree tree, const TilePriority& priority);

  // Returns 0 if not drawable.
  ResourceProvider::ResourceId GetResourceId() const;

  const gfx::Rect& opaque_rect() const { return opaque_rect_; }

  bool contents_swizzled() const { return managed_state_.contents_swizzled; }

 private:
  // Methods called by by tile manager.
  friend class TileManager;
  friend class BinComparator;
  ManagedTileState& managed_state() { return managed_state_; }
  const ManagedTileState& managed_state() const { return managed_state_; }
  size_t bytes_consumed_if_allocated() const;
  float contents_scale() const { return contents_scale_; }

  // Normal private methods.
  friend class base::RefCounted<Tile>;
  ~Tile();

  TileManager* tile_manager_;
  scoped_refptr<PicturePileImpl> picture_pile_;
  gfx::Rect tile_size_;
  GLenum format_;
  gfx::Rect content_rect_;
  float contents_scale_;
  gfx::Rect opaque_rect_;

  TilePriority priority_[2];
  ManagedTileState managed_state_;
};

}  // namespace cc

#endif  // CC_TILE_H_
