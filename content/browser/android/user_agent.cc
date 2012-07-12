// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "content/browser/android/user_agent.h"
#include "jni/user_agent_jni.h"
#include "chrome/common/chrome_version_info.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ScopedJavaLocalRef;
using base::android::ConvertJavaStringToUTF8;
using base::android::GetApplicationContext;

std::string GetUserAgentOSInfo() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> os_info =
      Java_UserAgent_getUserAgentOSInfo(env);
  CheckException(env);
  return ConvertJavaStringToUTF8(os_info);
}

std::string GetUserAgentMobile() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> mobile =
      Java_UserAgent_getUserAgentMobile(env, GetApplicationContext());
  CheckException(env);
  return ConvertJavaStringToUTF8(mobile);
}

std::string GetUserAgentChromeVersionInfo() {
  chrome::VersionInfo version_info;
  return version_info.Version();
}

bool RegisterUserAgent(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
