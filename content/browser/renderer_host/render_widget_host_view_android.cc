// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_android.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/utf_string_conversions.h"
#include "content/browser/android/device_info.h"
#include "content/browser/android/chrome_view.h"
#include "content/browser/gpu/gpu_process_host_ui_shim.h"
#include "content/browser/gpu/gpu_surface_tracker.h"
#include "content/browser/renderer_host/backing_store_android.h"
#include "content/browser/renderer_host/render_widget_host.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/common/view_messages.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/render_process_host.h"
#include "googleurl/src/gurl.h"
#include "ui/gfx/size.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkColorPriv.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebCompositionUnderline.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/android/WebInputEventFactory.h"

RenderWidgetHostViewAndroid::RenderWidgetHostViewAndroid(
    RenderWidgetHost* widget_host, const base::WeakPtr<ChromeView>& chrome_view)
    : host_(widget_host),
      // In m18, we only support TextureView version. is_hidden_ will be reset when the
      // SurfaceTexture is attached.
      is_hidden_(true),
      chrome_view_(chrome_view),
      ime_adapter_android_(ALLOW_THIS_IN_INITIALIZER_LIST(this)),
      scroll_active_(false),
      pinch_active_(false),
      cached_background_color_(SK_ColorWHITE),
      gpu_host_id_(0),
      surface_route_id_(MSG_ROUTING_NONE) {
  host_->SetView(this);
  // RenderWidgetHost is initialized as visible. If is_hidden_ is true, tell
  // RenderWidgetHost to hide.
  if (is_hidden_)
    host_->WasHidden();
}

RenderWidgetHostViewAndroid::~RenderWidgetHostViewAndroid() {
}

void RenderWidgetHostViewAndroid::InitAsChild(gfx::NativeView parent_view) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewAndroid::InitAsPopup(
    RenderWidgetHostView* parent_host_view, const gfx::Rect& pos) {
  NOTIMPLEMENTED();
}

void RenderWidgetHostViewAndroid::InitAsFullscreen(
    RenderWidgetHostView* reference_host_view) {
  NOTIMPLEMENTED();
}

RenderWidgetHost* RenderWidgetHostViewAndroid::GetRenderWidgetHost() const {
  return host_;
}

void RenderWidgetHostViewAndroid::DidBecomeSelected() {
  if (!is_hidden_)
    return;
  is_hidden_ = false;
  host_->WasRestored();
}

void RenderWidgetHostViewAndroid::WasHidden() {
  if (is_hidden_)
    return;

  // If we receive any more paint messages while we are hidden, we want to
  // ignore them so we don't re-allocate the backing store.  We will paint
  // everything again when we become visible again.
  //
  is_hidden_ = true;

  // Inform the renderer that we are being hidden so it can reduce its resource
  // utilization.
  host_->WasHidden();
}

void RenderWidgetHostViewAndroid::SetSize(const gfx::Size& size) {
  // Update the size of the RWH.
  if (requested_size_.width() != size.width() ||
      requested_size_.height() != size.height()) {
    requested_size_ = gfx::Size(size.width(), size.height());
    host_->WasResized();
  }
}

void RenderWidgetHostViewAndroid::SetBounds(const gfx::Rect& rect) {
  // TODO(clank): Looks like SetSize() sets requested_size_, but
  // nobody ever looks at that variable again.  Seems like we can do
  // a similar wrong thing here.
  if (rect.origin().x() || rect.origin().y()) {
    VLOG(0) << "SetBounds not implemented for (x,y)!=(0,0)";
  }
  gfx::Size size(rect.width(), rect.height());
  SetSize(size);
}

gfx::NativeView RenderWidgetHostViewAndroid::GetNativeView() const {
  return chrome_view_;
}

gfx::NativeViewId RenderWidgetHostViewAndroid::GetNativeViewId() const {
  return reinterpret_cast<gfx::NativeViewId>(
      const_cast<RenderWidgetHostViewAndroid*>(this));
}

gfx::NativeViewAccessible RenderWidgetHostViewAndroid::GetNativeViewAccessible() {
  NOTIMPLEMENTED();
  return NULL;
}

void RenderWidgetHostViewAndroid::MovePluginWindows(
    const std::vector<webkit::npapi::WebPluginGeometry>& moves) {
  // We don't have plugin windows on Android. Do nothing. Note: this is called
  // from RenderWidgetHost::OnMsgUpdateRect which is itself invoked while
  // processing the corresponding message from Renderer.
}

void RenderWidgetHostViewAndroid::Focus() {
  host_->Focus();
  host_->SetInputMethodActive(true);
}

void RenderWidgetHostViewAndroid::Blur() {
  host_->Send(new ViewMsg_ExecuteEditCommand(host_->routing_id(), "Unselect", ""));
  host_->SetInputMethodActive(false);
  host_->Blur();
}

bool RenderWidgetHostViewAndroid::HasFocus() const {
  if (!chrome_view_)
    return false;  // ChromeView not created yet.

  return chrome_view_->HasFocus();
}

void RenderWidgetHostViewAndroid::Show() {
  // TODO(huanr)
  // On other platforms, Show and Hide() cause the windowing system to show/
  // hide the widget and then call DidBecomeSelected / WasHidden. On Android,
  // We never want to show a white page while we are loading so Show() and
  // Hide() are simply non-implementation. If we ever want to show an empty
  // page while loading, make this method call DidBecomeSelected()
}

void RenderWidgetHostViewAndroid::Hide() {
  // See comment in the method Show() above.
}

bool RenderWidgetHostViewAndroid::IsShowing() {
  return !is_hidden_;
}

gfx::Rect RenderWidgetHostViewAndroid::GetViewBounds() const {
  if (chrome_view_) {
    return chrome_view_->GetBounds();
  } else {
    // The ChromeView has not been created yet. This only happens when renderer
    // asks for creating new window, for example, javascript window.open().
    return gfx::Rect(0, 0, 0, 0);
  }
}

void RenderWidgetHostViewAndroid::UpdateCursor(const WebCursor& cursor) {
  // There are no cursors on Android.
}

void RenderWidgetHostViewAndroid::SetIsLoading(bool is_loading) {
  // Do nothing. The UI notification is handled through ChromeViewClient which
  // is TabContentsDelegate.
}

void RenderWidgetHostViewAndroid::ImeUpdateTextInputState(
    const ViewHostMsg_TextInputState_Params& params) {
  if (is_hidden_)
    return;

  // See http://b/issue?id=3248163 about focus, IME and native text input.
  chrome_view_->ImeUpdateAdapter(
      reinterpret_cast<int>(&ime_adapter_android_),
      static_cast<int>(params.type), params.caret_rect,
      params.value, params.selection_start, params.selection_end,
      params.composition_start, params.composition_end,
      false /* show_ime_if_needed */,
      params.request_time);
}

int RenderWidgetHostViewAndroid::GetNativeImeAdapter() {
  return reinterpret_cast<int>(&ime_adapter_android_);
}

void RenderWidgetHostViewAndroid::ImeCancelComposition() {
  ime_adapter_android_.CancelComposition();
}

void RenderWidgetHostViewAndroid::DidUpdateBackingStore(
    const gfx::Rect& scroll_rect, int scroll_dx, int scroll_dy,
    const std::vector<gfx::Rect>& copy_rects) {
  if (is_hidden_)
    return;

  if (chrome_view_)
    chrome_view_->DidUpdateBackingStore();
}

void RenderWidgetHostViewAndroid::RenderViewGone(
    base::TerminationStatus status, int error_code) {
  Destroy();
}

void RenderWidgetHostViewAndroid::Destroy() {
  chrome_view_.reset();

  // The RenderWidgetHost's destruction led here, so don't call it.
  host_ = NULL;

  MessageLoop::current()->DeleteSoon(FROM_HERE, this);
}

void
RenderWidgetHostViewAndroid::SetTooltipText(const string16& tooltip_text) {
  // Tooltips don't makes sense on Android.
}

void RenderWidgetHostViewAndroid::SelectionChanged(const string16& text,
                                                   size_t offset,
                                                   const ui::Range& range) {
  RenderWidgetHostView::SelectionChanged(text, offset, range);

  if (text.empty() || range.is_empty() || !chrome_view_)
    return;
  size_t pos = range.GetMin() - offset;
  size_t n = range.length();

  DCHECK(pos + n <= text.length()) << "The text can not fully cover range.";
  if (pos >= text.length()) {
    NOTREACHED() << "The text can not cover range.";
    return;
  }

  std::string utf8_selection = UTF16ToUTF8(text.substr(pos, n));

  chrome_view_->OnSelectionChanged(utf8_selection);
}

void RenderWidgetHostViewAndroid::SelectionBoundsChanged(
    const gfx::Rect& start_rect,
    base::i18n::TextDirection start_dir,
    const gfx::Rect& end_rect,
    base::i18n::TextDirection end_dir) {
  if (chrome_view_ && host_) {
      chrome_view_->OnSelectionBoundsChanged(start_rect.x(),
                                             start_rect.bottom(),
                                             start_dir,
                                             end_rect.x(),
                                             end_rect.bottom(),
                                             end_dir);
  }
}

void RenderWidgetHostViewAndroid::ShowingContextMenu(bool showing) {
  NOTIMPLEMENTED();
}

BackingStore* RenderWidgetHostViewAndroid::AllocBackingStore(
    const gfx::Size& size) {
  // Allocate a backing store for software mode.
  return new BackingStoreAndroid(host_, size);
}

void RenderWidgetHostViewAndroid::SetBackground(const SkBitmap& background) {
  RenderWidgetHostView::SetBackground(background);
  host_->Send(new ViewMsg_SetBackground(host_->routing_id(), background));
}

void RenderWidgetHostViewAndroid::ShowPressStateAck(
    const std::vector<gfx::Rect>& rects) {
  if (is_hidden_ || !chrome_view_)
    return;

  DCHECK(!rects.empty());

  if (rects.size() == 1) {
    chrome_view_->SetLastPressAck(rects[0].x(),
                                  rects[0].y(),
                                  rects[0].width(),
                                  rects[0].height());
  }
}

void RenderWidgetHostViewAndroid::ProcessTouchEndAck(
    const ViewHostMsg_TextInputState_Params& params) {
  if (is_hidden_)
    return;

  // Show the keyboard.
  chrome_view_->ImeUpdateAdapter(
      reinterpret_cast<int>(&ime_adapter_android_),
      static_cast<int>(params.type), params.caret_rect,
      params.value, params.selection_start, params.selection_end,
      params.composition_start, params.composition_end,
      true /* show_ime_if_needed */, params.request_time);
}

void RenderWidgetHostViewAndroid::MultipleTargetsTouched(const gfx::Rect& rect,
                                                         const gfx::Size& size,
                                                         const TransportDIB* dib) {
  if (!chrome_view_)
    return;

  if (rect.width() <= 0 || rect.height() <= 0)
    return;
  chrome_view_->SetOnDemandZoomBitmap(size, dib->memory(), dib->size());
  chrome_view_->ShowOnDemandZoom(rect);
}

void RenderWidgetHostViewAndroid::OnAcceleratedCompositingStateChange() {
  const bool activated = host_->is_accelerated_compositing_active();
  if (chrome_view_)
    chrome_view_->OnAcceleratedCompositingStateChange(this, activated, false);
}

void RenderWidgetHostViewAndroid::AcceleratedSurfaceBuffersSwapped(
    const GpuHostMsg_AcceleratedSurfaceBuffersSwapped_Params& params,
    int gpu_host_id) {
  NOTREACHED();
}

void RenderWidgetHostViewAndroid::AcceleratedSurfacePostSubBuffer(
    const GpuHostMsg_AcceleratedSurfacePostSubBuffer_Params& params,
    int gpu_host_id) {
  NOTREACHED();
}

void RenderWidgetHostViewAndroid::NativeWindowChanged(bool attached) {
  if (chrome_view_)
    chrome_view_->OnNativeWindowChanged(this, attached);
}

void RenderWidgetHostViewAndroid::CompositorResized(uint64 timestamp,
                                                    const gfx::Size& size) {
  if (chrome_view_)
    chrome_view_->OnCompositorResized(timestamp, size);
}

void RenderWidgetHostViewAndroid::StartContentIntent(
    const GURL& content_url) {
  if (chrome_view_)
    chrome_view_->StartContentIntent(content_url);
}

gfx::PluginWindowHandle RenderWidgetHostViewAndroid::GetCompositingSurface() {
  // On Android, we cannot generate a window handle that can be passed to the
  // GPU process through the native side. Instead, we send the surface handle through
  // Binder after the compositing context has been created.
  return gfx::kDummyPluginWindow;
}

void RenderWidgetHostViewAndroid::GetScreenInfo(WebKit::WebScreenInfo* result) {
  // ScreenInfo isn't tied to the widget on Android. Always return the default.
  RenderWidgetHostView::GetDefaultScreenInfo(result);
}

// megamerge: this is a lie; it returns the WebView bounds, not the
// root window bounds.  However, the render_widget_host_view_views.cc
// implementation of GetViewBounds() calls GetScreenBounds(), which is
// also a bit of a lie.  TODO: find out the implications and answer
// correctly.
gfx::Rect RenderWidgetHostViewAndroid::GetRootWindowBounds() {
  return GetViewBounds();
}

void RenderWidgetHostViewAndroid::UnhandledWheelEvent(const WebKit::WebMouseWheelEvent& event) {
  // intentionally empty, like RenderWidgetHostViewViews
}

void RenderWidgetHostViewAndroid::ProcessTouchAck(bool processed) {
  // intentionally empty, like RenderWidgetHostViewViews
}

void RenderWidgetHostViewAndroid::SetHasHorizontalScrollbar(bool has_horizontal_scrollbar) {
  // intentionally empty, like RenderWidgetHostViewViews
}

void RenderWidgetHostViewAndroid::SetScrollOffsetPinning(
    bool is_pinned_to_left, bool is_pinned_to_right) {
  // intentionally empty, like RenderWidgetHostViewViews
}

bool RenderWidgetHostViewAndroid::LockMouse() {
  NOTIMPLEMENTED();
  return false;
}

void RenderWidgetHostViewAndroid::UnlockMouse() {
  NOTIMPLEMENTED();
}

// Methods called from the host to the render

void RenderWidgetHostViewAndroid::SendKeyEvent(
    const NativeWebKeyboardEvent& event) {
  if (host_)
    host_->ForwardKeyboardEvent(event);
}

void RenderWidgetHostViewAndroid::TouchEvent(
    const WebKit::WebTouchEvent& event) {
  if (host_)
    host_->ForwardTouchEvent(event);
}


void RenderWidgetHostViewAndroid::MouseEvent(
    const WebKit::WebMouseEvent& event) {
  if (host_)
    host_->ForwardMouseEvent(event);
}

void RenderWidgetHostViewAndroid::MouseWheelEvent(
    const WebKit::WebMouseWheelEvent& event) {
if (host_)
  host_->ForwardWheelEvent(event);
}

void RenderWidgetHostViewAndroid::ShowPressState(int x, int y) {
  if (host_)
    host_->Send(new ViewMsg_ShowPressState(host_->routing_id(), x, y));
}

void RenderWidgetHostViewAndroid::SingleTap(int x, int y,
                                            bool check_multiple_targets) {
  if (host_)
    host_->Send(new ViewMsg_SingleTap(host_->routing_id(), x, y,
                                      check_multiple_targets));
}

void RenderWidgetHostViewAndroid::DoubleTap(int x, int y) {
  if (host_)
    host_->Send(new ViewMsg_DoubleTap(host_->routing_id(), x, y));
}

void RenderWidgetHostViewAndroid::LongPress(int x, int y,
                                            bool check_multiple_targets) {
  if (host_)
    host_->Send(new ViewMsg_LongPress(host_->routing_id(), x, y,
                                      check_multiple_targets));
}

void RenderWidgetHostViewAndroid::SelectRange(const gfx::Point& start,
                                              const gfx::Point& end) {
  if (host_)
    host_->SelectRange(start, end);
}

void RenderWidgetHostViewAndroid::ZoomToRect(const gfx::Rect& rect) {
  if (host_)
    host_->Send(new ViewMsg_ZoomToRect(host_->routing_id(), rect));
}

void RenderWidgetHostViewAndroid::SetCachedBackgroundColor(SkColor color) {
  cached_background_color_ = color;
}

SkColor RenderWidgetHostViewAndroid::GetCachedBackgroundColor() const {
  return cached_background_color_;
}

void RenderWidgetHostViewAndroid::SetCachedPageScaleFactorLimits(
    float minimum_scale,
    float maximum_scale) {
  if (chrome_view_)
    chrome_view_->UpdatePageScaleLimits(minimum_scale, maximum_scale);
}

void RenderWidgetHostViewAndroid::UpdateFrameInfo(
    const gfx::Point& scroll_offset,
    float page_scale_factor,
    const gfx::Size& content_size) {
  if (chrome_view_) {
    chrome_view_->UpdateContentSize(content_size.width(),
                                    content_size.height());
    chrome_view_->UpdateScrollOffsetAndPageScaleFactor(scroll_offset.x(),
                                                       scroll_offset.y(),
                                                       page_scale_factor);
  }
}

void RenderWidgetHostViewAndroid::AcceleratedSurfaceNew(
    int route_id, int host_id) {
  surface_route_id_ = route_id;
  gpu_host_id_ = host_id;
}

void RenderWidgetHostViewAndroid::ScrollBegin(int x, int y) {
  if (host_) {
    WebKit::WebGestureEvent scroll_event;
    scroll_event.type = WebKit::WebInputEvent::GestureScrollBegin;
    scroll_event.x = x;
    scroll_event.y = y;
    host_->ForwardGestureEvent(scroll_event);
  }
}

void RenderWidgetHostViewAndroid::ScrollBy(int dx, int dy) {
  if (host_) {
    scroll_active_ = true;

    WebKit::WebGestureEvent scroll_event;
    scroll_event.type = WebKit::WebInputEvent::GestureScrollUpdate;
    scroll_event.deltaX = dx;
    scroll_event.deltaY = dy;
    host_->ForwardGestureEvent(scroll_event);
  }
}

void RenderWidgetHostViewAndroid::ScrollEnd() {
  if (host_) {
    scroll_active_ = false;

    WebKit::WebGestureEvent scroll_event;
    scroll_event.type = WebKit::WebInputEvent::GestureScrollEnd;
    host_->ForwardGestureEvent(scroll_event);
  }
}

void RenderWidgetHostViewAndroid::PinchBegin() {
  if (host_) {
    WebKit::WebGestureEvent pinch_event;
    pinch_event.type = WebKit::WebInputEvent::GesturePinchBegin;
    host_->ForwardGestureEvent(pinch_event);
  }
}

void RenderWidgetHostViewAndroid::PinchBy(float delta, int x, int y) {
  if (host_) {
    pinch_active_ = true;

    WebKit::WebGestureEvent pinch_event;
    pinch_event.type = WebKit::WebInputEvent::GesturePinchUpdate;
    pinch_event.x = x;
    pinch_event.y = y;
    pinch_event.deltaX = pinch_event.deltaY = delta;
    host_->ForwardGestureEvent(pinch_event);
  }
}

void RenderWidgetHostViewAndroid::PinchEnd() {
  if (host_) {
    pinch_active_ = false;

    WebKit::WebGestureEvent pinch_event;
    pinch_event.type = WebKit::WebInputEvent::GesturePinchEnd;
    host_->ForwardGestureEvent(pinch_event);
  }
}

void RenderWidgetHostViewAndroid::FlingStart(int x, int y, int vx, int vy) {
  if (host_) {
    WebKit::WebGestureEvent fling_event;
    fling_event.type = WebKit::WebInputEvent::GestureFlingStart;
    fling_event.x = x;
    fling_event.y = y;
    fling_event.deltaX = vx;
    fling_event.deltaY = vy;
    host_->ForwardGestureEvent(fling_event);
  }
}

void RenderWidgetHostViewAndroid::FlingCancel() {
  if (host_) {
    WebKit::WebGestureEvent fling_event;
    fling_event.type = WebKit::WebInputEvent::GestureFlingCancel;
    host_->ForwardGestureEvent(fling_event);
  }
}

void RenderWidgetHostViewAndroid::SetGPUProcess(base::ProcessHandle handle) {
  if (chrome_view_)
    chrome_view_->SetGPUProcess(handle);
}

void RenderWidgetHostViewAndroid::SetChromeView(
    const base::WeakPtr<ChromeView>& chrome_view) {
  chrome_view_ = chrome_view;
  if (host_) {
    GpuSurfaceTracker::Get()->SetSurfaceHandle(
        host_->surface_id(),
        chrome_view.get() ? gfx::kDummyPluginWindow : gfx::kNullPluginWindow);
  }
}

void RenderWidgetHostViewAndroid::AcknowledgeSwapBuffers() {
  GpuProcessHostUIShim* ui_shim = GpuProcessHostUIShim::FromID(gpu_host_id_);
  DCHECK(ui_shim && surface_route_id_ != MSG_ROUTING_NONE);
  if (ui_shim)
    ui_shim->Send(new AcceleratedSurfaceMsg_BuffersSwappedACK(
        surface_route_id_));
}

void RenderWidgetHostViewAndroid::SendVSyncEvent(
    base::TimeTicks frameBeginTime, base::TimeDelta currentFrameInterval) {
  if (host_)
    host_->ForwardVSyncEvent(frameBeginTime, currentFrameInterval);
}

// static
void RenderWidgetHostView::GetDefaultScreenInfo(
    WebKit::WebScreenInfo* results) {
  scoped_ptr<DeviceInfo> info(new DeviceInfo());
  const int width = info->GetWidth();
  const int height = info->GetHeight();
  results->horizontalDPI = 160 * info->GetDPIScale();
  results->verticalDPI = 160 * info->GetDPIScale();
  results->depth = info->GetBitsPerPixel();
  results->depthPerComponent = info->GetBitsPerComponent();
  results->isMonochrome = (results->depthPerComponent == 0);
  results->rect = WebKit::WebRect(0, 0, width, height);
  // TODO(husky): Remove any system controls from availableRect.
  results->availableRect = WebKit::WebRect(0, 0, width, height);
}

////////////////////////////////////////////////////////////////////////////////
// RenderWidgetHostView, public:

// static
RenderWidgetHostView* RenderWidgetHostView::CreateViewForWidget(
    RenderWidgetHost* widget) {
  return new RenderWidgetHostViewAndroid(widget, base::WeakPtr<ChromeView>());
}
