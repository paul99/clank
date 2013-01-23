// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/chrome_video_view.h"

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "content/browser/media/media_player_delegate_android.h"
#include "content/browser/android/jni_helper.h"
#include "jni/chrome_video_view_jni.h"
#include "webkit/media/media_metadata_android.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ScopedJavaGlobalRef;
using base::android::GetClass;
using base::android::GetMethodID;

void ChromeVideoView::OnPlaybackComplete() {
  if (!j_chrome_video_view_.is_null())
    Java_ChromeVideoView_onPlaybackComplete(AttachCurrentThread(),
                                            j_chrome_video_view_.obj());
}

void ChromeVideoView::OnBufferingUpdate(int percent) {
  if (!j_chrome_video_view_.is_null())
    Java_ChromeVideoView_onBufferingUpdate(AttachCurrentThread(),
                                           j_chrome_video_view_.obj(),
                                           percent);
}

void ChromeVideoView::OnVideoSizeChanged(int width, int height) {
  if (!j_chrome_video_view_.is_null())
    Java_ChromeVideoView_onVideoSizeChanged(AttachCurrentThread(),
                                            j_chrome_video_view_.obj(),
                                            width, height);
}

void ChromeVideoView::OnError(int error_type) {
  if (!j_chrome_video_view_.is_null())
    Java_ChromeVideoView_onError(AttachCurrentThread(),
                                 j_chrome_video_view_.obj(),
                                 error_type);
}

void ChromeVideoView::CreateFullscreenView(MediaPlayerDelegateAndroid* delegate) {
  delegate_ = delegate;
  if (j_chrome_video_view_.is_null()) {
    JNIEnv* env = AttachCurrentThread();
    j_chrome_video_view_.Reset(Java_ChromeVideoView_createFullScreenView(env,
        reinterpret_cast<jint>(this)));
  } else {
    // Just ask video view to reopen the video.
    Java_ChromeVideoView_openVideo(AttachCurrentThread(),
                                   j_chrome_video_view_.obj());
  }
}

void ChromeVideoView::DestroyFullscreenView() {
  if (!j_chrome_video_view_.is_null()) {
    Java_ChromeVideoView_destroyFullscreenView(AttachCurrentThread(),
                                               j_chrome_video_view_.obj());
    j_chrome_video_view_.Reset();
  }
}

void ChromeVideoView::UpdateMediaMetadata() {
  if (!j_chrome_video_view_.is_null())
    UpdateMediaMetadata(AttachCurrentThread(), j_chrome_video_view_.obj());
}

ChromeVideoView::ChromeVideoView() : delegate_(NULL) {
}

ChromeVideoView::~ChromeVideoView() {
  delegate_ = NULL;

  // If the browser process crashed, just kill the fullscreen view.
  DestroyFullscreenView();
}

int ChromeVideoView::GetVideoWidth(JNIEnv*, jobject obj) const {
  if (delegate_) {
    MediaPlayerBridge* player = delegate_->GetFullscreenPlayer();
    if (player)
      return player->GetVideoWidth();
  }

  return 0;
}

int ChromeVideoView::GetVideoHeight(JNIEnv*, jobject obj) const {
  if (delegate_) {
    MediaPlayerBridge* player = delegate_->GetFullscreenPlayer();
    if (player)
      return player->GetVideoHeight();
  }

  return 0;
}

int ChromeVideoView::GetDuration(JNIEnv*, jobject obj) const {
  if (delegate_) {
    MediaPlayerBridge* player = delegate_->GetFullscreenPlayer();
    if (player)
      return player->GetDuration();
  }

  return -1;
}

int ChromeVideoView::GetCurrentPosition(JNIEnv*, jobject obj) const {
  if (delegate_) {
    MediaPlayerBridge* player = delegate_->GetFullscreenPlayer();
    if (player)
      return player->GetCurrentPosition();
  }

  return 0;
}

bool ChromeVideoView::IsPlaying(JNIEnv*, jobject obj) {
  if (delegate_) {
    MediaPlayerBridge* player = delegate_->GetFullscreenPlayer();
    if (player)
      return player->IsPlaying();
  }

  return false;
}

void ChromeVideoView::SeekTo(JNIEnv*, jobject obj, jint msec) {
  if (delegate_)
    delegate_->FullscreenVideoSeek(msec);
}

void ChromeVideoView::Play(JNIEnv*, jobject obj) {
  if (delegate_)
    delegate_->FullscreenVideoPlay();
}

void ChromeVideoView::Pause(JNIEnv*, jobject obj) {
  if (delegate_)
    delegate_->FullscreenVideoPause();
}

void ChromeVideoView::ExitFullscreen(JNIEnv*, jobject,
                                     jboolean release_media_player) {
  if (delegate_)
    delegate_->ExitFullscreen(release_media_player);

  if (!j_chrome_video_view_.is_null())
    j_chrome_video_view_.Reset();
}

void ChromeVideoView::SetSurface(JNIEnv* env, jobject obj,
                                 jobject surface) {
  if (delegate_) {
    MediaPlayerBridge* player = delegate_->GetFullscreenPlayer();
    if (player) {
      player->SetVideoSurface(surface, true);
      return;
    }
  }
}

void ChromeVideoView::UpdateMediaMetadata(JNIEnv* env, jobject obj) {
  if (delegate_) {
    MediaPlayerBridge* player = delegate_->GetFullscreenPlayer();
    if (player)
      Java_ChromeVideoView_updateMediaMetadata(env,
                                               obj,
                                               player->GetVideoWidth(),
                                               player->GetVideoHeight(),
                                               player->GetDuration(),
                                               player->can_pause(),
                                               player->can_seek_forward(),
                                               player->can_seek_backward());
  }
}

bool RegisterChromeVideoView(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
