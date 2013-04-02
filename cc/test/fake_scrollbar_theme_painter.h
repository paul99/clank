// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_FAKE_SCROLLBAR_THEME_PAINTER_H_
#define CC_TEST_FAKE_SCROLLBAR_THEME_PAINTER_H_

#include "base/memory/scoped_ptr.h"
#include "cc/scrollbar_theme_painter.h"
#include "third_party/skia/include/core/SkColor.h"

namespace cc {

class FakeScrollbarThemePainter : public ScrollbarThemePainter {
 public:
  static scoped_ptr<FakeScrollbarThemePainter> Create(bool paint) {
    return make_scoped_ptr(new FakeScrollbarThemePainter(paint));
  }
  virtual ~FakeScrollbarThemePainter() {}

  virtual void PaintScrollbarBackground(SkCanvas* canvas, const gfx::Rect& rect)
      OVERRIDE;
  virtual void PaintTrackBackground(SkCanvas* canvas, const gfx::Rect& rect)
      OVERRIDE;
  virtual void PaintBackTrackPart(SkCanvas* canvas, const gfx::Rect& rect)
      OVERRIDE;
  virtual void PaintForwardTrackPart(SkCanvas* canvas, const gfx::Rect& rect)
      OVERRIDE;
  virtual void PaintBackButtonStart(SkCanvas* canvas, const gfx::Rect& rect)
      OVERRIDE;
  virtual void PaintBackButtonEnd(SkCanvas* canvas, const gfx::Rect& rect)
      OVERRIDE;
  virtual void PaintForwardButtonStart(SkCanvas* canvas, const gfx::Rect& rect)
      OVERRIDE;
  virtual void PaintForwardButtonEnd(SkCanvas* canvas, const gfx::Rect& rect)
      OVERRIDE;
  virtual void PaintTickmarks(SkCanvas* canvas, const gfx::Rect& rect)
      OVERRIDE;
  virtual void PaintThumb(SkCanvas* canvas, const gfx::Rect& rect)
      OVERRIDE;

  void set_paint(bool paint) { paint_ = paint; }

 private:
  explicit FakeScrollbarThemePainter(bool paint)
      : paint_(paint),
        fill_color_(0) {}

  void Paint(SkCanvas* canvas, gfx::Rect rect);

  bool paint_;
  SkColor fill_color_;

  DISALLOW_COPY_AND_ASSIGN(FakeScrollbarThemePainter);
};

}  // namespace cc

#endif  // CC_TEST_FAKE_SCROLLBAR_BACKGROUND_PAINTER_H_
