// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/media/media_player_bridge_manager_impl.h"

#include "content/browser/android/media_player_bridge.h"

// Threshold on the number of media players per renderer before we start
// attempting to release inactive media players.
static const int kMediaPlayerThreshold = 1;

namespace webkit_glue {

MediaPlayerBridgeManagerImpl::MediaPlayerBridgeManagerImpl(
    int threshold)
    : active_player_threshold_(threshold) {
}

MediaPlayerBridgeManagerImpl::~MediaPlayerBridgeManagerImpl() {
}

void MediaPlayerBridgeManagerImpl::RequestMediaResources(
    MediaPlayerBridge* player) {
  if (player == NULL)
    return;

  int num_active_player = 0;
  std::vector<MediaPlayerBridge*>::iterator it;
  for (it = players_.begin(); it != players_.end(); ++it) {
    // The player is already active, ignore it.
    if ((*it) == player)
      return;

    num_active_player++;
  }

  if (num_active_player < active_player_threshold_) {
    players_.push_back(player);
    return;
  }

  // Get a list of the players to free.
  std::vector<MediaPlayerBridge*> players_to_free;
  for (it = players_.begin(); it != players_.end(); ++it) {
    if ((*it)->prepared() && !(*it)->IsPlaying() && !(*it)->fullscreen())
      players_to_free.push_back(*it);
  }

  for (it = players_to_free.begin(); it != players_to_free.end(); ++it)
    (*it)->Release();

  players_.push_back(player);
}

void MediaPlayerBridgeManagerImpl::ReleaseMediaResources(
    MediaPlayerBridge* player) {
  for (std::vector<MediaPlayerBridge*>::iterator it = players_.begin();
      it != players_.end(); ++it) {
    if ((*it) == player) {
      players_.erase(it);
      return;
    }
  }
}

}  // namespace webkit_glue
