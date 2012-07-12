// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/chrome_video_view.h"

#include "base/android/jni_android.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "content/browser/media/media_player_delegate_android.h"
#include "content/browser/android/jni_helper.h"
#include "content/browser/android/sandboxed_process_service.h"
#include "content/browser/android/surface_texture_peer.h"
#include "content/public/common/content_switches.h"
#include "jni/chrome_video_view_jni.h"
#include "webkit/media/media_metadata_android.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ScopedJavaGlobalRef;

// The timeout value we should wait after user click the back button on the
// fullscreen view.
const int kTimeout = 1000;  // 1 second.

class JNIMediaPlayerListener: public MediaPlayerListenerAndroid {
 public:
  JNIMediaPlayerListener(JNIEnv* env, jobject video_view_weak_ref);
  ~JNIMediaPlayerListener();
  void Notify(int msg, int ext1, int ext2);

 private:
  JNIMediaPlayerListener();

  // Reference to the ChromeVideoView Java object to call on.
  ScopedJavaGlobalRef<jobject> j_video_view_;
};

JNIMediaPlayerListener::JNIMediaPlayerListener(JNIEnv* env,
                                               jobject java_video_view) {
  j_video_view_.Reset(env, java_video_view);
}

JNIMediaPlayerListener::~JNIMediaPlayerListener() {
}

// ----------------------------------------------------------------------------
// Methods that call to Java via JNI
// ----------------------------------------------------------------------------

void JNIMediaPlayerListener::Notify(int msg, int ext1, int ext2) {
  Java_ChromeVideoView_postEventFromNative(AttachCurrentThread(),
                                           j_video_view_.obj(),
                                           msg,
                                           ext1,
                                           ext2);
}

void ChromeVideoView::CreateFullscreenView(MediaPlayerDelegateAndroid* player) {
  player_ = player;
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
    Java_ChromeVideoView_destroyFullscreenView(AttachCurrentThread());
    j_chrome_video_view_.Reset();
  }
}

void ChromeVideoView::UpdateMediaMetadata() {
  if (!j_chrome_video_view_.is_null())
    UpdateMediaMetadata(AttachCurrentThread(), j_chrome_video_view_.obj());
}

// ----------------------------------------------------------------------------

ChromeVideoView::ChromeVideoView() : player_(NULL) {
}

ChromeVideoView::~ChromeVideoView() {
  if (player_) {
    player_->SetJNIListener(NULL);
    listener_.reset();
  }
  player_ = NULL;

  // If the browser process crashed, just kill the fullscreen view.
  DestroyFullscreenView();
}

// ----------------------------------------------------------------------------
// All these functions are called on UI thread

int ChromeVideoView::GetVideoWidth(JNIEnv*, jobject obj) const {
  return player_ ? player_->GetVideoWidth() : 0;
}

int ChromeVideoView::GetVideoHeight(JNIEnv*, jobject obj) const {
  return player_ ? player_->GetVideoHeight() : 0;
}

int ChromeVideoView::GetDuration(JNIEnv*, jobject obj) const {
  return player_ ? static_cast<int>(player_->Duration() * 1000) : -1;
}

int ChromeVideoView::GetCurrentPosition(JNIEnv*, jobject obj) const {
  return player_ ? static_cast<int>(player_->CurrentTime() * 1000) : 0;
}

bool ChromeVideoView::IsPlaying(JNIEnv*, jobject obj) {
  return player_ ? player_->IsPlaying() : false;
}

void ChromeVideoView::SeekTo(JNIEnv*, jobject obj, jint msec) {
  if (player_)
    player_->Seek(static_cast<float>(msec / 1000.0));
}

webkit_glue::MediaMetadataAndroid* ChromeVideoView::GetMediaMetadata() {
  if (!player_)
    return NULL;

  return new webkit_glue::MediaMetadataAndroid(player_->GetVideoWidth(),
                                               player_->GetVideoHeight(),
                                               player_->Duration(),
                                               player_->CurrentTime(),
                                               !player_->IsPlaying(),
                                               player_->CanPause(),
                                               player_->CanSeekForward(),
                                               player_->CanSeekForward());
}

int ChromeVideoView::GetPlayerId(JNIEnv*, jobject obj) const {
  return player_ ? player_->player_id() : -1;
}

int ChromeVideoView::GetRouteId(JNIEnv*, jobject obj) const {
  return player_ ? player_->route_id() : -1;
}

int ChromeVideoView::GetRenderHandle(JNIEnv*, jobject obj) const {
  static bool single_process =
      CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess);
  if (!player_)
    return -1;

  if (single_process)
    return 0;
  return player_->render_handle();
}

void ChromeVideoView::Play(JNIEnv*, jobject obj) {
  if (player_)
    player_->Play();
}

void ChromeVideoView::Pause(JNIEnv*, jobject obj) {
  if (player_)
    player_->Pause();
}

void ChromeVideoView::OnTimeout() {
  timeout_.Stop();
  DestroyFullscreenView();
}

// ----------------------------------------------------------------------------
// Methods called from Java via JNI
// ----------------------------------------------------------------------------
void ChromeVideoView::Init(JNIEnv* env, jobject obj, jobject weak_this) {
  listener_.reset(new JNIMediaPlayerListener(env, weak_this));
  if (player_)
    player_->SetJNIListener(listener_.get());
}

void ChromeVideoView::ExitFullscreen(JNIEnv*, jobject,
                                     jboolean release_media_player) {
  if (player_) {
    player_->ExitFullscreen(release_media_player);
    player_->SetJNIListener(NULL);
    listener_.reset();

    // Fire off a timer so that we will close the fullscreen view in case the
    // renderer crashes.
    timeout_.Start(FROM_HERE,
                   base::TimeDelta::FromMilliseconds(kTimeout),
                   this, &ChromeVideoView::OnTimeout);
  }
}

void ChromeVideoView::SetSurface(JNIEnv* env, jobject obj,
                                 jobject surface,
                                 jint route_id,
                                 jint player_id) {
  SetSurfaceAsync(env,
                  surface,
                  SurfaceTexturePeer::SET_VIDEO_SURFACE_TEXTURE,
                  route_id,
                  player_id,
                  NULL);
}

void ChromeVideoView::UpdateMediaMetadata(JNIEnv* env, jobject obj) {
  scoped_ptr<webkit_glue::MediaMetadataAndroid> metadata(GetMediaMetadata());
  Java_ChromeVideoView_updateMediaMetadata(env,
                                           obj,
                                           metadata->width(),
                                           metadata->height(),
                                           (int) metadata->duration() * 1000,
                                           metadata->can_pause(),
                                           metadata->can_seek_forward(),
                                           metadata->can_seek_backward());
}

bool RegisterChromeVideoView(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
