// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_GPU_STREAM_TEXTURE_HOST_ANDROID_H_
#define CONTENT_RENDERER_GPU_STREAM_TEXTURE_HOST_ANDROID_H_
#pragma once

#include "base/memory/weak_ptr.h"
#include "content/browser/android/surface_texture_peer.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_message.h"

namespace gfx {
class Size;
}

class GpuChannelHost;

struct GpuStreamTextureMsg_MatrixChanged_Params;

class StreamTextureHost : public IPC::Channel::Listener {
 public:
  StreamTextureHost(GpuChannelHost* channel);
  virtual ~StreamTextureHost();

  bool Initialize(int stream_id, const gfx::Size& initial_size);

  class Listener {
   public:
    virtual void OnFrameAvailable() = 0;
    virtual void OnMatrixChanged(const float mtx[16]) = 0;
    virtual ~Listener() {}
  };

  void SetListener(Listener* listener) { listener_ = listener; }

  void EstablishPeer(SurfaceTexturePeer::SurfaceTextureTarget type,
                     int32 primary_id, int32 secondary_id);

  // IPC::Channel::Listener implementation:
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;
  virtual void OnChannelError() OVERRIDE;

 private:
  // Message handlers:
  void OnFrameAvailable();
  void OnMatrixChanged(const GpuStreamTextureMsg_MatrixChanged_Params& param);

  int route_id_;
  int stream_id_;
  Listener* listener_;
  scoped_refptr<GpuChannelHost> channel_;
  base::WeakPtrFactory<StreamTextureHost> weak_ptr_factory_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(StreamTextureHost);
};

#endif  // CONTENT_RENDERER_GPU_STREAM_TEXTURE_HOST_ANDROID_H_
