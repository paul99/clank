// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/device_orientation/device_orientation_android.h"

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "content/browser/device_orientation/orientation.h"
#include "jni/device_orientation_jni.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::GetClass;
using base::android::GetMethodID;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;

namespace device_orientation {

namespace {

// This should match ProviderImpl::kDesiredSamplingIntervalMs.
// TODO(husky): Make that constant public so we can use it directly.
const int kPeriodInMilliseconds = 100;

base::LazyInstance<ScopedJavaGlobalRef<jobject> >
     g_jni_obj = LAZY_INSTANCE_INITIALIZER;

}  // namespace

DeviceOrientationAndroid::DeviceOrientationAndroid() {
}

void DeviceOrientationAndroid::Init(JNIEnv* env) {
  bool result = RegisterNativesImpl(env);
  DCHECK(result);

  ScopedJavaLocalRef<jclass> clazz = GetClass(env, kDeviceOrientationClassPath);
  jmethodID constructor = GetMethodID(env, clazz, "<init>", "()V");

  ScopedJavaLocalRef<jobject> obj(env,
      env->NewObject(clazz.obj(), constructor));
  DCHECK(!obj.is_null());
  CheckException(env);

  g_jni_obj.Get().Reset(obj);
}

DataFetcher* DeviceOrientationAndroid::Create() {
  scoped_ptr<DeviceOrientationAndroid> fetcher(new DeviceOrientationAndroid);
  if (fetcher->Start(kPeriodInMilliseconds))
    return fetcher.release();

  LOG(ERROR) << "DeviceOrientationAndroid::Start failed!";
  return NULL;
}

DeviceOrientationAndroid::~DeviceOrientationAndroid() {
  Stop();
}

bool DeviceOrientationAndroid::GetOrientation(Orientation* orientation) {
  // Do we have a new orientation value? (It's safe to do this outside the lock
  // because we only skip the lock if the value is null. We always enter the
  // lock if we're going to make use of the new value.)
  if (next_orientation_.get()) {
    base::AutoLock autolock(next_orientation_lock_);
    next_orientation_.swap(current_orientation_);
  }
  if (current_orientation_.get()) {
    *orientation = *current_orientation_;
  }
  return true;
}

void DeviceOrientationAndroid::GotOrientation(
    JNIEnv*, jobject, double alpha, double beta, double gamma) {
  base::AutoLock autolock(next_orientation_lock_);
  next_orientation_.reset(
      new Orientation(true, alpha, true, beta, true, gamma));
}

bool DeviceOrientationAndroid::Start(int rateInMilliseconds) {
  DCHECK(!g_jni_obj.Get().is_null());
  return Java_DeviceOrientation_start(AttachCurrentThread(),
                                      g_jni_obj.Get().obj(),
                                      reinterpret_cast<jint>(this),
                                      rateInMilliseconds);
}

void DeviceOrientationAndroid::Stop() {
  DCHECK(!g_jni_obj.Get().is_null());
  Java_DeviceOrientation_stop(AttachCurrentThread(), g_jni_obj.Get().obj());
}

}  // namespace device_orientation
