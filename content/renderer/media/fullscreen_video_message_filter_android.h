// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_FULLSCREEN_VIDEO_MESSAGE_FILTER_ANDROID_H_
#define CONTENT_RENDERER_MEDIA_FULLSCREEN_VIDEO_MESSAGE_FILTER_ANDROID_H_

#include "base/message_loop_proxy.h"
#include "ipc/ipc_channel_proxy.h"

namespace media {

class FullscreenVideoProxyImplAndroid;

class FullscreenVideoMessageFilterAndroid :
  public IPC::ChannelProxy::MessageFilter {
 public:
  FullscreenVideoMessageFilterAndroid(
      FullscreenVideoProxyImplAndroid* fullscreen_proxy);
  virtual ~FullscreenVideoMessageFilterAndroid();

 private:
  // Message handlers;
  void OnFullscreenVideoPlay();
  void OnFullscreenVideoSeek(float seconds);
  void OnFullscreenVideoPause();
  void OnExitFullscreen(bool release_media_player);

  // IPC::ChannelProxy::MessageFilter implementation:
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  FullscreenVideoProxyImplAndroid* fullscreen_proxy_;

  scoped_refptr<base::MessageLoopProxy> render_message_loop_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenVideoMessageFilterAndroid);
};

}  // namespace media

#endif  // CONTENT_RENDERER_MEDIA_FULLSCREEN_VIDEO_MESSAGE_FILTER_ANDROID_H_
