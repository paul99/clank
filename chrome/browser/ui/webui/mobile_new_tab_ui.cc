// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/mobile_new_tab_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/bookmarks_handler.h"
#include "chrome/browser/ui/webui/favicon_bg_color_handler.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/ntp/context_menu_handler.h"
#include "chrome/browser/ui/webui/ntp/foreign_session_handler.h"
#include "content/browser/tab_contents/tab_contents.h"

using content::WebContents;

MobileNewTabUI::MobileNewTabUI(content::WebUI* web_ui) : NewTabUI(web_ui) {
  Profile* profile =
    Profile::FromBrowserContext(web_ui->GetWebContents()->GetBrowserContext());

  web_ui->AddMessageHandler((new BookmarksHandler()));
  web_ui->AddMessageHandler((new browser_sync::ForeignSessionHandler()));
  web_ui->AddMessageHandler((new ContextMenuHandler()));
  web_ui->AddMessageHandler((new FaviconBgColorHandler(profile)));

  if (profile->IsOffTheRecord()) {
    profile->GetChromeURLDataManager()->AddDataSource(
        new FaviconSource(profile, FaviconSource::ANY));
  }
}

MobileNewTabUI::~MobileNewTabUI() {
}
