// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_BACKING_STORE_ANDROID_H_
#define CHROME_BROWSER_RENDERER_HOST_BACKING_STORE_ANDROID_H_
#pragma once

#include <jni.h>

#include "base/compiler_specific.h"

#include "content/browser/renderer_host/backing_store.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/size.h"

class SkCanvas;

namespace gfx {
class Point;
}

// Interface for a backing store for use with browser tiling.  Contains
// methods complementing BackingStore.
class TileBackingStore {
 public:
  virtual ~TileBackingStore();

  // Like PaintToBackingStore() but provide an offset instead of assuming the
  // backing store starts at (0,0).
  virtual void PaintToBackingStoreWithOffset(
      content::RenderProcessHost* process,
      TransportDIB::Id bitmap,
      const gfx::Rect& bitmap_rect,
      const std::vector<gfx::Rect>& copy_rects,
      const gfx::Point& backing_store_offset) = 0;

  // Fill the tile with an SkBitmap.
  virtual void PaintSkBitmapToBackingStore(const SkBitmap& bitmap) = 0;
  // Fill the tile with a color.
  virtual void FillWithColor(SkColor color) = 0;

  // Fill the tile with the given com.android.Bitmap from the
  // specified offset.
  virtual void PaintJavaBitmapToBackingStoreWithOffset(
      jobject jsource_bitmap,
      const gfx::Rect& source_rect,
      const gfx::Rect& dest_rect) = 0;

  // Available in software mode.
  virtual jweak GetJavaBitmap();

 protected:
  TileBackingStore();

 private:
  DISALLOW_COPY_AND_ASSIGN(TileBackingStore);
};

// Software backing store backed by a Java Bitmap.  Can be used either as a
// fullscreen backing store or for an individual browser tile (in browser
// tiling mode).
class BackingStoreAndroid : public BackingStore, public TileBackingStore {
 public:
  // Create a backing store on Android.
  BackingStoreAndroid(RenderWidgetHost* widget, const gfx::Size& size);

  virtual ~BackingStoreAndroid();

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

  // TileBackingStore implementation.
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

  virtual jweak GetJavaBitmap() OVERRIDE;

 private:
  void PaintBuf(const void* src_pixels,
                const gfx::Rect& bitmap_rect,
                const std::vector<gfx::Rect>& copy_rects,
                const gfx::Point& backing_store_offset);

  jweak java_bitmap_;

  DISALLOW_COPY_AND_ASSIGN(BackingStoreAndroid);
};

#endif  // CHROME_BROWSER_RENDERER_HOST_BACKING_STORE_ANDROID_H_
