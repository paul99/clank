// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_MEDIA_ANDROID_WEBMEDIAPLAYER_IN_PROCESS_PROXY_ANDROID_H_
#define WEBKIT_MEDIA_ANDROID_WEBMEDIAPLAYER_IN_PROCESS_PROXY_ANDROID_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "webkit/media/webmediaplayer_in_process_android.h"

namespace base {
class MessageLoopProxy;
}

namespace webkit_glue {

// Acts as a thread proxy between media::MediaPlayerBridge and
// WebMediaPlayerInProcessAndroid so that callbacks are posted onto
// the render thread.
class WebMediaPlayerInProcessProxyAndroid
    : public base::RefCountedThreadSafe<WebMediaPlayerInProcessProxyAndroid> {
 public:
  WebMediaPlayerInProcessProxyAndroid(
      const scoped_refptr<base::MessageLoopProxy>& render_loop,
      base::WeakPtr<WebMediaPlayerInProcessAndroid> webmediaplayer);

  // Callbacks from media::MediaPlayerBridge to WebMediaPlayerAndroid.
  void MediaErrorCallback(int player_id, int error_type);
  void VideoSizeChangedCallback(int player_id, int width, int height);
  void BufferingUpdateCallback(int player_id, int percent);
  void PlaybackCompleteCallback(int player_id);
  void SeekCompleteCallback(int player_id, int msec);
  void MediaPreparedCallback(int player_id, int duration);
  void TimeUpdateCallback(int player_id, int msec) {}

 private:
  friend class base::RefCountedThreadSafe<WebMediaPlayerInProcessProxyAndroid>;
  virtual ~WebMediaPlayerInProcessProxyAndroid();

  // The render message loop where WebKit lives.
  scoped_refptr<base::MessageLoopProxy> render_loop_;

  // The WebMediaPlayerAndroid object all the callbacks should be send to.
  base::WeakPtr<WebMediaPlayerInProcessAndroid> webmediaplayer_;

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerInProcessProxyAndroid);
};

}  // namespace webkit_glue

#endif  // WEBKIT_MEDIA_ANDROID_WEBMEDIAPLAYER_IN_PROCESS_PROXY_ANDROID_H_
