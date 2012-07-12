// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(ENABLE_GPU)

#include "base/bind.h"
#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/common/gpu/image_transport_surface.h"
#include "ui/gfx/gl/gl_surface.h"
#include "ui/gfx/gl/native_window_interface_android.h"

namespace {
class PassThroughImageTransportSurfaceAndroid
    : public PassThroughImageTransportSurface {
 public:
  PassThroughImageTransportSurfaceAndroid(GpuChannelManager* manager,
                                          GpuCommandBufferStub* stub,
                                          gfx::GLSurface* surface);
  // GLSurface implementation.
  virtual bool SwapBuffers() OVERRIDE;
  virtual std::string GetExtensions() OVERRIDE;

  // PassThroughImageTransportSurface implementation.
  virtual void OnBuffersSwappedACK() OVERRIDE;
  virtual void OnResizeViewACK() OVERRIDE;
  virtual void OnResize(gfx::Size size) OVERRIDE;
  virtual bool DeferSwapBuffers() OVERRIDE;
  virtual void OnNewSurfaceACK(
      uint64 surface_handle, TransportDIB::Handle shm_handle) OVERRIDE;
  virtual void SetNativeWindow(gfx::NativeWindowInterface* window) OVERRIDE;

 private:
  bool swap_ack_pending_;
  bool have_native_window_;
  bool was_descheduled_from_next_frame_;
  DISALLOW_COPY_AND_ASSIGN(PassThroughImageTransportSurfaceAndroid);
};

PassThroughImageTransportSurfaceAndroid::PassThroughImageTransportSurfaceAndroid(
    GpuChannelManager* manager,
    GpuCommandBufferStub* stub,
    gfx::GLSurface* surface)
    : PassThroughImageTransportSurface(manager,
                                       stub,
                                       surface),
    swap_ack_pending_(false),
    have_native_window_(false),
    was_descheduled_from_next_frame_(false) {
}

bool PassThroughImageTransportSurfaceAndroid::SwapBuffers() {
  DCHECK(!swap_ack_pending_);
  swap_ack_pending_ = true;

  // Fake the ACK while using a pbuffer just to get a bit of throttling.
  if (!have_native_window_)
    MessageLoop::current()->PostDelayedTask(
        FROM_HERE, base::Bind(
            &PassThroughImageTransportSurfaceAndroid::OnBuffersSwappedACK,
            this),
        base::TimeDelta::FromMilliseconds(10));

  return gfx::GLSurfaceAdapter::SwapBuffers();
}

bool PassThroughImageTransportSurfaceAndroid::DeferSwapBuffers() {
  if (swap_ack_pending_) {
    TRACE_EVENT0("surface", "DeferSwapBuffers");
    was_descheduled_from_next_frame_ = true;
    Helper().SetScheduled(false);
    return true;
  }

  return false;
}

std::string PassThroughImageTransportSurfaceAndroid::GetExtensions() {
  std::string extensions = gfx::GLSurface::GetExtensions();
  extensions += extensions.empty() ? "" : " ";
  extensions += "GL_CHROMIUM_front_buffer_cached";
  return extensions;
}

void PassThroughImageTransportSurfaceAndroid::OnBuffersSwappedACK() {
  TRACE_EVENT0("surface", "SwapACK");
  if (was_descheduled_from_next_frame_) {
    DCHECK(swap_ack_pending_);
    Helper().SetScheduled(true);
    was_descheduled_from_next_frame_ = false;
  }
  swap_ack_pending_ = false;
}

void PassThroughImageTransportSurfaceAndroid::SetNativeWindow(
    gfx::NativeWindowInterface* window) {
  have_native_window_ = window->GetNativeHandle();
  if (!have_native_window_) {
    // When we destroy and detach a SurfaceTexture (hide a tab or navigate
    // away) the final frame might not get dequeued and acknowledged.
    if (was_descheduled_from_next_frame_) {
      DCHECK(swap_ack_pending_);
      Helper().SetScheduled(true);
      was_descheduled_from_next_frame_ = false;
    }
    swap_ack_pending_ = false;
  }

  PassThroughImageTransportSurface::SetNativeWindow(window);
}

void PassThroughImageTransportSurfaceAndroid::OnResizeViewACK() {
}

void PassThroughImageTransportSurfaceAndroid::OnNewSurfaceACK(
    uint64 surface_handle, TransportDIB::Handle shm_handle) {
  Helper().SetScheduled(true);
}

void PassThroughImageTransportSurfaceAndroid::OnResize(gfx::Size size) {
  // On Android, resizing is driven from the UI, unlike on other platforms.
  // Therefore the window has already been resized, and we mainly resize
  // the existing surface here to not produce an invalid frame.
  // For the same reason, we don't need to do an ACK roundtrip to the browser.
  // We simply notify the UI of the new dimensions and timestamp of the first
  // frame after resizing, so it knows when valid frames are being received.
  GLSurfaceAdapter::Resize(size);
  PassThroughImageTransportSurface::OnResize(size);
  PassThroughImageTransportSurface::OnResizeViewACK();
}

} // anonymous namespace

// static
scoped_refptr<gfx::GLSurface> ImageTransportSurface::CreateSurface(
    GpuChannelManager* manager,
    GpuCommandBufferStub* stub,
    gfx::PluginWindowHandle handle) {
  scoped_refptr<gfx::GLSurface> surface;
  surface = gfx::GLSurface::CreateViewGLSurface(false, handle);
  if (!surface.get()) {
    LOG(ERROR) << "Failed to create view GL surface.";
    return NULL;
  }

  scoped_refptr<PassThroughImageTransportSurfaceAndroid> transport_surface(
      new PassThroughImageTransportSurfaceAndroid(
          manager, stub, surface.get()));
  if (!transport_surface->Initialize()) {
    LOG(ERROR) << "Failed to initialize image transport surface.";
    return NULL;
  }

  GpuHostMsg_AcceleratedSurfaceNew_Params params;
  transport_surface->Helper().SendAcceleratedSurfaceNew(params);
  transport_surface->Helper().SetScheduled(false);

  return transport_surface;
}

#endif  // defined(ENABLE_GPU)
