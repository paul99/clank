// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_MEDIA_PLAYER_DELEGATE_ANDROID_H_
#define CONTENT_BROWSER_MEDIA_MEDIA_PLAYER_DELEGATE_ANDROID_H_

#include <map>

#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop_proxy.h"
#include "content/browser/android/chrome_video_view.h"
#include "content/browser/android/media_player_bridge.h"
#include "content/public/browser/render_view_host_observer.h"
#include "media/base/media_player_bridge_manager.h"

// This class manages all the mediaplayer objects. It receives the control
// information from the ChromeVideoView or the renderer process, and forward
// them to corresponding mediaplayers. Media notification messages are sent
// this class first, before it sends them to the renderer process or
// ChromeVideoView.
class MediaPlayerDelegateAndroid :
    public content::RenderViewHostObserver,
    public media::MediaPlayerBridgeManager {
 public:
  MediaPlayerDelegateAndroid(RenderViewHost* render_view_host);
  virtual ~MediaPlayerDelegateAndroid();

  // RenderViewHostObserver overrides.
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  // Playback controls.
  void FullscreenVideoPlay();
  void FullscreenVideoPause();
  void FullscreenVideoSeek(int msec);
  void ExitFullscreen(bool release_media_player);

  // An internal method that checks for current time routinely and generates
  // time update events.
  void OnTimeUpdate(int player_id, int msec);

  // Callbacks.
  void OnPrepared(int player_id, int duration);
  void OnPlaybackComplete(int player_id);
  void OnBufferingUpdate(int player_id, int percentage);
  void OnSeekComplete(int player_id, int msec);
  void OnError(int player_id, int error);
  void OnVideoSizeChanged(int player_id, int width, int height);

  // media::MediaPlayerBridgeManager overrides.
  virtual void RequestMediaResources(MediaPlayerBridge* player) OVERRIDE;
  virtual void ReleaseMediaResources(MediaPlayerBridge* player) OVERRIDE;

  void CleanUpAllPlayers();

  MediaPlayerBridge* GetFullscreenPlayer();
  MediaPlayerBridge* GetPlayer(int player_id);

  int32 route_id() { return routing_id(); }

 private:
  void OnEnterFullscreen(int player_id);
  void OnLeaveFullscreen(int player_id);

  void OnLoad(int player_id, const std::string& url,
              const std::string& first_party_for_cookies);
  void OnStart(int player_id);
  void OnSeek(int player_id, int msec);
  void OnPause(int player_id);
  void OnReleaseResources(int player_id);
  void OnDestroyPlayer(int player_id);

  void OnMediaPlayerPlay();
  void OnMediaPlayerSeek(float seconds);
  void OnMediaPlayerPause();
  void OnExitFullscreen(bool release_media_player);

  // Information needed to manage WebMediaPlayerAndroid.
  ScopedVector<MediaPlayerBridge> players_;

  int fullscreen_player_id_;

  scoped_ptr<ChromeVideoView> video_view_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerDelegateAndroid);
};

#endif  // CONTENT_BROWSER_MEDIA_MEDIA_PLAYER_DELEGATE_ANDROID_H_
