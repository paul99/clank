// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/surface_texture_listener.h"

#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop_proxy.h"
#include "content/browser/android/surface_texture_bridge.h"
#include "jni/surface_texture_listener_jni.h"

// static
jobject SurfaceTextureListener::CreateSurfaceTextureListener(
    JNIEnv* env,
    const base::Closure& callback) {
  jmethodID constructor =
      base::android::GetMethodID(env, g_SurfaceTextureListener_clazz,
                                 "<init>", "(I)V");

  // The java listener object owns and releases the native instance.
  // This is necessary to avoid races with incoming notifications.
  jobject listener = env->NewObject(g_SurfaceTextureListener_clazz,
                                    constructor,
                                    reinterpret_cast<int>(
                                        new SurfaceTextureListener(callback)));

  DCHECK(listener);
  return listener;
}

SurfaceTextureListener::SurfaceTextureListener(const base::Closure& callback)
    : callback_(callback),
      loop_(base::MessageLoopProxy::current()) {
}

SurfaceTextureListener::~SurfaceTextureListener() {
}

void SurfaceTextureListener::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

void SurfaceTextureListener::FrameAvailable(JNIEnv* env, jobject obj) {
  // These notifications should be coming in on a thread private to Java.
  // Should this ever change, we can try to avoid reposting to the same thread.
  DCHECK(!loop_->BelongsToCurrentThread());
  loop_->PostTask(FROM_HERE, callback_);
}

bool RegisterSurfaceTextureListener(JNIEnv* env) {
  bool ret = RegisterNativesImpl(env);
  DCHECK(g_SurfaceTextureListener_clazz);
  return ret;
}
