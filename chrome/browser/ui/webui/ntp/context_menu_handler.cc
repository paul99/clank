// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/ntp/context_menu_handler.h"

#include "base/bind.h"
#include "base/string16.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/ui/webui/ntp/on_context_menu_item_selected_callback.h"
#include "content/browser/android/chrome_view.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/common/page_transition_types.h"

ContextMenuHandler::ContextMenuHandler() {
}

ContextMenuHandler::~ContextMenuHandler() {
  ReleaseCallback();
}

void ContextMenuHandler::OnItemSelected(int item_id) {
  base::FundamentalValue value(item_id);
  web_ui()->CallJavascriptFunction("onCustomMenuSelected", value);
  // Callback has called us, we can release our reference so it gets deleted
  // right away.
  ReleaseCallback();
}

void ContextMenuHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("showContextMenu",
      base::Bind(&ContextMenuHandler::HandleShowContextMenu,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("hideContextMenu",
      base::Bind(&ContextMenuHandler::HandleHideContextMenu,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("openInNewTab",
      base::Bind(&ContextMenuHandler::HandleOpenInNewTab,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("openInIncognitoTab",
      base::Bind(&ContextMenuHandler::HandleOpenInIncognitoTab,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("openAsNonAutoBookmark",
      base::Bind(&ContextMenuHandler::HandleOpenAsNonAutoBookmark,
                 base::Unretained(this)));
}

void ContextMenuHandler::HandleShowContextMenu(
    const ListValue* menu_list_values) {
  // If we have a pending callback, it is now obsolete.
  ReleaseCallback();

  if (menu_list_values->empty()) {
    LOG(WARNING) << "Ignoring request for empty context menu.";
    return;
  }

  // We expect menu_list_values to be of the form:
  // [ [ 1, "title1" ], [ 2, "title2" ], ...]
  // Where the first value in the sub-array is the item id and the second its
  // title.
  std::vector<MenuItemInfo> menu_items;
  for (size_t i = 0; i < menu_list_values->GetSize(); ++i) {
    ListValue* item_list_value = NULL;
    bool valid_value = menu_list_values->GetList(i, &item_list_value);
    if (!valid_value) {
      LOG(ERROR) << "Invalid context menu request: menu item info " << i <<
          " is not a list.";
      return;
    }
    MenuItemInfo item_info;
    // TODO(jcivelli): figure-out why we get a double instead of an int.
    double id;
    valid_value = item_list_value->GetDouble(0, &id);
    if (!valid_value) {
      Value* value = NULL;
      item_list_value->Get(0, &value);
      LOG(ERROR) << "Invalid context menu request:  menu item " << i <<
          " expected int value for first parameter (got " <<
          value->GetType() << ").";
      return;
    }
    item_info.id = static_cast<int>(id);
    valid_value = item_list_value->GetString(1, &(item_info.label));
    if (!valid_value) {
      Value* value = NULL;
      item_list_value->Get(0, &value);
      LOG(ERROR) << "Invalid context menu request:  menu item " << i <<
          " expected string value for second parameter (got " <<
          value->GetType() << ").";
      return;
    }
    menu_items.push_back(item_info);
  }

  on_item_selected_callback_ = new OnContextMenuItemSelectedCallBack(this);

  ChromeView* view = web_ui()->GetWebContents()->GetNativeView();
  view->ShowCustomContextMenu(menu_items, on_item_selected_callback_.get());
}

void ContextMenuHandler::HandleHideContextMenu(const ListValue* args) {
  NOTIMPLEMENTED();
}

void ContextMenuHandler::HandleOpenInNewTab(const ListValue* args) {
  std::string url = GetURLFromParams(args);
  if (!url.empty()) {
    web_ui()->GetWebContents()->OpenURL(content::OpenURLParams(
        GURL(url), content::Referrer(), NEW_FOREGROUND_TAB,
        content::PAGE_TRANSITION_AUTO_BOOKMARK, false));
  }
}

void ContextMenuHandler::HandleOpenInIncognitoTab(const ListValue* args) {
  std::string url = GetURLFromParams(args);
  if (!url.empty()) {
    web_ui()->GetWebContents()->OpenURL(content::OpenURLParams(
        GURL(url), content::Referrer(), OFF_THE_RECORD,
        content::PAGE_TRANSITION_AUTO_BOOKMARK, false));
  }
}

void ContextMenuHandler::HandleOpenAsNonAutoBookmark(const ListValue* args) {
  std::string url = GetURLFromParams(args);
  if (!url.empty()) {
    web_ui()->GetWebContents()->OpenURL(content::OpenURLParams(
        GURL(url), content::Referrer(), CURRENT_TAB,
        content::PAGE_TRANSITION_LINK, false));
  }
}

void ContextMenuHandler::SetOnItemSelectedCallback(
    OnContextMenuItemSelectedCallBack* callback) {
  DCHECK(!on_item_selected_callback_.get());
  on_item_selected_callback_ = callback;
}

void ContextMenuHandler::ReleaseCallback() {
  if (on_item_selected_callback_.get()) {
    on_item_selected_callback_->ClearContextMenuHandler();
    on_item_selected_callback_ = NULL;
  }
}

std::string ContextMenuHandler::GetURLFromParams(const ListValue* args) {
  DCHECK(args->GetSize() == 1U);
  std::string url;
  if (!args->GetString(0, &url))
    NOTREACHED() << "Failed to retrieve parameter.";
  return url;
}
