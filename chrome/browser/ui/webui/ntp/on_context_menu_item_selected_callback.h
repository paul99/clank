// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_NTP_ON_CONTEXT_MENU_ITEM_SELECTED_CALLBACK_H_
#define CHROME_BROWSER_UI_WEBUI_NTP_ON_CONTEXT_MENU_ITEM_SELECTED_CALLBACK_H_

#include "base/memory/ref_counted.h"

class ContextMenuHandler;

// This class is used by the ContextMenuHandler to create an intermediary object
// as ContextMenuHandler is not RefCounted and therefore cannot be used in a
// callback (and we cannot use an Unretained callbacks as we have no guarantee
// that the ContextMenuHandler will outlive the callback).
class OnContextMenuItemSelectedCallBack
    : public base::RefCounted<OnContextMenuItemSelectedCallBack> {
 public:
  explicit OnContextMenuItemSelectedCallBack(ContextMenuHandler* context_menu_handler);

  // Called by the object responsible for showing the menu when a menu is
  // selected (in Clank's case, ChromeView).
  void ItemSelected(int item_id);

  // Called by the ContextMenuHandler when its is destroyed so we can clear our
  // reference to it.
  void ClearContextMenuHandler();

 private:
  friend class base::RefCounted<OnContextMenuItemSelectedCallBack>;
  ~OnContextMenuItemSelectedCallBack();

  // Weak reference; we have a contract with this ContextMenuHandler to call our
  // ClearContextMenuHander method when it becomes invalid.
  ContextMenuHandler* context_menu_handler_;

  DISALLOW_COPY_AND_ASSIGN(OnContextMenuItemSelectedCallBack);
};

#endif  // CHROME_BROWSER_UI_WEBUI_NTP_ON_CONTEXT_MENU_ITEM_SELECTED_CALLBACK_H_
