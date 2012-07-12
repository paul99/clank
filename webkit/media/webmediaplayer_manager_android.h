// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_MEDIA_WEBMEDIAPLAYER_MANAGER_ANDROID_H_
#define WEBKIT_MEDIA_WEBMEDIAPLAYER_MANAGER_ANDROID_H_

#include <jni.h>
#include <map>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "webkit/media/fullscreen_video_proxy_android.h"

namespace webkit_glue {

class FullscreenVideoProxyAndroid;
class WebMediaPlayerAndroid;

class WebMediaPlayerManagerAndroid {
 public:
  WebMediaPlayerManagerAndroid(FullscreenVideoProxyAndroid* fullscreen_proxy,
                               int32 routing_id);
  virtual ~WebMediaPlayerManagerAndroid();

  virtual int RegisterMediaPlayer(WebMediaPlayerAndroid* player);
  virtual void UnRegisterMediaPlayer(int player_id);
  WebMediaPlayerAndroid* GetWebMediaPlayer(int player_id);

  virtual void ReleaseMediaResources();

  virtual void SetVideoSurface(jobject j_surface, int player_id);

  virtual bool IsFullscreenMode();

  virtual void SetActivePlayerLimit(int limit);

  // Check if the current player is allowed for preload. Returns false if
  // active decoders are hitting the limit.
  virtual bool AllowedForPreload(int player_id);

  // Mediaplayer needs resources to start playing video. If active decoders
  // are hitting the limit, release some resources.
  virtual void RequestForMediaResources(int player_id);

  FullscreenVideoProxyAndroid* fullscreen_proxy() {
    return fullscreen_proxy_.get();
  }

  int32 routing_id() { return routing_id_; }

 private:
  // Information needed to manage WebMediaPlayerAndroid.
  struct MediaPlayerInfo {
    bool is_active_;
    webkit_glue::WebMediaPlayerAndroid* player;
  };

  // Info for all available WebMediaPlayerAndroid on a page; kept so that
  // we can enumerate them to send updates about tab focus and visibily.
  std::map<int32, MediaPlayerInfo> media_players_;

  // ID used to create the next media player for passing the Surface.
  int32 next_media_player_id_;

  int32 routing_id_;

  // Limit on active players.
  int active_player_limit_;

  // Number of active players.
  int active_players_;

  // Player id for the fullscreen video.
  int fullscreen_player_id_;

  // Fullscreen proxy used to communicate between the mediaplayer and
  // ChromeVideoView.
  scoped_ptr<FullscreenVideoProxyAndroid> fullscreen_proxy_;

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerManagerAndroid);
};

}  // namespace webkit_glue

#endif  // WEBKIT_MEDIA_WEBMEDIAPLAYER_MANAGER_ANDROID_H_
