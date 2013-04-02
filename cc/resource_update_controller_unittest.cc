// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resource_update_controller.h"

#include "cc/prioritized_resource_manager.h"
#include "cc/single_thread_proxy.h" // For DebugScopedSetImplThread
#include "cc/test/fake_output_surface.h"
#include "cc/test/fake_proxy.h"
#include "cc/test/scheduler_test_common.h"
#include "cc/test/test_web_graphics_context_3d.h"
#include "cc/test/tiled_layer_test_common.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/khronos/GLES2/gl2ext.h"

using namespace WebKit;
using testing::Test;

namespace cc {
namespace {

const int kFlushPeriodFull = 4;
const int kFlushPeriodPartial = kFlushPeriodFull;

class ResourceUpdateControllerTest;

class WebGraphicsContext3DForUploadTest : public TestWebGraphicsContext3D {
public:
    WebGraphicsContext3DForUploadTest(ResourceUpdateControllerTest *test)
        : m_test(test)
        , m_supportShallowFlush(true)
    { }

    virtual void flush(void);
    virtual void shallowFlushCHROMIUM(void);
    virtual void texSubImage2D(WGC3Denum target,
                               WGC3Dint level,
                               WGC3Dint xoffset,
                               WGC3Dint yoffset,
                               WGC3Dsizei width,
                               WGC3Dsizei height,
                               WGC3Denum format,
                               WGC3Denum type,
                               const void* pixels);
    virtual GrGLInterface* onCreateGrGLInterface() { return 0; }

    virtual WebString getString(WGC3Denum name)
    {
        if (m_supportShallowFlush)
            return WebString("GL_CHROMIUM_shallow_flush");
        return WebString("");
    }

    virtual void getQueryObjectuivEXT(WebGLId, WGC3Denum, WGC3Duint*);

private:
    ResourceUpdateControllerTest* m_test;
    bool m_supportShallowFlush;
};


class ResourceUpdateControllerTest : public Test {
public:
    ResourceUpdateControllerTest()
        : m_proxy(scoped_ptr<Thread>(NULL))
        , m_queue(make_scoped_ptr(new ResourceUpdateQueue))
        , m_resourceManager(PrioritizedResourceManager::create(&m_proxy))
        , m_fullUploadCountExpected(0)
        , m_partialCountExpected(0)
        , m_totalUploadCountExpected(0)
        , m_maxUploadCountPerUpdate(0)
        , m_numConsecutiveFlushes(0)
        , m_numDanglingUploads(0)
        , m_numTotalUploads(0)
        , m_numTotalFlushes(0)
        , m_queryResultsAvailable(0)
    {
    }

    virtual ~ResourceUpdateControllerTest()
    {
        DebugScopedSetImplThreadAndMainThreadBlocked
            implThreadAndMainThreadBlocked(&m_proxy);
        m_resourceManager->clearAllMemory(m_resourceProvider.get());
    }

public:
    void onFlush()
    {
        // Check for back-to-back flushes.
        EXPECT_EQ(0, m_numConsecutiveFlushes) << "Back-to-back flushes detected.";

        m_numDanglingUploads = 0;
        m_numConsecutiveFlushes++;
        m_numTotalFlushes++;
    }

    void onUpload()
    {
        // Check for too many consecutive uploads
        if (m_numTotalUploads < m_fullUploadCountExpected)
            EXPECT_LT(m_numDanglingUploads, kFlushPeriodFull) << "Too many consecutive full uploads detected.";
        else
            EXPECT_LT(m_numDanglingUploads, kFlushPeriodPartial) << "Too many consecutive partial uploads detected.";

        m_numConsecutiveFlushes = 0;
        m_numDanglingUploads++;
        m_numTotalUploads++;
    }

    bool isQueryResultAvailable()
    {
        if (!m_queryResultsAvailable)
            return false;

        m_queryResultsAvailable--;
        return true;
    }

protected:
    virtual void SetUp()
    {
        m_outputSurface = FakeOutputSurface::Create3d(scoped_ptr<WebKit::WebGraphicsContext3D>(new WebGraphicsContext3DForUploadTest(this)));
        m_bitmap.setConfig(SkBitmap::kARGB_8888_Config, 300, 150);
        m_bitmap.allocPixels();

        for (int i = 0; i < 4; i++) {
            m_textures[i] = PrioritizedResource::create(
                m_resourceManager.get(), gfx::Size(300, 150), GL_RGBA);
            m_textures[i]->setRequestPriority(
                PriorityCalculator::visiblePriority(true));
        }
        m_resourceManager->prioritizeTextures();

        m_resourceProvider = ResourceProvider::create(m_outputSurface.get());
    }


    void appendFullUploadsOfIndexedTextureToUpdateQueue(int count, int textureIndex)
    {
        m_fullUploadCountExpected += count;
        m_totalUploadCountExpected += count;

        const gfx::Rect rect(0, 0, 300, 150);
        const ResourceUpdate upload = ResourceUpdate::Create(
            m_textures[textureIndex].get(), &m_bitmap, rect, rect, gfx::Vector2d());
        for (int i = 0; i < count; i++)
            m_queue->appendFullUpload(upload);
    }

    void appendFullUploadsToUpdateQueue(int count)
    {
        appendFullUploadsOfIndexedTextureToUpdateQueue(count, 0);
    }

    void appendPartialUploadsOfIndexedTextureToUpdateQueue(int count, int textureIndex)
    {
        m_partialCountExpected += count;
        m_totalUploadCountExpected += count;

        const gfx::Rect rect(0, 0, 100, 100);
        const ResourceUpdate upload = ResourceUpdate::Create(
            m_textures[textureIndex].get(), &m_bitmap, rect, rect, gfx::Vector2d());
        for (int i = 0; i < count; i++)
            m_queue->appendPartialUpload(upload);
    }

    void appendPartialUploadsToUpdateQueue(int count)
    {
        appendPartialUploadsOfIndexedTextureToUpdateQueue(count, 0);
    }

    void setMaxUploadCountPerUpdate(int count)
    {
        m_maxUploadCountPerUpdate = count;
    }

    void updateTextures()
    {
        DebugScopedSetImplThreadAndMainThreadBlocked
            implThreadAndMainThreadBlocked(&m_proxy);
        scoped_ptr<ResourceUpdateController> updateController =
            ResourceUpdateController::create(
                NULL,
                m_proxy.implThread(),
                m_queue.Pass(),
                m_resourceProvider.get(),
                m_proxy.hasImplThread());
        updateController->finalize();
    }

    void makeQueryResultAvailable()
    {
        m_queryResultsAvailable++;
    }

protected:
    // Classes required to interact and test the ResourceUpdateController
    FakeProxy m_proxy;
    scoped_ptr<OutputSurface> m_outputSurface;
    scoped_ptr<ResourceProvider> m_resourceProvider;
    scoped_ptr<ResourceUpdateQueue> m_queue;
    scoped_ptr<PrioritizedResource> m_textures[4];
    scoped_ptr<PrioritizedResourceManager> m_resourceManager;
    SkBitmap m_bitmap;
    int m_queryResultsAvailable;

    // Properties / expectations of this test
    int m_fullUploadCountExpected;
    int m_partialCountExpected;
    int m_totalUploadCountExpected;
    int m_maxUploadCountPerUpdate;

    // Dynamic properties of this test
    int m_numConsecutiveFlushes;
    int m_numDanglingUploads;
    int m_numTotalUploads;
    int m_numTotalFlushes;
};

void WebGraphicsContext3DForUploadTest::flush(void)
{
    m_test->onFlush();
}

void WebGraphicsContext3DForUploadTest::shallowFlushCHROMIUM(void)
{
    m_test->onFlush();
}

void WebGraphicsContext3DForUploadTest::texSubImage2D(WGC3Denum target,
                                                      WGC3Dint level,
                                                      WGC3Dint xoffset,
                                                      WGC3Dint yoffset,
                                                      WGC3Dsizei width,
                                                      WGC3Dsizei height,
                                                      WGC3Denum format,
                                                      WGC3Denum type,
                                                      const void* pixels)
{
    m_test->onUpload();
}

void WebGraphicsContext3DForUploadTest::getQueryObjectuivEXT(
    WebGLId,
    WGC3Denum pname,
    WGC3Duint* params) {
    if (pname == GL_QUERY_RESULT_AVAILABLE_EXT)
        *params = m_test->isQueryResultAvailable();
}

// ZERO UPLOADS TESTS
TEST_F(ResourceUpdateControllerTest, ZeroUploads)
{
    appendFullUploadsToUpdateQueue(0);
    appendPartialUploadsToUpdateQueue(0);
    updateTextures();

    EXPECT_EQ(0, m_numTotalFlushes);
    EXPECT_EQ(0, m_numTotalUploads);
}


// ONE UPLOAD TESTS
TEST_F(ResourceUpdateControllerTest, OneFullUpload)
{
    appendFullUploadsToUpdateQueue(1);
    appendPartialUploadsToUpdateQueue(0);
    updateTextures();

    EXPECT_EQ(1, m_numTotalFlushes);
    EXPECT_EQ(1, m_numTotalUploads);
    EXPECT_EQ(0, m_numDanglingUploads) << "Last upload wasn't followed by a flush.";
}

TEST_F(ResourceUpdateControllerTest, OnePartialUpload)
{
    appendFullUploadsToUpdateQueue(0);
    appendPartialUploadsToUpdateQueue(1);
    updateTextures();

    EXPECT_EQ(1, m_numTotalFlushes);
    EXPECT_EQ(1, m_numTotalUploads);
    EXPECT_EQ(0, m_numDanglingUploads) << "Last upload wasn't followed by a flush.";
}

TEST_F(ResourceUpdateControllerTest, OneFullOnePartialUpload)
{
    appendFullUploadsToUpdateQueue(1);
    appendPartialUploadsToUpdateQueue(1);
    updateTextures();

    EXPECT_EQ(1, m_numTotalFlushes);
    EXPECT_EQ(2, m_numTotalUploads);
    EXPECT_EQ(0, m_numDanglingUploads) << "Last upload wasn't followed by a flush.";
}


// This class of tests upload a number of textures that is a multiple of the flush period.
const int fullUploadFlushMultipler = 7;
const int fullCount = fullUploadFlushMultipler * kFlushPeriodFull;

const int partialUploadFlushMultipler = 11;
const int partialCount = partialUploadFlushMultipler * kFlushPeriodPartial;

TEST_F(ResourceUpdateControllerTest, ManyFullUploads)
{
    appendFullUploadsToUpdateQueue(fullCount);
    appendPartialUploadsToUpdateQueue(0);
    updateTextures();

    EXPECT_EQ(fullUploadFlushMultipler, m_numTotalFlushes);
    EXPECT_EQ(fullCount, m_numTotalUploads);
    EXPECT_EQ(0, m_numDanglingUploads) << "Last upload wasn't followed by a flush.";
}

TEST_F(ResourceUpdateControllerTest, ManyPartialUploads)
{
    appendFullUploadsToUpdateQueue(0);
    appendPartialUploadsToUpdateQueue(partialCount);
    updateTextures();

    EXPECT_EQ(partialUploadFlushMultipler, m_numTotalFlushes);
    EXPECT_EQ(partialCount, m_numTotalUploads);
    EXPECT_EQ(0, m_numDanglingUploads) << "Last upload wasn't followed by a flush.";
}

TEST_F(ResourceUpdateControllerTest, ManyFullManyPartialUploads)
{
    appendFullUploadsToUpdateQueue(fullCount);
    appendPartialUploadsToUpdateQueue(partialCount);
    updateTextures();

    EXPECT_EQ(fullUploadFlushMultipler + partialUploadFlushMultipler, m_numTotalFlushes);
    EXPECT_EQ(fullCount + partialCount, m_numTotalUploads);
    EXPECT_EQ(0, m_numDanglingUploads) << "Last upload wasn't followed by a flush.";
}

class FakeResourceUpdateControllerClient : public cc::ResourceUpdateControllerClient {
public:
    FakeResourceUpdateControllerClient() { reset(); }
    void reset() { m_readyToFinalizeCalled = false; }
    bool readyToFinalizeCalled() const { return m_readyToFinalizeCalled; }

    virtual void readyToFinalizeTextureUpdates() OVERRIDE { m_readyToFinalizeCalled = true; }

protected:
    bool m_readyToFinalizeCalled;
};

class FakeResourceUpdateController : public cc::ResourceUpdateController {
public:
    static scoped_ptr<FakeResourceUpdateController> create(cc::ResourceUpdateControllerClient* client, cc::Thread* thread, scoped_ptr<ResourceUpdateQueue> queue, ResourceProvider* resourceProvider)
    {
        return make_scoped_ptr(new FakeResourceUpdateController(client, thread, queue.Pass(), resourceProvider));
    }

    void setNow(base::TimeTicks time) { m_now = time; }
    virtual base::TimeTicks now() const OVERRIDE { return m_now; }
    void setUpdateMoreTexturesTime(base::TimeDelta time) { m_updateMoreTexturesTime = time; }
    virtual base::TimeDelta updateMoreTexturesTime() const OVERRIDE { return m_updateMoreTexturesTime; }
    void setUpdateMoreTexturesSize(size_t size) { m_updateMoreTexturesSize = size; }
    virtual size_t updateMoreTexturesSize() const OVERRIDE { return m_updateMoreTexturesSize; }

protected:
    FakeResourceUpdateController(cc::ResourceUpdateControllerClient* client, cc::Thread* thread, scoped_ptr<ResourceUpdateQueue> queue, ResourceProvider* resourceProvider)
        : cc::ResourceUpdateController(client, thread, queue.Pass(), resourceProvider, false)
        , m_updateMoreTexturesSize(0) { }

    base::TimeTicks m_now;
    base::TimeDelta m_updateMoreTexturesTime;
    size_t m_updateMoreTexturesSize;
};

static void runPendingTask(FakeThread* thread, FakeResourceUpdateController* controller)
{
    EXPECT_TRUE(thread->hasPendingTask());
    controller->setNow(controller->now() + base::TimeDelta::FromMilliseconds(thread->pendingDelayMs()));
    thread->runPendingTask();
}

TEST_F(ResourceUpdateControllerTest, UpdateMoreTextures)
{
    FakeResourceUpdateControllerClient client;
    FakeThread thread;

    setMaxUploadCountPerUpdate(1);
    appendFullUploadsToUpdateQueue(3);
    appendPartialUploadsToUpdateQueue(0);

    DebugScopedSetImplThreadAndMainThreadBlocked
        implThreadAndMainThreadBlocked(&m_proxy);
    scoped_ptr<FakeResourceUpdateController> controller(FakeResourceUpdateController::create(&client, &thread, m_queue.Pass(), m_resourceProvider.get()));

    controller->setNow(
        controller->now() + base::TimeDelta::FromMilliseconds(1));
    controller->setUpdateMoreTexturesTime(
        base::TimeDelta::FromMilliseconds(100));
    controller->setUpdateMoreTexturesSize(1);
    // Not enough time for any updates.
    controller->performMoreUpdates(
        controller->now() + base::TimeDelta::FromMilliseconds(90));
    EXPECT_FALSE(thread.hasPendingTask());

    controller->setUpdateMoreTexturesTime(
        base::TimeDelta::FromMilliseconds(100));
    controller->setUpdateMoreTexturesSize(1);
    // Only enough time for 1 update.
    controller->performMoreUpdates(
        controller->now() + base::TimeDelta::FromMilliseconds(120));
    EXPECT_FALSE(thread.hasPendingTask());
    EXPECT_EQ(1, m_numTotalUploads);

    // Complete one upload.
    makeQueryResultAvailable();

    controller->setUpdateMoreTexturesTime(
        base::TimeDelta::FromMilliseconds(100));
    controller->setUpdateMoreTexturesSize(1);
    // Enough time for 2 updates.
    controller->performMoreUpdates(
        controller->now() + base::TimeDelta::FromMilliseconds(220));
    runPendingTask(&thread, controller.get());
    EXPECT_FALSE(thread.hasPendingTask());
    EXPECT_TRUE(client.readyToFinalizeCalled());
    EXPECT_EQ(3, m_numTotalUploads);
}

TEST_F(ResourceUpdateControllerTest, NoMoreUpdates)
{
    FakeResourceUpdateControllerClient client;
    FakeThread thread;

    setMaxUploadCountPerUpdate(1);
    appendFullUploadsToUpdateQueue(2);
    appendPartialUploadsToUpdateQueue(0);

    DebugScopedSetImplThreadAndMainThreadBlocked
        implThreadAndMainThreadBlocked(&m_proxy);
    scoped_ptr<FakeResourceUpdateController> controller(FakeResourceUpdateController::create(&client, &thread, m_queue.Pass(), m_resourceProvider.get()));

    controller->setNow(
        controller->now() + base::TimeDelta::FromMilliseconds(1));
    controller->setUpdateMoreTexturesTime(
        base::TimeDelta::FromMilliseconds(100));
    controller->setUpdateMoreTexturesSize(1);
    // Enough time for 3 updates but only 2 necessary.
    controller->performMoreUpdates(
        controller->now() + base::TimeDelta::FromMilliseconds(310));
    runPendingTask(&thread, controller.get());
    EXPECT_FALSE(thread.hasPendingTask());
    EXPECT_TRUE(client.readyToFinalizeCalled());
    EXPECT_EQ(2, m_numTotalUploads);

    controller->setUpdateMoreTexturesTime(
        base::TimeDelta::FromMilliseconds(100));
    controller->setUpdateMoreTexturesSize(1);
    // Enough time for updates but no more updates left.
    controller->performMoreUpdates(
        controller->now() + base::TimeDelta::FromMilliseconds(310));
    // 0-delay task used to call readyToFinalizeTextureUpdates().
    runPendingTask(&thread, controller.get());
    EXPECT_FALSE(thread.hasPendingTask());
    EXPECT_TRUE(client.readyToFinalizeCalled());
    EXPECT_EQ(2, m_numTotalUploads);
}

TEST_F(ResourceUpdateControllerTest, UpdatesCompleteInFiniteTime)
{
    FakeResourceUpdateControllerClient client;
    FakeThread thread;

    setMaxUploadCountPerUpdate(1);
    appendFullUploadsToUpdateQueue(2);
    appendPartialUploadsToUpdateQueue(0);

    DebugScopedSetImplThreadAndMainThreadBlocked
        implThreadAndMainThreadBlocked(&m_proxy);
    scoped_ptr<FakeResourceUpdateController> controller(FakeResourceUpdateController::create(&client, &thread, m_queue.Pass(), m_resourceProvider.get()));

    controller->setNow(
        controller->now() + base::TimeDelta::FromMilliseconds(1));
    controller->setUpdateMoreTexturesTime(
        base::TimeDelta::FromMilliseconds(500));
    controller->setUpdateMoreTexturesSize(1);

    for (int i = 0; i < 100; i++) {
        if (client.readyToFinalizeCalled())
            break;

        // Not enough time for any updates.
        controller->performMoreUpdates(
            controller->now() + base::TimeDelta::FromMilliseconds(400));

        if (thread.hasPendingTask())
            runPendingTask(&thread, controller.get());
    }

    EXPECT_FALSE(thread.hasPendingTask());
    EXPECT_TRUE(client.readyToFinalizeCalled());
    EXPECT_EQ(2, m_numTotalUploads);
}

}  // namespace
}  // namespace cc
