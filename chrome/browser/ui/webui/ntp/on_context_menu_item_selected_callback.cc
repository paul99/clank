// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/ntp/on_context_menu_item_selected_callback.h"

#include "chrome/browser/ui/webui/ntp/context_menu_handler.h"

OnContextMenuItemSelectedCallBack::OnContextMenuItemSelectedCallBack(
    ContextMenuHandler* context_menu_handler)
    : context_menu_handler_(context_menu_handler) {
  context_menu_handler_->SetOnItemSelectedCallback(this);
}

OnContextMenuItemSelectedCallBack::~OnContextMenuItemSelectedCallBack() {
}

void OnContextMenuItemSelectedCallBack::ItemSelected(int item_id) {
  if (context_menu_handler_)
    context_menu_handler_->OnItemSelected(item_id);
}

void OnContextMenuItemSelectedCallBack::ClearContextMenuHandler() {
  context_menu_handler_ = NULL;
}
