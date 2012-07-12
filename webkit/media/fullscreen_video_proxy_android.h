// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_MEDIA_FULLSCREEN_VIDEO_PROXY_H_
#define WEBKIT_GLUE_MEDIA_FULLSCREEN_VIDEO_PROXY_H_

namespace webkit_glue {

class MediaMetadataAndroid;
class WebMediaPlayerAndroid;

// A proxy class to facilitate the IPC between the renderer side
// android::MediaPlayer and the fullscreen view at the browser process.
class FullscreenVideoProxyAndroid {
 public:
  virtual ~FullscreenVideoProxyAndroid() {}

  virtual void Notify(int msg, int ext1, int ext2) = 0;
  virtual void UpdateCurrentTime(float seconds) = 0;
  virtual void UpdateMetadata(MediaMetadataAndroid* metadata) = 0;

  virtual void EnterFullscreen(
      webkit_glue::WebMediaPlayerAndroid* web_media_player) = 0;
  virtual void LeaveFullscreen() = 0;

  virtual bool IsFullscreenMode() = 0;

  virtual void SetWebMediaPlayer(
      webkit_glue::WebMediaPlayerAndroid* web_media_player) = 0;
};

}  // namespace webkit_glue

#endif  // WEBKIT_GLUE_MEDIA_FULLSCREEN_VIDEO_PROXY_H_
