// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CONSTRAINED_WINDOW_ANDROID_H_
#define CHROME_BROWSER_ANDROID_CONSTRAINED_WINDOW_ANDROID_H_

#include "chrome/browser/ui/constrained_window.h"

#include "base/compiler_specific.h"

class ConstrainedWindowAndroidDelegate {
 public:
  virtual ~ConstrainedWindowAndroidDelegate() {}

  virtual int GetWidgetRoot() = 0;

  virtual void DeleteDelegate() = 0;
};

class ConstrainedWindowAndroid : public ConstrainedWindow {
 public:
  virtual ~ConstrainedWindowAndroid();

  virtual void ShowConstrainedWindow() OVERRIDE;

  virtual void CloseConstrainedWindow() OVERRIDE;
};

#endif  // CHROME_BROWSER_ANDROID_CONSTRAINED_WINDOW_ANDROID_H_
