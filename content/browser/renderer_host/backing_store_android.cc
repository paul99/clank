// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/backing_store_android.h"

#include "base/android/jni_android.h"
#include "content/browser/android/chrome_view.h"
#include "content/browser/android/jni_helper.h"
#include "content/browser/renderer_host/render_widget_host.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/public/browser/render_process_host.h"
#include "ui/gfx/rect.h"
#include "third_party/skia/include/core/SkCanvas.h"

using base::android::AttachCurrentThread;

TileBackingStore::TileBackingStore() {}
TileBackingStore::~TileBackingStore() {}

jobject TileBackingStore::GetJavaBitmap() {
  LOG(ERROR) << "Called GetJavaBitmap on TileBackingStore, returning NULL";
  return NULL;
}

/*
int gNumBackingStores = 0;
*/

BackingStoreAndroid::BackingStoreAndroid(RenderWidgetHost* widget,
                                         const gfx::Size& size)
    : BackingStore(widget, size) {
  base::android::ScopedJavaLocalRef<jobject> bitmap =
      CreateJavaBitmap(size, true);

  // Convert to a weak global ref, since we should not rely on local refs
  // remaining consistent over a long period.  (We don't use a regular
  // global ref because a Java-side data structure holds it -- this approach
  // makes the memory usage more visible to memory analysis tools.)
  JNIEnv* env = AttachCurrentThread();
  java_bitmap_ = env->NewWeakGlobalRef(bitmap.obj());

  /*
  LOG(INFO) << "Creating backing store " << ++gNumBackingStores;
  */
}

BackingStoreAndroid::~BackingStoreAndroid() {
  if (java_bitmap_) {
    JNIEnv* env = AttachCurrentThread();
    jobject local_bitmap = env->NewLocalRef(java_bitmap_);
    env->DeleteWeakGlobalRef(java_bitmap_);
    // Tell the Java data structure to release the memory.
    DeleteJavaBitmap(local_bitmap);
    env->DeleteLocalRef(local_bitmap);

    /*
    LOG(INFO) << "Destructing backing store " << --gNumBackingStores;
    */
  }
}

void BackingStoreAndroid::PaintToBackingStore(
    content::RenderProcessHost* process,
    TransportDIB::Id bitmap,
    const gfx::Rect& bitmap_rect,
    const std::vector<gfx::Rect>& copy_rects,
    const base::Closure& completion_callback,
    bool* scheduled_completion_callback) {
  *scheduled_completion_callback = false;
  PaintToBackingStoreWithOffset(process, bitmap, bitmap_rect,
                                copy_rects, gfx::Point(0, 0));
}

void BackingStoreAndroid::PaintToBackingStoreWithOffset(
    content::RenderProcessHost* process,
    TransportDIB::Id bitmap,
    const gfx::Rect& bitmap_rect,
    const std::vector<gfx::Rect>& copy_rects,
    const gfx::Point& backing_store_offset) {
  if (bitmap_rect.IsEmpty())
    return;

  TransportDIB* dib = process->GetTransportDIB(bitmap);
  if (!dib)
    return;
  const void* src_pixels = dib->memory();

  PaintBuf(src_pixels, bitmap_rect, copy_rects, backing_store_offset);
}

void BackingStoreAndroid::PaintSkBitmapToBackingStore(
    const SkBitmap& bitmap) {
  gfx::Rect rect(0, 0, bitmap.width(), bitmap.height());
  std::vector<gfx::Rect> copy_rects;
  copy_rects.push_back(rect);
  SkAutoLockPixels src_lock(bitmap);
  PaintBuf(bitmap.getPixels(), rect, copy_rects, gfx::Point(0, 0));
}

void BackingStoreAndroid::FillWithColor(SkColor color) {
  AutoLockJavaBitmap dst_lock(java_bitmap_);
  int* dst_pixels = static_cast<int*>(dst_lock.pixels());

#if SK_B32_SHIFT
  // Convert color from BGRA to the Java convention of RGBA.
  color = (color & 0xff00ff00) | ((color & 0xff) << 16) |
      ((color & 0xff0000) >> 16);
#endif

  // TODO(aelias): There's an android_memset32 in
  // system/core/include/cutils/memory.h.  However, it wouldn't be NDK-compliant
  // to link it in.  The real optimization would be to avoid color fills almost
  // entirely in any case.
  int num_pixels = size().width() * size().height();
  for (int i = 0; i < num_pixels; i++) {
    dst_pixels[i] = color;
  }
}

void BackingStoreAndroid::PaintJavaBitmapToBackingStoreWithOffset(
    jobject jsource_bitmap,
    const gfx::Rect& source_rect,
    const gfx::Rect& dest_rect) {
  PaintJavaBitmapToJavaBitmap(jsource_bitmap, source_rect,
                              java_bitmap_, dest_rect);
}

jweak BackingStoreAndroid::GetJavaBitmap() {
  return java_bitmap_;
}

void BackingStoreAndroid::PaintBuf(
    const void* src_pixels,
    const gfx::Rect& bitmap_rect,
    const std::vector<gfx::Rect>& copy_rects,
    const gfx::Point& backing_store_offset) {
  if (!src_pixels)
    return;

  const int src_width = bitmap_rect.width();
  const int src_height = bitmap_rect.height();
  if (src_width <= 0 || src_height <= 0) {
    NOTREACHED() << "PaintToBackingStore: bad bitmap";
    return;
  }

  AutoLockJavaBitmap dst_lock(java_bitmap_);
  void* dst_pixels = dst_lock.pixels();

  // Short-cut for the full screen copy. This saves 5% of the copy time.
  if (copy_rects.size() == 1 &&
      bitmap_rect.size() == size() &&
      copy_rects[0] == gfx::Rect(0, 0, src_width, src_height) &&
      backing_store_offset == gfx::Point(0, 0)) {
    memcpy(dst_pixels, src_pixels, (src_width * src_height) << 2);
    return;
  }

  // Blitter than handles a bitmap and copy rect which don't
  // completely intersect with the cached_bitmap.  We hit this case
  // when using the browser-side tile cache, since an update may come
  // from the renderer which needs to get split across multiple tiles.
  //
  // Example input:
  // bitmap_rect: 0,-400 480x2877 copy_rect[0]: 0,-400 480x2877
  // cached_bitmap w.h: 240,200
  //
  // In contrast, the normal software path has a single backing store
  // that covers the entire screen bounds, and offscreen updates do
  // not happen.
  for (size_t i = 0; i < copy_rects.size(); i++) {
    gfx::Rect copy_rect = copy_rects[i];

    DCHECK(bitmap_rect.Contains(copy_rect));

    // Get overlap area
    gfx::Rect cached_bitmap_rect = gfx::Rect(backing_store_offset, size());
    const gfx::Rect intersection = copy_rect.Intersect(cached_bitmap_rect);
    if (intersection.IsEmpty())
      continue;

    const uint32* src_base_ptr = static_cast<const uint32*>(src_pixels);
    int src_x_start = intersection.x() - bitmap_rect.x();
    int src_y_start = intersection.y() - bitmap_rect.y();
    int src_rowpixels = bitmap_rect.width();

    uint32* dst_base_ptr = static_cast<uint32*>(dst_pixels);
    int dst_x_start = intersection.x() - backing_store_offset.x();
    int dst_y_start = intersection.y() - backing_store_offset.y();
    int dst_rowpixels = size().width();

    // variables for both
    int copy_width = intersection.width() << 2;  // memcpy is in bytes

    for (int y = 0; y < intersection.height(); y++) {
      memcpy(dst_base_ptr + (dst_rowpixels*(dst_y_start+y)) + dst_x_start,
             src_base_ptr + (src_rowpixels*(src_y_start+y)) + src_x_start,
             copy_width);
    }
  }
}

bool BackingStoreAndroid::CopyFromBackingStore(const gfx::Rect& rect,
                                               skia::PlatformCanvas* output) {
  DCHECK(size() == rect.size());
  if (size() != rect.size())
    return false;

  if (!output->initialize(rect.width(), rect.height(), true))
    return false;

  // PlatformCanvas on Android is just a bitmap. So do a simple bitmap copy.
  const SkBitmap& bitmap = GetTopDevice(*output)->accessBitmap(true);
  if (bitmap.bytesPerPixel() != 4)
    return false;

  // Lock the target SkBitmap.
  SkAutoLockPixels dst_lock(bitmap);
  void* dst_pixels = bitmap.getPixels();
  DCHECK(dst_pixels);

  // Lock the source Java bitmap.
  AutoLockJavaBitmap src_lock(java_bitmap_);
  void* src_pixels = src_lock.pixels();

  memcpy(dst_pixels, src_pixels, bitmap.getSize());

  return true;
}

void BackingStoreAndroid::ScrollBackingStore(int dx, int dy,
                                             const gfx::Rect& clip_rect,
                                             const gfx::Size& view_size) {
  if (dx == 0 && dy == 0)
    return;  // Nothing to do.

  DCHECK(size() == view_size);
  if (size() != view_size)
    return;

  gfx::Rect clip = clip_rect.Intersect(gfx::Rect(size()));

  // Compute the set of pixels we'll actually end up painting.
  gfx::Rect dest_rect = clip;
  dest_rect.Offset(dx, dy);
  dest_rect = dest_rect.Intersect(clip);
  if (dest_rect.IsEmpty())
    return;  // Nothing to do.

  // Compute the source pixels that will map to the dest_rect
  gfx::Rect src_rect = dest_rect;
  src_rect.Offset(-dx, -dy);

  // Get a pointer to the pixels.
  AutoLockJavaBitmap lock(java_bitmap_);
  uint32* pixels = static_cast<uint32*>(lock.pixels());

  int width = size().width();
  size_t row_bytes = dest_rect.width() << 2;
  if (dy != 0 && dx == 0) {
      // Vertical-only scroll. Use memmove which handles overlap.
      memmove(pixels + dest_rect.y() * width,
              pixels + src_rect.y() * width,
              row_bytes * dest_rect.height());
  } else if (dy > 0) {
    // Data is moving down, copy from the bottom up.
    for (int y = dest_rect.height() - 1; y >= 0; y--) {
      memcpy(pixels + dest_rect.x() + (dest_rect.y() + y) * width,
             pixels + src_rect.x() + (src_rect.y() + y) * width,
             row_bytes);
    }
  } else if (dy < 0) {
    // Data is moving up, copy from the top down.
    for (int y = 0; y < dest_rect.height(); y++) {
      memcpy(pixels + dest_rect.x() + (dest_rect.y() + y) * width,
             pixels + src_rect.x() + (src_rect.y() + y) * width,
             row_bytes);
    }
  } else {
    // Horizontal-only scroll. Use memmove which handles overlap.
    for (int y = 0; y < dest_rect.height(); y++) {
      memmove(pixels + dest_rect.x() + (dest_rect.y() + y) * width,
              pixels + src_rect.x() + (src_rect.y() + y) * width,
              row_bytes);
    }
  }
}
