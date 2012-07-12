// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_WEB_SETTINGS_H_
#define CONTENT_BROWSER_ANDROID_WEB_SETTINGS_H_
#pragma once

#include "base/memory/scoped_ptr.h"
#include "content/browser/android/jni_helper.h"

class WebSettings {
 public:
  WebSettings(JNIEnv* env, jobject obj);
  ~WebSettings();
  void Destroy(JNIEnv* env, jobject obj);

  void Sync(JNIEnv* env, jobject obj, jint nativeChromeView);

 private:
  struct FieldIds;

  void InitializeFieldIds(JNIEnv* env);

  scoped_ptr<FieldIds> field_ids_;
};

bool RegisterWebSettings(JNIEnv* env);

#endif  // CONTENT_BROWSER_ANDROID_WEB_SETTINGS_H_
