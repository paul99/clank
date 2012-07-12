// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/media_player_delegate_android.h"

#include "content/browser/media/media_player_listener_android.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_thread.h"
#include "webkit/media/media_metadata_android.h"
#include "webkit/media/media_player_constants_android.h"

MediaPlayerDelegateAndroid::MediaPlayerDelegateAndroid(
    RenderViewHost* render_view_host)
    : RenderViewHostObserver(render_view_host),
      player_id_(0),
      render_handle_(0),
      paused_(true),
      duration_(-1),
      width_(0),
      height_(0),
      current_time_(0),
      can_pause_(true),
      can_seek_forward_(true),
      can_seek_backward_(true),
      jni_listener_(NULL) {
  video_view_.reset(new ChromeVideoView());
}

MediaPlayerDelegateAndroid::~MediaPlayerDelegateAndroid() {
}

///////////////////////////////////////////////////////////////////////////////
// RenderViewHost, IPC message handlers:

bool MediaPlayerDelegateAndroid::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MediaPlayerDelegateAndroid, msg)
    IPC_MESSAGE_HANDLER(ViewHostMsg_CreateVideoSurface,
                        OnCreateFullscreenView)
    IPC_MESSAGE_HANDLER(ViewHostMsg_DestroyFullscreenView,
                        OnDestroyFullscreenView)
    IPC_MESSAGE_HANDLER(ViewHostMsg_MediaPlayerNotifications,
                        OnNotify)
    IPC_MESSAGE_HANDLER(ViewHostMsg_CurrentTime,
                        OnTimeUpdate)
    IPC_MESSAGE_HANDLER(ViewHostMsg_UpdateMetadata,
                        OnUpdateMetadata)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void MediaPlayerDelegateAndroid::SetJNIListener(
    MediaPlayerListenerAndroid* listener) {
  jni_listener_ = listener;
}

void MediaPlayerDelegateAndroid::Play() {
  paused_ = false;
  Send(new ViewMsg_FullscreenVideoPlay(routing_id()));
}

void MediaPlayerDelegateAndroid::Pause() {
  paused_ = true;
  Send(new ViewMsg_FullscreenVideoPause(routing_id()));
}

void MediaPlayerDelegateAndroid::Seek(float seconds) {
  Send(new ViewMsg_FullscreenVideoSeek(routing_id(), seconds));
}

bool MediaPlayerDelegateAndroid::Paused() const {
  return paused_;
}

float MediaPlayerDelegateAndroid::Duration() const {
  return duration_;
}

float MediaPlayerDelegateAndroid::CurrentTime() const {
  return current_time_;
}

int MediaPlayerDelegateAndroid::GetVideoWidth() const {
  return width_;
}

int MediaPlayerDelegateAndroid::GetVideoHeight() const {
  return height_;
}

bool MediaPlayerDelegateAndroid::CanPause() const {
  return can_pause_;
}

bool MediaPlayerDelegateAndroid::CanSeekForward() const {
  return can_seek_forward_;
}

bool MediaPlayerDelegateAndroid::CanSeekBackward() const {
  return can_seek_backward_;
}

bool MediaPlayerDelegateAndroid::IsPlaying() const {
  return !paused_;
}

void MediaPlayerDelegateAndroid::ExitFullscreen(bool release_media_player) {
  Send(new ViewMsg_ExitFullscreen(routing_id(), release_media_player));
}

void MediaPlayerDelegateAndroid::OnCreateFullscreenView(
    int player_id,
    webkit_glue::MediaMetadataAndroid metadata) {
  player_id_ = player_id;
  render_handle_ = render_view_host()->process()->GetHandle();
  OnUpdateMetadata(metadata);
  video_view_->CreateFullscreenView(this);
}

void MediaPlayerDelegateAndroid::OnNotify(int msg, int ext1, int ext2) {
  if (msg == MEDIA_PLAYBACK_COMPLETE)
    paused_ = true;
  if (jni_listener_) {
    // If JNIlistener exists, forward the message to it.
    jni_listener_->Notify(msg, ext1, ext2);
  }
}

void MediaPlayerDelegateAndroid::OnTimeUpdate(float seconds) {
  current_time_ = seconds;
}

void MediaPlayerDelegateAndroid::OnDestroyFullscreenView() {
  if (video_view_.get()) {
    video_view_->DestroyFullscreenView();
  }
}

void MediaPlayerDelegateAndroid::OnUpdateMetadata(
    webkit_glue::MediaMetadataAndroid metadata) {
  paused_ = metadata.paused();
  duration_ = metadata.duration();
  width_ = metadata.width();
  height_ = metadata.height();
  current_time_ = metadata.current_time();
  can_pause_ = metadata.can_pause();
  can_seek_forward_ = metadata.can_seek_forward();
  can_seek_backward_ = metadata.can_seek_backward();
  if (video_view_.get()) {
    video_view_->UpdateMediaMetadata();
  }
}
