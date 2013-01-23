// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_MEDIA_PLAYER_LISTENER_H_
#define CONTENT_BROWSER_ANDROID_MEDIA_PLAYER_LISTENER_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"

namespace base {
class MessageLoopProxy;
}

class MediaPlayerBridge;

// Acts as a thread proxy between java MediaPlayerListener object and
// MediaPlayerBridge so that callbacks are posted onto the right thread.
class MediaPlayerListener
    : public base::RefCountedThreadSafe<MediaPlayerListener> {
 public:
  MediaPlayerListener(
      const scoped_refptr<base::MessageLoopProxy>& message_loop,
      base::WeakPtr<MediaPlayerBridge> mediaplayer);

  // Called by the Java MediaPlayerListener and mirrored to corresponding
  // callbacks.
  void OnMediaError(JNIEnv* /* env */, jobject /* obj */, jint error_type);
  void OnVideoSizeChanged(JNIEnv* /* env */, jobject /* obj */,
                          jint width, jint height);
  void OnBufferingUpdate(JNIEnv* /* env */, jobject /* obj */, jint percent);
  void OnPlaybackComplete(JNIEnv* /* env */, jobject /* obj */);
  void OnSeekComplete(JNIEnv* /* env */, jobject /* obj */);
  void OnMediaPrepared(JNIEnv* /* env */, jobject /* obj */);

  // Create a Java MediaPlayerListener object.
  base::android::ScopedJavaLocalRef<jobject> CreateMediaPlayerListener();

 private:
  friend class base::RefCountedThreadSafe<MediaPlayerListener>;
  virtual ~MediaPlayerListener();

  // The message loop where |mediaplayer_| lives.
  scoped_refptr<base::MessageLoopProxy> message_loop_;

  // The MediaPlayerBridge object all the callbacks should be send to.
  base::WeakPtr<MediaPlayerBridge> mediaplayer_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerListener);
};

bool RegisterMediaPlayerListener(JNIEnv* env);

#endif  // CONTENT_BROWSER_ANDROID_MEDIA_PLAYER_LISTENER_H_
