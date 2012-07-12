// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "chrome/browser/browser_process.h"
#include "clank/native/framework/chrome/network_connectivity_receiver.h"
#include "content/browser/android/jni_helper.h"
#include "net/base/network_change_notifier.h"
#include "net/android/network_change_notifier.h"

#include "jni/network_connectivity_receiver_jni.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;

static void UpdateConnectivity(JNIEnv* env,
                               jclass clazz,
                               jboolean is_connected) {
  DCHECK(g_browser_process);
  static_cast<net::android::NetworkChangeNotifierAndroid*>(
      g_browser_process->network_change_notifier())->SetConnectivityOnline(
          is_connected);
}

NetworkChangeNotifierObserver::NetworkChangeNotifierObserver(JNIEnv* env,
                                                             jobject obj)
    : weak_java_tab_(env, obj) {}

NetworkChangeNotifierObserver::~NetworkChangeNotifierObserver() {}

void NetworkChangeNotifierObserver::OnOnlineStateChanged(bool connected) {
  JNIEnv* env = AttachCurrentThread();
  Java_NetworkChangeNotifierObserver_setReceivedNotification(
      env, weak_java_tab_.get(env).obj());
}

static jint Init(JNIEnv* env, jobject obj) {
  NetworkChangeNotifierObserver* observer =
      new NetworkChangeNotifierObserver(env, obj);
  net::NetworkChangeNotifier::AddOnlineStateObserver(observer);
  return reinterpret_cast<jint>(observer);
}

void NetworkChangeNotifierObserver::Destroy(JNIEnv* env, jobject obj) {
  net::NetworkChangeNotifier::RemoveOnlineStateObserver(this);
  delete this;
}

// Register native methods

bool RegisterNetworkConnectivityReceiver(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
