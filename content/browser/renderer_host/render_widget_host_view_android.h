// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_ANDROID_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_ANDROID_H_
#pragma once

#include "base/compiler_specific.h"
#include "base/i18n/rtl.h"
#include "base/process.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time.h"
#include "content/browser/renderer_host/ime_adapter_android.h"
#include "content/browser/renderer_host/render_widget_host_view.h"
#include "ui/gfx/path.h"
#include "ui/gfx/size.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebTextInputInfo.h"

class ChromeView;
class GURL;
struct NativeWebKeyboardEvent;
class VideoLayer;
struct ViewHostMsg_TextInputState_Params;

struct GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params;
struct GpuHostMsg_AcceleratedSurfacePostSubBuffer_Params;

namespace WebKit {
class WebTouchEvent;
class WebMouseEvent;
}

// -----------------------------------------------------------------------------
// See comments in render_widget_host_view.h about this class and its members.
// -----------------------------------------------------------------------------
class RenderWidgetHostViewAndroid : public RenderWidgetHostView {
 public:
  RenderWidgetHostViewAndroid(RenderWidgetHost* widget,
                              const base::WeakPtr<ChromeView>& chrome_view);
  virtual ~RenderWidgetHostViewAndroid();

  // RenderWidgetHostView implementation.
  virtual void InitAsChild(gfx::NativeView parent_view) OVERRIDE;
  virtual void InitAsPopup(RenderWidgetHostView* parent_host_view,
                           const gfx::Rect& pos) OVERRIDE;
  virtual void InitAsFullscreen(
      RenderWidgetHostView* reference_host_view) OVERRIDE;
  virtual RenderWidgetHost* GetRenderWidgetHost() const OVERRIDE;
  virtual void DidBecomeSelected() OVERRIDE;
  virtual void WasHidden() OVERRIDE;
  virtual void SetSize(const gfx::Size& size) OVERRIDE;
  virtual void SetBounds(const gfx::Rect& rect) OVERRIDE;
  virtual gfx::NativeView GetNativeView() const OVERRIDE;
  virtual gfx::NativeViewId GetNativeViewId() const OVERRIDE;
  virtual gfx::NativeViewAccessible GetNativeViewAccessible() OVERRIDE;
  virtual void MovePluginWindows(
      const std::vector<webkit::npapi::WebPluginGeometry>& moves) OVERRIDE;
  virtual void Focus() OVERRIDE;
  virtual void Blur() OVERRIDE;
  virtual bool HasFocus() const OVERRIDE;
  virtual void Show() OVERRIDE;
  virtual void Hide() OVERRIDE;
  virtual bool IsShowing() OVERRIDE;
  virtual gfx::Rect GetViewBounds() const OVERRIDE;
  virtual void UpdateCursor(const WebCursor& cursor) OVERRIDE;
  virtual void SetIsLoading(bool is_loading) OVERRIDE;
  virtual void ImeUpdateTextInputState(
      const ViewHostMsg_TextInputState_Params& params) OVERRIDE;
  virtual void ImeCancelComposition() OVERRIDE;
  virtual void DidUpdateBackingStore(
      const gfx::Rect& scroll_rect, int scroll_dx, int scroll_dy,
      const std::vector<gfx::Rect>& copy_rects) OVERRIDE;
  virtual void RenderViewGone(base::TerminationStatus status,
                              int error_code) OVERRIDE;
  virtual void Destroy() OVERRIDE;
  virtual void SetTooltipText(const string16& tooltip_text) OVERRIDE;
  virtual void SelectionChanged(const string16& text,
                                size_t offset,
                                const ui::Range& range) OVERRIDE;
  virtual void SelectionBoundsChanged(const gfx::Rect& start_rect,
                                      base::i18n::TextDirection start_dir,
                                      const gfx::Rect& end_rect,
                                      base::i18n::TextDirection end_dir) OVERRIDE;
  virtual void ShowingContextMenu(bool showing) OVERRIDE;
  virtual BackingStore* AllocBackingStore(const gfx::Size& size) OVERRIDE;
  virtual void OnAcceleratedCompositingStateChange() OVERRIDE;
  virtual void AcceleratedSurfaceBuffersSwapped(
      const GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params& params,
      int gpu_host_id) OVERRIDE;
  virtual void AcceleratedSurfacePostSubBuffer(
      const GpuHostMsg_AcceleratedSurfacePostSubBuffer_Params& params,
      int gpu_host_id) OVERRIDE;
  virtual void SetBackground(const SkBitmap& background) OVERRIDE;
  virtual void ShowPressStateAck(const std::vector<gfx::Rect>& rects) OVERRIDE;
  virtual void ProcessTouchEndAck(
      const ViewHostMsg_TextInputState_Params&) OVERRIDE;
  virtual void MultipleTargetsTouched(const gfx::Rect& rect,
                                      const gfx::Size& size,
                                      const TransportDIB* dib) OVERRIDE;
  virtual void NativeWindowChanged(bool attached) OVERRIDE;
  virtual void CompositorResized(uint64 timestamp,
                                 const gfx::Size& size) OVERRIDE;
  virtual void StartContentIntent(const GURL& content_url) OVERRIDE;

  virtual gfx::PluginWindowHandle GetCompositingSurface() OVERRIDE;

  virtual void SetGPUProcess(base::ProcessHandle handle) OVERRIDE;
  virtual void GetScreenInfo(WebKit::WebScreenInfo* results) OVERRIDE;
  virtual gfx::Rect GetRootWindowBounds() OVERRIDE;
  virtual void UnhandledWheelEvent(
      const WebKit::WebMouseWheelEvent& event) OVERRIDE;
  virtual void ProcessTouchAck(bool processed) OVERRIDE;
  virtual void SetHasHorizontalScrollbar(
      bool has_horizontal_scrollbar) OVERRIDE;
  virtual void SetScrollOffsetPinning(
      bool is_pinned_to_left, bool is_pinned_to_right) OVERRIDE;
  virtual bool LockMouse() OVERRIDE;
  virtual void UnlockMouse() OVERRIDE;

  void SendKeyEvent(const NativeWebKeyboardEvent& event);
  void TouchEvent(const WebKit::WebTouchEvent& event);
  void MouseEvent(const WebKit::WebMouseEvent& event);
  void MouseWheelEvent(const WebKit::WebMouseWheelEvent& event);

  void ShowPressState(int x, int y);

  void SingleTap(int x, int y, bool check_multiple_targets);
  void DoubleTap(int x, int y);
  void LongPress(int x, int y, bool check_multiple_targets);
  void SelectRange(const gfx::Point& start, const gfx::Point& end);

  void ZoomToRect(const gfx::Rect& rect);

  virtual void SetCachedBackgroundColor(SkColor color) OVERRIDE;
  SkColor GetCachedBackgroundColor() const;

  virtual void SetCachedPageScaleFactorLimits(float minimum_scale,
                                              float maximum_scale) OVERRIDE;

  virtual void UpdateFrameInfo(const gfx::Point& scroll_offset,
                               float page_scale_factor,
                               const gfx::Size& content_size) OVERRIDE;

  virtual void AcceleratedSurfaceNew(int route_id, int host_id) OVERRIDE;

  void ScrollBegin(int x, int y);
  void ScrollBy(int dx, int dy);
  void ScrollEnd();
  bool IsScrolling() { return scroll_active_; }

  // Pinch zoom, affecting page scale factor.
  void PinchBegin();
  void PinchBy(float delta, int anchor_x, int anchor_y);
  void PinchEnd();
  bool IsPinching() { return pinch_active_; }

  // Fling from (x, y) with velocity (vx, vy)
  void FlingStart(int x, int y, int vx, int vy);
  void FlingCancel();

  void SetChromeView(const base::WeakPtr<ChromeView>& chrome_view);

  // Do not store the returned pointer, as it may become invalid.
  ChromeView* GetChromeView() const { return chrome_view_; }

  int GetNativeImeAdapter();

  void AcknowledgeSwapBuffers();
  void SendVSyncEvent(base::TimeTicks frameBeginTime,
                      base::TimeDelta currentFrameInterval);

 private:
  // The model object.
  RenderWidgetHost* host_;

  // Whether or not this widget is hidden.
  bool is_hidden_;

  base::WeakPtr<ChromeView> chrome_view_;

  ImeAdapterAndroid ime_adapter_android_;

  // The size that we want the renderer to be.  We keep this in a separate
  // variable because resizing is async.
  gfx::Size requested_size_;

  // Is a scroll currently active?  (That is, ScrollBegin was called, not
  // yet ScrollEnd).
  bool scroll_active_;

  // Is a pinch zoom currently active?
  bool pinch_active_;

  // Body background color of the underlying document.
  SkColor cached_background_color_;

  int gpu_host_id_;
  int surface_route_id_;
};

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_ANDROID_H_
