// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_NETWORK_INFO_H_
#define CONTENT_BROWSER_ANDROID_NETWORK_INFO_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/logging.h"

class NetworkInfo {
 public:
  NetworkInfo();
  ~NetworkInfo();

  bool IsWifiNetwork();

 private:
  // Java instance and methods for the Android ConnectivityManager
  base::android::ScopedJavaGlobalRef<jobject> connectivity_manager_;
  jmethodID get_active_network_info_;

  DISALLOW_COPY_AND_ASSIGN(NetworkInfo);
};

#endif  // CONTENT_BROWSER_ANDROID_NETWORK_INFO_H_
