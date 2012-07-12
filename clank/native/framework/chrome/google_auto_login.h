// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_GOOGLE_AUTO_LOGIN_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_GOOGLE_AUTO_LOGIN_H_

#include <jni.h>

#include "chrome/browser/profiles/profile.h"
#include "content/browser/android/jni_helper.h"
#include "content/public/common/url_fetcher.h"
#include "content/public/common/url_fetcher_delegate.h"

class GoogleAutoLogin : content::URLFetcherDelegate {
 public:
  GoogleAutoLogin(JNIEnv* env, jobject obj);

  void LogIn(JNIEnv*, jobject obj, jstring url);

 private:
  Profile* profile_;
  JavaObjectWeakGlobalRef weak_google_auto_login_;
  scoped_ptr<content::URLFetcher> fetcher_;

  // content::URLFetcherDelegate
  virtual void OnURLFetchComplete(const content::URLFetcher* source) OVERRIDE;
};

bool RegisterGoogleAutoLogin(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_GOOGLE_AUTO_LOGIN_H_
