// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEBMEDIAPLAYER_PROXY_IMPL_ANDROID_H_
#define CONTENT_RENDERER_MEDIA_WEBMEDIAPLAYER_PROXY_IMPL_ANDROID_H_

#include <map>

#include "content/public/renderer/render_view_observer.h"
#include "ipc/ipc_channel_proxy.h"
#include "webkit/media/webmediaplayer_proxy_android.h"

namespace webkit_glue {
class WebMediaPlayerManagerAndroid;
}

namespace media {

// This class manages all the IPC communications between WebMediaPlayerAndroid
// and the android::Mediaplayer in the browser process.
class WebMediaPlayerProxyImplAndroid :
  public content::RenderViewObserver,
  public webkit_glue::WebMediaPlayerProxyAndroid {
 public:
  explicit WebMediaPlayerProxyImplAndroid(
      content::RenderView* render_view,
      webkit_glue::WebMediaPlayerManagerAndroid* manager);
  virtual ~WebMediaPlayerProxyImplAndroid();

  bool OnMessageReceived(const IPC::Message& msg) OVERRIDE;

  virtual void Load(int player_id, const std::string& url,
                    const std::string& first_party_for_cookies) OVERRIDE;
  virtual void Start(int player_id) OVERRIDE;
  virtual void Pause(int player_id) OVERRIDE;
  virtual void Seek(int player_id, int msec) OVERRIDE;

  virtual void EnterFullscreen(int player_id) OVERRIDE;
  virtual void LeaveFullscreen(int player_id) OVERRIDE;

  virtual void ReleaseResources(int player_id) OVERRIDE;
  virtual void DestroyPlayer(int player_id) OVERRIDE;

  virtual bool CanEnterFullscreen(int player_id) OVERRIDE;
  virtual bool IsFullscreenMode() OVERRIDE;

 private:
  void OnMediaPrepared(int player_id, float duration);
  void OnMediaPlaybackCompleted(int player_id);
  void OnMediaBufferingUpdate(int player_id, int percent);
  void OnMediaSeekCompleted(int player_id, int msec);
  void OnMediaError(int player_id, int error);
  void OnVideoSizeChanged(
      int player_id, int width, int height);
  void OnSurfaceReleased(int player_id);
  void OnTimeUpdate(int player_id, int msec);
  void OnExitFullscreen(int player_id);
  void OnPlayerPlay(int player_id);
  void OnPlayerPause(int player_id);

  bool fullscreen_;

  // Using a fullscreen player id to check whether a particular player can
  // enter fullscreen is wrong. However, we don't have FULLSCREEN_API here.
  // We should use Document::webkitFullscreenElement() to check whether
  // the element is allowed to enter fullscreen.
  int fullscreen_player_id_;

  webkit_glue::WebMediaPlayerManagerAndroid* manager_;

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerProxyImplAndroid);
};

}  // namespace media

#endif  // CONTENT_RENDERER_MEDIA_WEBMEDIAPLAYER_PROXY_IMPL_ANDROID_H_
