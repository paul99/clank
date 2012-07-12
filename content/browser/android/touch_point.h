// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_TOUCH_POINT_H_
#define CONTENT_BROWSER_ANDROID_TOUCH_POINT_H_
#pragma once

#include "content/browser/android/jni_helper.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"

class TouchPoint {
 public:
  static void BuildWebTouchEvent(JNIEnv* env, jint type, jlong time,
      jobjectArray pts, WebKit::WebTouchEvent& event);
};

bool RegisterTouchPoint(JNIEnv* env);

#endif  // CONTENT_BROWSER_ANDROID_CHROME_VIEW_H_
