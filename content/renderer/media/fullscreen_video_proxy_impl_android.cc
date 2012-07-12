// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/fullscreen_video_proxy_impl_android.h"

#include "content/common/view_messages.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/media/fullscreen_video_message_filter_android.h"
#include "webkit/media/media_metadata_android.h"
#include "webkit/media/webmediaplayer_android.h"

using content::RenderThread;

namespace media {

FullscreenVideoProxyImplAndroid::FullscreenVideoProxyImplAndroid(
    IPC::Message::Sender* sender,
    int routing_id)
    : sender_(sender),
      routing_id_(routing_id),
      web_media_player_(NULL),
      message_filter_(NULL),
      is_fullscreen_mode_(false) {
}

FullscreenVideoProxyImplAndroid::~FullscreenVideoProxyImplAndroid() {
  LeaveFullscreen();
}

void FullscreenVideoProxyImplAndroid::Notify(int msg, int ext1, int ext2) {
  // TODO (qinmin): to reduce traffic, we don't need to forward all the messages
  Send(new ViewHostMsg_MediaPlayerNotifications(
      routing_id_, msg, ext1, ext2));
}

void FullscreenVideoProxyImplAndroid::UpdateCurrentTime(float seconds) {
  Send(new ViewHostMsg_CurrentTime(
      routing_id_, seconds));
}

void FullscreenVideoProxyImplAndroid::UpdateMetadata(
    webkit_glue::MediaMetadataAndroid* metadata) {
  Send(new ViewHostMsg_UpdateMetadata(
      routing_id_, *metadata));
  // TODO (qinmin): should switch to use Scoped_ptr.Pass() for metadata.
  delete metadata;
}

bool FullscreenVideoProxyImplAndroid::Send(IPC::Message* message) {
  if (!sender_) {
    delete message;
    return false;
  }

  return sender_->Send(message);
}

void FullscreenVideoProxyImplAndroid::OnFullscreenVideoPlay() {
  if (web_media_player_) {
    web_media_player_->OnMediaPlayerPlay();
  }
}

void FullscreenVideoProxyImplAndroid::OnFullscreenVideoSeek(float seconds) {
  if (web_media_player_) {
    web_media_player_->OnMediaPlayerSeek(seconds);
  }
}

void FullscreenVideoProxyImplAndroid::OnFullscreenVideoPause() {
  if (web_media_player_) {
    web_media_player_->OnMediaPlayerPause();
  }
}

void FullscreenVideoProxyImplAndroid::OnExitFullscreen(
    bool release_media_player) {
  if (web_media_player_) {
    web_media_player_->OnExitFullscreen(release_media_player);
  }
}

void FullscreenVideoProxyImplAndroid::EnterFullscreen(
    webkit_glue::WebMediaPlayerAndroid* web_media_player) {
  if (web_media_player_) {
    // The current fullscreen video has not exited fullscreen yet.
    return;
  }

  web_media_player_ = web_media_player;

  message_filter_ = new FullscreenVideoMessageFilterAndroid(this);
  RenderThread::Get()->AddFilter(message_filter_);
  Send(new ViewHostMsg_CreateVideoSurface(
      routing_id_,
      web_media_player->player_id(),
      *(web_media_player->GetMetadata())));

  is_fullscreen_mode_ = true;
}

void FullscreenVideoProxyImplAndroid::LeaveFullscreen() {
  web_media_player_ = NULL;

  // Remove the filter if EnterFullscreen() was called and the filter was
  // added.
  if (message_filter_) {
    RenderThread::Get()->RemoveFilter(message_filter_);
    message_filter_ = NULL;
  }

  Send(new ViewHostMsg_DestroyFullscreenView(routing_id_));
  is_fullscreen_mode_ = false;
}

bool FullscreenVideoProxyImplAndroid::IsFullscreenMode() {
  return is_fullscreen_mode_;
}

void FullscreenVideoProxyImplAndroid::SetWebMediaPlayer(
    webkit_glue::WebMediaPlayerAndroid* web_media_player) {
  web_media_player_ = web_media_player;
}

}  // namespace media
