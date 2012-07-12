// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MOBILE_NEW_TAB_UI_H_
#define CHROME_BROWSER_UI_WEBUI_MOBILE_NEW_TAB_UI_H_
#pragma once

#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"

// The TabContents used for the Mobile New Tab page. It supports the bookmarks,
// recently closed pages, and most visited.
class MobileNewTabUI : public NewTabUI {
 public:
  explicit MobileNewTabUI(content::WebUI* web_ui);
  virtual ~MobileNewTabUI();

 private:
  DISALLOW_COPY_AND_ASSIGN(MobileNewTabUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_MOBILE_NEW_TAB_UI_H_

