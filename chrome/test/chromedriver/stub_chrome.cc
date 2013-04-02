// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/stub_chrome.h"
#include "chrome/test/chromedriver/status.h"
#include "chrome/test/chromedriver/web_view.h"

StubChrome::StubChrome() {}

StubChrome::~StubChrome() {}

Status StubChrome::GetWebViews(std::list<WebView*>* web_views) {
  return Status(kOk);
}

Status StubChrome::Quit() {
  return Status(kOk);
}
