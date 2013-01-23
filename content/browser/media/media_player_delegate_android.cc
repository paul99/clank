// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/media_player_delegate_android.h"

#include "base/bind.h"
#include "content/browser/android/cookies_retriever_impl.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"

// Threshold on the number of media players per renderer before we start
// attempting to release inactive media players.
static const int kMediaPlayerThreshold = 1;

MediaPlayerDelegateAndroid::MediaPlayerDelegateAndroid(
    RenderViewHost* render_view_host)
    : RenderViewHostObserver(render_view_host),
      fullscreen_player_id_(-1) {
  video_view_.reset(new ChromeVideoView());
}

MediaPlayerDelegateAndroid::~MediaPlayerDelegateAndroid() {
}

///////////////////////////////////////////////////////////////////////////////
// RenderViewHost, IPC message handlers:

bool MediaPlayerDelegateAndroid::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(MediaPlayerDelegateAndroid, msg)
    IPC_MESSAGE_HANDLER(ViewHostMsg_EnterFullscreen,
                        OnEnterFullscreen)
    IPC_MESSAGE_HANDLER(ViewHostMsg_LeaveFullscreen,
                        OnLeaveFullscreen)
    IPC_MESSAGE_HANDLER(ViewHostMsg_MediaPlayerLoad,
                        OnLoad)
    IPC_MESSAGE_HANDLER(ViewHostMsg_MediaPlayerStart,
                        OnStart)
    IPC_MESSAGE_HANDLER(ViewHostMsg_MediaPlayerSeek,
	                OnSeek)
    IPC_MESSAGE_HANDLER(ViewHostMsg_MediaPlayerPause,
                        OnPause)
    IPC_MESSAGE_HANDLER(ViewHostMsg_MediaPlayerRelease,
                        OnReleaseResources)
    IPC_MESSAGE_HANDLER(ViewHostMsg_DestroyMediaPlayer,
                        OnDestroyPlayer)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void MediaPlayerDelegateAndroid::FullscreenVideoPlay() {
  MediaPlayerBridge* player = GetFullscreenPlayer();
  if (player) {
    player->Start();
    Send(new ViewMsg_FullscreenVideoPlay(routing_id(), fullscreen_player_id_));
  }
}

void MediaPlayerDelegateAndroid::FullscreenVideoPause() {
  MediaPlayerBridge* player = GetFullscreenPlayer();
  if (player) {
    player->Pause();
    Send(new ViewMsg_FullscreenVideoPause(routing_id(), fullscreen_player_id_));
  }
}

void MediaPlayerDelegateAndroid::FullscreenVideoSeek(int msec) {
  MediaPlayerBridge* player = GetFullscreenPlayer();
  if (player)
    player->SeekTo(msec);
}

void MediaPlayerDelegateAndroid::ExitFullscreen(bool release_media_player) {
  Send(new ViewMsg_ExitFullscreen(routing_id(), fullscreen_player_id_));
  MediaPlayerBridge* player = GetFullscreenPlayer();
  fullscreen_player_id_ = -1;
  if (!player)
    return;
  if (release_media_player)
    player->Release();
  else
    player->SetVideoSurface(NULL, false);
}

void MediaPlayerDelegateAndroid::OnLoad(
    int player_id, const std::string& url,
    const std::string& first_party_for_cookies) {
  for (ScopedVector<MediaPlayerBridge>::iterator it = players_.begin();
      it != players_.end(); ++it) {
    if ((*it)->player_id() == player_id) {
      players_.erase(it);
      break;
    }
  }

  content::RenderProcessHost* host = render_view_host()->process();
  content::BrowserContext* context =
      render_view_host()->process()->GetBrowserContext();
  players_.push_back(new MediaPlayerBridge(
      player_id, url, first_party_for_cookies,
      new content::CookiesRetrieverImpl(context, host->GetID(), routing_id()),
      context->IsOffTheRecord(), this,
      base::Bind(&MediaPlayerDelegateAndroid::OnError, base::Unretained(this)),
      base::Bind(&MediaPlayerDelegateAndroid::OnVideoSizeChanged,
          base::Unretained(this)),
      base::Bind(&MediaPlayerDelegateAndroid::OnBufferingUpdate,
          base::Unretained(this)),
      base::Bind(&MediaPlayerDelegateAndroid::OnPrepared,
          base::Unretained(this)),
      base::Bind(&MediaPlayerDelegateAndroid::OnPlaybackComplete,
          base::Unretained(this)),
      base::Bind(&MediaPlayerDelegateAndroid::OnSeekComplete,
          base::Unretained(this)),
      base::Bind(&MediaPlayerDelegateAndroid::OnTimeUpdate,
          base::Unretained(this))));
}

void MediaPlayerDelegateAndroid::OnStart(int player_id) {
  MediaPlayerBridge* player = GetPlayer(player_id);
  if (player)
    player->Start();
}

void MediaPlayerDelegateAndroid::OnSeek(int player_id, int msec) {
  MediaPlayerBridge* player = GetPlayer(player_id);
  if (player)
    player->SeekTo(msec);
}

void MediaPlayerDelegateAndroid::OnPause(int player_id) {
  MediaPlayerBridge* player = GetPlayer(player_id);
  if (player)
    player->Pause();
}

void MediaPlayerDelegateAndroid::OnEnterFullscreen(int player_id) {
  if (fullscreen_player_id_ != -1 && fullscreen_player_id_ != player_id)
    return;

  fullscreen_player_id_ = player_id;
  video_view_->CreateFullscreenView(this);
}

void MediaPlayerDelegateAndroid::OnLeaveFullscreen(int player_id) {
  if (fullscreen_player_id_ == player_id) {
    MediaPlayerBridge* player = GetPlayer(player_id);
    // Do nothing is player has already exited fullscreen.
    if (player && player->fullscreen())
      player->SetVideoSurface(NULL, false);
    video_view_->DestroyFullscreenView();
    fullscreen_player_id_ = -1;
  }
}

void MediaPlayerDelegateAndroid::OnReleaseResources(int player_id) {
  MediaPlayerBridge* player = GetPlayer(player_id);
  // Don't release the fullscreen player when tab visibility changes,
  // it will be released when user hit the back/home button or when
  // OnDestroyPlayer is called.
  if (player && !player->fullscreen())
    player->Release();
}

void MediaPlayerDelegateAndroid::OnDestroyPlayer(int player_id) {
  for (ScopedVector<MediaPlayerBridge>::iterator it = players_.begin();
      it != players_.end(); ++it) {
    if ((*it)->player_id() == player_id) {
      players_.erase(it);
      break;
    }
  }
  if (fullscreen_player_id_ == player_id)
    fullscreen_player_id_ = -1;
}

MediaPlayerBridge* MediaPlayerDelegateAndroid::GetPlayer(int player_id) {
  for (ScopedVector<MediaPlayerBridge>::iterator it = players_.begin();
      it != players_.end(); ++it) {
    if ((*it)->player_id() == player_id)
      return *it;
  }
  return NULL;
}

MediaPlayerBridge* MediaPlayerDelegateAndroid::GetFullscreenPlayer() {
  return GetPlayer(fullscreen_player_id_);
}

void MediaPlayerDelegateAndroid::OnPrepared(int player_id, int duration) {
  float dur = static_cast<float>(duration / 1000.0);

  Send(new ViewMsg_MediaPrepared(routing_id(), player_id, dur));
  if (fullscreen_player_id_ != -1)
    video_view_->UpdateMediaMetadata();
}

void MediaPlayerDelegateAndroid::OnPlaybackComplete(int player_id) {
  Send(new ViewMsg_MediaPlaybackCompleted(routing_id(), player_id));
  if (fullscreen_player_id_ != -1)
    video_view_->OnPlaybackComplete();
}

void MediaPlayerDelegateAndroid::OnBufferingUpdate(
    int player_id, int percentage) {
  Send(new ViewMsg_MediaBufferingUpdate(routing_id(), player_id, percentage));
  if (fullscreen_player_id_ != -1)
    video_view_->OnBufferingUpdate(percentage);
}

void MediaPlayerDelegateAndroid::OnSeekComplete(int player_id, int msec) {
  Send(new ViewMsg_MediaSeekCompleted(routing_id(), player_id, msec));
}

void MediaPlayerDelegateAndroid::OnError(int player_id, int error) {
  Send(new ViewMsg_MediaError(routing_id(), player_id, error));
  if (fullscreen_player_id_ != -1)
    video_view_->OnError(error);
}

void MediaPlayerDelegateAndroid::OnVideoSizeChanged(int player_id,
    int width, int height) {
  Send(new ViewMsg_MediaVideoSizeChanged(routing_id(), player_id,
      width, height));
  if (fullscreen_player_id_ != -1)
    video_view_->OnVideoSizeChanged(width, height);
}

void MediaPlayerDelegateAndroid::OnTimeUpdate(int player_id, int msec) {
  Send(new ViewMsg_MediaTimeUpdate(routing_id(), player_id, msec));
}

void MediaPlayerDelegateAndroid::RequestMediaResources(
    MediaPlayerBridge* player) {
  if (player == NULL)
    return;

  int num_active_player = 0;
  ScopedVector<MediaPlayerBridge>::iterator it;
  for (it = players_.begin(); it != players_.end(); ++it) {
    if (!(*it)->prepared())
      continue;

    // The player is already active, ignore it.
    if ((*it) == player)
      return;
    else
      num_active_player++;
  }

  // Number of active players are less than the threshold, do nothing.
  if (num_active_player < kMediaPlayerThreshold)
    return;

  for (it = players_.begin(); it != players_.end(); ++it) {
    if ((*it)->prepared() && !(*it)->IsPlaying() && !(*it)->fullscreen()) {
      (*it)->Release();
      Send(new ViewMsg_MediaVideoSurfaceReleased(routing_id(),
          (*it)->player_id()));
    }
  }
}

void MediaPlayerDelegateAndroid::ReleaseMediaResources(
    MediaPlayerBridge* player) {
  // Since player->prepared() will be false, nothing needs to be done.
}

void MediaPlayerDelegateAndroid::CleanUpAllPlayers() {
  players_.reset();
  if (fullscreen_player_id_ != -1) {
    video_view_->DestroyFullscreenView();
    fullscreen_player_id_ = -1;
  }
}

