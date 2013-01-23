// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_MEDIA_PLAYER_BRIDGE_MANAGER_H_
#define MEDIA_BASE_MEDIA_PLAYER_BRIDGE_MANAGER_H_

class MediaPlayerBridge;

namespace media {

// Class for managing active MediaPlayerBridge objects.
class MediaPlayerBridgeManager {
 public:
  virtual ~MediaPlayerBridgeManager() {};

  virtual void RequestMediaResources(MediaPlayerBridge* player) = 0;
  virtual void ReleaseMediaResources(MediaPlayerBridge* player) = 0;
};

}  // namespace media

#endif  // MEDIA_BASE_MEDIA_PLAYER_BRIDGE_MANAGER_H_
