// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_H_

#include <list>

class Status;
class WebView;

class Chrome {
 public:
  virtual ~Chrome() {}

  // Return a list of opened WebViews.
  virtual Status GetWebViews(std::list<WebView*>* web_views) = 0;

  // Quits Chrome.
  virtual Status Quit() = 0;
};

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_H_
