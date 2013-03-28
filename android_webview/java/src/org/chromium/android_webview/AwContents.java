// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.net.http.SslCertificate;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.ValueCallback;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;
import org.chromium.base.ThreadUtils;
import org.chromium.content.browser.ContentSettings;
import org.chromium.content.browser.ContentViewCore;
import org.chromium.content.browser.LoadUrlParams;
import org.chromium.content.browser.NavigationHistory;
import org.chromium.content.browser.PageTransitionTypes;
import org.chromium.content.common.CleanupReference;
import org.chromium.content.components.navigation_interception.InterceptNavigationDelegate;
import org.chromium.net.X509Util;
import org.chromium.ui.gfx.NativeWindow;

import java.io.File;
import java.net.MalformedURLException;
import java.net.URL;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;

/**
 * Exposes the native AwContents class, and together these classes wrap the ContentViewCore
 * and Browser components that are required to implement Android WebView API. This is the
 * primary entry point for the WebViewProvider implementation; it holds a 1:1 object
 * relationship with application WebView instances.
 * (We define this class independent of the hidden WebViewProvider interfaces, to allow
 * continuous build & test in the open source SDK-based tree).
 */
@JNINamespace("android_webview")
public class AwContents {
    private static final String TAG = AwContents.class.getSimpleName();

    private static final String WEB_ARCHIVE_EXTENSION = ".mht";

    /**
     * WebKit hit test related data strcutre. These are used to implement
     * getHitTestResult, requestFocusNodeHref, requestImageRef methods in WebView.
     * All values should be updated together. The native counterpart is
     * AwHitTestData.
     */
    public static class HitTestData {
        // Used in getHitTestResult.
        public int hitTestResultType;
        public String hitTestResultExtraData;

        // Used in requestFocusNodeHref (all three) and requestImageRef (only imgSrc).
        public String href;
        public String anchorText;
        public String imgSrc;
    }

    private int mNativeAwContents;
    private ViewGroup mContainerView;
    private ContentViewCore mContentViewCore;
    private AwContentsClient mContentsClient;
    private AwContentsIoThreadClient mIoThreadClient;
    private InterceptNavigationDelegateImpl mInterceptNavigationDelegate;
    private ContentViewCore.InternalAccessDelegate mInternalAccessAdapter;
    // This can be accessed on any thread after construction. See AwContentsIoThreadClient.
    private final AwSettings mSettings;
    private final ClientCallbackHandler mClientCallbackHandler;
    private boolean mIsPaused;

    // Must call nativeUpdateLastHitTestData first to update this before use.
    private final HitTestData mPossiblyStaleHitTestData;

    private static final class DestroyRunnable implements Runnable {
        private int mNativeAwContents;
        private DestroyRunnable(int nativeAwContents) {
            mNativeAwContents = nativeAwContents;
        }
        @Override
        public void run() {
            nativeDestroy(mNativeAwContents);
        }
    }

    private CleanupReference mCleanupReference;

    private static class DownloadInfo {
        final String mUrl;
        final String mUserAgent;
        final String mContentDisposition;
        final String mMimeType;
        final long mContentLength;

        DownloadInfo(String url,
                     String userAgent,
                     String contentDisposition,
                     String mimeType,
                     long contentLength) {
            mUrl = url;
            mUserAgent = userAgent;
            mContentDisposition = contentDisposition;
            mMimeType = mimeType;
            mContentLength = contentLength;
        }
    }

    // This class is responsible for calling certain client callbacks on the UI thread. Most
    // callbacks do no go through here, but get forwarded to AwContentsClient directly.
    // The messages processed here may originate from the IO or UI thread.
    // TODO(mkosiba): merge the handler in AwContentsClient.WebContentsDelegateAdapter into this.
    private class ClientCallbackHandler extends Handler {
        public static final int MSG_ON_LOAD_RESOURCE = 1;
        public static final int MSG_ON_PAGE_STARTED = 2;
        public static final int MSG_ON_DOWNLOAD_START = 3;

        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
                case MSG_ON_LOAD_RESOURCE: {
                    final String url = (String) msg.obj;
                    AwContents.this.mContentsClient.onLoadResource(url);
                    break;
                }
                case MSG_ON_PAGE_STARTED: {
                    final String url = (String) msg.obj;
                    AwContents.this.mContentsClient.onPageStarted(url);
                    break;
                }
                case MSG_ON_DOWNLOAD_START: {
                    DownloadInfo info = (DownloadInfo)msg.obj;
                    AwContents.this.mContentsClient.onDownloadStart(info.mUrl,
                                                                    info.mUserAgent,
                                                                    info.mContentDisposition,
                                                                    info.mMimeType,
                                                                    info.mContentLength);
                    break;
                }
                default:
                    throw new IllegalStateException(
                            "IoThreadClientHandler: unhandled message " + msg.what);
            }
        }
    }

    private class IoThreadClientImpl implements AwContentsIoThreadClient {
        // All methods are called on the IO thread.

        @Override
        public int getCacheMode() {
            return AwContents.this.mSettings.getCacheMode();
        }

        @Override
        public InterceptedRequestData shouldInterceptRequest(final String url) {
            InterceptedRequestData interceptedRequestData =
                AwContents.this.mContentsClient.shouldInterceptRequest(url);
            if (interceptedRequestData == null) {
                mClientCallbackHandler.sendMessage(
                        mClientCallbackHandler.obtainMessage(
                            ClientCallbackHandler.MSG_ON_LOAD_RESOURCE,
                            url));
            }
            return interceptedRequestData;
        }

        @Override
        public boolean shouldBlockContentUrls() {
            return !AwContents.this.mSettings.getAllowContentAccess();
        }

        @Override
        public boolean shouldBlockFileUrls() {
            return !AwContents.this.mSettings.getAllowFileAccess();
        }

        @Override
        public boolean shouldBlockNetworkLoads() {
            return AwContents.this.mSettings.getBlockNetworkLoads();
        }

        @Override
        public void onDownloadStart(String url,
                                    String userAgent,
                                    String contentDisposition,
                                    String mimeType,
                                    long contentLength) {
            DownloadInfo info = new DownloadInfo(url,
                                                 userAgent,
                                                 contentDisposition,
                                                 mimeType,
                                                 contentLength);
            mClientCallbackHandler.sendMessage(
                    mClientCallbackHandler.obtainMessage(
                          ClientCallbackHandler.MSG_ON_DOWNLOAD_START,
                          info));
        }
    }

    private class InterceptNavigationDelegateImpl implements InterceptNavigationDelegate {
        private String mLastLoadUrlAddress;

        public void onUrlLoadRequested(String url) {
            mLastLoadUrlAddress = url;
        }

        @Override
        public boolean shouldIgnoreNavigation(String url, boolean isPost, boolean hasUserGesture,
                                              int pageTransition) {
            boolean ignoreNavigation = false;
            if (mLastLoadUrlAddress != null && mLastLoadUrlAddress.equals(url)) {
                // Support the case where the user clicks on a link that takes them back to the
                // same page.
                mLastLoadUrlAddress = null;

                // If the embedder requested the load of a certain URL via the loadUrl API, then we
                // do not offer it to AwContentsClient.shouldIgnoreNavigation.
                // The embedder is also not allowed to intercept POST requests because of
                // crbug.com/155250.
            } else if (!isPost) {
                ignoreNavigation = AwContents.this.mContentsClient.shouldIgnoreNavigation(url);
            }

            // The existing contract is that shouldIgnoreNavigation callbacks are delivered before
            // onPageStarted callbacks; third party apps depend on this behavior.
            // Using a ResouceThrottle to implement the navigation interception feature results in
            // the WebContentsObserver.didStartLoading callback happening before the
            // ResourceThrottle has a chance to run.
            // To preserve the ordering the onPageStarted callback is synthesized from the
            // shouldIgnoreNavigationCallback, and only if the navigation was not ignored (this
            // balances out with the onPageFinished callback, which is suppressed in the
            // AwContentsClient if the navigation was ignored).
            if (!ignoreNavigation) {
                // The shouldIgnoreNavigation call might have resulted in posting messages to the
                // UI thread. Using sendMessage here (instead of calling onPageStarted directly)
                // will allow those to run.
                mClientCallbackHandler.sendMessage(
                        mClientCallbackHandler.obtainMessage(
                            ClientCallbackHandler.MSG_ON_PAGE_STARTED,
                            url));
            }

            return ignoreNavigation;
        }
    }

    /**
     * @param containerView the view-hierarchy item this object will be bound to.
     * @param internalAccessAdapter to access private methods on containerView.
     * @param contentsClient will receive API callbacks from this WebView Contents
     * @param privateBrowsing whether this is a private browsing instance of WebView.
     * @param isAccessFromFileURLsGrantedByDefault passed to ContentViewCore.initialize.
     * TODO(benm): Remove the nativeWindow parameter.
     */
    public AwContents(ViewGroup containerView,
            ContentViewCore.InternalAccessDelegate internalAccessAdapter,
            AwContentsClient contentsClient,
            NativeWindow nativeWindow, boolean privateBrowsing,
            boolean isAccessFromFileURLsGrantedByDefault) {
        mContainerView = containerView;
        mInternalAccessAdapter = internalAccessAdapter;
        // Note that ContentViewCore must be set up before AwContents, as ContentViewCore
        // setup performs process initialisation work needed by AwContents.
        mContentViewCore = new ContentViewCore(containerView.getContext(),
                ContentViewCore.PERSONALITY_VIEW);
        mNativeAwContents = nativeInit(contentsClient.getWebContentsDelegate(), privateBrowsing);
        mContentsClient = contentsClient;
        mCleanupReference = new CleanupReference(this, new DestroyRunnable(mNativeAwContents));
        mClientCallbackHandler = new ClientCallbackHandler();

        mContentViewCore.initialize(containerView, internalAccessAdapter,
                nativeGetWebContents(mNativeAwContents),
                new AwNativeWindow(mContainerView.getContext()),
                isAccessFromFileURLsGrantedByDefault);
        mContentViewCore.setContentViewClient(mContentsClient);
        mContentsClient.installWebContentsObserver(mContentViewCore);

        mSettings = new AwSettings(mContentViewCore.getContext());
        setIoThreadClient(new IoThreadClientImpl());
        setInterceptNavigationDelegate(new InterceptNavigationDelegateImpl());

        mPossiblyStaleHitTestData = new HitTestData();
        nativeDidInitializeContentViewCore(mNativeAwContents,
                mContentViewCore.getNativeContentViewCore());
    }

    public ContentViewCore getContentViewCore() {
        return mContentViewCore;
    }

    // Can be called from any thread.
    public AwSettings getSettings() {
        return mSettings;
    }

    public void setIoThreadClient(AwContentsIoThreadClient ioThreadClient) {
        mIoThreadClient = ioThreadClient;
        nativeSetIoThreadClient(mNativeAwContents, mIoThreadClient);
    }

    private void setInterceptNavigationDelegate(InterceptNavigationDelegateImpl delegate) {
        mInterceptNavigationDelegate = delegate;
        nativeSetInterceptNavigationDelegate(mNativeAwContents, delegate);
    }

    public void destroy() {
        mContentViewCore.destroy();
        // We explicitly do not null out the mContentViewCore reference here
        // because ContentViewCore already has code to deal with the case
        // methods are called on it after it's been destroyed, and other
        // code relies on AwContents.getContentViewCore to return non-null.
        mCleanupReference.cleanupNow();
        mNativeAwContents = 0;
    }

    public static int getAwDrawGLFunction() {
        return nativeGetAwDrawGLFunction();
    }

    public int getAwDrawGLViewContext() {
        // Using the native pointer as the returned viewContext. This is matched by the
        // reinterpret_cast back to AwContents pointer in the native DrawGLFunction.
        return mNativeAwContents;
    }

    public boolean onPrepareDrawGL(Canvas canvas) {
        // TODO(joth): Ensure the HW path is setup and read any required params out of canvas.
        Log.e(TAG, "Not implemented: AwContents.onPrepareDrawGL()");

        // returning false will cause a fallback to SW path.
        return true;
    }

    public void onDraw(Canvas canvas) {
        // TODO(joth): Implement. For now, just clear the canvas to red.
        canvas.drawRGB(200, 1, 4);
    }

    public int findAllSync(String searchString) {
        if (mNativeAwContents == 0) return 0;
        return nativeFindAllSync(mNativeAwContents, searchString);
    }

    public void findAllAsync(String searchString) {
        if (mNativeAwContents == 0) return;
        nativeFindAllAsync(mNativeAwContents, searchString);
    }

    public void findNext(boolean forward) {
        if (mNativeAwContents == 0) return;
        nativeFindNext(mNativeAwContents, forward);
    }

    public void clearMatches() {
        if (mNativeAwContents == 0) return;
        nativeClearMatches(mNativeAwContents);
    }

    /**
     * @return load progress of the WebContents.
     */
    public int getMostRecentProgress() {
        // WebContentsDelegateAndroid conveniently caches the most recent notified value for us.
        return mContentsClient.getWebContentsDelegate().getMostRecentProgress();
    }

    public Bitmap getFavicon() {
        // To be implemented.
        return null;
    }

    /**
     * Load url without fixing up the url string. Consumers of ContentView are responsible for
     * ensuring the URL passed in is properly formatted (i.e. the scheme has been added if left
     * off during user input).
     *
     * @param pararms Parameters for this load.
     */
    public void loadUrl(LoadUrlParams params) {
        if (params.getLoadUrlType() == LoadUrlParams.LOAD_TYPE_DATA &&
            !params.isBaseUrlDataScheme()) {
            // This allows data URLs with a non-data base URL access to file:///android_asset/ and
            // file:///android_res/ URLs. If AwSettings.getAllowFileAccess permits, it will also
            // allow access to file:// URLs (subject to OS level permission checks).
            params.setCanLoadLocalResources(true);
        }

        // If we are reloading the same url, then set transition type as reload.
        if (params.getUrl() != null &&
            params.getUrl().equals(mContentViewCore.getUrl()) &&
            params.getTransitionType() == PageTransitionTypes.PAGE_TRANSITION_LINK) {
            params.setTransitionType(PageTransitionTypes.PAGE_TRANSITION_RELOAD);
        }

        mContentViewCore.loadUrl(params);

        if (mInterceptNavigationDelegate != null) {
            // getUrl returns a sanitized address in the same format that will be used for
            // callbacks, so it's safe to use string comparison as an equality check later on.
            mInterceptNavigationDelegate.onUrlLoadRequested(mContentViewCore.getUrl());
        }
    }

    /**
     * Called on the "source" AwContents that is opening the popup window to
     * provide the AwContents to host the pop up content.
     */
    public void supplyContentsForPopup(AwContents newContents) {
        int popupWebContents = nativeReleasePopupWebContents(mNativeAwContents);
        assert popupWebContents != 0;
        newContents.setNewWebContents(popupWebContents);
    }

    private void setNewWebContents(int newWebContentsPtr) {
        // When setting a new WebContents, we new up a ContentViewCore that will
        // wrap it and then swap it.
        ContentViewCore newCore = new ContentViewCore(mContainerView.getContext(),
                ContentViewCore.PERSONALITY_VIEW);
        // Note we pass false for isAccessFromFileURLsGrantedByDefault as we'll
        // set it correctly when when we copy the settings from the old ContentViewCore
        // into the new one.
        newCore.initialize(mContainerView, mInternalAccessAdapter,
                newWebContentsPtr, new AwNativeWindow(mContainerView.getContext()),
                false);
        newCore.setContentViewClient(mContentsClient);
        mContentsClient.installWebContentsObserver(newCore);

        ContentSettings oldSettings = mContentViewCore.getContentSettings();
        newCore.getContentSettings().initFrom(oldSettings);

        // Now swap the Java side reference.
        mContentViewCore.destroy();
        mContentViewCore = newCore;

        // Now rewire native side to use the new WebContents.
        nativeSetWebContents(mNativeAwContents, newWebContentsPtr);
        nativeSetIoThreadClient(mNativeAwContents, mIoThreadClient);
        nativeSetInterceptNavigationDelegate(mNativeAwContents, mInterceptNavigationDelegate);

        // Finally poke the new ContentViewCore with the size of the container view and show it.
        if (mContainerView.getWidth() != 0 || mContainerView.getHeight() != 0) {
            mContentViewCore.onSizeChanged(
                    mContainerView.getWidth(), mContainerView.getHeight(), 0, 0);
        }
        nativeDidInitializeContentViewCore(mNativeAwContents,
                mContentViewCore.getNativeContentViewCore());
        if (mContainerView.getVisibility() == View.VISIBLE) {
            // The popup window was hidden when we prompted the embedder to display
            // it, so show it again now we have a container.
            mContentViewCore.onShow();
        }
    }

    //--------------------------------------------------------------------------------------------
    //  WebView[Provider] method implementations (where not provided by ContentViewCore)
    //--------------------------------------------------------------------------------------------

    /**
     * @see android.webkit.WebView#pauseTimers()
     */
    public void pauseTimers() {
        mContentViewCore.onActivityPause();
    }

    /**
     * @see android.webkit.WebView#resumeTimers()
     */
    public void resumeTimers() {
        mContentViewCore.onActivityResume();
    }

    /**
     * @see android.webkit.WebView#onPause()
     */
    public void onPause() {
        mIsPaused = true;
        mContentViewCore.onHide();
    }

    /**
     * @see android.webkit.WebView#onResume()
     */
    public void onResume() {
        mContentViewCore.onShow();
        mIsPaused = false;
    }

    /**
     * @see android.webkit.WebView#isPaused()
     */
    public boolean isPaused() {
        return mIsPaused;
    }

    /**
     * Clears the resource cache. Note that the cache is per-application, so this will clear the
     * cache for all WebViews used.
     *
     * @param includeDiskFiles if false, only the RAM cache is cleared
     */
    public void clearCache(boolean includeDiskFiles) {
        if (mNativeAwContents == 0) return;
        nativeClearCache(mNativeAwContents, includeDiskFiles);
    }

    public void documentHasImages(Message message) {
        if (mNativeAwContents == 0) return;
        nativeDocumentHasImages(mNativeAwContents, message);
    }

    public void saveWebArchive(
            final String basename, boolean autoname, final ValueCallback<String> callback) {
        if (!autoname) {
            saveWebArchiveInternal(basename, callback);
            return;
        }
        // If auto-generating the file name, handle the name generation on a background thread
        // as it will require I/O access for checking whether previous files existed.
        new AsyncTask<Void, Void, String>() {
            @Override
            protected String doInBackground(Void... params) {
                return generateArchiveAutoNamePath(getOriginalUrl(), basename);
            }

            @Override
            protected void onPostExecute(String result) {
                saveWebArchiveInternal(result, callback);
            }
        }.execute();
    }

    public String getOriginalUrl() {
        NavigationHistory history = mContentViewCore.getNavigationHistory();
        int currentIndex = history.getCurrentEntryIndex();
        if (currentIndex >= 0 && currentIndex < history.getEntryCount()) {
            return history.getEntryAtIndex(currentIndex).getOriginalUrl();
        }
        return null;
    }

    public String[] getHttpAuthUsernamePassword(String host, String realm) {
        return HttpAuthDatabase.getInstance(mContentViewCore.getContext())
                .getHttpAuthUsernamePassword(host, realm);
    }

    public void setHttpAuthUsernamePassword(String host, String realm, String username,
            String password) {
        HttpAuthDatabase.getInstance(mContentViewCore.getContext())
                .setHttpAuthUsernamePassword(host, realm, username, password);
    }

    /**
     * @see android.webkit.WebView#getCertificate()
     */
    public SslCertificate getCertificate() {
        if (mNativeAwContents == 0) return null;
        byte[] derBytes = nativeGetCertificate(mNativeAwContents);
        if (derBytes == null) {
            return null;
        }

        try {
            X509Certificate x509Certificate =
                    X509Util.createCertificateFromBytes(derBytes);
            return new SslCertificate(x509Certificate);
        } catch (CertificateException e) {
            // Intentional fall through
            // A SSL related exception must have occured.  This shouldn't happen.
            Log.w(TAG, "Could not read certificate: " + e);
        } catch (KeyStoreException e) {
            // Intentional fall through
            // A SSL related exception must have occured.  This shouldn't happen.
            Log.w(TAG, "Could not read certificate: " + e);
        } catch (NoSuchAlgorithmException e) {
            // Intentional fall through
            // A SSL related exception must have occured.  This shouldn't happen.
            Log.w(TAG, "Could not read certificate: " + e);
        }
        return null;
    }

    /**
     * Method to return all hit test values relevant to public WebView API.
     * Note that this expose more data than needed for WebView.getHitTestResult.
     * Unsafely returning reference to mutable internal object to avoid excessive
     * garbage allocation on repeated calls.
     */
    public HitTestData getLastHitTestResult() {
        if (mNativeAwContents == 0) return null;
        nativeUpdateLastHitTestData(mNativeAwContents);
        return mPossiblyStaleHitTestData;
    }

    /**
     * @see android.webkit.WebView#requestFocusNodeHref()
     */
    public void requestFocusNodeHref(Message msg) {
        if (msg == null || mNativeAwContents == 0) return;

        nativeUpdateLastHitTestData(mNativeAwContents);
        Bundle data = msg.getData();
        data.putString("url", mPossiblyStaleHitTestData.href);
        data.putString("title", mPossiblyStaleHitTestData.anchorText);
        data.putString("src", mPossiblyStaleHitTestData.imgSrc);
        msg.setData(data);
        msg.sendToTarget();
    }

    /**
     * @see android.webkit.WebView#requestImageRef()
     */
    public void requestImageRef(Message msg) {
        if (msg == null || mNativeAwContents == 0) return;

        nativeUpdateLastHitTestData(mNativeAwContents);
        Bundle data = msg.getData();
        data.putString("url", mPossiblyStaleHitTestData.imgSrc);
        msg.setData(data);
        msg.sendToTarget();
    }

    //--------------------------------------------------------------------------------------------
    //  View and ViewGroup method implementations
    //--------------------------------------------------------------------------------------------

    /**
     * @see android.webkit.View#onTouchEvent()
     */
    public boolean onTouchEvent(MotionEvent event) {
        if (mNativeAwContents == 0) return false;
        boolean rv = mContentViewCore.onTouchEvent(event);

        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            int actionIndex = event.getActionIndex();

            // Note this will trigger IPC back to browser even if nothing is hit.
            nativeRequestNewHitTestDataAt(mNativeAwContents,
                                          Math.round(event.getX(actionIndex)),
                                          Math.round(event.getY(actionIndex)));
        }

        return rv;
    }

    /**
     * @see android.view.View#onHoverEvent()
     */
    public boolean onHoverEvent(MotionEvent event) {
        return mContentViewCore.onHoverEvent(event);
    }

    /**
     * @see android.view.View#onGenericMotionEvent()
     */
    public boolean onGenericMotionEvent(MotionEvent event) {
        return mContentViewCore.onGenericMotionEvent(event);
    }

    /**
     * @see android.view.View#onConfigurationChanged()
     */
    public void onConfigurationChanged(Configuration newConfig) {
        mContentViewCore.onConfigurationChanged(newConfig);
    }

    /**
     * @see android.view.View#onAttachedToWindow()
     */
    public void onAttachedToWindow() {
        mContentViewCore.onAttachedToWindow();
        nativeOnAttachedToWindow(mNativeAwContents, mContainerView.getWidth(),
                mContainerView.getHeight());

        // This is for the case where this is created by restoreState, which
        // needs to call to NavigationController::LoadIfNecessary to actually
        // load the restored page.
        if (!mIsPaused) onResume();
    }

    /**
     * @see android.view.View#onDetachedFromWindow()
     */
    public void onDetachedFromWindow() {
        if (mNativeAwContents == 0) return;
        nativeOnDetachedFromWindow(mNativeAwContents);
        mContentViewCore.onDetachedFromWindow();
    }

    /**
     * @see android.view.View#onWindowFocusChanged()
     */
    public void onWindowFocusChanged(boolean hasWindowFocus) {
    }

    /**
     * @see android.view.View#onFocusChanged()
     */
    public void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
        mContentViewCore.onFocusChanged(focused, direction, previouslyFocusedRect);
    }

    /**
     * @see android.view.View#onSizeChanged()
     */
    public void onSizeChanged(int w, int h, int ow, int oh) {
        if (mNativeAwContents == 0) return;
        mContentViewCore.onSizeChanged(w, h, ow, oh);
        nativeOnSizeChanged(mNativeAwContents, w, h, ow, oh);
    }

    /**
     * @see android.view.View#onVisibilityChanged()
     */
    public void onVisibilityChanged(View changedView, int visibility) {
        updateVisiblityState();
    }

    /**
     * @see android.view.View#onWindowVisibilityChanged()
     */
    public void onWindowVisibilityChanged(int visibility) {
        updateVisiblityState();
    }

    private void updateVisiblityState() {
        if (mNativeAwContents == 0 || mIsPaused) return;
        boolean windowVisible = mContainerView.getWindowVisibility() == View.VISIBLE;
        boolean viewVisible = mContainerView.getVisibility() == View.VISIBLE;
        nativeSetWindowViewVisibility(mNativeAwContents, windowVisible, viewVisible);

        if (viewVisible) {
          mContentViewCore.onShow();
        } else {
          mContentViewCore.onHide();
        }
    }


    /**
     * Key for opaque state in bundle. Note this is only public for tests.
     */
    public static final String SAVE_RESTORE_STATE_KEY = "WEBVIEW_CHROMIUM_STATE";

    /**
     * Save the state of this AwContents into provided Bundle.
     * @return False if saving state failed.
     */
    public boolean saveState(Bundle outState) {
        if (outState == null) return false;

        byte[] state = nativeGetOpaqueState(mNativeAwContents);
        if (state == null) return false;

        outState.putByteArray(SAVE_RESTORE_STATE_KEY, state);
        return true;
    }

    /**
     * Restore the state of this AwContents into provided Bundle.
     * @param inState Must be a bundle returned by saveState.
     * @return False if restoring state failed.
     */
    public boolean restoreState(Bundle inState) {
        if (inState == null) return false;

        byte[] state = inState.getByteArray(SAVE_RESTORE_STATE_KEY);
        if (state == null) return false;

        boolean result = nativeRestoreFromOpaqueState(mNativeAwContents, state);

        // The onUpdateTitle callback normally happens when a page is loaded,
        // but is optimized out in the restoreState case because the title is
        // already restored. See WebContentsImpl::UpdateTitleForEntry. So we
        // call the callback explicitly here.
        if (result) mContentsClient.onUpdateTitle(mContentViewCore.getTitle());

        return result;
    }

    //--------------------------------------------------------------------------------------------
    //  Methods called from native via JNI
    //--------------------------------------------------------------------------------------------

    @CalledByNative
    private static void onDocumentHasImagesResponse(boolean result, Message message) {
        message.arg1 = result ? 1 : 0;
        message.sendToTarget();
    }

    /** Callback for generateMHTML. */
    @CalledByNative
    private static void generateMHTMLCallback(
            String path, long size, ValueCallback<String> callback) {
        if (callback == null) return;
        callback.onReceiveValue(size < 0 ? null : path);
    }

    @CalledByNative
    private void onReceivedHttpAuthRequest(AwHttpAuthHandler handler, String host, String realm) {
        mContentsClient.onReceivedHttpAuthRequest(handler, host, realm);
    }

    @CalledByNative
    public void onFindResultReceived(int activeMatchOrdinal, int numberOfMatches,
            boolean isDoneCounting) {
        mContentsClient.onFindResultReceived(activeMatchOrdinal, numberOfMatches, isDoneCounting);
    }

    // Called as a result of nativeUpdateLastHitTestData.
    @CalledByNative
    private void updateHitTestData(
            int type, String extra, String href, String anchorText, String imgSrc) {
        mPossiblyStaleHitTestData.hitTestResultType = type;
        mPossiblyStaleHitTestData.hitTestResultExtraData = extra;
        mPossiblyStaleHitTestData.href = href;
        mPossiblyStaleHitTestData.anchorText = anchorText;
        mPossiblyStaleHitTestData.imgSrc = imgSrc;
    }

    @CalledByNative
    private void invalidate() {
        mContainerView.invalidate();
    }

    @CalledByNative
    private boolean performLongClick() {
        return mContainerView.performLongClick();
    }

    // -------------------------------------------------------------------------------------------
    // Helper methods
    // -------------------------------------------------------------------------------------------

    private void saveWebArchiveInternal(String path, final ValueCallback<String> callback) {
        if (path == null || mNativeAwContents == 0) {
            ThreadUtils.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    callback.onReceiveValue(null);
                }
            });
        } else {
            nativeGenerateMHTML(mNativeAwContents, path, callback);
        }
    }

    /**
     * Try to generate a pathname for saving an MHTML archive. This roughly follows WebView's
     * autoname logic.
     */
    private static String generateArchiveAutoNamePath(String originalUrl, String baseName) {
        String name = null;
        if (originalUrl != null && !originalUrl.isEmpty()) {
            try {
                String path = new URL(originalUrl).getPath();
                int lastSlash = path.lastIndexOf('/');
                if (lastSlash > 0) {
                    name = path.substring(lastSlash + 1);
                } else {
                    name = path;
                }
            } catch (MalformedURLException e) {
                // If it fails parsing the URL, we'll just rely on the default name below.
            }
        }

        if (TextUtils.isEmpty(name)) name = "index";

        String testName = baseName + name + WEB_ARCHIVE_EXTENSION;
        if (!new File(testName).exists()) return testName;

        for (int i = 1; i < 100; i++) {
            testName = baseName + name + "-" + i + WEB_ARCHIVE_EXTENSION;
            if (!new File(testName).exists()) return testName;
        }

        Log.e(TAG, "Unable to auto generate archive name for path: " + baseName);
        return null;
    }

    @CalledByNative
    private void handleJsAlert(String url, String message, JsResultReceiver receiver) {
        mContentsClient.handleJsAlert(url, message, receiver);
    }

    @CalledByNative
    private void handleJsBeforeUnload(String url, String message, JsResultReceiver receiver) {
        mContentsClient.handleJsBeforeUnload(url, message, receiver);
    }

    @CalledByNative
    private void handleJsConfirm(String url, String message, JsResultReceiver receiver) {
        mContentsClient.handleJsConfirm(url, message, receiver);
    }

    @CalledByNative
    private void handleJsPrompt(String url, String message, String defaultValue,
            JsPromptResultReceiver receiver) {
        mContentsClient.handleJsPrompt(url, message, defaultValue, receiver);
    }

    //--------------------------------------------------------------------------------------------
    //  Native methods
    //--------------------------------------------------------------------------------------------

    private native int nativeInit(AwWebContentsDelegate webViewWebContentsDelegate,
            boolean privateBrowsing);
    private static native void nativeDestroy(int nativeAwContents);
    private static native int nativeGetAwDrawGLFunction();

    private native int nativeGetWebContents(int nativeAwContents);
    private native void nativeDidInitializeContentViewCore(int nativeAwContents,
            int nativeContentViewCore);

    private native void nativeDocumentHasImages(int nativeAwContents, Message message);
    private native void nativeGenerateMHTML(
            int nativeAwContents, String path, ValueCallback<String> callback);

    private native void nativeSetIoThreadClient(int nativeAwContents,
            AwContentsIoThreadClient ioThreadClient);
    private native void nativeSetInterceptNavigationDelegate(int nativeAwContents,
            InterceptNavigationDelegate navigationInterceptionDelegate);

    private native int nativeFindAllSync(int nativeAwContents, String searchString);
    private native void nativeFindAllAsync(int nativeAwContents, String searchString);
    private native void nativeFindNext(int nativeAwContents, boolean forward);
    private native void nativeClearMatches(int nativeAwContents);
    private native void nativeClearCache(int nativeAwContents, boolean includeDiskFiles);
    private native byte[] nativeGetCertificate(int nativeAwContents);
    private native void nativeRequestNewHitTestDataAt(int nativeAwContents, int x, int y);
    private native void nativeUpdateLastHitTestData(int nativeAwContents);
    private native void nativeOnSizeChanged(int nativeAwContents, int w, int h, int ow, int oh);
    private native void nativeSetWindowViewVisibility(int nativeAwContents, boolean windowVisible,
            boolean viewVisible);
    private native void nativeOnAttachedToWindow(int nativeAwContents, int w, int h);
    private native void nativeOnDetachedFromWindow(int nativeAwContents);

    // Returns null if save state fails.
    private native byte[] nativeGetOpaqueState(int nativeAwContents);

    // Returns false if restore state fails.
    private native boolean nativeRestoreFromOpaqueState(int nativeAwContents, byte[] state);

    private native int nativeReleasePopupWebContents(int nativeAwContents);
    private native void nativeSetWebContents(int nativeAwContents, int nativeNewWebContents);
}
