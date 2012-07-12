// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/media/webmediaplayer_manager_android.h"

#include "webkit/media/webmediaplayer_android.h"

// The number of maximum active player allowed in a renderer.
static const int kMaxMediaPlayerLimit = 2;

namespace webkit_glue {

WebMediaPlayerManagerAndroid::WebMediaPlayerManagerAndroid(
    FullscreenVideoProxyAndroid* fullscreen_proxy,
    int32 routing_id)
    : next_media_player_id_(0),
      routing_id_(routing_id),
      active_player_limit_(kMaxMediaPlayerLimit),
      active_players_(0),
      fullscreen_player_id_(0),
      fullscreen_proxy_(fullscreen_proxy) {
}

WebMediaPlayerManagerAndroid::~WebMediaPlayerManagerAndroid() {
}

void WebMediaPlayerManagerAndroid::SetActivePlayerLimit(int limit) {
  active_player_limit_ = limit;
}

int WebMediaPlayerManagerAndroid::RegisterMediaPlayer(
    WebMediaPlayerAndroid* player) {
  MediaPlayerInfo info;
  info.is_active_ = false;
  info.player = player;
  media_players_[next_media_player_id_] = info;
  return next_media_player_id_++;
}

void WebMediaPlayerManagerAndroid::UnRegisterMediaPlayer(int player_id) {
  std::map<int32, MediaPlayerInfo>::iterator iter =
        media_players_.find(player_id);
  DCHECK(iter != media_players_.end());

  if ((iter->second).is_active_)
    active_players_--;
  media_players_.erase(player_id);
}

void WebMediaPlayerManagerAndroid::ReleaseMediaResources() {
  std::map<int32, MediaPlayerInfo>::iterator player_it;
  for (player_it = media_players_.begin();
      player_it != media_players_.end(); ++player_it) {
    (player_it->second).is_active_ = false;
    (player_it->second).player->ReleaseMediaResources();
  }
  active_players_ = 0;
}

bool WebMediaPlayerManagerAndroid::AllowedForPreload(int player_id) {
  std::map<int32, MediaPlayerInfo>::iterator iter =
      media_players_.find(player_id);
  DCHECK(iter != media_players_.end());

  if (!((iter->second).is_active_) && active_players_ >= active_player_limit_)
    return false;

  return true;
}

void WebMediaPlayerManagerAndroid::RequestForMediaResources(int player_id) {
  std::map<int32, MediaPlayerInfo>::iterator iter =
      media_players_.find(player_id);
  DCHECK(iter != media_players_.end());

  if ((iter->second).is_active_)
    return;

  // Currently we only release active players that are paused. As a result,
  // the number of active players could go beyond the limit.
  // TODO (qinmin): We should release active video players when we reach the
  // limit even if they are playing.
  if (active_players_ >= active_player_limit_) {
    std::map<int32, MediaPlayerInfo>::iterator player_it;
    for (player_it = media_players_.begin();
        player_it != media_players_.end(); ++player_it) {
      // Skip the current fullscreen video if there is one.
      if (IsFullscreenMode() && player_it->first == fullscreen_player_id_)
        continue;
      // Release active players that are paused
      if ((player_it->second).is_active_
          && (player_it->second).player->paused()) {
        (player_it->second).player->ReleaseMediaResources();
        (player_it->second).is_active_ = false;
        active_players_--;
      }
    }
  }

  (iter->second).is_active_ = true;
  active_players_++;
}

WebMediaPlayerAndroid* WebMediaPlayerManagerAndroid::GetWebMediaPlayer(
    int player_id) {
  std::map<int32, MediaPlayerInfo>::iterator iter =
      media_players_.find(player_id);
  if (iter != media_players_.end())
    return (iter->second).player;
  else
    return NULL;
}

void WebMediaPlayerManagerAndroid::SetVideoSurface(jobject j_surface,
                                                   int player_id) {
  WebMediaPlayerAndroid* player = GetWebMediaPlayer(player_id);
  if (IsFullscreenMode()) {
    fullscreen_player_id_ = player_id;
  }

  if (player != NULL)
    player->SetVideoSurface(j_surface);
}

bool WebMediaPlayerManagerAndroid::IsFullscreenMode() {
  if (fullscreen_proxy_.get())
    return fullscreen_proxy_->IsFullscreenMode();

  return false;
}

}  // namespace webkit_glue
