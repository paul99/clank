// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_MEDIA_WEBMEDIAPLAYER_ANDROID_H_
#define WEBKIT_MEDIA_WEBMEDIAPLAYER_ANDROID_H_

#include <jni.h>

#include "base/basictypes.h"

namespace webkit_glue {

class WebMediaPlayerAndroid {
 public:
  virtual ~WebMediaPlayerAndroid() {}

  // This function is called by the MediaPlayerManager to pause the video
  // and release the media player and surface texture when we switch tabs.
  // However, the actual GlTexture is not released to keep the video screenshot.
  virtual void ReleaseMediaResources() = 0;

  // Event handlers
  virtual void OnMediaPrepared(float duration) = 0;
  virtual void OnPlaybackComplete()  = 0;
  virtual void OnBufferingUpdate(int percentage)  = 0;
  virtual void OnVideoSizeChanged(int width, int height)  = 0;
  virtual void OnSurfaceReleased() {}
  virtual void OnSeekComplete(int msec)  = 0;
  virtual void OnMediaError(int error) = 0;
  virtual void OnMediaPlayerPlay() {}
  virtual void OnMediaPlayerPause() {}
  virtual void OnExitFullscreen() {}
  virtual void OnTimeUpdate(int msec) {}

  // Method to set the surface for video.
  virtual void SetVideoSurface(jobject j_surface) {}
};

}  // namespace webkit_glue

#endif  // WEBKIT_MEDIA_WEBMEDIAPLAYER_ANDROID_H_
