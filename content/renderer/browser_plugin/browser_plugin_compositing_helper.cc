// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/browser_plugin/browser_plugin_compositing_helper.h"

#include "cc/solid_color_layer.h"
#include "cc/texture_layer.h"
#include "content/common/browser_plugin_messages.h"
#include "content/renderer/browser_plugin/browser_plugin_manager.h"
#include "content/renderer/render_thread_impl.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebGraphicsContext3D.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebSharedGraphicsContext3D.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebPluginContainer.h"
#include "webkit/compositor_bindings/web_layer_impl.h"

namespace content {

BrowserPluginCompositingHelper::BrowserPluginCompositingHelper(
    WebKit::WebPluginContainer* container,
    BrowserPluginManager* manager,
    int instance_id,
    int host_routing_id)
    : instance_id_(instance_id),
      host_routing_id_(host_routing_id),
      last_gpu_route_id_(0),
      last_gpu_host_id_(0),
      last_mailbox_valid_(false),
      ack_pending_(true),
      ack_pending_for_crashed_guest_(false),
      container_(container),
      browser_plugin_manager_(manager) {
}

BrowserPluginCompositingHelper::~BrowserPluginCompositingHelper() {
}

void BrowserPluginCompositingHelper::EnableCompositing(bool enable) {
  if (enable && !texture_layer_) {
    texture_layer_ = cc::TextureLayer::createForMailbox();
    texture_layer_->setIsDrawable(true);
    texture_layer_->setContentsOpaque(true);

    background_layer_ = cc::SolidColorLayer::create();
    background_layer_->setMasksToBounds(true);
    background_layer_->setBackgroundColor(
        SkColorSetARGBInline(255, 255, 255, 255));
    background_layer_->addChild(texture_layer_);
    web_layer_.reset(new WebKit::WebLayerImpl(background_layer_));
  }

  container_->setWebLayer(enable ? web_layer_.get() : NULL);
}

// If we have a mailbox that was freed up from the compositor,
// but we are not expected to return it to the guest renderer
// via an ACK, we should free it because we now own it.
// To free the mailbox memory, we need a context to consume it
// into a texture ID and then delete this texture ID.
// We use a shared graphics context accessible from the main
// thread to do it.
void BrowserPluginCompositingHelper::FreeMailboxMemory(
    const std::string& mailbox_name,
    unsigned sync_point) {
  if (mailbox_name.empty())
    return;

  WebKit::WebGraphicsContext3D *context =
     WebKit::WebSharedGraphicsContext3D::mainThreadContext();
  DCHECK(context);
  // When a buffer is released from the compositor, we also get a
  // sync point that specifies when in the command buffer
  // it's safe to use it again.
  // If the sync point is non-zero, we need to tell our context
  // to wait until this sync point is reached before we can safely
  // delete the buffer.
  if (sync_point)
    context->waitSyncPoint(sync_point);

  unsigned texture_id = context->createTexture();
  context->bindTexture(GL_TEXTURE_2D, texture_id);
  context->consumeTextureCHROMIUM(
      GL_TEXTURE_2D,
      reinterpret_cast<const int8*>(mailbox_name.data()));
  context->deleteTexture(texture_id);
}

void BrowserPluginCompositingHelper::MailboxReleased(
    const std::string& mailbox_name,
    int gpu_route_id,
    int gpu_host_id,
    unsigned sync_point) {
  // This means the GPU process crashed and we have nothing further to do.
  // Nobody is expecting an ACK and the buffer doesn't need to be deleted
  // because it went away with the GPU process.
  if (last_gpu_host_id_ != gpu_host_id)
    return;

  // This means the guest crashed.
  // Either ACK the last buffer, so texture transport could
  // be destroyed of delete the mailbox if nobody wants it back.
  if (last_gpu_route_id_ != gpu_route_id) {
    if (!ack_pending_for_crashed_guest_) {
      FreeMailboxMemory(mailbox_name, sync_point);
    } else {
      ack_pending_for_crashed_guest_ = false;
      browser_plugin_manager_->Send(
          new BrowserPluginHostMsg_BuffersSwappedACK(
              host_routing_id_,
              instance_id_,
              gpu_route_id,
              gpu_host_id,
              mailbox_name,
              sync_point));
    }
    return;
  }

  // We need to send an ACK to TextureImageTransportSurface
  // for every buffer it sends us. However, if a buffer is freed up from
  // the compositor in cases like switching back to SW mode without a new
  // buffer arriving, no ACK is needed and we destroy this buffer.
  if (!ack_pending_) {
    FreeMailboxMemory(mailbox_name, sync_point);
    last_mailbox_valid_ = false;
    return;
  }
  ack_pending_ = false;
  browser_plugin_manager_->Send(
      new BrowserPluginHostMsg_BuffersSwappedACK(
          host_routing_id_,
          instance_id_,
          gpu_route_id,
          gpu_host_id,
          mailbox_name,
          sync_point));
}

void BrowserPluginCompositingHelper::OnContainerDestroy() {
  if (container_)
    container_->setWebLayer(NULL);
  container_ = NULL;

  texture_layer_ = NULL;
  background_layer_ = NULL;
  web_layer_.reset();
}

void BrowserPluginCompositingHelper::OnBuffersSwapped(
    const gfx::Size& size,
    const std::string& mailbox_name,
    int gpu_route_id,
    int gpu_host_id) {
  // If the guest crashed but the GPU process didn't, we may still have
  // a transport surface waiting on an ACK, which we must send to
  // avoid leaking.
  if (last_gpu_route_id_ != gpu_route_id && last_gpu_host_id_ == gpu_host_id)
    ack_pending_for_crashed_guest_ = ack_pending_;

  // If these mismatch, we are either just starting up, GPU process crashed or
  // guest renderer crashed.
  // In this case, we are communicating with a new image transport
  // surface and must ACK with the new ID's and an empty mailbox.
  if (last_gpu_route_id_ != gpu_route_id || last_gpu_host_id_ != gpu_host_id)
    last_mailbox_valid_ = false;

  last_gpu_route_id_ = gpu_route_id;
  last_gpu_host_id_ = gpu_host_id;

  ack_pending_ = true;
  // Browser plugin getting destroyed, do a fast ACK.
  if (!texture_layer_) {
    MailboxReleased(mailbox_name, gpu_route_id, gpu_host_id, 0);
    return;
  }

  // The size of browser plugin container is not always equal to the size
  // of the buffer that arrives here. This could be for a number of reasons,
  // including autosize and a resize in progress.
  // During resize, the container size changes first and then some time
  // later, a new buffer with updated size will arrive. During this process,
  // we need to make sure that things are still displayed pixel perfect.
  // We accomplish this by modifying bounds of the texture layer only
  // when a new buffer arrives.
  // Visually, this will either display a smaller part of the buffer
  // or introduce a gutter around it.
  if (buffer_size_ != size) {
    buffer_size_ = size;
    texture_layer_->setBounds(buffer_size_);
  }

  bool current_mailbox_valid = !mailbox_name.empty();
  if (!last_mailbox_valid_) {
    MailboxReleased(std::string(), gpu_route_id, gpu_host_id, 0);
    if (!current_mailbox_valid)
      return;
  }

  cc::TextureMailbox::ReleaseCallback callback;
  if (current_mailbox_valid) {
    callback = base::Bind(&BrowserPluginCompositingHelper::MailboxReleased,
                          scoped_refptr<BrowserPluginCompositingHelper>(this),
                          mailbox_name,
                          gpu_route_id,
                          gpu_host_id);
  }
  texture_layer_->setTextureMailbox(cc::TextureMailbox(mailbox_name,
                                                       callback));
  last_mailbox_valid_ = current_mailbox_valid;
}

}  // namespace content
