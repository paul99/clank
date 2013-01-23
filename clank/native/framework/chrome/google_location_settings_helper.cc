// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/google_location_settings_helper.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "chrome/browser/geolocation/chrome_geolocation_permission_context.h"
#include "jni/google_location_settings_helper_jni.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;

namespace geolocation {

const std::string GetButtonOKTextFromGoogleAppsLocationSetting() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> inforbar_allow_text =
      Java_GoogleLocationSettingsHelper_getInfoBarAllowTextFromLocationSetting(env);
  return ConvertJavaStringToUTF8(inforbar_allow_text);
}

const void ShowGoogleLocationSettings() {
  JNIEnv* env = AttachCurrentThread();
  Java_GoogleLocationSettingsHelper_startGoogleLocationSettingsActivity(env);
}

const bool isGoogleAppsLocationSettingEnabled() {
  JNIEnv* env = AttachCurrentThread();
  return Java_GoogleLocationSettingsHelper_isGoogleAppsLocationSettingEnabled(env);
}

}  // namespace geolocation

// Register native methods

bool RegisterGoogleLocationSettingsHelper(JNIEnv* env) {
  bool register_natives_impl_result = RegisterNativesImpl(env);
  return register_natives_impl_result;
}
