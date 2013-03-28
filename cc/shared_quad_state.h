// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_SHARED_QUAD_STATE_H_
#define CC_SHARED_QUAD_STATE_H_

#include "base/memory/scoped_ptr.h"
#include "cc/cc_export.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/transform.h"

namespace cc {

class CC_EXPORT SharedQuadState {
 public:
  static scoped_ptr<SharedQuadState> Create();
  ~SharedQuadState();

  scoped_ptr<SharedQuadState> Copy() const;

  void SetAll(const gfx::Transform& content_to_target_transform,
              const gfx::Rect& visible_content_rect,
              const gfx::Rect& clipped_rect_in_target,
              const gfx::Rect& clip_rect,
              bool is_clipped,
              float opacity);

  // Transforms from quad's original content space to its target content space.
  gfx::Transform content_to_target_transform;
  // This rect lives in the content space for the quad's originating layer.
  gfx::Rect visible_content_rect;
  gfx::Rect clipped_rect_in_target;
  gfx::Rect clip_rect;
  bool is_clipped;
  float opacity;

 private:
  SharedQuadState();
};

}

#endif  // CC_SHARED_QUAD_STATE_H_
