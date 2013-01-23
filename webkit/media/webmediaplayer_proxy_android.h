// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_WEBMEDIAPLAYER_PROXY_ANDROID_H_
#define WEBKIT_GLUE_WEBMEDIAPLAYER_PROXY_ANDROID_H_

namespace webkit_glue {

// A interface to facilitate the IPC between the browser side
// android::MediaPlayer and the WebMediaPlayerAndroid in the renderer process.
// The implementation is provided by WebMediaPlayerProxyImplAndroid.
class WebMediaPlayerProxyAndroid {
 public:
  virtual ~WebMediaPlayerProxyAndroid() {};

  virtual void Load(int player_id, const std::string& url,
                    const std::string& first_party_for_cookies) = 0;
  virtual void Start(int player_id) = 0;
  virtual void Pause(int player_id) = 0;
  virtual void Seek(int player_id, int msec) = 0;
  virtual void ReleaseResources(int player_id) = 0;
  virtual void DestroyPlayer(int player_id) = 0;

  virtual void EnterFullscreen(int player_id) = 0;
  virtual void LeaveFullscreen(int player_id) = 0;

  virtual bool CanEnterFullscreen(int player_id) = 0;
  virtual bool IsFullscreenMode() = 0;
};

}  // namespace webkit_glue

#endif  // WEBKIT_GLUE_WEBMEDIAPLAYER_PROXY_ANDROID_H_
