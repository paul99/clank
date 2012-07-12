// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_CHROME_VIDEO_VIEW_H_
#define CONTENT_BROWSER_ANDROID_CHROME_VIDEO_VIEW_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/timer.h"
#include "content/browser/media/media_player_listener_android.h"

class MediaPlayerDelegateAndroid;

namespace webkit_glue {
  class MediaMetadataAndroid;
}

class ChromeVideoView {
 public:
  explicit ChromeVideoView();
  ~ChromeVideoView();

  void Init(JNIEnv*, jobject obj, jobject weak_this);
  int GetVideoWidth(JNIEnv*, jobject obj) const;
  int GetVideoHeight(JNIEnv*, jobject obj) const;
  int GetDuration(JNIEnv*, jobject obj) const;
  int GetPlayerId(JNIEnv*, jobject obj) const;
  int GetRouteId(JNIEnv*, jobject obj) const;
  void UpdateMediaMetadata(JNIEnv*, jobject obj);
  int GetCurrentPosition(JNIEnv*, jobject obj) const;
  int GetRenderHandle(JNIEnv*, jobject obj) const;
  void Play(JNIEnv*, jobject obj);
  void PrepareAsync();
  void Pause(JNIEnv*, jobject obj);
  void SeekTo(JNIEnv*, jobject obj, jint msec);
  bool IsPlaying(JNIEnv*, jobject obj);
  void DestroyFullscreenView(JNIEnv*, jobject);
  void ExitFullscreen(JNIEnv*, jobject, jboolean release_media_player);
  void CreateFullscreenView(MediaPlayerDelegateAndroid* player);
  void DestroyFullscreenView();
  void SetSurface(JNIEnv*, jobject obj, jobject surface,
                  jint route_id, jint player_id);
  void UpdateMediaMetadata();

 private:
  // In some certain cases if the renderer crashes, the ExitFullscreen message
  // will never acknowledged by the renderer. We have to destroy the fullscreen
  // view in this case to prevent the ChromeFlashView from sitting on top of the
  // screen.
  void OnTimeout();

  webkit_glue::MediaMetadataAndroid* GetMediaMetadata();

  MediaPlayerDelegateAndroid* player_;
  scoped_ptr<MediaPlayerListenerAndroid> listener_;

  base::android::ScopedJavaGlobalRef<jobject> j_chrome_video_view_;

  // A timer to keep track of when the acknowledgement of exitfullscreen
  // message times out.
  base::OneShotTimer<ChromeVideoView> timeout_;

  DISALLOW_COPY_AND_ASSIGN(ChromeVideoView);
};

// Registers the ChromeVideoView native methods through JNI.
bool RegisterChromeVideoView(JNIEnv* env);

#endif  // CONTENT_BROWSER_ANDROID_CHROME_VIDEO_VIEW_H_
