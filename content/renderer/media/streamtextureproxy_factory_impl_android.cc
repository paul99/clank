// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/streamtextureproxy_factory_impl_android.h"

#include "content/browser/android/surface_texture_peer.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/renderer/gpu/gpu_channel_host.h"
#include "content/renderer/render_thread_impl.h"
#include "ui/gfx/size.h"

namespace {

class StreamTextureProxyImpl : public WebKit::WebStreamTextureProxy,
                               public base::NonThreadSafe {
 public:
  // WebKit::WebStreamTextureProxy implementation:
  virtual void setListener(Listener* listener);

 private:
  friend class media::StreamTextureProxyFactoryImpl;
  StreamTextureProxyImpl(StreamTextureHost* host);
  virtual ~StreamTextureProxyImpl();

  scoped_ptr<StreamTextureHost> host_;
  scoped_ptr<StreamTextureHost::Listener> listener_;
  DISALLOW_COPY_AND_ASSIGN(StreamTextureProxyImpl);
};

// StreamTextureHost::Listener -> StreamTextureProxyImplAndroid::Listener
class ListenerProxy : public StreamTextureHost::Listener {
public:
  explicit ListenerProxy(WebKit::WebStreamTextureProxy::Listener* listener)
      : listener_(listener) {
    DCHECK(listener);
  }
  ~ListenerProxy() {}

  // StreamTextureHost::Listener implementation:
  virtual void OnFrameAvailable() {
    listener_->onFrameAvailable();
  }

  virtual void OnMatrixChanged(const float mtx[16]) {
    listener_->onMatrixChanged(mtx);
  }

private:
  WebKit::WebStreamTextureProxy::Listener* listener_;
};

StreamTextureProxyImpl::StreamTextureProxyImpl(StreamTextureHost* host)
    : host_(host) {
  DCHECK(host);
}

StreamTextureProxyImpl::~StreamTextureProxyImpl() {
}

void StreamTextureProxyImpl::setListener(Listener* listener) {
  DCHECK(CalledOnValidThread());
  if (listener) {
    listener_.reset(new ListenerProxy(listener));
    host_->SetListener(listener_.get());
  } else {
    listener_.reset(NULL);
    host_->SetListener(NULL);
  }
}

} // anonymous namespace

namespace webkit_media {

// static
StreamTextureProxyFactory* StreamTextureProxyFactory::Create(
    int view_id, int player_id) {
   // This must be called from the main thread.
  RenderThreadImpl* render_thread = RenderThreadImpl::current();
  DCHECK(render_thread);
  GpuChannelHost* channel = render_thread->GetGpuChannel();

  if (channel) {
    return new media::StreamTextureProxyFactoryImpl(
        channel, view_id, player_id);
  }

  return NULL;
}

} // namespace webkit_media

namespace media {

StreamTextureProxyFactoryImpl::StreamTextureProxyFactoryImpl(
    GpuChannelHost* channel, int view_id, int player_id)
    : channel_(channel),
      view_id_(view_id),
      player_id_(player_id) {
    DCHECK(channel);
}

StreamTextureProxyFactoryImpl::~StreamTextureProxyFactoryImpl() {
}

WebKit::WebStreamTextureProxy* StreamTextureProxyFactoryImpl::CreateProxy(
    int stream_id, int width, int height) {
  DCHECK(channel_.get());
  StreamTextureHost* host = new StreamTextureHost(channel_.get());
  if (host->Initialize(stream_id, gfx::Size(width, height))) {
    return new StreamTextureProxyImpl(host);
  }

  return NULL;
}

void StreamTextureProxyFactoryImpl::ReestablishPeer(int stream_id) {
  DCHECK(channel_.get());
  channel_->Send(new GpuChannelMsg_EstablishStreamTexture(
      stream_id, SurfaceTexturePeer::SET_VIDEO_SURFACE_TEXTURE,
      view_id_, player_id_));
}

}  // namespace media
