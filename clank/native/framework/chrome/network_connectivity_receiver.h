// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_NETWORK_CONNECTIVITY_RECEIVER_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_NETWORK_CONNECTIVITY_RECEIVER_H_

#include <jni.h>

#include "base/compiler_specific.h"
#include "content/browser/android/jni_helper.h"
#include "net/base/network_change_notifier.h"

bool RegisterNetworkConnectivityReceiver(JNIEnv* env);

class NetworkChangeNotifierObserver
    : public net::NetworkChangeNotifier::OnlineStateObserver {
 public:
  NetworkChangeNotifierObserver(JNIEnv* env, jobject obj);
  void Destroy(JNIEnv* env, jobject obj);

  // net::NetworkChangeNotifier::OnlineStateObserver:
  virtual void OnOnlineStateChanged(bool) OVERRIDE;

 private:
  virtual ~NetworkChangeNotifierObserver();

  JavaObjectWeakGlobalRef weak_java_tab_;
};

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_NETWORK_CONNECTIVITY_RECEIVER_H_
