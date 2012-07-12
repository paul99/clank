// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/google_auto_login.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "jni/google_auto_login_jni.h"

GoogleAutoLogin::GoogleAutoLogin(JNIEnv* env, jobject obj)
    : weak_google_auto_login_(env, obj) {
  profile_ = NULL;
  if (g_browser_process == NULL ||
      g_browser_process->profile_manager() == NULL) {
    LOG(ERROR) << "Browser process or profile manager not initialized";
    return;
  }

  profile_ = g_browser_process->profile_manager()->GetDefaultProfile();
  if (profile_ == NULL) {
    LOG(ERROR) << "Sync Init: Profile not found.";
    return;
  }
}

void GoogleAutoLogin::LogIn(JNIEnv* env, jobject obj, jstring j_url) {
  GURL url(base::android::ConvertJavaStringToUTF8(env, j_url));
  content::URLFetcher* fetcher = content::URLFetcher::Create(
      0, url, content::URLFetcher::GET, this);
  fetcher->SetRequestContext(profile_->GetRequestContext());
  fetcher_.reset(fetcher);
  fetcher->Start();
}

void GoogleAutoLogin::OnURLFetchComplete(const content::URLFetcher* source) {
  delete this;
}

static int Init(JNIEnv* env, jobject obj) {
  return reinterpret_cast<jint>(new GoogleAutoLogin(env, obj));
}

bool RegisterGoogleAutoLogin(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
