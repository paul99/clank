// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/browser_version.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "chrome/browser/browser_about_handler.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"
#include "jni/browser_version_jni.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"


using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetApplicationContext;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

namespace {

class BrowserVersion {
 public:
  // Lazily instantiated on first access by Singleton<>.
  BrowserVersion() {
    JNIEnv* env = AttachCurrentThread();
    jobject context = GetApplicationContext();
    ScopedJavaLocalRef<jstring> str =
        Java_BrowserVersion_getApplicationLabel(env, context);
    app_label_ = ConvertJavaStringToUTF8(str);

    str = Java_BrowserVersion_getAppVersionName(env, context);
    version_name_ = ConvertJavaStringToUTF8(str);

    str = Java_BrowserVersion_getAppVersionCode(env, context);
    version_code_ = ConvertJavaStringToUTF8(str);
  }

  static std::string GetAppLabel() {
    return BrowserVersion::GetInstance()->app_label_;
  }

  static std::string GetAppVersionName() {
    return BrowserVersion::GetInstance()->version_name_;
  }

  static std::string GetAppVersionCode() {
    return BrowserVersion::GetInstance()->version_code_;
  }

  static BrowserVersion* GetInstance() {
    return Singleton<BrowserVersion>::get();
  }

 private:
  std::string app_label_;
  std::string version_name_;
  std::string version_code_;
};

}  // namespace

// This mimics the functionality in ChromeView's browser_process_main.cc.
// TODO(dfalcantara): If ChromeView and Clank will always return the same value
//                    for this, find a way to reuse the original.
static jboolean IsOfficialBuild(JNIEnv* env, jclass /* clazz */) {
#if defined(OFFICIAL_BUILD)
  return true;
#else
  return false;
#endif
}

static jstring GetTermsOfServiceHtml(JNIEnv* env, jclass /* clazz */) {
  base::StringPiece text = ResourceBundle::GetSharedInstance()
      .GetRawDataResource(IDR_TERMS_HTML);
  return ConvertUTF8ToJavaString(env, text).Release();
}

static jstring GetTermsOfServiceString(JNIEnv* env, jclass /* clazz */) {
  return ConvertUTF8ToJavaString(env,
      l10n_util::GetStringUTF8(IDS_FIRSTRUN_TOS_EXPLANATION)).Release();
}

bool RegisterBrowserVersion(JNIEnv* env) {
  AboutAndroidApp::RegisterGetters(&BrowserVersion::GetAppLabel,
                                   &BrowserVersion::GetAppVersionName,
                                   &BrowserVersion::GetAppVersionCode);
  return RegisterNativesImpl(env);
}
