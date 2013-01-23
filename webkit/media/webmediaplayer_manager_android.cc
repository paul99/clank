// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/media/webmediaplayer_manager_android.h"

namespace webkit_glue {

WebMediaPlayerManagerAndroid::WebMediaPlayerManagerAndroid()
    : next_media_player_id_(0) {
}

WebMediaPlayerManagerAndroid::~WebMediaPlayerManagerAndroid() {
    ReleaseMediaResources();
}

int WebMediaPlayerManagerAndroid::RegisterMediaPlayer(
    WebMediaPlayerAndroid* player) {
  media_players_[next_media_player_id_] = player;
  return next_media_player_id_++;
}

void WebMediaPlayerManagerAndroid::ReleaseMediaResources() {
  std::map<int32, WebMediaPlayerAndroid*>::iterator player_it;
  for (player_it = media_players_.begin();
      player_it != media_players_.end(); ++player_it) {
    (player_it->second)->ReleaseMediaResources();
  }
}

void WebMediaPlayerManagerAndroid::UnregisterMediaPlayer(int player_id) {
  media_players_.erase(player_id);
}

WebMediaPlayerAndroid* WebMediaPlayerManagerAndroid::GetMediaPlayer(
    int player_id) {
  std::map<int32, WebMediaPlayerAndroid*>::iterator iter =
      media_players_.find(player_id);
  if (iter != media_players_.end())
    return iter->second;
  return NULL;
}

}  // namespace webkit_glue
