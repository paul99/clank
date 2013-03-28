// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PICTURE_LAYER_H_
#define CC_PICTURE_LAYER_H_

#include "cc/contents_scaling_layer.h"
#include "cc/layer.h"
#include "cc/picture_pile.h"
#include "cc/occlusion_tracker.h"

namespace cc {

class ContentLayerClient;
class ResourceUpdateQueue;
struct RenderingStats;

class CC_EXPORT PictureLayer : public ContentsScalingLayer {
public:
  static scoped_refptr<PictureLayer> create(ContentLayerClient*);

  void clearClient() { client_ = 0; }

  // Implement Layer interface
  virtual bool drawsContent() const OVERRIDE;
  virtual scoped_ptr<LayerImpl> createLayerImpl(
      LayerTreeImpl* treeImpl) OVERRIDE;
  virtual void pushPropertiesTo(LayerImpl*) OVERRIDE;
  virtual void setNeedsDisplayRect(const gfx::RectF& layerRect) OVERRIDE;
  virtual void update(ResourceUpdateQueue&, const OcclusionTracker*,
                      RenderingStats&) OVERRIDE;

protected:
  explicit PictureLayer(ContentLayerClient*);
  virtual ~PictureLayer();

private:
  ContentLayerClient* client_;
  PicturePile pile_;
  // Invalidation to use the next time update is called.
  Region pending_invalidation_;
  // Invalidation from the last time update was called.
  Region pile_invalidation_;
};

}  // namespace cc

#endif  // CC_PICTURE_LAYER_H_
