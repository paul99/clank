// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TAB_CONTENTS_TAB_CONTENTS_VIEW_ANDROID_H_
#define CONTENT_BROWSER_TAB_CONTENTS_TAB_CONTENTS_VIEW_ANDROID_H_
#pragma once

#include "base/memory/weak_ptr.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/web_contents_view.h"
#include "content/browser/tab_contents/tab_contents_view_helper.h"

class ChromeView;

class TabContentsViewAndroid : public content::WebContentsView,
                               public content::NotificationObserver {
 public:
  explicit TabContentsViewAndroid(content::WebContents* web_contents);
  virtual ~TabContentsViewAndroid();

  void SetChromeView(const base::WeakPtr<ChromeView>& chrome_view);

  // WebContentsView implementation --------------------------------------------

  virtual void CreateView(const gfx::Size& initial_size) OVERRIDE;

  virtual RenderWidgetHostView* CreateViewForWidget(
      RenderWidgetHost* render_widget_host) OVERRIDE;

  virtual gfx::NativeView GetNativeView() const OVERRIDE;

  virtual gfx::NativeView GetContentNativeView() const OVERRIDE;

  virtual gfx::NativeWindow GetTopLevelNativeWindow() const OVERRIDE;

  virtual void GetContainerBounds(gfx::Rect *out) const OVERRIDE;

  virtual void SetPageTitle(const string16& title) OVERRIDE;

  virtual void OnTabCrashed(base::TerminationStatus status,
                            int error_code,
                            const base::ProcessHandle handle) OVERRIDE;

  virtual void SizeContents(const gfx::Size& size) OVERRIDE;

  virtual void RenderViewCreated(RenderViewHost* host) OVERRIDE;

  virtual void Focus() OVERRIDE;

  virtual void SetInitialFocus() OVERRIDE;

  virtual void StoreFocus() OVERRIDE;

  virtual void RestoreFocus() OVERRIDE;

  virtual void UpdatePreferredSize(const gfx::Size& pref_size);
  virtual bool IsDoingDrag() const OVERRIDE;
  virtual void CancelDragAndCloseTab() OVERRIDE;
  virtual bool IsEventTracking() const OVERRIDE;
  virtual void CloseTabAfterEventTracking() OVERRIDE;

  virtual void GetViewBounds(gfx::Rect* out) const OVERRIDE;

  virtual void ConfirmTouchEvent(bool handled) OVERRIDE;

  virtual void DidSetNeedTouchEvents(bool need_touch_events) OVERRIDE;

  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  virtual void CreateNewWindow(
      int route_id,
      const ViewHostMsg_CreateWindow_Params& params) OVERRIDE;
  virtual void CreateNewWidget(int route_id,
                               WebKit::WebPopupType popup_type) OVERRIDE;
  virtual void CreateNewFullscreenWidget(int route_id) OVERRIDE;
  virtual void ShowCreatedWindow(int route_id,
                                 WindowOpenDisposition disposition,
                                 const gfx::Rect& initial_pos,
                                 bool user_gesture) OVERRIDE;
  virtual void ShowCreatedWidget(int route_id,
                                 const gfx::Rect& initial_pos) OVERRIDE;
  virtual void ShowCreatedFullscreenWidget(int route_id) OVERRIDE;
  virtual void ShowContextMenu(const ContextMenuParams& params) OVERRIDE;

  virtual void ShowPopupMenu(const gfx::Rect& bounds,
                             int item_height,
                             double item_font_size,
                             int selected_item,
                             const std::vector<WebMenuItem>& items,
                             bool right_aligned,
                             bool multiple) OVERRIDE;

  virtual void StartDragging(const WebDropData& drop_data,
                             WebKit::WebDragOperationsMask allowed_ops,
                             const SkBitmap& image,
                             const gfx::Point& image_offset) OVERRIDE;

  virtual void UpdateDragCursor(WebKit::WebDragOperation operation) OVERRIDE;

  virtual void GotFocus() OVERRIDE;

  virtual void TakeFocus(bool reverse) OVERRIDE;

  virtual void InstallOverlayView(gfx::NativeView view) OVERRIDE;
  virtual void RemoveOverlayView() OVERRIDE;

 private:
  // The WebContents whose contents we display.
  content::WebContents* web_contents_;

  // Common implementations of some RenderViewHostDelegate::View methods.
  TabContentsViewHelper tab_contents_view_helper_;

  base::WeakPtr<ChromeView> chrome_view_;

  DISALLOW_COPY_AND_ASSIGN(TabContentsViewAndroid);
};

#endif  // CONTENT_BROWSER_TAB_CONTENTS_TAB_CONTENTS_VIEW_ANDROID_H_
