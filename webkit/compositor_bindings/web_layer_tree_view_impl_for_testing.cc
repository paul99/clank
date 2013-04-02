// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/compositor_bindings/web_layer_tree_view_impl_for_testing.h"

#include "base/command_line.h"
#include "base/string_number_conversions.h"
#include "cc/fake_web_graphics_context_3d.h"
#include "cc/font_atlas.h"
#include "cc/input_handler.h"
#include "cc/layer.h"
#include "cc/layer_tree_host.h"
#include "cc/switches.h"
#include "cc/thread.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebGraphicsContext3D.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebInputHandler.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebLayer.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebLayerTreeView.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebLayerTreeViewClient.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebRenderingStats.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebSize.h"
#include "webkit/compositor_bindings/web_compositor_support_output_surface.h"
#include "webkit/compositor_bindings/web_layer_impl.h"
#include "webkit/compositor_bindings/web_rendering_stats_impl.h"
#include "webkit/compositor_bindings/web_to_ccinput_handler_adapter.h"

namespace WebKit {

WebLayerTreeViewImplForTesting::WebLayerTreeViewImplForTesting() {}

WebLayerTreeViewImplForTesting::~WebLayerTreeViewImplForTesting() {}

bool WebLayerTreeViewImplForTesting::initialize() {
  layer_tree_host_ = cc::LayerTreeHost::create(this, cc::LayerTreeSettings(),
                                               scoped_ptr<cc::Thread>());
  if (!layer_tree_host_.get())
    return false;
  return true;
}

void WebLayerTreeViewImplForTesting::setSurfaceReady() {
  layer_tree_host_->setSurfaceReady();
}

void WebLayerTreeViewImplForTesting::setRootLayer(const WebLayer& root) {
  layer_tree_host_->setRootLayer(
      static_cast<const WebLayerImpl*>(&root)->layer());
}

void WebLayerTreeViewImplForTesting::clearRootLayer() {
  layer_tree_host_->setRootLayer(scoped_refptr<cc::Layer>());
}

void WebLayerTreeViewImplForTesting::setViewportSize(
    const WebSize& layout_viewport_size, const WebSize& device_viewport_size) {
  layer_tree_host_->setViewportSize(layout_viewport_size, device_viewport_size);
}

WebSize WebLayerTreeViewImplForTesting::layoutViewportSize() const {
  return layer_tree_host_->layoutViewportSize();
}

WebSize WebLayerTreeViewImplForTesting::deviceViewportSize() const {
  return layer_tree_host_->deviceViewportSize();
}

void WebLayerTreeViewImplForTesting::setDeviceScaleFactor(
    float device_scale_factor) {
  layer_tree_host_->setDeviceScaleFactor(device_scale_factor);
}

float WebLayerTreeViewImplForTesting::deviceScaleFactor() const {
  return layer_tree_host_->deviceScaleFactor();
}

void WebLayerTreeViewImplForTesting::setBackgroundColor(WebColor color) {
  layer_tree_host_->setBackgroundColor(color);
}

void WebLayerTreeViewImplForTesting::setHasTransparentBackground(
    bool transparent) {
  layer_tree_host_->setHasTransparentBackground(transparent);
}

void WebLayerTreeViewImplForTesting::setVisible(bool visible) {
  layer_tree_host_->setVisible(visible);
}

void WebLayerTreeViewImplForTesting::setPageScaleFactorAndLimits(
    float page_scale_factor, float minimum, float maximum) {
  layer_tree_host_->setPageScaleFactorAndLimits(page_scale_factor, minimum,
                                               maximum);
}

void WebLayerTreeViewImplForTesting::startPageScaleAnimation(
    const WebPoint& scroll, bool use_anchor, float new_page_scale,
    double duration_sec) {
}

void WebLayerTreeViewImplForTesting::setNeedsAnimate() {
  layer_tree_host_->setNeedsAnimate();
}

void WebLayerTreeViewImplForTesting::setNeedsRedraw() {
  layer_tree_host_->setNeedsRedraw();
}

bool WebLayerTreeViewImplForTesting::commitRequested() const {
  return layer_tree_host_->commitRequested();
}

void WebLayerTreeViewImplForTesting::composite() {
  layer_tree_host_->composite();
}

void WebLayerTreeViewImplForTesting::updateAnimations(
    double frame_begin_timeSeconds) {
  base::TimeTicks frame_begin_time = base::TimeTicks::FromInternalValue(
      frame_begin_timeSeconds * base::Time::kMicrosecondsPerMillisecond);
  layer_tree_host_->updateAnimations(frame_begin_time);
}

void WebLayerTreeViewImplForTesting::didStopFlinging() {
}

bool WebLayerTreeViewImplForTesting::compositeAndReadback(void* pixels,
                                                          const WebRect& rect) {
  return layer_tree_host_->compositeAndReadback(pixels, rect);
}

void WebLayerTreeViewImplForTesting::finishAllRendering() {
  layer_tree_host_->finishAllRendering();
}

void WebLayerTreeViewImplForTesting::setDeferCommits(bool defer_commits) {
  layer_tree_host_->setDeferCommits(defer_commits);
}

void WebLayerTreeViewImplForTesting::renderingStats(WebRenderingStats&) const {
}

void WebLayerTreeViewImplForTesting::willBeginFrame() {
}

void WebLayerTreeViewImplForTesting::didBeginFrame() {
}

void WebLayerTreeViewImplForTesting::animate(
    double monotonic_frame_begin_time) { }

void WebLayerTreeViewImplForTesting::layout() { }

void WebLayerTreeViewImplForTesting::applyScrollAndScale(
    gfx::Vector2d scroll_delta, float page_scale) {
}

scoped_ptr<cc::OutputSurface>
    WebLayerTreeViewImplForTesting::createOutputSurface() {
  scoped_ptr<WebGraphicsContext3D> context3d(
      new cc::FakeWebGraphicsContext3D);
  return webkit::WebCompositorSupportOutputSurface::Create3d(
      context3d.Pass()).PassAs<cc::OutputSurface>();
}

void WebLayerTreeViewImplForTesting::didRecreateOutputSurface(bool success) { }

scoped_ptr<cc::InputHandler>
    WebLayerTreeViewImplForTesting::createInputHandler() {
  return scoped_ptr<cc::InputHandler>();
}

void WebLayerTreeViewImplForTesting::willCommit() { }

void WebLayerTreeViewImplForTesting::didCommit() { }

void WebLayerTreeViewImplForTesting::didCommitAndDrawFrame() { }

void WebLayerTreeViewImplForTesting::didCompleteSwapBuffers() { }

void WebLayerTreeViewImplForTesting::scheduleComposite() { }

scoped_ptr<cc::FontAtlas> WebLayerTreeViewImplForTesting::createFontAtlas() {
  return scoped_ptr<cc::FontAtlas>();
}

}  // namespace WebKit
