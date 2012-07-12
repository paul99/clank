// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_MEDIA_PLAYER_LISTENER_H_
#define CONTENT_BROWSER_ANDROID_MEDIA_PLAYER_LISTENER_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"

class MediaPlayerBridge;

// Static factory method for the creation of Java MediaPlayerListener object.
// This can be called from any thread.
base::android::ScopedJavaLocalRef<jobject> CreateMediaPlayerListener(
    JNIEnv* env,
    MediaPlayerBridge* native_ptr);

bool RegisterMediaPlayerListener(JNIEnv* env);

#endif  // CONTENT_BROWSER_ANDROID_MEDIA_PLAYER_LISTENER_H_
