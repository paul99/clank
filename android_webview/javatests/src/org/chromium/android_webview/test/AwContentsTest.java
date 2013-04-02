// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview.test;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.test.UiThreadTest;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.SmallTest;
import android.util.Pair;

import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.test.util.CommonResources;
import org.chromium.base.test.util.DisabledTest;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.content.browser.test.util.CallbackHelper;
import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;
import org.chromium.net.test.util.TestWebServer;

import java.io.InputStream;
import java.net.URL;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.Callable;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * AwContents tests.
 */
public class AwContentsTest extends AndroidWebViewTestBase {
    private TestAwContentsClient mContentsClient = new TestAwContentsClient();

    @SmallTest
    @Feature({"AndroidWebView"})
    @UiThreadTest
    public void testCreateDestroy() throws Throwable {
        // NOTE this test runs on UI thread, so we cannot call any async methods.
        createAwTestContainerView(mContentsClient).getAwContents().destroy();
    }

    /*
     * @LargeTest
     * @Feature({"AndroidWebView"})
     * Disabled until we switch to final rendering pipeline.
     */
    @DisabledTest
    public void testCreateLoadDestroyManyTimes() throws Throwable {
        final int CREATE_AND_DESTROY_REPEAT_COUNT = 10;
        for (int i = 0; i < CREATE_AND_DESTROY_REPEAT_COUNT; ++i) {
            AwTestContainerView testView = createAwTestContainerViewOnMainSync(mContentsClient);
            AwContents awContents = testView.getAwContents();

            loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(), "about:blank");
            destroyAwContentsOnMainSync(awContents);
        }
    }

    /*
     * @LargeTest
     * @Feature({"AndroidWebView"})
     * Disabled until we switch to final rendering pipeline.
     */
    @DisabledTest
    public void testCreateLoadDestroyManyAtOnce() throws Throwable {
        final int CREATE_AND_DESTROY_REPEAT_COUNT = 10;
        AwTestContainerView views[] = new AwTestContainerView[CREATE_AND_DESTROY_REPEAT_COUNT];

        for (int i = 0; i < views.length; ++i) {
            views[i] = createAwTestContainerViewOnMainSync(mContentsClient);
            loadUrlSync(views[i].getAwContents(), mContentsClient.getOnPageFinishedHelper(),
                    "about:blank");
        }

        for (int i = 0; i < views.length; ++i) {
            destroyAwContentsOnMainSync(views[i].getAwContents());
            views[i] = null;
        }
    }


    private int callDocumentHasImagesSync(final AwContents awContents)
            throws Throwable, InterruptedException {
        // Set up a container to hold the result object and a semaphore to
        // make the test wait for the result.
        final AtomicInteger val = new AtomicInteger();
        final Semaphore s = new Semaphore(0);
        final Message msg = Message.obtain(new Handler(Looper.getMainLooper()) {
            @Override
            public void handleMessage(Message msg) {
                val.set(msg.arg1);
                s.release();
            }
        });
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
              awContents.documentHasImages(msg);
            }
        });
        assertTrue(s.tryAcquire(WAIT_TIMEOUT_SECONDS, TimeUnit.SECONDS));
        int result = val.get();
        return result;
    }

    @SmallTest
    @Feature({"AndroidWebView"})
    public void testDocumentHasImages() throws Throwable {
        AwTestContainerView testView = createAwTestContainerViewOnMainSync(mContentsClient);
        AwContents awContents = testView.getAwContents();

        final CallbackHelper loadHelper = mContentsClient.getOnPageFinishedHelper();

        final String mime = "text/html";
        final String emptyDoc = "<head/><body/>";
        final String imageDoc = "<head/><body><img/><img/></body>";

        // Make sure a document that does not have images returns 0
        loadDataSync(awContents, loadHelper, emptyDoc, mime, false);
        int result = callDocumentHasImagesSync(awContents);
        assertEquals(0, result);

        // Make sure a document that does have images returns 1
        loadDataSync(awContents, loadHelper, imageDoc, mime, false);
        result = callDocumentHasImagesSync(awContents);
        assertEquals(1, result);
    }

    @SmallTest
    @Feature({"AndroidWebView"})
    public void testClearCacheMemoryAndDisk() throws Throwable {
        final TestAwContentsClient contentClient = new TestAwContentsClient();
        final AwTestContainerView testContainer =
                createAwTestContainerViewOnMainSync(contentClient);
        final AwContents awContents = testContainer.getAwContents();

        TestWebServer webServer = null;
        try {
            webServer = new TestWebServer(false);
            final String pagePath = "/clear_cache_test.html";
            List<Pair<String, String>> headers = new ArrayList<Pair<String, String>>();
            // Set Cache-Control headers to cache this request. One century should be long enough.
            headers.add(Pair.create("Cache-Control", "max-age=3153600000"));
            headers.add(Pair.create("Last-Modified", "Wed, 3 Oct 2012 00:00:00 GMT"));
            final String pageUrl = webServer.setResponse(
                    pagePath, "<html><body>foo</body></html>", headers);

            // First load to populate cache.
            clearCacheOnUiThread(awContents, true);
            loadUrlSync(awContents,
                        contentClient.getOnPageFinishedHelper(),
                        pageUrl);
            assertEquals(1, webServer.getRequestCount(pagePath));

            // Load about:blank so next load is not treated as reload by webkit and force
            // revalidate with the server.
            loadUrlSync(awContents,
                        contentClient.getOnPageFinishedHelper(),
                        "about:blank");

            // No clearCache call, so should be loaded from cache.
            loadUrlSync(awContents,
                        contentClient.getOnPageFinishedHelper(),
                        pageUrl);
            assertEquals(1, webServer.getRequestCount(pagePath));

            // Same as above.
            loadUrlSync(awContents,
                        contentClient.getOnPageFinishedHelper(),
                        "about:blank");

            // Clear cache, so should hit server again.
            clearCacheOnUiThread(awContents, true);
            loadUrlSync(awContents,
                        contentClient.getOnPageFinishedHelper(),
                        pageUrl);
            assertEquals(2, webServer.getRequestCount(pagePath));
        } finally {
            if (webServer != null) webServer.shutdown();
        }
    }

    @SmallTest
    @Feature({"AndroidWebView"})
    public void testClearCacheInQuickSuccession() throws Throwable {
        final AwTestContainerView testContainer =
                createAwTestContainerViewOnMainSync(new TestAwContentsClient());
        final AwContents awContents = testContainer.getAwContents();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
              for (int i = 0; i < 10; ++i) {
                  awContents.clearCache(true);
              }
            }
        });
    }

    private static final long TEST_TIMEOUT = 20000L;
    private static final int CHECK_INTERVAL = 100;

    @SmallTest
    @Feature({"AndroidWebView"})
    public void testGetFavicon() throws Throwable {
        final AwTestContainerView testView = createAwTestContainerViewOnMainSync(mContentsClient);
        final AwContents awContents = testView.getAwContents();

        TestWebServer webServer = null;
        try {
            webServer = new TestWebServer(false);

            final String faviconUrl = webServer.setResponseBase64(
                    "/" + CommonResources.FAVICON_FILENAME, CommonResources.FAVICON_DATA_BASE64,
                    CommonResources.getImagePngHeaders(false));
            final String pageUrl = webServer.setResponse("/favicon.html",
                    CommonResources.FAVICON_STATIC_HTML, null);

            // The getFavicon will return the right icon a certain time after
            // the page load completes which makes it slightly hard to test.
            final Bitmap defaultFavicon = awContents.getFavicon();

            getContentSettingsOnUiThread(awContents).setImagesEnabled(true);
            loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);

            assertTrue(CriteriaHelper.pollForCriteria(new Criteria() {
                @Override
                public boolean isSatisfied() {
                    return awContents.getFavicon() != null &&
                        !awContents.getFavicon().sameAs(defaultFavicon);
                }
            }, TEST_TIMEOUT, CHECK_INTERVAL));

            final Object originalFaviconSource = (new URL(faviconUrl)).getContent();
            final Bitmap originalFavicon =
                BitmapFactory.decodeStream((InputStream)originalFaviconSource);
            assertNotNull(originalFavicon);

            assertTrue(awContents.getFavicon().sameAs(originalFavicon));

        } finally {
            if (webServer != null) webServer.shutdown();
        }
    }

    @Feature({"AndroidWebView", "Downloads"})
    @SmallTest
    public void testDownload() throws Throwable {
        AwTestContainerView testView = createAwTestContainerViewOnMainSync(mContentsClient);
        AwContents awContents = testView.getAwContents();

        final String data = "download data";
        final String contentDisposition = "attachment;filename=\"download.txt\"";
        final String mimeType = "text/plain";

        List<Pair<String, String>> downloadHeaders = new ArrayList<Pair<String, String>>();
        downloadHeaders.add(Pair.create("Content-Disposition", contentDisposition));
        downloadHeaders.add(Pair.create("Content-Type", mimeType));
        downloadHeaders.add(Pair.create("Content-Length", Integer.toString(data.length())));

        TestWebServer webServer = null;
        try {
            webServer = new TestWebServer(false);
            final String pageUrl = webServer.setResponse(
                    "/download.txt", data, downloadHeaders);
            loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);

            assertTrue(pollOnUiThread(new Callable<Boolean>() {
                @Override
                public Boolean call() {
                    // Assert failures are treated as return false.
                    assertEquals(pageUrl, mContentsClient.mLastDownloadUrl);
                    assertEquals(contentDisposition,
                                 mContentsClient.mLastDownloadContentDisposition);
                    assertEquals(mimeType,
                                 mContentsClient.mLastDownloadMimeType);
                    assertEquals(data.length(),
                                 mContentsClient.mLastDownloadContentLength);
                    return true;
                }
            }));
        } finally {
            if (webServer != null) webServer.shutdown();
        }
    }

    @Feature({"AndroidWebView"})
    @SmallTest
    public void testUpdateVisitedHistoryCallback() throws Throwable {
        AwTestContainerView testView = createAwTestContainerViewOnMainSync(mContentsClient);
        AwContents awContents = testView.getAwContents();

        final String path = "/testUpdateVisitedHistoryCallback.html";
        final String html = "testUpdateVisitedHistoryCallback";

        TestWebServer webServer = null;
        try {
            webServer = new TestWebServer(false);
            final String pageUrl = webServer.setResponse(path, html, null);

            loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);
            assertEquals(pageUrl, mContentsClient.mLastVisitedUrl);
            assertEquals(false, mContentsClient.mLastVisitIsReload);

            // Reload
            loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);
            assertEquals(pageUrl, mContentsClient.mLastVisitedUrl);
            assertEquals(true, mContentsClient.mLastVisitIsReload);
        } finally {
            if (webServer != null) webServer.shutdown();
        }
    }

    @Feature({"AndroidWebView"})
    @SmallTest
    public void testGetVisitedHistoryExerciseCodePath() throws Throwable {
        // Due to security/privacy restrictions around the :visited css property, it is not
        // possible test this end to end without using the flaky and brittle capturing picture of
        // the web page. So we are doing the next best thing, exercising all the code paths.

        mContentsClient.mSaveGetVisitedHistoryCallback = true;
        AwTestContainerView testView = createAwTestContainerViewOnMainSync(mContentsClient);
        AwContents awContents = testView.getAwContents();

        final String path = "/testGetVisitedHistoryExerciseCodePath.html";
        final String visitedLinks[] = {"http://foo.com", "http://bar.com", null};
        final String html = "<a src=\"http://foo.com\">foo</a><a src=\"http://bar.com\">bar</a>";

        TestWebServer webServer = null;
        try {
            webServer = new TestWebServer(false);
            final String pageUrl = webServer.setResponse(path, html, null);

            loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);
            assertNotNull(mContentsClient.mGetVisitedHistoryCallback);

            mContentsClient.mGetVisitedHistoryCallback.onReceiveValue(visitedLinks);
            mContentsClient.mGetVisitedHistoryCallback.onReceiveValue(null);

            loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);
        } finally {
            if (webServer != null) webServer.shutdown();
        }
    }

    /*
     * @Feature({"AndroidWebView"})
     * @SmallTest
     * Exercising code after destroy causes gpu related crashes. See crbug.com/172184.
     */
    @DisabledTest
    public void testGetVisitedHistoryCallbackAfterDestroy() throws Throwable {
        mContentsClient.mSaveGetVisitedHistoryCallback = true;
        AwTestContainerView testView = createAwTestContainerViewOnMainSync(mContentsClient);
        AwContents awContents = testView.getAwContents();

        loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(), "about:blank");
        assertNotNull(mContentsClient.mGetVisitedHistoryCallback);

        destroyAwContentsOnMainSync(awContents);
        mContentsClient.mGetVisitedHistoryCallback.onReceiveValue(new String[] {"abc.def"});
        mContentsClient.mGetVisitedHistoryCallback.onReceiveValue(null);
    }

    private void autoLoginTestHelper(final String testName, final String xAutoLoginHeader,
            final String expectedRealm, final String expectedAccount, final String expectedArgs)
            throws Throwable {
        AwTestContainerView testView = createAwTestContainerViewOnMainSync(mContentsClient);
        AwContents awContents = testView.getAwContents();

        final String path = "/" + testName + ".html";
        final String html = testName;
        List<Pair<String, String>> headers = new ArrayList<Pair<String, String>>();
        headers.add(Pair.create("x-auto-login", xAutoLoginHeader));

        TestWebServer webServer = null;
        try {
            webServer = new TestWebServer(false);
            final String pageUrl = webServer.setResponse(path, html, headers);

            loadUrlSync(awContents, mContentsClient.getOnPageFinishedHelper(), pageUrl);

            assertEquals(expectedRealm, mContentsClient.mLastAutoLoginRealm);
            assertEquals(expectedAccount, mContentsClient.mLastAutoLoginAccount);
            assertEquals(expectedArgs, mContentsClient.mLastAutoLoginArgs);
        } finally {
            if (webServer != null) webServer.shutdown();
        }
    }

    @Feature({"AndroidWebView"})
    @SmallTest
    public void testAutoLoginOnGoogleCom() throws Throwable {
        autoLoginTestHelper(
                "testAutoLoginOnGoogleCom",  /* testName */
                "realm=com.google&account=foo%40bar.com&args=random_string", /* xAutoLoginHeader */
                "com.google",  /* expectedRealm */
                "foo@bar.com",  /* expectedAccount */
                "random_string"  /* expectedArgs */);

    }

    @Feature({"AndroidWebView"})
    @SmallTest
    public void testAutoLoginWithNullAccount() throws Throwable {
        autoLoginTestHelper(
                "testAutoLoginOnGoogleCom",  /* testName */
                "realm=com.google&args=not.very.inventive", /* xAutoLoginHeader */
                "com.google",  /* expectedRealm */
                null,  /* expectedAccount */
                "not.very.inventive"  /* expectedArgs */);
    }

    @Feature({"AndroidWebView"})
    @SmallTest
    public void testAutoLoginOnNonGoogle() throws Throwable {
        autoLoginTestHelper(
                "testAutoLoginOnGoogleCom",  /* testName */
                "realm=com.bar&account=foo%40bar.com&args=args", /* xAutoLoginHeader */
                "com.bar",  /* expectedRealm */
                "foo@bar.com",  /* expectedAccount */
                "args"  /* expectedArgs */);
    }
}
