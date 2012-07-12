// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_EXTERNAL_POPUP_MENU_H_
#define CONTENT_RENDERER_EXTERNAL_POPUP_MENU_H_

#include <vector>

#include "base/basictypes.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebExternalPopupMenu.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebPopupMenuInfo.h"

class RenderViewImpl;
namespace WebKit {
class WebExternalPopupMenuClient;
}

class ExternalPopupMenu : public WebKit::WebExternalPopupMenu {
 public:
  ExternalPopupMenu(RenderViewImpl* render_view,
                    const WebKit::WebPopupMenuInfo& popup_menu_info,
                    WebKit::WebExternalPopupMenuClient* popup_menu_client);

  virtual ~ExternalPopupMenu() {}

  // Called when the user has selected an item. |selected_item| is -1 if the
  // user canceled the popup.
  void DidSelectItem(int selected_index);

#if defined(OS_ANDROID)
  // Called when the user has selected items or canceled the popup.
  void DidSelectItems(bool canceled, const std::vector<int>& selected_indices);
#endif

  // WebKit::WebExternalPopupMenu implementation:
  virtual void show(const WebKit::WebRect& bounds);
  virtual void close();

 private:
  RenderViewImpl* render_view_;
  WebKit::WebPopupMenuInfo popup_menu_info_;
  WebKit::WebExternalPopupMenuClient* popup_menu_client_;

  DISALLOW_COPY_AND_ASSIGN(ExternalPopupMenu);
};

#endif  // CONTENT_RENDERER_EXTERNAL_POPUP_MENU_H_
