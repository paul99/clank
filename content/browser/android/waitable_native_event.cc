// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/waitable_native_event.h"

#include "base/time.h"

#include "jni/waitable_native_event_jni.h"

static jint Init(JNIEnv* env, jobject obj) {
  WaitableNativeEvent* waitable = new WaitableNativeEvent();
  return reinterpret_cast<jint>(waitable);
}

WaitableNativeEvent::WaitableNativeEvent()
    : base::WaitableEvent(true, false) {
}

WaitableNativeEvent::~WaitableNativeEvent() {
}

void WaitableNativeEvent::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

jboolean WaitableNativeEvent::Wait(
    JNIEnv* env, jobject obj, jint timeout_in_seconds) {
  return base::WaitableEvent::TimedWait(
      base::TimeDelta::FromSeconds(timeout_in_seconds));
}

bool RegisterWaitableNativeEvent(JNIEnv* env) {
  bool ret = RegisterNativesImpl(env);
  DCHECK(g_WaitableNativeEvent_clazz);
  return ret;
}
