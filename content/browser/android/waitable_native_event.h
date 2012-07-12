// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_WAITABLE_NATIVE_EVENT_H_
#define CONTENT_BROWSER_ANDROID_WAITABLE_NATIVE_EVENT_H_

#include "base/synchronization/waitable_event.h"

#include <jni.h>

class WaitableNativeEvent : public base::WaitableEvent {
 public:
  WaitableNativeEvent();
  void Destroy(JNIEnv* env, jobject obj);

  jboolean Wait(JNIEnv* env, jobject obj, jint timeout_in_seconds);

 private:
  DISALLOW_COPY_AND_ASSIGN(WaitableNativeEvent);
  ~WaitableNativeEvent();
};

bool RegisterWaitableNativeEvent(JNIEnv* env);

#endif  // CONTENT_BROWSER_ANDROID_WAITABLE_NATIVE_EVENT_H_
