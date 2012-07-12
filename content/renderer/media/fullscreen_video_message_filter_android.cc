// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// MessageFilter that handles fullscreen video messages and callbacks
// FullscreenVideoProxyImplAndroid. This class is created on render thread.
// It receives incoming messages on the IO thread and post a task back to
// the render thread.

#include "content/renderer/media/fullscreen_video_message_filter_android.h"

#include "base/bind.h"
#include "base/message_loop.h"
#include "content/common/view_messages.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/media/fullscreen_video_proxy_impl_android.h"
namespace media {

FullscreenVideoMessageFilterAndroid::FullscreenVideoMessageFilterAndroid(
    FullscreenVideoProxyImplAndroid* fullscreen_proxy)
    : fullscreen_proxy_(fullscreen_proxy),
    render_message_loop_(base::MessageLoopProxy::current()) {
}

FullscreenVideoMessageFilterAndroid::~FullscreenVideoMessageFilterAndroid() {
}

bool FullscreenVideoMessageFilterAndroid::OnMessageReceived(
    const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(FullscreenVideoMessageFilterAndroid, msg)
    IPC_MESSAGE_HANDLER(ViewMsg_FullscreenVideoPlay,
                        OnFullscreenVideoPlay)
    IPC_MESSAGE_HANDLER(ViewMsg_FullscreenVideoSeek,
                        OnFullscreenVideoSeek)
    IPC_MESSAGE_HANDLER(ViewMsg_FullscreenVideoPause,
                        OnFullscreenVideoPause)
    IPC_MESSAGE_HANDLER(ViewMsg_ExitFullscreen,
                        OnExitFullscreen)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void FullscreenVideoMessageFilterAndroid::OnFullscreenVideoPlay() {
  if (!render_message_loop_->BelongsToCurrentThread()) {
    render_message_loop_->PostTask(
        FROM_HERE,
        base::Bind(
            &FullscreenVideoMessageFilterAndroid::OnFullscreenVideoPlay,
            this));
    return;
  }
  if (fullscreen_proxy_) {
    fullscreen_proxy_->OnFullscreenVideoPlay();
  }
}

void FullscreenVideoMessageFilterAndroid::OnFullscreenVideoSeek(float seconds) {
  if (!render_message_loop_->BelongsToCurrentThread()) {
    render_message_loop_->PostTask(
        FROM_HERE,
        base::Bind(
            &FullscreenVideoMessageFilterAndroid::OnFullscreenVideoSeek,
            this,
            seconds));
    return;
  }
  if (fullscreen_proxy_) {
    fullscreen_proxy_->OnFullscreenVideoSeek(seconds);
  }
}

void FullscreenVideoMessageFilterAndroid::OnFullscreenVideoPause() {
  if (!render_message_loop_->BelongsToCurrentThread()) {
    render_message_loop_->PostTask(
        FROM_HERE,
        base::Bind(
            &FullscreenVideoMessageFilterAndroid::OnFullscreenVideoPause,
            this));
    return;
  }
  if (fullscreen_proxy_) {
    fullscreen_proxy_->OnFullscreenVideoPause();
  }
}

void FullscreenVideoMessageFilterAndroid::OnExitFullscreen(
    bool release_media_player) {
  if (!render_message_loop_->BelongsToCurrentThread()) {
    render_message_loop_->PostTask(
        FROM_HERE,
        base::Bind(
            &FullscreenVideoMessageFilterAndroid::OnExitFullscreen,
            this,
            release_media_player));
    return;
  }
  if (fullscreen_proxy_) {
    fullscreen_proxy_->OnExitFullscreen(release_media_player);
  }
}

}  // namespace media
