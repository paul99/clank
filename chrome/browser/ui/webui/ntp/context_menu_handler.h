// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_NTP_CONTEXT_MENU_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_NTP_CONTEXT_MENU_HANDLER_H_

#include "base/memory/ref_counted.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
class ListValue;
}

class OnContextMenuItemSelectedCallBack;

// The handler for Javascript messages related to the context menus.
// Its the job of the actual HTML page to intercept the contextmenu event,
// disable it and show its own custom menu.
class ContextMenuHandler : public content::WebUIMessageHandler {
 public:
  ContextMenuHandler();
  virtual ~ContextMenuHandler();

  // WebUIMessageHandler override and implementation.
  virtual void RegisterMessages() OVERRIDE;

  // Invoked (by on_item_selected_callback_) when an item has been selected.
  void OnItemSelected(int item_id);

  // Callback for the "showContextMenu" message.
  void HandleShowContextMenu(const base::ListValue* args);

  // Callback for the "hideContextMenu" message.
  void HandleHideContextMenu(const base::ListValue* args);

  // Below are the message that are so far only triggered by the context menu.
  // They should be moved to other files is they become used from other places.

  // Callback for the "openInNewTab" message.
  void HandleOpenInNewTab(const base::ListValue* args);

  // Callback for the "openInIncognitoTab" message.
  void HandleOpenInIncognitoTab(const base::ListValue* args);

  // Called by the OnContextMenuItemSelectedCallBack.
  void SetOnItemSelectedCallback(OnContextMenuItemSelectedCallBack* callback);

  // Called by "Recently closed items" to prevent duplicates in the NTP.
  // TODO(jcivelli): b/5862826 This is a hack to prevent duplicates in the NTP
  //                 when recently closed from redirected pages is clicked.
  //                 Should be removed when that bug is fixed.
  void HandleOpenAsNonAutoBookmark(const base::ListValue* args);

 private:
  // Clears our reference to the callback and clears the callback's reference to
  // us.
  void ReleaseCallback();

  static std::string GetURLFromParams(const base::ListValue* args);

  // The callback invoked when an item is selected. That callback might outlive
  // us, so we make sure to clear the reference the callback has to us when
  // we are destroyed.
  scoped_refptr<OnContextMenuItemSelectedCallBack> on_item_selected_callback_;

  DISALLOW_COPY_AND_ASSIGN(ContextMenuHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_NTP_CONTEXT_MENU_HANDLER_H_
