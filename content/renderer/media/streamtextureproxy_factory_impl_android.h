// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAMTEXTUREPROXY_IMPL_ANDROID_H_
#define CONTENT_RENDERER_MEDIA_STREAMTEXTUREPROXY_IMPL_ANDROID_H_

#include "base/memory/scoped_ptr.h"
#include "base/threading/non_thread_safe.h"
#include "content/renderer/gpu/stream_texture_host_android.h"
#include "webkit/media/streamtextureproxy_factory_android.h"
#include "webkit/media/webmediaplayer_android.h"

class GpuChannelHost;

namespace media {

class StreamTextureProxyFactoryImpl
    : public webkit_media::StreamTextureProxyFactory {
 public:
  virtual ~StreamTextureProxyFactoryImpl() OVERRIDE;

  // webkit_media::StreamTextureProxyFactory implementation:
  virtual WebKit::WebStreamTextureProxy* CreateProxy(
      int stream_id, int width, int height) OVERRIDE;

  virtual void ReestablishPeer(int stream_id) OVERRIDE;

 private:
  friend class StreamTextureProxyFactory;
  StreamTextureProxyFactoryImpl(
      GpuChannelHost* channel, int view_id, int player_id);

  scoped_refptr<GpuChannelHost> channel_;
  int view_id_;
  int player_id_;
  DISALLOW_COPY_AND_ASSIGN(StreamTextureProxyFactoryImpl);
};

}  // namespace media

#endif  // CONTENT_RENDERER_MEDIA_STREAMTEXTUREPROXY_IMPL_ANDROID_H_
