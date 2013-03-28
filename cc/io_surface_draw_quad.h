// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_IO_SURFACE_DRAW_QUAD_H_
#define CC_IO_SURFACE_DRAW_QUAD_H_

#include "base/memory/scoped_ptr.h"
#include "cc/cc_export.h"
#include "cc/draw_quad.h"
#include "ui/gfx/size.h"

namespace cc {

class CC_EXPORT IOSurfaceDrawQuad : public DrawQuad {
 public:
  enum Orientation {
    FLIPPED,
    UNFLIPPED
  };

  static scoped_ptr<IOSurfaceDrawQuad> Create();

  void SetNew(const SharedQuadState* shared_quad_state,
              gfx::Rect rect,
              gfx::Rect opaque_rect,
              gfx::Size io_surface_size,
              unsigned io_surface_texture_id,
              Orientation orientation);

  void SetAll(const SharedQuadState* shared_quad_state,
              gfx::Rect rect,
              gfx::Rect opaque_rect,
              gfx::Rect visible_rect,
              bool needs_blending,
              gfx::Size io_surface_size,
              unsigned io_surface_texture_id,
              Orientation orientation);

  gfx::Size io_surface_size;
  unsigned io_surface_texture_id;
  Orientation orientation;

  static const IOSurfaceDrawQuad* MaterialCast(const DrawQuad*);
 private:
  IOSurfaceDrawQuad();
};

}

#endif  // CC_IO_SURFACE_DRAW_QUAD_H_
