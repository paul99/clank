// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/media_player_listener.h"

#include "base/android/jni_android.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop_proxy.h"
#include "content/browser/android/media_player_bridge.h"

#include "jni/media_player_listener_jni.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ScopedJavaLocalRef;

MediaPlayerListener::MediaPlayerListener(
    const scoped_refptr<base::MessageLoopProxy>& message_loop,
    base::WeakPtr<MediaPlayerBridge> mediaplayer)
    : message_loop_(message_loop),
      mediaplayer_(mediaplayer) {
  DCHECK(message_loop_);
  DCHECK(mediaplayer_);
}

MediaPlayerListener::~MediaPlayerListener() {
}

ScopedJavaLocalRef<jobject> MediaPlayerListener::CreateMediaPlayerListener() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID constructor = base::android::GetMethodID(
      env, g_MediaPlayerListener_clazz, "<init>", "(I)V");
  ScopedJavaLocalRef<jobject> listener(env,
      env->NewObject(g_MediaPlayerListener_clazz, constructor,
                     reinterpret_cast<jint>(this)));
  CheckException(env);
  DCHECK(!listener.is_null());
  return listener;
}

void MediaPlayerListener::OnMediaError(
    JNIEnv* /* env */, jobject /* obj */, jint error_type) {
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &MediaPlayerBridge::OnMediaError, mediaplayer_, error_type));
}

void MediaPlayerListener::OnVideoSizeChanged(
    JNIEnv* /* env */, jobject /* obj */, jint width, jint height) {
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &MediaPlayerBridge::OnVideoSizeChanged, mediaplayer_,
      width, height));
}

void MediaPlayerListener::OnBufferingUpdate(
    JNIEnv* /* env */, jobject /* obj */, jint percent) {
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &MediaPlayerBridge::OnBufferingUpdate, mediaplayer_, percent));
}

void MediaPlayerListener::OnPlaybackComplete(
    JNIEnv* /* env */, jobject /* obj */) {
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &MediaPlayerBridge::OnPlaybackComplete, mediaplayer_));
}

void MediaPlayerListener::OnSeekComplete(
    JNIEnv* /* env */, jobject /* obj */) {
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &MediaPlayerBridge::OnSeekComplete, mediaplayer_));
}

void MediaPlayerListener::OnMediaPrepared(
    JNIEnv* /* env */, jobject /* obj */) {
  message_loop_->PostTask(FROM_HERE, base::Bind(
      &MediaPlayerBridge::OnMediaPrepared, mediaplayer_));
}

bool RegisterMediaPlayerListener(JNIEnv* env) {
  bool ret = RegisterNativesImpl(env);
  DCHECK(g_MediaPlayerListener_clazz);
  return ret;
}
