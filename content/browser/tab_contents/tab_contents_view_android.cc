// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tab_contents/tab_contents_view_android.h"

#include "base/logging.h"
#include "content/public/browser/notification_observer.h"
#include "content/browser/android/chrome_view.h"
#include "content/browser/media/media_player_delegate_android.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/renderer_host/render_widget_host.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/browser/renderer_host/render_view_host_factory.h"
#include "content/browser/tab_contents/interstitial_page.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/public/browser/web_contents_delegate.h"

TabContentsViewAndroid::TabContentsViewAndroid(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {
}

TabContentsViewAndroid::~TabContentsViewAndroid() {
}

void TabContentsViewAndroid::SetChromeView(
    const base::WeakPtr<ChromeView>& chrome_view) {
  chrome_view_ = chrome_view;
  RenderWidgetHostViewAndroid* rwhv = static_cast<RenderWidgetHostViewAndroid*>(
      web_contents_->GetRenderWidgetHostView());
  if (rwhv)
    rwhv->SetChromeView(chrome_view_);
  if (web_contents_->ShowingInterstitialPage()) {
    rwhv = static_cast<RenderWidgetHostViewAndroid*>(
        web_contents_->GetInterstitialPage()->GetRenderViewHost()->view());
    if (rwhv)
      rwhv->SetChromeView(chrome_view_);
  }
}

void TabContentsViewAndroid::CreateView(const gfx::Size& initial_size) {
}

RenderWidgetHostView* TabContentsViewAndroid::CreateViewForWidget(
    RenderWidgetHost* render_widget_host) {
  if (render_widget_host->view()) {
    // During testing, the view will already be set up in most cases to the
    // test view, so we don't want to clobber it with a real one. To verify that
    // this actually is happening (and somebody isn't accidentally creating the
    // view twice), we check for the RVH Factory, which will be set when we're
    // making special ones (which go along with the special views).
    DCHECK(RenderViewHostFactory::has_factory());
    return render_widget_host->view();
  }
  // Note that while this instructs the render widget host to reference
  // |native_view_|, this has no effect without also instructing the
  // native view (i.e. ChromeView) how to obtain a reference to this widget in
  // order to paint it. See ChromeView::GetRenderWidgetHostViewAndroid for an
  // example of how this is achieved for InterstitialPages.
  return new RenderWidgetHostViewAndroid(render_widget_host, chrome_view_);
}

gfx::NativeView TabContentsViewAndroid::GetNativeView() const {
  return chrome_view_;
}

gfx::NativeView TabContentsViewAndroid::GetContentNativeView() const {
  return chrome_view_;
}

gfx::NativeWindow TabContentsViewAndroid::GetTopLevelNativeWindow() const {
  return chrome_view_;
}

void TabContentsViewAndroid::GetContainerBounds(gfx::Rect* out) const {
  if (chrome_view_) {
    *out = chrome_view_->GetBounds();
  }
}

void TabContentsViewAndroid::SetPageTitle(const string16& title) {
  if (chrome_view_)
    chrome_view_->SetTitle(title);
}

void TabContentsViewAndroid::OnTabCrashed(base::TerminationStatus status,
                                          int error_code,
                                          const base::ProcessHandle handle) {
  RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh && rvh->media_player_delegate())
    rvh->media_player_delegate()->CleanUpAllPlayers();

  if (chrome_view_)
    chrome_view_->OnTabCrashed(handle);
}

void TabContentsViewAndroid::SizeContents(const gfx::Size& size) {
  // TODO(klobag): Do we need to do anything else?
  RenderWidgetHostView* rwhv = web_contents_->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetSize(size);
}

void TabContentsViewAndroid::RenderViewCreated(RenderViewHost* host) {
}

void TabContentsViewAndroid::Focus() {
  if (web_contents_->ShowingInterstitialPage()) {
    web_contents_->GetInterstitialPage()->Focus();
  } else {
    web_contents_->GetRenderWidgetHostView()->Focus();
  }
}

void TabContentsViewAndroid::SetInitialFocus() {
  if (web_contents_->FocusLocationBarByDefault())
    web_contents_->SetFocusToLocationBar(false);
  else
    Focus();
}

void TabContentsViewAndroid::StoreFocus() {
  NOTIMPLEMENTED();
}

void TabContentsViewAndroid::RestoreFocus() {
  NOTIMPLEMENTED();
}

void TabContentsViewAndroid::UpdatePreferredSize(const gfx::Size& pref_size) {
}

bool TabContentsViewAndroid::IsDoingDrag() const {
  return false;
}

void TabContentsViewAndroid::CancelDragAndCloseTab() {
}

bool TabContentsViewAndroid::IsEventTracking() const {
  return false;
}

void TabContentsViewAndroid::CloseTabAfterEventTracking() {
}

void TabContentsViewAndroid::GotFocus() {
  // This is only used in the views FocusManager stuff but it bleeds through
  // all subclasses. http://crbug.com/21875
}

// This is called when we the renderer asks us to take focus back (i.e., it has
// iterated past the last focusable element on the page).
void TabContentsViewAndroid::TakeFocus(bool reverse) {
  if (web_contents_->GetDelegate() &&
      web_contents_->GetDelegate()->TakeFocus(reverse))
    return;
  web_contents_->GetRenderWidgetHostView()->Focus();
}

void TabContentsViewAndroid::GetViewBounds(gfx::Rect* out) const {
  if (chrome_view_) {
    gfx::Rect view_size = chrome_view_->GetBounds();
    out->SetRect(view_size.x(),
                 view_size.y(),
                 view_size.width(),
                 view_size.height());
  }
}

void TabContentsViewAndroid::ConfirmTouchEvent(bool handled) {
  if (chrome_view_)
    chrome_view_->ConfirmTouchEvent(handled);
}

void TabContentsViewAndroid::DidSetNeedTouchEvents(bool need_touch_events) {
  if (chrome_view_)
    chrome_view_->DidSetNeedTouchEvents(need_touch_events);
}

void TabContentsViewAndroid::Observe(int type,
                                     const content::NotificationSource& source,
                                     const content::NotificationDetails& details) {
  switch (type) {
    default:
      NOTREACHED()<< "Got a notification we didn't register for.";
      break;
  }
}

void TabContentsViewAndroid::CreateNewWindow(
    int route_id,
    const ViewHostMsg_CreateWindow_Params& params) {
  tab_contents_view_helper_.CreateNewWindow(
      web_contents_, route_id, params);
}

void TabContentsViewAndroid::CreateNewWidget(
    int route_id, WebKit::WebPopupType popup_type) {
  tab_contents_view_helper_.CreateNewWidget(
      web_contents_, route_id, false, popup_type);
}

void TabContentsViewAndroid::CreateNewFullscreenWidget(int route_id) {
  tab_contents_view_helper_.CreateNewWidget(
      web_contents_, route_id, true, WebKit::WebPopupTypeNone);
}

void TabContentsViewAndroid::ShowCreatedWindow(int route_id,
                                           WindowOpenDisposition disposition,
                                           const gfx::Rect& initial_pos,
                                           bool user_gesture) {
  tab_contents_view_helper_.ShowCreatedWindow(
      web_contents_, route_id, disposition, initial_pos, user_gesture);
}

void TabContentsViewAndroid::ShowCreatedWidget(
    int route_id, const gfx::Rect& initial_pos) {
  tab_contents_view_helper_.ShowCreatedWidget(
      web_contents_, route_id, false, initial_pos);
}

void TabContentsViewAndroid::ShowCreatedFullscreenWidget(int route_id) {
  tab_contents_view_helper_.ShowCreatedWidget(
      web_contents_, route_id, true, gfx::Rect(0, 0));
}

void TabContentsViewAndroid::ShowContextMenu(const ContextMenuParams& params) {
  if (chrome_view_)
    chrome_view_->ShowContextMenu(params);
}

void TabContentsViewAndroid::ShowPopupMenu(
    const gfx::Rect& bounds,
    int item_height,
    double item_font_size,
    int selected_item,
    const std::vector<WebMenuItem>& items,
    bool right_aligned,
    bool multiple) {
  if (chrome_view_)
    chrome_view_->ShowSelectPopupMenu(items, selected_item, multiple);
}

void TabContentsViewAndroid::StartDragging(
    const WebDropData& drop_data,
    WebKit::WebDragOperationsMask allowed_ops,
    const SkBitmap& image,
    const gfx::Point& image_offset) {
  NOTIMPLEMENTED();
}

void TabContentsViewAndroid::UpdateDragCursor(WebKit::WebDragOperation op) {
  NOTIMPLEMENTED();
}

void TabContentsViewAndroid::InstallOverlayView(gfx::NativeView view) {
  NOTIMPLEMENTED();
}

void TabContentsViewAndroid::RemoveOverlayView() {
  NOTIMPLEMENTED();
}
