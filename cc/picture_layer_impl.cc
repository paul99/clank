// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/picture_layer_impl.h"

#include "base/time.h"
#include "cc/append_quads_data.h"
#include "cc/checkerboard_draw_quad.h"
#include "cc/debug_border_draw_quad.h"
#include "cc/debug_colors.h"
#include "cc/layer_tree_impl.h"
#include "cc/math_util.h"
#include "cc/quad_sink.h"
#include "cc/solid_color_draw_quad.h"
#include "cc/tile_draw_quad.h"
#include "ui/gfx/quad_f.h"

namespace cc {

PictureLayerImpl::PictureLayerImpl(LayerTreeImpl* treeImpl, int id)
    : LayerImpl(treeImpl, id),
      tilings_(this),
      pile_(PicturePileImpl::Create()),
      last_update_time_(0) {
}

PictureLayerImpl::~PictureLayerImpl() {
}

const char* PictureLayerImpl::layerTypeAsString() const {
  return "PictureLayer";
}

void PictureLayerImpl::appendQuads(QuadSink& quadSink,
                                   AppendQuadsData& appendQuadsData) {

  const gfx::Rect& rect = visibleContentRect();
  gfx::Rect content_rect(gfx::Point(), contentBounds());

  SharedQuadState* sharedQuadState =
      quadSink.useSharedQuadState(createSharedQuadState());
  bool clipped = false;
  gfx::QuadF target_quad = MathUtil::mapQuad(
      drawTransform(),
      gfx::QuadF(rect),
      clipped);
  bool isAxisAlignedInTarget = !clipped && target_quad.IsRectilinear();
  bool useAA = !isAxisAlignedInTarget;

  if (showDebugBorders()) {
    for (PictureLayerTilingSet::Iterator iter(&tilings_,
                                              contentsScaleX(),
                                              rect);
         iter;
         ++iter) {
      SkColor color;
      float width;
      if (*iter && iter->GetResourceId()) {
        color = DebugColors::TileBorderColor();
        width = DebugColors::TileBorderWidth(layerTreeImpl());
      } else {
        color = DebugColors::MissingTileBorderColor();
        width = DebugColors::MissingTileBorderWidth(layerTreeImpl());
      }

      scoped_ptr<DebugBorderDrawQuad> debugBorderQuad =
          DebugBorderDrawQuad::Create();
      gfx::Rect geometry_rect = iter.geometry_rect();
      debugBorderQuad->SetNew(sharedQuadState, geometry_rect, color, width);
      quadSink.append(debugBorderQuad.PassAs<DrawQuad>(), appendQuadsData);
    }
  }

  for (PictureLayerTilingSet::Iterator iter(&tilings_, contentsScaleX(), rect);
       iter;
       ++iter) {
    ResourceProvider::ResourceId resource = 0;
    if (*iter)
      resource = iter->GetResourceId();

    gfx::Rect geometry_rect = iter.geometry_rect();

    if (!resource) {
      if (drawCheckerboardForMissingTiles()) {
        // TODO(enne): Figure out how to show debug "invalidated checker" color
        scoped_ptr<CheckerboardDrawQuad> quad = CheckerboardDrawQuad::Create();
        SkColor color = DebugColors::DefaultCheckerboardColor();
        quad->SetNew(sharedQuadState, geometry_rect, color);
        if (quadSink.append(quad.PassAs<DrawQuad>(), appendQuadsData))
            appendQuadsData.numMissingTiles++;
      } else {
        scoped_ptr<SolidColorDrawQuad> quad = SolidColorDrawQuad::Create();
        quad->SetNew(sharedQuadState, geometry_rect, backgroundColor());
        if (quadSink.append(quad.PassAs<DrawQuad>(), appendQuadsData))
            appendQuadsData.numMissingTiles++;
      }
      continue;
    }

    gfx::RectF texture_rect = iter.texture_rect();
    gfx::Rect opaque_rect = iter->opaque_rect();
    opaque_rect.Intersect(content_rect);

    bool outside_left_edge = geometry_rect.x() == content_rect.x();
    bool outside_top_edge = geometry_rect.y() == content_rect.y();
    bool outside_right_edge = geometry_rect.right() == content_rect.right();
    bool outside_bottom_edge = geometry_rect.bottom() == content_rect.bottom();

    scoped_ptr<TileDrawQuad> quad = TileDrawQuad::Create();
    quad->SetNew(sharedQuadState,
                 geometry_rect,
                 opaque_rect,
                 resource,
                 texture_rect,
                 iter.texture_size(),
                 iter->contents_swizzled(),
                 outside_left_edge && useAA,
                 outside_top_edge && useAA,
                 outside_right_edge && useAA,
                 outside_bottom_edge && useAA);
    quadSink.append(quad.PassAs<DrawQuad>(), appendQuadsData);
  }
}

void PictureLayerImpl::dumpLayerProperties(std::string*, int indent) const {
  // TODO(enne): implement me
}

void PictureLayerImpl::didUpdateTransforms() {
  if (drawsContent()) {
    // TODO(enne): Add tilings during pinch zoom
    // TODO(enne): Consider culling old tilings after pinch finishes.
    if (!tilings_.num_tilings()) {
      gfx::Size tile_size = layerTreeImpl()->settings().defaultTileSize;
      AddTiling(contentsScaleX(), tile_size);
      // TODO(enne): Add a low-res tiling as well.
    }
  } else {
    // TODO(enne): This should be unnecessary once there are two trees.
    tilings_.Reset();
  }

  gfx::Transform  current_screen_space_transform = screenSpaceTransform();
  double current_time =
      (base::TimeTicks::Now() - base::TimeTicks()).InSecondsF();
  double time_delta = 0;
  if (last_update_time_ != 0 && last_bounds_ == bounds() &&
      last_content_bounds_ == contentBounds() &&
      last_content_scale_x_ == contentsScaleX() &&
      last_content_scale_y_ == contentsScaleY()) {
    time_delta = current_time - last_update_time_;
  }
  tilings_.UpdateTilePriorities(layerTreeImpl()->device_viewport_size(),
                                contentsScaleX(),
                                contentsScaleY(),
                                last_screen_space_transform_,
                                current_screen_space_transform,
                                time_delta);

  last_screen_space_transform_ = current_screen_space_transform;
  last_update_time_ = current_time;
  last_bounds_ = bounds();
  last_content_bounds_ = contentBounds();
  last_content_scale_x_ = contentsScaleX();
  last_content_scale_y_ = contentsScaleY();
}

void PictureLayerImpl::didUpdateBounds() {
  tilings_.SetLayerBounds(bounds());
}

scoped_refptr<Tile> PictureLayerImpl::CreateTile(PictureLayerTiling* tiling,
                                                 gfx::Rect rect) {
  TileManager* tile_manager = layerTreeImpl()->tile_manager();

  return make_scoped_refptr(new Tile(
      tile_manager,
      pile_.get(),
      rect.size(),
      GL_RGBA,
      rect,
      tiling->contents_scale()));
}

void PictureLayerImpl::SyncFromActiveLayer() {
  DCHECK(layerTreeImpl()->IsPendingTree());
  if (!drawsContent())
    return;

  // If there is an active tree version of this layer, get a copy of its
  // tiles.  This needs to be done last, after setting invalidation and the
  // pile.
  PictureLayerImpl* active_twin = static_cast<PictureLayerImpl*>(
      layerTreeImpl()->FindActiveTreeLayerById(id()));
  if (!active_twin)
    return;
  SyncFromActiveLayer(active_twin);
}

void PictureLayerImpl::SyncFromActiveLayer(const PictureLayerImpl* other) {
  tilings_.CloneAll(other->tilings_, invalidation_);
}

void PictureLayerImpl::SyncTiling(
    const PictureLayerTiling* tiling) {
  tilings_.Clone(tiling, invalidation_);
}

void PictureLayerImpl::AddTiling(float contents_scale, gfx::Size tile_size) {
  const PictureLayerTiling* tiling = tilings_.AddTiling(
      contents_scale,
      tile_size);

  // If a new tiling is created on the active tree, sync it to the pending tree
  // so that it can share the same tiles.
  if (layerTreeImpl()->IsActiveTree())
    return;

  PictureLayerImpl* pending_twin = static_cast<PictureLayerImpl*>(
      layerTreeImpl()->FindPendingTreeLayerById(id()));
  if (!pending_twin)
    return;
  DCHECK_EQ(id(), pending_twin->id());
  pending_twin->SyncTiling(tiling);
}

}  // namespace cc
