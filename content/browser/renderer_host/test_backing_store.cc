// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/test_backing_store.h"

TestBackingStore::TestBackingStore(RenderWidgetHost* widget,
                                   const gfx::Size& size)
    : BackingStore(widget, size) {
}

TestBackingStore::~TestBackingStore() {
}

void TestBackingStore::PaintToBackingStore(
    content::RenderProcessHost* process,
    TransportDIB::Id bitmap,
    const gfx::Rect& bitmap_rect,
    const std::vector<gfx::Rect>& copy_rects,
    const base::Closure& completion_callback,
    bool* scheduled_completion_callback) {
  *scheduled_completion_callback = false;
}

bool TestBackingStore::CopyFromBackingStore(const gfx::Rect& rect,
                                            skia::PlatformCanvas* output) {
  return false;
}

void TestBackingStore::ScrollBackingStore(int dx, int dy,
                                          const gfx::Rect& clip_rect,
                                          const gfx::Size& view_size) {
}

#if defined(OS_ANDROID)
TestTileBackingStore::TestTileBackingStore(const gfx::Size& size) {}

TestTileBackingStore::~TestTileBackingStore() {}

void TestTileBackingStore::PaintToBackingStoreWithOffset(
    content::RenderProcessHost* process,
    TransportDIB::Id bitmap,
    const gfx::Rect& bitmap_rect,
    const std::vector<gfx::Rect>& copy_rects,
    const gfx::Point& backing_store_offset) {}

void TestTileBackingStore::PaintSkBitmapToBackingStore(
    const SkBitmap& bitmap) {}

void TestTileBackingStore::FillWithColor(SkColor color) {}

void TestTileBackingStore::PaintJavaBitmapToBackingStoreWithOffset(
    jobject jsource_bitmap,
    const gfx::Rect& source_rect,
    const gfx::Rect& dest_rect) {}
#endif
