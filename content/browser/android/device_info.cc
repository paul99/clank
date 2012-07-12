// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/device_info.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/logging.h"
#include "content/browser/android/jni_helper.h"
#include "jni/device_info_jni.h"

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;
using base::android::GetMethodID;
using base::android::CheckException;
using base::android::ScopedJavaLocalRef;

DeviceInfo::DeviceInfo() {
  JNIEnv* env = AttachCurrentThread();
  jmethodID j_device_info_init = GetMethodID(
      env,
      g_DeviceInfo_clazz,
      "<init>",
      "(Landroid/content/Context;)V");
  ScopedJavaLocalRef<jobject> tmp(env, env->NewObject(
      g_DeviceInfo_clazz, j_device_info_init,
      base::android::GetApplicationContext()));
  CheckException(env);
  j_device_info_.Reset(tmp);
}

DeviceInfo::~DeviceInfo() {
}

int DeviceInfo::GetHeight() {
  JNIEnv* env = AttachCurrentThread();
  jint result =
      Java_DeviceInfo_getHeight(env, j_device_info_.obj());
  return static_cast<int>(result);
}

int DeviceInfo::GetWidth() {
  JNIEnv* env = AttachCurrentThread();
  jint result =
      Java_DeviceInfo_getWidth(env, j_device_info_.obj());
  return static_cast<int>(result);
}

int DeviceInfo::GetBitsPerPixel() {
  JNIEnv* env = AttachCurrentThread();
  jint result =
      Java_DeviceInfo_getBitsPerPixel(env, j_device_info_.obj());
  return static_cast<int>(result);
}

int DeviceInfo::GetBitsPerComponent() {
  JNIEnv* env = AttachCurrentThread();
  jint result =
      Java_DeviceInfo_getBitsPerComponent(env, j_device_info_.obj());
  return static_cast<int>(result);
}

double DeviceInfo::GetDPIScale() {
  JNIEnv* env = AttachCurrentThread();
  jdouble result =
      Java_DeviceInfo_getDPIScale(env, j_device_info_.obj());
  return static_cast<double>(result);
}

double DeviceInfo::GetRefreshRate() {
  JNIEnv* env = AttachCurrentThread();
  jdouble result =
      Java_DeviceInfo_getRefreshRate(env, j_device_info_.obj());
  return static_cast<double>(result);
}

std::string DeviceInfo::GetNetworkCountryIso() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> result =
      Java_DeviceInfo_getNetworkCountryIso(env, j_device_info_.obj());
  return ConvertJavaStringToUTF8(result);
}

bool RegisterDeviceInfo(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
