// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/render_pass.h"

namespace cc {

scoped_ptr<RenderPass> RenderPass::Create() {
  return make_scoped_ptr(new RenderPass);
}

RenderPass::RenderPass()
    : id(Id(-1, -1)),
      has_transparent_background(true),
      has_occlusion_from_outside_target_surface(false) {
}

RenderPass::~RenderPass() {
}

scoped_ptr<RenderPass> RenderPass::Copy(Id new_id) const {
  DCHECK(new_id != id);

  scoped_ptr<RenderPass> copy_pass(Create());
  copy_pass->SetAll(new_id,
                    output_rect,
                    damage_rect,
                    transform_to_root_target,
                    has_transparent_background,
                    has_occlusion_from_outside_target_surface,
                    filters,
                    filter,
                    background_filters);
  return copy_pass.Pass();
}

void RenderPass::SetNew(Id id,
                        gfx::Rect output_rect,
                        gfx::RectF damage_rect,
                        const gfx::Transform& transform_to_root_target) {
  DCHECK_GT(id.layer_id, 0);
  DCHECK_GE(id.index, 0);

  this->id = id;
  this->output_rect = output_rect;
  this->damage_rect = damage_rect;
  this->transform_to_root_target = transform_to_root_target;

  DCHECK(quad_list.isEmpty());
  DCHECK(shared_quad_state_list.isEmpty());
}

void RenderPass::SetAll(Id id,
                        gfx::Rect output_rect,
                        gfx::RectF damage_rect,
                        const gfx::Transform& transform_to_root_target,
                        bool has_transparent_background,
                        bool has_occlusion_from_outside_target_surface,
                        const WebKit::WebFilterOperations& filters,
                        const skia::RefPtr<SkImageFilter>& filter,
                        const WebKit::WebFilterOperations& background_filters) {
  DCHECK_GT(id.layer_id, 0);
  DCHECK_GE(id.index, 0);

  this->id = id;
  this->output_rect = output_rect;
  this->damage_rect = damage_rect;
  this->transform_to_root_target = transform_to_root_target;
  this->has_transparent_background = has_transparent_background;
  this->has_occlusion_from_outside_target_surface =
      has_occlusion_from_outside_target_surface;
  this->filters = filters;
  this->filter = filter;
  this->background_filters = background_filters;

  DCHECK(quad_list.isEmpty());
  DCHECK(shared_quad_state_list.isEmpty());
}

}  // namespace cc
