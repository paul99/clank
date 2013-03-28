// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_LAYER_TREE_TEST_COMMON_H_
#define CC_TEST_LAYER_TREE_TEST_COMMON_H_

#include "base/hash_tables.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread.h"
#include "cc/layer_tree_host.h"
#include "cc/layer_tree_host_impl.h"
#include "cc/scoped_thread_proxy.h"
#include "cc/test/compositor_fake_web_graphics_context_3d.h"
#include "testing/gtest/include/gtest/gtest.h"
#include <public/WebAnimationDelegate.h>

namespace cc {
class LayerImpl;
class LayerTreeHost;
class LayerTreeHostClient;
class LayerTreeHostImpl;
class Thread;


// Used by test stubs to notify the test when something interesting happens.
class TestHooks : public WebKit::WebAnimationDelegate {
public:
    virtual void beginCommitOnThread(LayerTreeHostImpl*) { }
    virtual void commitCompleteOnThread(LayerTreeHostImpl*) { }
    virtual bool prepareToDrawOnThread(LayerTreeHostImpl*);
    virtual void drawLayersOnThread(LayerTreeHostImpl*) { }
    virtual void animateLayers(LayerTreeHostImpl*, base::TimeTicks monotonicTime) { }
    virtual void willAnimateLayers(LayerTreeHostImpl*, base::TimeTicks monotonicTime) { }
    virtual void applyScrollAndScale(gfx::Vector2d, float) { }
    virtual void animate(base::TimeTicks monotonicTime) { }
    virtual void layout() { }
    virtual void didRecreateOutputSurface(bool succeeded) { }
    virtual void didAddAnimation() { }
    virtual void didCommit() { }
    virtual void didCommitAndDrawFrame() { }
    virtual void scheduleComposite() { }
    virtual void didDeferCommit() { }

    // Implementation of WebAnimationDelegate
    virtual void notifyAnimationStarted(double time) OVERRIDE { }
    virtual void notifyAnimationFinished(double time) OVERRIDE { }

    virtual scoped_ptr<OutputSurface> createOutputSurface();
};

class TimeoutTask;
class BeginTask;

class MockLayerImplTreeHostClient : public LayerTreeHostClient {
};

// The ThreadedTests runs with the main loop running. It instantiates a single MockLayerTreeHost and associated
// MockLayerTreeHostImpl/ThreadedMockLayerTreeHostClient.
//
// beginTest() is called once the main message loop is running and the layer tree host is initialized.
//
// Key stages of the drawing loop, e.g. drawing or commiting, redirect to ThreadedTest methods of similar names.
// To track the commit process, override these functions.
//
// The test continues until someone calls endTest. endTest can be called on any thread, but be aware that
// ending the test is an asynchronous process.
class ThreadedTest : public testing::Test, public TestHooks {
public:
    virtual ~ThreadedTest();

    virtual void afterTest() = 0;
    virtual void beginTest() = 0;

    void endTest();
    void endTestAfterDelay(int delayMilliseconds);

    void postSetNeedsAnimateToMainThread();
    void postAddAnimationToMainThread(Layer*);
    void postAddInstantAnimationToMainThread();
    void postSetNeedsCommitToMainThread();
    void postAcquireLayerTextures();
    void postSetNeedsRedrawToMainThread();
    void postSetVisibleToMainThread(bool visible);
    void postDidAddAnimationToMainThread();

    void doBeginTest();
    void timeout();

    LayerTreeHost* layerTreeHost() { return m_layerTreeHost.get(); }

protected:
    ThreadedTest();

    virtual void initializeSettings(LayerTreeSettings&) { }

    virtual void scheduleComposite() OVERRIDE;

    void realEndTest();

    void dispatchSetNeedsAnimate();
    void dispatchAddInstantAnimation();
    void dispatchAddAnimation(Layer*);
    void dispatchSetNeedsCommit();
    void dispatchAcquireLayerTextures();
    void dispatchSetNeedsRedraw();
    void dispatchSetVisible(bool);
    void dispatchComposite();
    void dispatchDidAddAnimation();

    virtual void runTest(bool threaded);

    Thread* implThread() { return proxy() ? proxy()->implThread() : 0; }
    Proxy* proxy() const { return m_layerTreeHost ? m_layerTreeHost->proxy() : 0; }

    LayerTreeSettings m_settings;
    scoped_ptr<MockLayerImplTreeHostClient> m_client;
    scoped_ptr<LayerTreeHost> m_layerTreeHost;

protected:
    scoped_refptr<ScopedThreadProxy> m_mainThreadProxy;

private:
    bool m_beginning;
    bool m_endWhenBeginReturns;
    bool m_timedOut;
    bool m_scheduled;
    bool m_started;

    scoped_ptr<Thread> m_mainCCThread;
    scoped_ptr<base::Thread> m_implThread;
    base::CancelableClosure m_timeout;
};

class ThreadedTestThreadOnly : public ThreadedTest {
public:
    void runTestThreaded()
    {
        ThreadedTest::runTest(true);
    }
};

// Adapts LayerTreeHostImpl for test. Runs real code, then invokes test hooks.
class MockLayerTreeHostImpl : public LayerTreeHostImpl {
public:
    static scoped_ptr<MockLayerTreeHostImpl> create(TestHooks*, const LayerTreeSettings&, LayerTreeHostImplClient*, Proxy*);

    virtual void beginCommit() OVERRIDE;
    virtual void commitComplete() OVERRIDE;
    virtual bool prepareToDraw(FrameData&) OVERRIDE;
    virtual void drawLayers(FrameData&) OVERRIDE;

    // Make these public.
    typedef std::vector<LayerImpl*> LayerList;
    using LayerTreeHostImpl::calculateRenderSurfaceLayerList;

protected:
    virtual void animateLayers(base::TimeTicks monotonicTime, base::Time wallClockTime) OVERRIDE;
    virtual base::TimeDelta lowFrequencyAnimationInterval() const OVERRIDE;

private:
    MockLayerTreeHostImpl(TestHooks*, const LayerTreeSettings&, LayerTreeHostImplClient*, Proxy*);

    TestHooks* m_testHooks;
};

class CompositorFakeWebGraphicsContext3DWithTextureTracking : public CompositorFakeWebGraphicsContext3D {
public:
    static scoped_ptr<CompositorFakeWebGraphicsContext3DWithTextureTracking> create(Attributes);
    virtual ~CompositorFakeWebGraphicsContext3DWithTextureTracking();

    virtual WebKit::WebGLId createTexture();

    virtual void deleteTexture(WebKit::WebGLId texture);

    virtual void bindTexture(WebKit::WGC3Denum target, WebKit::WebGLId texture);

    int numTextures() const;
    int texture(int texture) const;
    void resetTextures();

    int numUsedTextures() const;
    bool usedTexture(int texture) const;
    void resetUsedTextures();

private:
    explicit CompositorFakeWebGraphicsContext3DWithTextureTracking(Attributes attrs);

    std::vector<WebKit::WebGLId> m_textures;
    base::hash_set<WebKit::WebGLId> m_usedTextures;
};

} // namespace cc

#define SINGLE_AND_MULTI_THREAD_TEST_F(TEST_FIXTURE_NAME) \
    TEST_F(TEST_FIXTURE_NAME, runSingleThread)            \
    {                                                     \
        runTest(false);                                   \
    }                                                     \
    TEST_F(TEST_FIXTURE_NAME, runMultiThread)             \
    {                                                     \
        runTest(true);                                    \
    }

#endif  // CC_TEST_LAYER_TREE_TEST_COMMON_H_
