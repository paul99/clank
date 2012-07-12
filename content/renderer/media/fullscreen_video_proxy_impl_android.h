// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_FULLSCREEN_VIDEO_PROXY_IMPL_ANDROID_H_
#define CONTENT_RENDERER_MEDIA_FULLSCREEN_VIDEO_PROXY_IMPL_ANDROID_H_

#include "ipc/ipc_channel_proxy.h"
#include "webkit/media/fullscreen_video_proxy_android.h"

namespace media {

class FullscreenVideoProxyImplAndroid :
  public webkit_glue::FullscreenVideoProxyAndroid {
 public:
  FullscreenVideoProxyImplAndroid(IPC::Message::Sender* sender,
                                  int routing_id);
  virtual ~FullscreenVideoProxyImplAndroid();

  // Send the mediaplayer notification message to the browser process
  virtual void Notify(int msg, int ext1, int ext2) OVERRIDE;

  // Update the current time in the browser process
  virtual void UpdateCurrentTime(float seconds) OVERRIDE;

  // Update the metadata in the browser process
  virtual void UpdateMetadata(
      webkit_glue::MediaMetadataAndroid* metadata) OVERRIDE;

  // Request to enter fullscreen mode.
  virtual void EnterFullscreen(
      webkit_glue::WebMediaPlayerAndroid* web_media_player) OVERRIDE;
  // Leave the fullscreen mode.
  virtual void LeaveFullscreen() OVERRIDE;

  virtual void SetWebMediaPlayer(
      webkit_glue::WebMediaPlayerAndroid* web_media_player) OVERRIDE;

  virtual bool IsFullscreenMode() OVERRIDE;

  // Callbacks from the message filter
  virtual void OnFullscreenVideoPlay();
  virtual void OnFullscreenVideoSeek(float seconds);
  virtual void OnFullscreenVideoPause();
  virtual void OnExitFullscreen(bool release_media_player);

 private:

  // Helper method for sending IPC messages to the browser process
  bool Send(IPC::Message* message);
  IPC::Message::Sender* sender_;
  int routing_id_;

  webkit_glue::WebMediaPlayerAndroid* web_media_player_;

  scoped_refptr<IPC::ChannelProxy::MessageFilter> message_filter_;

  // Whether we are currently in fullscreen mode.
  bool is_fullscreen_mode_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenVideoProxyImplAndroid);
};

}  // namespace media

#endif  // CONTENT_RENDERER_MEDIA_FULLSCREEN_VIDEO_PROXY_IMPL_ANDROID_H_
