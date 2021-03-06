// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_GPU_GPU_CHANNEL_MANAGER_H_
#define CONTENT_COMMON_GPU_GPU_CHANNEL_MANAGER_H_
#pragma once

#include "base/hash_tables.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop_proxy.h"
#include "build/build_config.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_message.h"
#include "ui/gfx/native_widget_types.h"

namespace base {
class WaitableEvent;
}

namespace IPC {
struct ChannelHandle;
}

class ChildThread;
class GpuChannel;
class GpuWatchdog;
struct GPUCreateCommandBufferConfig;

#if defined(OS_ANDROID)
struct ANativeWindow;
#endif

// A GpuChannelManager is a thread responsible for issuing rendering commands
// managing the lifetimes of GPU channels and forwarding IPC requests from the
// browser process to them based on the corresponding renderer ID.
//
// A GpuChannelManager can also be hosted in the browser process in single
// process or in-process GPU modes. In this case there is no corresponding
// GpuChildThread and this is the reason the GpuChildThread is referenced via
// a pointer to IPC::Message::Sender, which can be implemented by other hosts
// to send IPC messages to the browser process IO thread on the
// GpuChannelManager's behalf.
class GpuChannelManager : public IPC::Channel::Listener,
                          public IPC::Message::Sender {
 public:
  GpuChannelManager(ChildThread* gpu_child_thread,
                    GpuWatchdog* watchdog,
                    base::MessageLoopProxy* io_message_loop,
                    base::WaitableEvent* shutdown_event);
  virtual ~GpuChannelManager();

  // Remove the channel for a particular renderer.
  void RemoveChannel(int client_id);

  // Listener overrides.
  virtual bool OnMessageReceived(const IPC::Message& msg) OVERRIDE;

  // Sender overrides.
  virtual bool Send(IPC::Message* msg) OVERRIDE;

  void LoseAllContexts();

  base::WeakPtrFactory<GpuChannelManager> weak_factory_;
#if defined(OS_ANDROID)
  void SetNativeWindow(int32 view_or_surface_id,
                       int32 renderer_id,
                       ANativeWindow* native_window);
#endif


  int GenerateRouteID();
  void AddRoute(int32 routing_id, IPC::Channel::Listener* listener);
  void RemoveRoute(int32 routing_id);

  GpuChannel* LookupChannel(int32 client_id);

 private:
  // Message handlers.
  void OnEstablishChannel(int client_id, int share_client_id);
  void OnCloseChannel(const IPC::ChannelHandle& channel_handle);
  void OnVisibilityChanged(
      int32 render_view_id, int32 client_id, bool visible);
  void OnCreateViewCommandBuffer(
      gfx::PluginWindowHandle window,
      int32 render_view_id,
      int32 client_id,
      const GPUCreateCommandBufferConfig& init_params);

  void OnLoseAllContexts();

  scoped_refptr<base::MessageLoopProxy> io_message_loop_;
  base::WaitableEvent* shutdown_event_;

  // Used to send and receive IPC messages from the browser process.
  ChildThread* gpu_child_thread_;

  // These objects manage channels to individual renderer processes there is
  // one channel for each renderer process that has connected to this GPU
  // process.
  typedef base::hash_map<int, scoped_refptr<GpuChannel> > GpuChannelMap;
  GpuChannelMap gpu_channels_;
  GpuWatchdog* watchdog_;

  DISALLOW_COPY_AND_ASSIGN(GpuChannelManager);
};

#endif  // CONTENT_COMMON_GPU_GPU_CHANNEL_MANAGER_H_
