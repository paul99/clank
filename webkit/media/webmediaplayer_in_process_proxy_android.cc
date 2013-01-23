// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/media/webmediaplayer_in_process_proxy_android.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/message_loop_proxy.h"
#include "webkit/media/webmediaplayer_in_process_android.h"

namespace webkit_glue {

WebMediaPlayerInProcessProxyAndroid::WebMediaPlayerInProcessProxyAndroid(
    const scoped_refptr<base::MessageLoopProxy>& render_loop,
    base::WeakPtr<WebMediaPlayerInProcessAndroid> webmediaplayer)
    : render_loop_(render_loop),
      webmediaplayer_(webmediaplayer) {
  DCHECK(render_loop_);
  DCHECK(webmediaplayer_);
}

WebMediaPlayerInProcessProxyAndroid::~WebMediaPlayerInProcessProxyAndroid() {
}

void WebMediaPlayerInProcessProxyAndroid::MediaErrorCallback(
    int player_id, int error_type) {
  render_loop_->PostTask(FROM_HERE, base::Bind(
      &WebMediaPlayerInProcessAndroid::OnMediaError, webmediaplayer_,
      error_type));
}

void WebMediaPlayerInProcessProxyAndroid::VideoSizeChangedCallback(
    int player_id, int width, int height) {
  render_loop_->PostTask(FROM_HERE, base::Bind(
      &WebMediaPlayerInProcessAndroid::OnVideoSizeChanged, webmediaplayer_,
      width, height));
}

void WebMediaPlayerInProcessProxyAndroid::BufferingUpdateCallback(
    int player_id, int percent) {
  render_loop_->PostTask(FROM_HERE, base::Bind(
      &WebMediaPlayerInProcessAndroid::OnBufferingUpdate, webmediaplayer_,
      percent));
}

void WebMediaPlayerInProcessProxyAndroid::PlaybackCompleteCallback(
    int player_id) {
  render_loop_->PostTask(FROM_HERE, base::Bind(
      &WebMediaPlayerInProcessAndroid::OnPlaybackComplete, webmediaplayer_));
}

void WebMediaPlayerInProcessProxyAndroid::SeekCompleteCallback(
    int player_id, int msec) {
  render_loop_->PostTask(FROM_HERE, base::Bind(
      &WebMediaPlayerInProcessAndroid::OnSeekComplete, webmediaplayer_, msec));
}

void WebMediaPlayerInProcessProxyAndroid::MediaPreparedCallback(
    int player_id, int duration) {
  render_loop_->PostTask(FROM_HERE, base::Bind(
      &WebMediaPlayerInProcessAndroid::OnMediaPrepared,
      webmediaplayer_, static_cast<float>(duration / 1000.0)));
}

}  // namespace webkit_glue
