// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_STUB_CHROME_H_
#define CHROME_TEST_CHROMEDRIVER_STUB_CHROME_H_

#include <list>

#include "base/compiler_specific.h"
#include "chrome/test/chromedriver/chrome.h"

class Status;
class WebView;

class StubChrome : public Chrome {
 public:
  StubChrome();
  virtual ~StubChrome();

  // Overridden from Chrome:
  virtual Status GetWebViews(std::list<WebView*>* web_views) OVERRIDE;
  virtual Status Quit() OVERRIDE;
};

#endif  // CHROME_TEST_CHROMEDRIVER_STUB_CHROME_H_
