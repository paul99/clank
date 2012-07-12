// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/network_info.h"

#include "base/android/jni_android.h"
#include "base/basictypes.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ClearException;
using base::android::GetApplicationContext;
using base::android::GetClass;
using base::android::GetMethodID;
using base::android::HasClass;
using base::android::ScopedJavaLocalRef;

const char* const NETWORK_INFO_CLASS_NAME = "android/net/NetworkInfo";

NetworkInfo::NetworkInfo() : get_active_network_info_(NULL) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  // Get the context class.
  ScopedJavaLocalRef<jclass> context_class(
      GetClass(env, "android/content/Context"));

  jobject j_context = GetApplicationContext();
  DCHECK(j_context);

  // Get the system service method.
  jmethodID get_system_service = GetMethodID(env, context_class,
      "getSystemService",
      "(Ljava/lang/String;)Ljava/lang/Object;");

  // Retrieve the system service.
  ScopedJavaLocalRef<jstring> service_name(env,
                                           env->NewStringUTF("connectivity"));
  ScopedJavaLocalRef<jobject> cm(env, env->CallObjectMethod(j_context,
      get_system_service, service_name.obj()));
  ClearException(env);
  DCHECK(!cm.is_null());

  // Make a global reference for our connectivity manager.
  connectivity_manager_.Reset(cm);
  DCHECK(!connectivity_manager_.is_null());

  // Retain the method we'll keep using.
  ScopedJavaLocalRef<jclass> connectivity_class(
      GetClass(env, "android/net/ConnectivityManager"));
  get_active_network_info_ = GetMethodID(env,
                                         connectivity_class,
                                         "getActiveNetworkInfo",
                                         "()Landroid/net/NetworkInfo;");
  DCHECK(get_active_network_info_);
}

NetworkInfo::~NetworkInfo() {
}

bool NetworkInfo::IsWifiNetwork() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  ScopedJavaLocalRef<jobject> network_info(env,
      env->CallObjectMethod(connectivity_manager_.obj(),
                            get_active_network_info_));
  CheckException(env);
  if (!network_info.obj())
    return false;

  if (!HasClass(env, NETWORK_INFO_CLASS_NAME))
    return false;

  ScopedJavaLocalRef<jclass> cls(GetClass(env, NETWORK_INFO_CLASS_NAME));

  jmethodID get_type = GetMethodID(env, cls, "getType", "()I");
  jint j_result = env->CallIntMethod(network_info.obj(), get_type);
  CheckException(env);

  return (1 == static_cast<int>(j_result));
}
