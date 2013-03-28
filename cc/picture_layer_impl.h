// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PICTURE_LAYER_IMPL_H_
#define CC_PICTURE_LAYER_IMPL_H_

#include "cc/layer_impl.h"
#include "cc/picture_layer_tiling.h"
#include "cc/picture_layer_tiling_set.h"
#include "cc/picture_pile_impl.h"
#include "cc/scoped_ptr_vector.h"

namespace cc {

struct AppendQuadsData;
class QuadSink;

class CC_EXPORT PictureLayerImpl : public LayerImpl,
                                   public PictureLayerTilingClient {
public:
  static scoped_ptr<PictureLayerImpl> create(LayerTreeImpl* treeImpl, int id)
  {
      return make_scoped_ptr(new PictureLayerImpl(treeImpl, id));
  }
  virtual ~PictureLayerImpl();

  // LayerImpl overrides.
  virtual const char* layerTypeAsString() const OVERRIDE;
  virtual void appendQuads(QuadSink&, AppendQuadsData&) OVERRIDE;
  virtual void dumpLayerProperties(std::string*, int indent) const OVERRIDE;
  virtual void didUpdateTransforms() OVERRIDE;

  void didUpdateBounds();

  // PictureLayerTilingClient overrides.
  virtual scoped_refptr<Tile> CreateTile(PictureLayerTiling*,
                                         gfx::Rect) OVERRIDE;

  // PushPropertiesTo active tree => pending tree
  void SyncFromActiveLayer();
  void SyncTiling(const PictureLayerTiling* tiling);

protected:
  PictureLayerImpl(LayerTreeImpl* treeImpl, int id);
  void AddTiling(float contents_scale, gfx::Size tile_size);
  void SyncFromActiveLayer(const PictureLayerImpl* other);

  PictureLayerTilingSet tilings_;
  scoped_refptr<PicturePileImpl> pile_;
  Region invalidation_;

  gfx::Transform last_screen_space_transform_;
  double last_update_time_;
  gfx::Size last_bounds_;
  gfx::Size last_content_bounds_;
  float last_content_scale_x_;
  float last_content_scale_y_;

  friend class PictureLayer;
  DISALLOW_COPY_AND_ASSIGN(PictureLayerImpl);
};

}

#endif  // CC_PICTURE_LAYER_IMPL_H_
