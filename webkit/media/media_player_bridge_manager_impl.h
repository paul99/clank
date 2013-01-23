// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_MEDIA_MEDIA_PLAYER_BRIDGE_MANAGER_IMPL_H_
#define WEBKIT_MEDIA_MEDIA_PLAYER_BRIDGE_MANAGER_IMPL_H_

#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "media/base/media_player_bridge_manager.h"

namespace webkit_glue {

// Class for managing active MediaPlayerBridge objects for if mediaplayer
// sits in render process.
class MediaPlayerBridgeManagerImpl :
    public media::MediaPlayerBridgeManager {
 public:
  explicit MediaPlayerBridgeManagerImpl(int threshold);
  virtual ~MediaPlayerBridgeManagerImpl();

  virtual void RequestMediaResources(MediaPlayerBridge* player) OVERRIDE;
  virtual void ReleaseMediaResources(MediaPlayerBridge* player) OVERRIDE;

 private:
  std::vector<MediaPlayerBridge*> players_;
  int active_player_threshold_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerBridgeManagerImpl);
};

}  // namespace webkit_glue

#endif  // WEBKIT_MEDIA_MEDIA_PLAYER_BRIDGE_MANAGER_IMPL_H_
