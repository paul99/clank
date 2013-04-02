// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/delegating_renderer.h"

#include "cc/test/fake_output_surface.h"
#include "cc/test/layer_tree_test_common.h"
#include "cc/test/render_pass_test_common.h"
#include "cc/test/render_pass_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {

class DelegatingRendererTest : public ThreadedTest {
 public:
  DelegatingRendererTest() : ThreadedTest(), output_surface_(NULL) {}
  virtual ~DelegatingRendererTest() {}

  virtual scoped_ptr<OutputSurface> createOutputSurface() OVERRIDE {
    scoped_ptr<TestWebGraphicsContext3D> context3d =
        TestWebGraphicsContext3D::Create(
            WebKit::WebGraphicsContext3D::Attributes());
    context3d_ = context3d.get();
    scoped_ptr<FakeOutputSurface> output_surface =
        FakeOutputSurface::CreateDelegating3d(
            context3d.PassAs<WebKit::WebGraphicsContext3D>());
    output_surface_ = output_surface.get();
    return output_surface.PassAs<OutputSurface>();
  }

 protected:
  TestWebGraphicsContext3D* context3d_;
  FakeOutputSurface* output_surface_;
};

class DelegatingRendererTestDraw : public DelegatingRendererTest {
 public:
  virtual void beginTest() OVERRIDE {
    m_layerTreeHost->setPageScaleFactorAndLimits(1.f, 0.5f, 4.f);
    postSetNeedsCommitToMainThread();
  }

  virtual void afterTest() OVERRIDE {}

  virtual bool prepareToDrawOnThread(
      LayerTreeHostImpl*, LayerTreeHostImpl::FrameData& frame, bool result)
      OVERRIDE {
    EXPECT_EQ(0u, output_surface_->num_sent_frames());

    CompositorFrame& last_frame = output_surface_->last_sent_frame();
    EXPECT_FALSE(last_frame.delegated_frame_data);
    EXPECT_FALSE(last_frame.gl_frame_data);
    EXPECT_EQ(0.f, last_frame.metadata.min_page_scale_factor);
    EXPECT_EQ(0.f, last_frame.metadata.max_page_scale_factor);
    return true;
  }

  virtual void drawLayersOnThread(LayerTreeHostImpl* host_impl) OVERRIDE {
    EXPECT_EQ(1u, output_surface_->num_sent_frames());

    CompositorFrame& last_frame = output_surface_->last_sent_frame();
    ASSERT_TRUE(last_frame.delegated_frame_data);
    EXPECT_FALSE(last_frame.gl_frame_data);
    EXPECT_EQ(
        host_impl->deviceViewportSize().ToString(),
        last_frame.delegated_frame_data->size.ToString());
    EXPECT_EQ(0.5f, last_frame.metadata.min_page_scale_factor);
    EXPECT_EQ(4.f, last_frame.metadata.max_page_scale_factor);

    EXPECT_EQ(
        0u, last_frame.delegated_frame_data->resource_list.resources.size());
    EXPECT_EQ(1u, last_frame.delegated_frame_data->render_pass_list.size());

    endTest();
  }
};

SINGLE_AND_MULTI_THREAD_TEST_F(DelegatingRendererTestDraw)

class DelegatingRendererTestResources : public DelegatingRendererTest {
 public:
  virtual void beginTest() OVERRIDE {
    postSetNeedsCommitToMainThread();
  }

  virtual void afterTest() OVERRIDE {}

  virtual bool prepareToDrawOnThread(
      LayerTreeHostImpl* host_impl,
      LayerTreeHostImpl::FrameData& frame,
      bool result) OVERRIDE {

    frame.renderPasses.clear();
    frame.renderPassesById.clear();

    TestRenderPass* child_pass = addRenderPass(
        frame.renderPasses,
        RenderPass::Id(2, 1),
        gfx::Rect(3, 3, 10, 10),
        gfx::Transform());
    child_pass->AppendOneOfEveryQuadType(
        host_impl->resourceProvider(), RenderPass::Id(0, 0));

    TestRenderPass* pass = addRenderPass(
        frame.renderPasses,
        RenderPass::Id(1, 1),
        gfx::Rect(3, 3, 10, 10),
        gfx::Transform());
    pass->AppendOneOfEveryQuadType(
        host_impl->resourceProvider(), child_pass->id);
    return true;
  }

  virtual void drawLayersOnThread(LayerTreeHostImpl* host_impl) OVERRIDE {
    EXPECT_EQ(1u, output_surface_->num_sent_frames());

    CompositorFrame& last_frame = output_surface_->last_sent_frame();
    ASSERT_TRUE(last_frame.delegated_frame_data);

    EXPECT_EQ(2u, last_frame.delegated_frame_data->render_pass_list.size());
    // Each render pass has 7 resources in it. And the root render pass has a
    // mask resource used when drawing the child render pass. The number 7 may
    // change if AppendOneOfEveryQuadType is updated, and the value here should
    // be updated accordingly.
    EXPECT_EQ(
        15u, last_frame.delegated_frame_data->resource_list.resources.size());

    endTest();
  }
};

SINGLE_AND_MULTI_THREAD_TEST_F(DelegatingRendererTestResources)

}  // namespace cc
