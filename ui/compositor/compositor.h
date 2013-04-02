// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_COMPOSITOR_H_
#define UI_COMPOSITOR_COMPOSITOR_H_

#include "base/hash_tables.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/observer_list.h"
#include "cc/layer_tree_host_client.h"
#include "ui/compositor/compositor_export.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/size.h"
#include "ui/gfx/transform.h"
#include "ui/gl/gl_share_group.h"

class SkBitmap;

namespace cc {
class FontAtlas;
class Layer;
class LayerTreeHost;
}

namespace gfx {
class GLContext;
class GLSurface;
class GLShareGroup;
class Point;
class Rect;
}

namespace WebKit {
class WebGraphicsContext3D;
}

namespace ui {

class Compositor;
class CompositorObserver;
class Layer;
class PostedSwapQueue;

// This class abstracts the creation of the 3D context for the compositor. It is
// a global object.
class COMPOSITOR_EXPORT ContextFactory {
 public:
  virtual ~ContextFactory() {}

  // Gets the global instance.
  static ContextFactory* GetInstance();

  // Sets the global instance. Caller keeps ownership.
  // If this function isn't called (for tests), a "default" factory will be
  // created on the first call of GetInstance.
  static void SetInstance(ContextFactory* instance);

  // Creates an output surface for the given compositor. The factory may keep
  // per-compositor data (e.g. a shared context), that needs to be cleaned up
  // by calling RemoveCompositor when the compositor gets destroyed.
  virtual cc::OutputSurface* CreateOutputSurface(
      Compositor* compositor) = 0;

  // Creates a context used for offscreen rendering. This context can be shared
  // with all compositors.
  virtual WebKit::WebGraphicsContext3D* CreateOffscreenContext() = 0;

  // Destroys per-compositor data.
  virtual void RemoveCompositor(Compositor* compositor) = 0;
};

// The default factory that creates in-process contexts.
class COMPOSITOR_EXPORT DefaultContextFactory : public ContextFactory {
 public:
  DefaultContextFactory();
  virtual ~DefaultContextFactory();

  // ContextFactory implementation
  virtual cc::OutputSurface* CreateOutputSurface(
      Compositor* compositor) OVERRIDE;
  virtual WebKit::WebGraphicsContext3D* CreateOffscreenContext() OVERRIDE;
  virtual void RemoveCompositor(Compositor* compositor) OVERRIDE;

  bool Initialize();

  void set_share_group(gfx::GLShareGroup* share_group) {
    share_group_ = share_group;
  }

 private:
  WebKit::WebGraphicsContext3D* CreateContextCommon(
      Compositor* compositor,
      bool offscreen);

  scoped_refptr<gfx::GLShareGroup> share_group_;

  DISALLOW_COPY_AND_ASSIGN(DefaultContextFactory);
};

// Texture provide an abstraction over the external texture that can be passed
// to a layer.
class COMPOSITOR_EXPORT Texture : public base::RefCounted<Texture> {
 public:
  Texture(bool flipped, const gfx::Size& size, float device_scale_factor);

  bool flipped() const { return flipped_; }
  gfx::Size size() const { return size_; }
  float device_scale_factor() const { return device_scale_factor_; }

  virtual unsigned int PrepareTexture() = 0;
  virtual WebKit::WebGraphicsContext3D* HostContext3D() = 0;

  // Replaces the texture with the texture from the specified mailbox.
  virtual void Consume(const std::string& mailbox_name,
                       const gfx::Size& new_size) {}

  // Moves the texture into the mailbox and returns the mailbox name.
  // The texture must have been previously consumed from a mailbox.
  virtual std::string Produce();

 protected:
  virtual ~Texture();
  gfx::Size size_;  // in pixel

 private:
  friend class base::RefCounted<Texture>;

  bool flipped_;
  float device_scale_factor_;

  DISALLOW_COPY_AND_ASSIGN(Texture);
};

// An interface to allow the compositor to communicate with its owner.
class COMPOSITOR_EXPORT CompositorDelegate {
 public:
  // Requests the owner to schedule a redraw of the layer tree.
  virtual void ScheduleDraw() = 0;

 protected:
  virtual ~CompositorDelegate() {}
};

// This class represents a lock on the compositor, that can be used to prevent
// commits to the compositor tree while we're waiting for an asynchronous
// event. The typical use case is when waiting for a renderer to produce a frame
// at the right size. The caller keeps a reference on this object, and drops the
// reference once it desires to release the lock.
// Note however that the lock is cancelled after a short timeout to ensure
// responsiveness of the UI, so the compositor tree should be kept in a
// "reasonable" state while the lock is held.
// Don't instantiate this class directly, use Compositor::GetCompositorLock.
class COMPOSITOR_EXPORT CompositorLock
    : public base::RefCounted<CompositorLock>,
      public base::SupportsWeakPtr<CompositorLock> {
 private:
  friend class base::RefCounted<CompositorLock>;
  friend class Compositor;

  explicit CompositorLock(Compositor* compositor);
  ~CompositorLock();

  void CancelLock();

  Compositor* compositor_;
  DISALLOW_COPY_AND_ASSIGN(CompositorLock);
};


// Compositor object to take care of GPU painting.
// A Browser compositor object is responsible for generating the final
// displayable form of pixels comprising a single widget's contents. It draws an
// appropriately transformed texture for each transformed view in the widget's
// view hierarchy.
class COMPOSITOR_EXPORT Compositor
    : NON_EXPORTED_BASE(public cc::LayerTreeHostClient) {
 public:
  Compositor(CompositorDelegate* delegate,
             gfx::AcceleratedWidget widget);
  virtual ~Compositor();

  static void Initialize(bool useThread);
  static void Terminate();

  // Schedules a redraw of the layer tree associated with this compositor.
  void ScheduleDraw();

  // Sets the root of the layer tree drawn by this Compositor. The root layer
  // must have no parent. The compositor's root layer is reset if the root layer
  // is destroyed. NULL can be passed to reset the root layer, in which case the
  // compositor will stop drawing anything.
  // The Compositor does not own the root layer.
  const Layer* root_layer() const { return root_layer_; }
  Layer* root_layer() { return root_layer_; }
  void SetRootLayer(Layer* root_layer);

  // Called when we need the compositor to preserve the alpha channel in the
  // output for situations when we want to render transparently atop something
  // else, e.g. Aero glass.
  void SetHostHasTransparentBackground(bool host_has_transparent_background);

  // The scale factor of the device that this compositor is
  // compositing layers on.
  float device_scale_factor() const { return device_scale_factor_; }

  // Draws the scene created by the layer tree and any visual effects. If
  // |force_clear| is true, this will cause the compositor to clear before
  // compositing.
  void Draw(bool force_clear);

  // Where possible, draws are scissored to a damage region calculated from
  // changes to layer properties.  This bypasses that and indicates that
  // the whole frame needs to be drawn.
  void ScheduleFullDraw();

  // Reads the region |bounds_in_pixel| of the contents of the last rendered
  // frame into the given bitmap.
  // Returns false if the pixels could not be read.
  bool ReadPixels(SkBitmap* bitmap, const gfx::Rect& bounds_in_pixel);

  // Sets the compositor's device scale factor and size.
  void SetScaleAndSize(float scale, const gfx::Size& size_in_pixel);

  // Returns the size of the widget that is being drawn to in pixel coordinates.
  const gfx::Size& size() const { return size_; }

  // Returns the widget for this compositor.
  gfx::AcceleratedWidget widget() const { return widget_; }

  // Compositor does not own observers. It is the responsibility of the
  // observer to remove itself when it is done observing.
  void AddObserver(CompositorObserver* observer);
  void RemoveObserver(CompositorObserver* observer);
  bool HasObserver(CompositorObserver* observer);

  // Creates a compositor lock. Returns NULL if it is not possible to lock at
  // this time (i.e. we're waiting to complete a previous unlock).
  scoped_refptr<CompositorLock> GetCompositorLock();

  // Internal functions, called back by command-buffer contexts on swap buffer
  // events.

  // Signals swap has been posted.
  void OnSwapBuffersPosted();

  // Signals swap has completed.
  void OnSwapBuffersComplete();

  // Signals swap has aborted (e.g. lost context).
  void OnSwapBuffersAborted();

  // LayerTreeHostClient implementation.
  virtual void willBeginFrame() OVERRIDE;
  virtual void didBeginFrame() OVERRIDE;
  virtual void animate(double frameBeginTime) OVERRIDE;
  virtual void layout() OVERRIDE;
  virtual void applyScrollAndScale(gfx::Vector2d scrollDelta,
                                   float pageScale) OVERRIDE;
  virtual scoped_ptr<cc::OutputSurface>
      createOutputSurface() OVERRIDE;
  virtual void didRecreateOutputSurface(bool success) OVERRIDE;
  virtual scoped_ptr<cc::InputHandler> createInputHandler() OVERRIDE;
  virtual void willCommit() OVERRIDE;
  virtual void didCommit() OVERRIDE;
  virtual void didCommitAndDrawFrame() OVERRIDE;
  virtual void didCompleteSwapBuffers() OVERRIDE;
  virtual void scheduleComposite() OVERRIDE;
  virtual scoped_ptr<cc::FontAtlas> createFontAtlas() OVERRIDE;


  int last_started_frame() { return last_started_frame_; }
  int last_ended_frame() { return last_ended_frame_; }

  bool IsLocked() { return compositor_lock_ != NULL; }

 private:
  friend class base::RefCounted<Compositor>;
  friend class CompositorLock;

  // Called by CompositorLock.
  void UnlockCompositor();

  // Called to release any pending CompositorLock
  void CancelCompositorLock();

  // Notifies the compositor that compositing is complete.
  void NotifyEnd();

  CompositorDelegate* delegate_;
  gfx::Size size_;

  // The root of the Layer tree drawn by this compositor.
  Layer* root_layer_;

  ObserverList<CompositorObserver> observer_list_;

  gfx::AcceleratedWidget widget_;
  scoped_refptr<cc::Layer> root_web_layer_;
  scoped_ptr<cc::LayerTreeHost> host_;

  // Used to verify that we have at most one draw swap in flight.
  scoped_ptr<PostedSwapQueue> posted_swaps_;

  // The device scale factor of the monitor that this compositor is compositing
  // layers on.
  float device_scale_factor_;

  int last_started_frame_;
  int last_ended_frame_;

  bool disable_schedule_composite_;

  CompositorLock* compositor_lock_;

  DISALLOW_COPY_AND_ASSIGN(Compositor);
};

}  // namespace ui

#endif  // UI_COMPOSITOR_COMPOSITOR_H_
