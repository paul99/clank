// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/media_player_listener.h"

#include "base/android/jni_android.h"
#include "base/logging.h"
#include "content/browser/android/media_player_bridge.h"
#include "jni/media_player_listener_jni.h"

using base::android::ScopedJavaLocalRef;

void Notify(JNIEnv* env, jobject obj,
            jint native_ptr, jint msg, jint ext1, jint ext2) {
  MediaPlayerBridge* bridge =
      reinterpret_cast<MediaPlayerBridge*>(native_ptr);
  CHECK(bridge);
  bridge->Notify(static_cast<int>(msg), static_cast<int>(ext1),
                 static_cast<int>(ext2));
}

ScopedJavaLocalRef<jobject> CreateMediaPlayerListener(
    JNIEnv* env, MediaPlayerBridge* native_ptr) {
  jmethodID constructor = base::android::GetMethodID(
      env, g_MediaPlayerListener_clazz, "<init>", "(I)V");
  ScopedJavaLocalRef<jobject> listener(env,
      env->NewObject(g_MediaPlayerListener_clazz, constructor,
                     reinterpret_cast<jint>(native_ptr)));
  DCHECK(!listener.is_null());
  return listener;
}

bool RegisterMediaPlayerListener(JNIEnv* env) {
  bool ret = RegisterNativesImpl(env);
  DCHECK(g_MediaPlayerListener_clazz);
  return ret;
}
