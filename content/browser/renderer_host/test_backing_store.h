// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_TEST_TEST_BACKING_STORE_H_
#define CONTENT_BROWSER_RENDERER_HOST_TEST_TEST_BACKING_STORE_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "content/browser/renderer_host/backing_store.h"

#if defined(OS_ANDROID)
#include "content/browser/renderer_host/backing_store_android.h"
#endif

class TestBackingStore : public BackingStore {
 public:
  TestBackingStore(RenderWidgetHost* widget, const gfx::Size& size);
  virtual ~TestBackingStore();

  // BackingStore implementation.
  virtual void PaintToBackingStore(
      content::RenderProcessHost* process,
      TransportDIB::Id bitmap,
      const gfx::Rect& bitmap_rect,
      const std::vector<gfx::Rect>& copy_rects,
      const base::Closure& completion_callback,
      bool* scheduled_completion_callback) OVERRIDE;
  virtual bool CopyFromBackingStore(const gfx::Rect& rect,
                                    skia::PlatformCanvas* output) OVERRIDE;
  virtual void ScrollBackingStore(int dx, int dy,
                                  const gfx::Rect& clip_rect,
                                  const gfx::Size& view_size) OVERRIDE;
 private:
  DISALLOW_COPY_AND_ASSIGN(TestBackingStore);
};

#if defined(OS_ANDROID)
class TestTileBackingStore : public TileBackingStore {
 public:
  explicit TestTileBackingStore(const gfx::Size& size);
  virtual ~TestTileBackingStore();

  virtual void PaintToBackingStoreWithOffset(
      content::RenderProcessHost* process,
      TransportDIB::Id bitmap,
      const gfx::Rect& bitmap_rect,
      const std::vector<gfx::Rect>& copy_rects,
      const gfx::Point& backing_store_offset) OVERRIDE;

  virtual void PaintSkBitmapToBackingStore(const SkBitmap& bitmap) OVERRIDE;
  virtual void FillWithColor(SkColor color) OVERRIDE;
  virtual void PaintJavaBitmapToBackingStoreWithOffset(
      jobject jsource_bitmap,
      const gfx::Rect& source_rect,
      const gfx::Rect& dest_rect) OVERRIDE;
 private:
  DISALLOW_COPY_AND_ASSIGN(TestTileBackingStore);
};
#endif


#endif  // CONTENT_BROWSER_RENDERER_HOST_TEST_TEST_BACKING_STORE_H_
