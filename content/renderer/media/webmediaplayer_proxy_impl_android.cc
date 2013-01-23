// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webmediaplayer_proxy_impl_android.h"

#include "base/bind.h"
#include "base/message_loop.h"
#include "content/common/view_messages.h"
#include "webkit/media/webmediaplayer_android.h"
#include "webkit/media/webmediaplayer_manager_android.h"

namespace media {

WebMediaPlayerProxyImplAndroid::WebMediaPlayerProxyImplAndroid(
    content::RenderView* render_view,
    webkit_glue::WebMediaPlayerManagerAndroid* manager)
    : content::RenderViewObserver(render_view),
      fullscreen_(false),
      fullscreen_player_id_(-1),
      manager_(manager) {
}

WebMediaPlayerProxyImplAndroid::~WebMediaPlayerProxyImplAndroid() {}

bool WebMediaPlayerProxyImplAndroid::OnMessageReceived(
    const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebMediaPlayerProxyImplAndroid, msg)
    IPC_MESSAGE_HANDLER(ViewMsg_MediaPrepared,
                        OnMediaPrepared)
    IPC_MESSAGE_HANDLER(ViewMsg_MediaPlaybackCompleted,
                        OnMediaPlaybackCompleted)
    IPC_MESSAGE_HANDLER(ViewMsg_MediaBufferingUpdate,
                        OnMediaBufferingUpdate)
    IPC_MESSAGE_HANDLER(ViewMsg_MediaSeekCompleted,
                        OnMediaSeekCompleted)
    IPC_MESSAGE_HANDLER(ViewMsg_MediaError,
                        OnMediaError)
    IPC_MESSAGE_HANDLER(ViewMsg_MediaVideoSizeChanged,
                        OnVideoSizeChanged)
    IPC_MESSAGE_HANDLER(ViewMsg_MediaTimeUpdate,
                        OnTimeUpdate)
    IPC_MESSAGE_HANDLER(ViewMsg_ExitFullscreen,
                        OnExitFullscreen)
    IPC_MESSAGE_HANDLER(ViewMsg_FullscreenVideoPlay,
                        OnPlayerPlay)
    IPC_MESSAGE_HANDLER(ViewMsg_FullscreenVideoPause,
                        OnPlayerPause)
    IPC_MESSAGE_HANDLER(ViewMsg_MediaVideoSurfaceReleased,
                        OnSurfaceReleased)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void WebMediaPlayerProxyImplAndroid::Load(
    int player_id, const std::string& url,
    const std::string& first_party_for_cookies) {
  Send(new ViewHostMsg_MediaPlayerLoad(
    routing_id(), player_id, url, first_party_for_cookies));
}

void WebMediaPlayerProxyImplAndroid::Start(int player_id) {
  Send(new ViewHostMsg_MediaPlayerStart(routing_id(), player_id));
}

void WebMediaPlayerProxyImplAndroid::Pause(int player_id) {
  Send(new ViewHostMsg_MediaPlayerPause(routing_id(), player_id));
}

void WebMediaPlayerProxyImplAndroid::Seek(int player_id, int msec) {
  Send(new ViewHostMsg_MediaPlayerSeek(routing_id(), player_id, msec));
}

void WebMediaPlayerProxyImplAndroid::ReleaseResources(int player_id) {
  Send(new ViewHostMsg_MediaPlayerRelease(routing_id(), player_id));
}

void WebMediaPlayerProxyImplAndroid::DestroyPlayer(int player_id) {
  Send(new ViewHostMsg_DestroyMediaPlayer(routing_id(), player_id));
  if (fullscreen_player_id_ == player_id) {
    fullscreen_player_id_ = -1;
  }
}

void WebMediaPlayerProxyImplAndroid::OnMediaPrepared(
    int player_id,
    float duration) {
  webkit_glue::WebMediaPlayerAndroid* player =
      manager_->GetMediaPlayer(player_id);
  if (player)
    player->OnMediaPrepared(duration);
}

void WebMediaPlayerProxyImplAndroid::OnMediaPlaybackCompleted(
    int player_id) {
  webkit_glue::WebMediaPlayerAndroid* player =
      manager_->GetMediaPlayer(player_id);
  if (player)
    player->OnPlaybackComplete();
}

void WebMediaPlayerProxyImplAndroid::OnMediaBufferingUpdate(
    int player_id, int percent) {
  webkit_glue::WebMediaPlayerAndroid* player =
      manager_->GetMediaPlayer(player_id);
  if (player)
    player->OnBufferingUpdate(percent);
}

void WebMediaPlayerProxyImplAndroid::OnMediaSeekCompleted(
    int player_id, int msec) {
  webkit_glue::WebMediaPlayerAndroid* player =
      manager_->GetMediaPlayer(player_id);
  if (player)
    player->OnSeekComplete(msec);
}

void WebMediaPlayerProxyImplAndroid::OnMediaError(
    int player_id, int error) {
  webkit_glue::WebMediaPlayerAndroid* player =
      manager_->GetMediaPlayer(player_id);
  if (player)
    player->OnMediaError(error);
}

void WebMediaPlayerProxyImplAndroid::OnVideoSizeChanged(
    int player_id, int width, int height) {
  webkit_glue::WebMediaPlayerAndroid* player =
      manager_->GetMediaPlayer(player_id);
  if (player)
    player->OnVideoSizeChanged(width, height);
}

void WebMediaPlayerProxyImplAndroid::OnSurfaceReleased(
    int player_id) {
  webkit_glue::WebMediaPlayerAndroid* player =
      manager_->GetMediaPlayer(player_id);
  if (player)
    player->OnSurfaceReleased();
}

void WebMediaPlayerProxyImplAndroid::OnTimeUpdate(
    int player_id, int msec) {
  webkit_glue::WebMediaPlayerAndroid* player =
      manager_->GetMediaPlayer(player_id);
  if (player)
    player->OnTimeUpdate(msec);
}

void WebMediaPlayerProxyImplAndroid::OnExitFullscreen(
    int player_id) {
  fullscreen_player_id_ = -1;
  fullscreen_ = false;
  webkit_glue::WebMediaPlayerAndroid* player =
      manager_->GetMediaPlayer(player_id);
  if (player)
    player->OnExitFullscreen();
}

void WebMediaPlayerProxyImplAndroid::OnPlayerPlay(int player_id) {
  webkit_glue::WebMediaPlayerAndroid* player =
      manager_->GetMediaPlayer(player_id);
  if (player)
    player->OnMediaPlayerPlay();
}

void WebMediaPlayerProxyImplAndroid::OnPlayerPause(int player_id) {
  webkit_glue::WebMediaPlayerAndroid* player =
      manager_->GetMediaPlayer(player_id);
  if (player)
    player->OnMediaPlayerPause();
}

void WebMediaPlayerProxyImplAndroid::EnterFullscreen(int player_id) {
  if (CanEnterFullscreen(player_id)) {
    fullscreen_player_id_ = player_id;
    fullscreen_ = true;
    Send(new ViewHostMsg_EnterFullscreen(routing_id(), player_id));
  }
}

void WebMediaPlayerProxyImplAndroid::LeaveFullscreen(int player_id) {
  if (fullscreen_player_id_ == player_id) {
    Send(new ViewHostMsg_LeaveFullscreen(routing_id(), player_id));
    fullscreen_player_id_ = -1;
    fullscreen_ = false;
  }
}

bool WebMediaPlayerProxyImplAndroid::CanEnterFullscreen(int player_id) {
  return !fullscreen_ || (fullscreen_player_id_ == -1) ||
      (player_id == fullscreen_player_id_);
}

bool WebMediaPlayerProxyImplAndroid::IsFullscreenMode() {
  return fullscreen_;
}


}  // namespace media
