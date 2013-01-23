// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/java/java_bridge_channel_host.h"

#include "base/atomicops.h"
#include "base/lazy_instance.h"
#include "base/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "content/common/java_bridge_messages.h"

// Extras for monitoring render process host termination.
#include "content/public/browser/browser_thread.h"
#include "base/bind.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"

using base::WaitableEvent;

namespace {
struct WaitableEventLazyInstanceTraits
    : public base::DefaultLazyInstanceTraits<WaitableEvent> {
  static WaitableEvent* New(void* instance) {
    // Use placement new to initialize our instance in our preallocated space.
    // The parenthesis is very important here to force POD type initialization.
    return new (instance) WaitableEvent(false, false);
  }
};
base::LazyInstance<WaitableEvent, WaitableEventLazyInstanceTraits> dummy_event =
    LAZY_INSTANCE_INITIALIZER;

base::subtle::AtomicWord g_last_id = 0;
}

JavaBridgeChannelHost* JavaBridgeChannelHost::GetJavaBridgeChannelHost(
    int renderer_id,
    base::MessageLoopProxy* ipc_message_loop) {
  std::string channel_name(StringPrintf("r%d.javabridge", renderer_id));
  // There's no need for a shutdown event here. If the browser is terminated
  // while the JavaBridgeChannelHost is blocked on a synchronous IPC call, the
  // renderer's shutdown event will cause the underlying channel to shut down,
  // thus terminating the IPC call.
  return static_cast<JavaBridgeChannelHost*>(NPChannelBase::GetChannel(
      channel_name,
      IPC::Channel::MODE_SERVER,
      ClassFactory,
      ipc_message_loop,
      true,
      dummy_event.Pointer()));
}

JavaBridgeChannelHost::JavaBridgeChannelHost() {
#if defined(OS_POSIX)
  if (content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    RegisterForNotification();
  } else {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::Bind(&JavaBridgeChannelHost::RegisterForNotification,
                   base::Unretained(this)));
  }
#endif
}

JavaBridgeChannelHost::~JavaBridgeChannelHost() {
#if defined(OS_POSIX)
  if (channel_handle_.socket.fd > 0)
    close(channel_handle_.socket.fd);
#endif
}

void JavaBridgeChannelHost::RegisterForNotification() {
  // TODO: We don't need any of these if ~JavaBridgeChannelHost was
  // called correctly. This is a hack to make sure we actually close all the FDs.

  // TODO: We should only need to listen to one event that triggers when a 
  // RenderProccess is terminated. However, we were unable to confirm the 
  // correctness of either of them so we are going to listen to
  // both to be on the safe side.
  registrar_.Add(this, content::NOTIFICATION_RENDERER_PROCESS_TERMINATED,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this, content::NOTIFICATION_RENDERER_PROCESS_CLOSED,
                 content::NotificationService::AllBrowserContextsAndSources());
}

void JavaBridgeChannelHost::Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) {
#if defined(OS_POSIX)
  content::RenderProcessHost* rph =
      content::Source<content::RenderProcessHost>(source).ptr();
  std::string channel_name(StringPrintf("r%d.javabridge", rph->GetID()));
  if (!this->channel_handle_.name.compare(0, channel_name.size(), channel_name)) {
    if (channel_handle_.socket.fd > 0) {
      close(channel_handle_.socket.fd);
      channel_handle_.socket.fd = -1;
    }
    registrar_.RemoveAll();
  }
#endif
}

int JavaBridgeChannelHost::ThreadsafeGenerateRouteID() {
  return base::subtle::NoBarrier_AtomicIncrement(&g_last_id, 1);
}

int JavaBridgeChannelHost::GenerateRouteID() {
  return ThreadsafeGenerateRouteID();
}

bool JavaBridgeChannelHost::Init(base::MessageLoopProxy* ipc_message_loop,
                                 bool create_pipe_now,
                                 WaitableEvent* shutdown_event) {
  if (!NPChannelBase::Init(ipc_message_loop, create_pipe_now, shutdown_event)) {
    return false;
  }

  // Finish populating our ChannelHandle.
#if defined(OS_POSIX)
  // ipc channel's client file descriptor will be closed after it starts to get
  // incoming message, see Channel::ChannelImpl::ProcessIncomingMessages().
  // Keep a copy as it is needed by JavaBridgeDispatcherHost later.
  if (channel_->GetClientFileDescriptor() > 0) {
    channel_handle_.socket.fd = dup(channel_->GetClientFileDescriptor());
  }
#endif
  return true;
}

bool JavaBridgeChannelHost::OnControlMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(JavaBridgeChannelHost, message)
    IPC_MESSAGE_HANDLER(JavaBridgeMsg_GenerateRouteID, OnGenerateRouteID)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void JavaBridgeChannelHost::OnGenerateRouteID(int* route_id) {
  *route_id = GenerateRouteID();
}
