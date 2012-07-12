// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "googleurl/src/gurl.h"
#include "jni/url_utilities_jni.h"
#include "net/base/registry_controlled_domain.h"

static jstring GetDomainAndRegistry(JNIEnv* env, jclass, jstring url) {
  GURL gurl = GURL(base::android::ConvertJavaStringToUTF8(env, url));
  if (gurl.is_empty())
    return NULL;

  // OK to release, JNI binding.
  return base::android::ConvertUTF8ToJavaString(
      env, net::RegistryControlledDomainService::GetDomainAndRegistry(gurl)).
          Release();
}

// Register native methods
bool RegisterURLUtilities(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
