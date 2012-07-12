// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome;

import static com.google.android.apps.chrome.ChromeNotificationCenter.broadcastNotification;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.TextureView;
import android.view.View;

import com.google.android.apps.chrome.NewTabPageUtil.NTPSection;
import com.google.android.apps.chrome.TabModel.TabLaunchType;
import com.google.android.apps.chrome.crash.MinidumpPreparationService;
import com.google.android.apps.chrome.infobar.InfoBar;
import com.google.android.apps.chrome.infobar.InfoBarContainer;
import com.google.android.apps.chrome.infobar.InfoBarDismissedListener;
import com.google.android.apps.chrome.infobar.MessageInfoBar;
import com.google.android.apps.chrome.infobar.PopupBlockedInfoBar;
import com.google.android.apps.chrome.infobar.PopupBlockedInfoBarListener;
import com.google.android.apps.chrome.infobar.SnapshotDownloadSuceededInfobar;
import com.google.android.apps.chrome.infobar.SnapshotDownloadingInfobar;
import com.google.android.apps.chrome.snapshot.SnapshotArchiveManager;
import com.google.android.apps.chrome.snapshot.SnapshotDocument;
import com.google.android.apps.chrome.snapshot.SnapshotViewableState;
import com.google.android.apps.chrome.uma.UmaSessionStats;
import com.google.android.apps.chrome.utilities.MathUtils;
import com.google.android.apps.chrome.utilities.StreamUtils;
import com.google.android.apps.chrome.utilities.URLUtilities;

import org.chromium.base.AccessedByNative;
import org.chromium.base.CalledByNative;
import org.chromium.content.browser.ChromeHttpAuthHandler;
import org.chromium.content.browser.ChromeView;
import org.chromium.content.browser.ChromeViewClient;
import org.chromium.content.browser.SelectFileDialog;
import org.chromium.content.browser.TraceEvent;

import com.google.common.annotations.VisibleForTesting;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URISyntaxException;
import java.security.InvalidKeyException;
import java.security.Key;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.List;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.FutureTask;

import javax.crypto.Cipher;
import javax.crypto.CipherInputStream;
import javax.crypto.CipherOutputStream;
import javax.crypto.KeyGenerator;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.SecretKey;
import javax.crypto.spec.SecretKeySpec;

public class Tab implements PopupBlockedInfoBarListener, TabThumbnailProvider {
    public static final int INVALID_TAB_ID = -1;
    public static final int NTP_TAB_ID = -2;

    private static final String TAG = Tab.class.getName();

    public static final int INVALID_RENDER_PROCESS_PID = Integer.MIN_VALUE;

    public static final String SAVED_STATE_FILE_PREFIX = "tab";
    public static final String SAVED_STATE_FILE_PREFIX_INCOGNITO = "cryptonito";

    /** The height and width of the loaded favicon. */
    private static final int FAVICON_DIMENSION = 16;

    /**
     * Use INVALID_TAB_ID
     */
    @Deprecated
    public static final int NO_PARENT_ID = INVALID_TAB_ID;

    static final long KEY_CHECKER = 0;

    // These should match the css/js values from the new tab page
    private static final int THUMBNAIL_W_TABLET = 230;
    private static final int THUMBNAIL_H_TABLET = 160;
    private static final int THUMBNAIL_W_PHONE = 108;
    private static final int THUMBNAIL_H_PHONE = 72;

    public static final String CHROME_SCHEME = "chrome://";

    /** If a URL starts with this, it is a snapshot. */
    private static final String SNAPSHOT_PREFIX = "chrome://snapshot/";
    private static final String PRINT_JOB_PREFIX = "chrome://printjob/";

    private static FutureTask<SecretKey> sIncognitoKeyGenerator;
    private static Key mKey = null;

    /** Native Tab pointer which will be set by nativeInit(). */
    @AccessedByNative
    private int mNativeTab = 0;

    /** Unique id of this tab (within its container). */
    private final int mId;

    /** The native window id in which this tab resides. Used by session sync */
    private int mWindowId;

    private final boolean mIncognito;

    /** Title of the ChromeViews webpage. */
    private String mTitle;

    /**
     * Url of the page currenly loading. Used as a fall-back in case tab restore fails.
     */
    private String mUrl;

    /**
     * Base URL when the main frame was committed. Used as the original URL for
     * snapshots.
     */
    private String mBaseUrl;

    private ChromeView mChromeView;

    private TabHost mTabHost;

    private boolean mIsHidden = true;

    /**
     * This byte array contains the state of this tab as returned by
     * nativeGetStateAsByteArray. It is non null if the state of this tab has
     * been saved and its ChromeView destroyed in order to save memory. The tab
     * can be restored by calling restoreState().
     */
    private byte[] mSavedState;

    private boolean mIsTabStateDirty = true;

    private ChromeViewClient mChromeViewClient;

    private InfoBarContainer mInfoBarContainer;

    /** Timestamp representing the last time this tab was shown. */
    private long mLastShownTimestamp = -1;

    /** Context for the application */
    private final Context mContext;

    /**
     * Favicon for the tab.
     */
    private Bitmap mCachedFavicon;

    /**
     * The URL associated with the favicon.
     */
    private String mCachedFaviconUrl;

    /**
     * Saves how this tab was launched (from a link, external app, etc) so that
     * we can determine the different circumstances in which it should be
     * closed. For example, a tab opened from an external app should be closed
     * when the back stack is empty and the user uses the back hardware key. A
     * standard tab however should be kept open and the entire activity should
     * be moved to the background.
     */
    private TabLaunchType mLaunchType;

    /**
     * If this tab was opened from another tab, store the id of the tab that
     * caused it to be opened so that we can activate it when this tab gets
     * closed.
     */
    private int mParentId = NO_PARENT_ID;

    private boolean mParentIsIncognito;

    /**
     * If this tab has crashed while being a background tab (e.g. it could be
     * killed due to low memory conditions) we will reload it on the next show.
     */
    private boolean mTabCrashedInBackground;

    private PopupBlockedInfoBar mPopupInfoBar;

    /**
     * Set to true to avoid launching new content intents during the testing.
     * The associated notification will be broadcasted in any case.
     */
    private boolean mDisableContentIntentsForTests;

    public interface FindMatchRectsListener {
        public void onReceiveFindMatchRects(int version, RectF[] rects, RectF activeRect);
    }

    private FindMatchRectsListener mFindMatchRectsListener = null;

    /**
     * When not null, indicates that the current page has a snapshot associated
     * with it.
     */
    private String mSnapshotId = null;

    /**
     * Holds the infobar for snapshots currently being downloaded. Cleared on
     * dismiss.
     */
    private SnapshotDownloadingInfobar mSnapshotDownloadingInfoBar = null;

    /**
     * Holds the infobar for snapshots that failed to download. Cleared on
     * dismiss.
     */
    private MessageInfoBar mSnapshotDownloadErrorInfoBar;

    // This is similar to the parent id, but it can be modified based on tab
    // open/close events.
    private int mOpenerTabId;

    // The external application that this tab is associated with (null if not
    // associated with any
    // app). Allows reusing of tabs opened from the same application.
    private String mAppAssociatedWith;

    private Activity mActivity;

    private Handler mHandler;

    private Runnable mThumbnailRunnable = new Runnable() {
        @Override
        public void run() {
            if (isHidden() ||
                    !canUpdateHistoryThumbnail() ||
                    !shouldUpdateThumbnail()) {
                return;
            }

            // Scale the image so we're not copying more than we need to (from
            // the GPU).
            int[] dim = new int[] {
                getWidth(), getHeight()
            };
            MathUtils.scaleToFitTargetSize(dim, mThumbnailWidth, mThumbnailHeight);

            Bitmap bitmap = null;
            try {
                bitmap = mChromeView.getBitmap(dim[0], dim[1]);
            } catch (OutOfMemoryError e) {
                Log.w(TAG, "OutOfMemoryError thrown while trying to fetch a tab bitmap.", e);
                ChromeNotificationCenter.broadcastImmediateNotification(
                    ChromeNotificationCenter.OUT_OF_MEMORY);
            }
            if (bitmap != null) {
                updateHistoryThumbnail(bitmap);
                bitmap.recycle();
            }
        }
    };

    private Runnable mCloseContentsRunnable = new Runnable() {
        @Override
        public void run() {
            mTabHost.closeTab(Tab.this);
        }
    };

    private static final int THUMBNAIL_CAPTURE_DELAY = 500;
    private int mThumbnailWidth;
    private int mThumbnailHeight;
    private String mLastLoadedUrl;

    private WelcomePageHelper mWelcomePageHelper;

    public void setFindMatchRectsListener(FindMatchRectsListener f) {
        mFindMatchRectsListener = f;
    }

    /**
     * Creates a minimal {@link Tab} for testing. Do not use outside testing.
     *
     * @param id        The id of the tab.
     * @param incognito Whether the tab is incognito.
     * @return          A {@link Tab} that is not initialized except for the parameters above.
     */
    @VisibleForTesting
    public static Tab createMockTab(int id, boolean incognito) {
        return new Tab(id, incognito);
    }

    /**
     * This constructor is PRIVATE and keep it like that because it do not initialize all the
     * things that are needed for the tab to properly work.
     *
     * @param id        The id of the tab.
     * @param incognito Whether the tab is incognito.
     */
    private Tab(int id, boolean incognito) {
        mId = id;
        mIncognito = incognito;
        mContext = null;
        mActivity = null;
    }

    Tab(int id, TabHost host, Activity activity, boolean incognito, TabLaunchType type,
            int parentId) {
        this(id, host, activity, incognito, 0, type, parentId);
    }

    Tab(int id, TabHost host, final Activity activity, boolean incognito,
            int nativeWebContents, TabLaunchType type, int parentId) {
        mContext = activity.getApplicationContext();
        mTabHost = host;
        mActivity = activity;
        mIncognito = incognito;
        mId = id;
        mLaunchType = type;
        mParentId = parentId;
        mOpenerTabId = parentId;
        mNativeTab = nativeInit();
        mHandler = new Handler();
        initHistoryThumbnailSize(activity);
        final UrlHandler urlHandler = new UrlHandler(activity);
        mChromeViewClient = new ChromeViewClient() {
            @Override
            public void openNewTab(String url, boolean incognito) {
                mTabHost.openNewTab(url, getId(), incognito);
            }

            @Override
            public void onPageStarted(String url, Bitmap favicon) {
                mBaseUrl = null;
                mUrl = url;
                mInfoBarContainer.onPageStarted(url);
                updateTitle();
                nativeSaveTabIdForSessionSync(mNativeTab, mChromeView);
                mChromeView.stopCurrentAccessibilityNotifications();
                notifyPageLoad(ChromeNotificationCenter.PAGE_LOAD_STARTED);
            }

            @Override
            public void onPageFinished(String url) {
                mIsTabStateDirty = true;
                updateTitle();
                notifyPageLoad(ChromeNotificationCenter.PAGE_LOAD_FINISHED);

                mHandler.removeCallbacks(mThumbnailRunnable);
                mHandler.postDelayed(mThumbnailRunnable, THUMBNAIL_CAPTURE_DELAY);

                if (isIncognito())
                    triggerIncognitoKeyGeneration();
            }

            @Override
            public void onLoadStarted() {
                notifyPageLoad(ChromeNotificationCenter.LOAD_STARTED);
            }

            @Override
            public void onLoadStopped() {
                notifyPageLoad(ChromeNotificationCenter.LOAD_STOPPED);
            }

            @Override
            public void onReceivedError(int errorCode, String description, String failingUrl) {
                notifyPageLoadFailed(errorCode);
            }

            @Override
            public void onReceivedHttpAuthRequest(ChromeHttpAuthHandler authHandler, String host,
                                                  String realm) {
                HttpAuthDialog authDialog = new HttpAuthDialog(mActivity, authHandler);
                authHandler.setAutofillObserver(authDialog);
                authDialog.show();
            }

            @Override
            public void onMainFrameCommitted(String url, String baseUrl) {
                mBaseUrl = baseUrl;
            }

            // onUpdateTitle is unreliable, particularly for navigating
            // backwards and forwards in the history stack, so pull the correct
            // title whenever the page changes. onUpdateTitle is only called
            // when the title of a navigation entry changes. When the user goes
            // back a page the navigation entry exists with the correct title,
            // thus the title is not actually changed, and no notification is
            // sent.
            private void updateTitle() {
                if (mChromeView != null) {
                    String title = mChromeView.getTitle();
                    mIsTabStateDirty |= !TextUtils.equals(mTitle, title);
                    mTitle = title;
                }
            }

            private void updateNTPToolbarUrl(String url) {
                NewTabPageToolbar toolBar =
                        NewTabPageToolbar.findOrCreateIfNeeded(Tab.this, mActivity, url);
                if (toolBar != null) {
                    toolBar.urlChanged();
                }
            }

            @Override
            public void onTabHeaderStateChanged() {
                updateTitle();
                mCachedFavicon = null;
                notifyPageTitleChanged();
                notifyFaviconHasChanged();
            }

            @Override
            public void onLoadProgressChanged(double progress) {
                if (progress == 1) {
                    mLastLoadedUrl = getUrl();
                } else {
                    mLastLoadedUrl = null;
                }
                notifyLoadProgress();
            }

            @Override
            public void onUpdateTitle(String title) {
                mIsTabStateDirty |= !TextUtils.equals(mTitle, title);

                mTitle = title;
                notifyPageTitleChanged();
            }

            @Override
            public void onReceiveFindMatchRects(int version,
                    float[] rect_data, RectF activeRect) {
                RectF[] rects = new RectF[rect_data.length / 4];
                int i = 0, j = 0;
                while (i < rects.length) {
                    rects[i++] = new RectF(rect_data[j++], rect_data[j++],
                            rect_data[j++], rect_data[j++]);
                }
                if (mFindMatchRectsListener != null) {
                    mFindMatchRectsListener.onReceiveFindMatchRects(version,
                            rects,
                            activeRect);
                }
            }

            @Override
            public void onTabCrash(int pid) {
                if (mIsHidden)
                    mTabCrashedInBackground = true;
                NewTabPageToolbar toolBar =
                        NewTabPageToolbar.findOrCreateIfNeeded(Tab.this, mActivity, getUrl());
                if (toolBar != null)
                    toolBar.onCrash();
                logTabCrashed();
                // Update the most recent minidump file with the logcat. Doing
                // this asynchronously adds a race condition in the case of
                // multiple simultaneously renderer crashses but because the
                // data will be the same for all of them it is innocuous.
                handleTabCrash(pid);
                notifyTabCrashed();
            }

            @Override
            public void onPopupBlockStateChanged() {
                if (isTabBlockingPopups()) {
                    if (mPopupInfoBar == null) {
                        mPopupInfoBar = new PopupBlockedInfoBar(Tab.this, Tab.this.getUrl());
                        mInfoBarContainer.addInfoBar(mPopupInfoBar);
                    } else {
                        mPopupInfoBar.updateBlockedPopupText();
                    }
                } else {
                    if (mPopupInfoBar != null) {
                        mInfoBarContainer.removeInfoBar(mPopupInfoBar);
                        mPopupInfoBar = null;
                    }
                }
            }

            @Override
            public boolean addNewContents(int nativeWebContents, int disposition) {
                // TODO(johnme): Open tabs in same order as Chrome.
                if (mTabHost == null)
                    return false;

                mTabHost.createTabWithNativeContents(nativeWebContents, mId, null);

                return true;
            }

            @Override
            public void closeContents() {
                if (mTabHost != null) {
                    // Execute outside of callback, otherwise we end up deleting the native
                    // objects in the middle of executing methods on them.
                    mHandler.removeCallbacks(mCloseContentsRunnable);
                    mHandler.post(mCloseContentsRunnable);
                }
            }

            @Override
            public void onUrlStarredChanged(boolean starred) {
                notifyUrlStarredChanged(starred);
            }

            @Override
            public boolean shouldOverrideUrlLoading(String url) {
                // See if this url is for a snapshot
                if (handleSnapshotOrPrintJobUrl(url)) {
                    return true;
                }

                if (mTabHost != null && mTabHost.supportIntentFilters()
                        && mChromeView != null) {
                    if (urlHandler.shouldOverrideUrlLoading(mChromeView, url)) {
                        // Before leaving Chrome, close the empty child tab.
                        // If a new tab is created through JavaScript open to load this
                        // url, we would like to close it as we will load this url in a
                        // different Activity.
                        if (!mChromeView.canGoBackOrForward(0)) {
                            closeContents();
                        }
                        return true;
                    }
                }
                return false;
            }

            @Override
            public void showSelectFileDialog(SelectFileDialog dialog) {
                new FileUploadHandler(activity).chooseFile(dialog);
            }

            @Override
            public boolean shouldOverrideScroll(float deltaX, float deltaY, float currX,
                    float currY) {
                if (mTabHost != null)
                    return mTabHost.overrideScroll(deltaX, deltaY, currX, currY);

                return false;
            }

            @Override
            public void onUpdateUrl(String url) {
                mIsTabStateDirty = true;
                updateNTPToolbarUrl(url);
                notifyPageUrlChanged();
            }

            @Override
            public void onEvaluateJavaScriptResult(int id, String jsonResult) {
                notifyEvaluateJavaScriptResultReceived(id, jsonResult);
            }

            @Override
            public void onNewTabPageReady() {
                notifyPageLoad(ChromeNotificationCenter.NEW_TAB_PAGE_READY);
            }

            @Override
            public void onContextualActionBarShown() {
                notifyContextualActionBarStateChanged(true);
            }

            @Override
            public void onContextualActionBarHidden() {
                notifyContextualActionBarStateChanged(false);
            }

            @Override
            public void onStartContentIntent(ChromeView chromeView, String contentUrl) {
                if (mDisableContentIntentsForTests) {
                    Intent intent;
                    // Perform generic parsing of the URI to turn it into an Intent.
                    try {
                        intent = Intent.parseUri(contentUrl, Intent.URI_INTENT_SCHEME);
                    } catch (URISyntaxException ex) {
                        Log.w(TAG, "Bad URI " + contentUrl + ": " + ex.getMessage());
                        return;
                    }

                    // Send a notification so that testing can catch this up.
                    Bundle b = new Bundle();
                    b.putString("contentUrl", intent.toUri(Intent.URI_INTENT_SCHEME));
                    broadcastNotification(ChromeNotificationCenter.STARTING_CONTENT_INTENT, b);
                    return;
                }

                super.onStartContentIntent(chromeView, contentUrl);
            }

            public void onImeEvent() {
                // Some text was set in the page. Don't reuse it if a tab is
                // open from the same external application, we might lose some
                // user data.
                mAppAssociatedWith = null;
            }

            @Override
            public void onInterstitialShown() {
                mInfoBarContainer.setVisibility(View.INVISIBLE);
                notifyInterstitialShown();
            }

            @Override
            public void onInterstitialHidden() {
                mInfoBarContainer.setVisibility(View.VISIBLE);
                updateNTPToolbarUrl(mChromeView.getUrl());
            }

            @Override
            public void onUnhandledKeyEvent(KeyEvent event) {
                if (mActivity instanceof Main) {
                    ((Main) mActivity).onKeyShortcut(event.getKeyCode(), event);
                }
            }

            @Override
            public boolean takeFocus(boolean reverse) {
                if (reverse) {
                    View menuButton = mActivity.findViewById(R.id.menu_button);
                    if (menuButton != null && menuButton.isShown()) {
                        return menuButton.requestFocus();
                    } else if (mActivity.findViewById(R.id.options_button) != null) {
                        return mActivity.findViewById(R.id.options_button).requestFocus();
                    }
                    return false;
                } else {
                    return getLocationBar(mActivity).findViewById(R.id.url_bar).requestFocus();
                }
            }

            @Override
            public void addShortcutToBookmark(String url, String title, Bitmap favicon, int rValue,
                    int gValue, int bValue) {
                mActivity.sendBroadcast(ChromeBrowserProvider.getShortcutToBookmark(url, title,
                        favicon, rValue, gValue, bValue, mActivity));
            }
        };
        initChromeView(activity, nativeWebContents);
        finishInit(activity);
    }

    private void handleTabCrash(int pid) {
        Intent intent = MinidumpPreparationService.createMinidumpPreparationIntent(
            mContext, pid, null);
        mContext.startService(intent);
        UmaRecordAction.crashUploadAttempt();
    }

    /**
     * @return Whether or not this {@link Tab} has had it's internal state
     *         updated since the last call to {@link Tab#tabStateWasPersisted()}
     *         .
     */
    public boolean shouldTabStateBePersisted() {
        return mIsTabStateDirty;
    }

    /**
     * Clears the internal dirty bit that determines whether or not this
     * {@link Tab} needs to have its state saved.
     */
    public void tabStateWasPersisted() {
        mIsTabStateDirty = false;
    }

    @Override
    public void onInfoBarDismissed(InfoBar infoBar) {
        if (infoBar == mPopupInfoBar) {
            mPopupInfoBar = null;
        }
    }

    @Override
    public void onLaunchBlockedPopups() {
        if (mChromeView != null)
            nativeLaunchBlockedPopups(mChromeView);
    }

    @Override
    public int getBlockedPopupCount() {
        return mChromeView != null ? nativeGetPopupBlockedCount(mChromeView) : 0;
    }

    private void initChromeView(Activity activity, int nativeWebContents) {
        mChromeView = new ChromeView(activity, mIncognito, nativeWebContents,
                ChromeView.Personality.BROWSER);
        // The InfoBarContainer is tied to the ChromeView; if we recreated it,
        // we would lose open
        // infobars.
        mInfoBarContainer = new InfoBarContainer(activity, mChromeView, this);
    }

    // Create all objects which are dependent on having a proper ChromeView.
    // Called at init time; also called when our ChromeView changes due to
    // prefetch.
    private void finishInit(Context context) {
        mChromeView.setChromeViewClient(mChromeViewClient);
        mTabHost.registerForContextMenu(mChromeView);
        mChromeView.setDownloadListener2(new ChromeDownloadListener(context, this));
        nativeSetWindowId(mChromeView, mWindowId);
        if (mWelcomePageHelper != null) {
            mWelcomePageHelper.destroy();
        }
        mWelcomePageHelper = new WelcomePageHelper(this);
    }

    // Absorb the contents of |tab|, prefetched for us.
    public void importPrefetchTab(InstantTab tab) {
        List<InfoBar> previousInfoBars = mInfoBarContainer.getInfoBars();
        destroyInternal(false);
        mChromeView = tab.getView();
        mInfoBarContainer = tab.acquireInfoBarContainer();
        mInfoBarContainer.instantCommited(this, previousInfoBars);
        mTitle = mChromeView.getTitle();
        finishInit(mChromeView.getContext());
        showInternal(null);
        notifyPageTitleChanged();
        notifyTabPrefetchCommitted();
        if (tab.didReceivePageFinished()) {
            // The instant page had fully loaded, make sure to send the page
            // load finished
            // notification (it was eaten by the instant tab when it happened).
            notifyPageLoad(ChromeNotificationCenter.PAGE_LOAD_FINISHED);
        }
    }

    public void stopLoading() {
        if (mChromeView == null)
            return;

        if (isLoading())
            notifyPageLoad(ChromeNotificationCenter.PAGE_LOAD_FINISHED);

        mChromeView.stopLoading();
    }

    public void reload() {
        // TODO(dtrainor): Should we try to call restoreState(Activity) here if
        // mChromeView is null? Might make sense if we are trying to reload this
        // tab? I'm not sure we could ever really hit this case though.
        if (mChromeView != null)
            mChromeView.reload();
    }

    public boolean isHidden() {
        return mIsHidden;
    }

    @Override
    public int getId() {
        return mId;
    }

    @Override
    public int getProgress() {
        return mChromeView != null ? mChromeView.getProgress() : 100;
    }

    public boolean isInitialNavigation() {
        return nativeIsInitialNavigation(mChromeView);
    }

    @Override
    public boolean isSavedAndViewDestroyed() {
        return mChromeView == null;
    }

    @Override
    public String getUrl() {
        return mChromeView != null ? mChromeView.getUrl() : "";
    }

    /**
     * @return The displayHost of the current url. This could be something like
     *         "google.com" for "http://www.google.com/blah/blah/blah"
     */
    public String getDisplayHost() {
        return mChromeView != null ? nativeGetTabContentDisplayHost(mChromeView) : "";
    }

    public String getTitle() {
        // When restoring the tabs, the title will no longer be populated, so
        // request it from the chrome view (if present).
        if (mTitle == null && mChromeView != null) {
            mTitle = mChromeView.getTitle();
        }
        return mTitle;
    }

    /**
     * @return The base URL when the main frame was committed (before the base
     *         elements were processed). Used as the original URL for snapshots.
     */
    public String getBaseUrl() {
        return mBaseUrl;
    }

    /**
     * @return The bitmap of the favicon scaled to 16x16dp. null if no favicon
     *         is specified or it requires the default favicon.
     */
    public Bitmap getFavicon() {
        String currentUrl = getUrl();
        // If the URL associated with the cached favicon is different than
        // the current one, then generate the icon again.
        if (currentUrl == null || !currentUrl.equals(mCachedFaviconUrl)) {
            mCachedFavicon = null;
            mCachedFaviconUrl = null;
        }

        if (mCachedFavicon == null) {
            // If the view is gone from underneath, just return an empty bitmap.
            if (mChromeView == null) {
                return null;
            }
            // Calculate the favicon dimensions based on the screen density.
            int correctedFaviconDimension = (int) (FAVICON_DIMENSION
                        * mContext.getResources().getDisplayMetrics().density);
            Bitmap b = getScaledFavicon(correctedFaviconDimension, correctedFaviconDimension);
            // If the favicon is not yet valid (i.e. it's either blank or a
            // placeholder), then do not cache the results.
            if (b == null || !mChromeView.faviconIsValid()) {
                return b;
            }
            mCachedFavicon = b;
            mCachedFaviconUrl = currentUrl;
        }
        return mCachedFavicon;
    }

    /**
     * Obtains the favicon for the current tab (applying the necessary scaling
     * for it to fit in the given width and height).
     *
     * @return Favicon bitmap.
     * @hide For phone tab stack.
     */
    private Bitmap getScaledFavicon(int width, int height) {
        Bitmap orig = mChromeView.getFavicon();
        if (orig != null) {
            return Bitmap.createScaledBitmap(orig, width, height, true);
        } else {
            return null;
        }
    }

    public long getLastShownTimestamp() {
        return mLastShownTimestamp;
    }

    private void setLastShownTimestamp(long timestamp) {
        mLastShownTimestamp = timestamp;
    }

    /**
     * Saves the state of the tab to save memory. The state can then be restored
     * calling restoreState().
     */
    public void saveStateAndDestroy() {
        // Can't save the state of the currently active tab.
        if (mIsHidden && mChromeView != null) {
            // If we already have a saved state, use that one.
            if (mSavedState == null) {
                mSavedState = nativeGetStateAsByteArray(mNativeTab, mChromeView);
            }
            destroyInternal(false);
        }
    }

    public void restoreState(Activity activity) {
        if (mSavedState == null) {
            Log.w(TAG, "Trying to restore tab from state but no state was previously saved.");
        }

        int tabContents = mSavedState != null ? nativeRestoreStateFromByteArray(mSavedState) : 0;

        mSavedState = null;
        initChromeView(activity, tabContents);
        finishInit(activity);
        if (tabContents == 0) {
            if (mUrl == null) {
                Log.i(TAG, "Unable to restore TabContents from saved state so falling back to NTP");
                loadUrl(ChromeViewListAdapter.NTP_URL + "#" + NTPSection.MOST_VISITED);
            } else {
                loadUrl(mUrl);
            }
        }
    }

    public boolean isLoading() {
        return mChromeView != null && mChromeView.getProgress() < 100;
    }

    public boolean isIncognito() {
        return mIncognito;
    }

    /**
     * Return the InfoBar container.
     *
     * @return infobar container
     */
    InfoBarContainer getInfoBarContainer() {
        return mInfoBarContainer;
    }

    void destroy() {
        destroyInternal(true);
    }

    /**
     * Called when the Main activity gets resumed.
     */
    public void onActivityResume() {
        NewTabPageToolbar toolbar = NewTabPageToolbar.
                findOrCreateIfNeeded(this, mActivity, getUrl());
        if (toolbar != null) toolbar.onActivityResume();
    }


    void createHistoricalTab() {
        // We don't ever use the tab index when restoring, so -1 is fine.
        if (mChromeView != null) {
            nativeCreateHistoricalTab(mChromeView, -1);
        } else if (mSavedState != null) {
            nativeCreateHistoricalTabFromState(mSavedState, -1);
        }
    }

    /**
     * Destroy the {@link #mChromeView} and {@link #mInfoBarContainer}, and
     * optionally {@link #mNativeTab}.
     *
     * This call must be "recoverable". Specifically, this call must not change
     * any state which cannot be rebuilt by a call like importPrefetchTab().
     *
     * @param destroyNativeTab
     *            If true, destroy the native tab after destroying the
     *            ChromeView but before destroying the InfoBarContainer. The
     *            native tab should be destroyed before the infobar container as
     *            destroying the native tab cleanups up any remaining infobars.
     *            The info bar container expects all info bars to be cleaned up
     *            before its own destruction.
     */
    private void destroyInternal(boolean destroyNativeTab) {
        if (destroyNativeTab && mWelcomePageHelper != null) {
            mWelcomePageHelper.destroy();
            mWelcomePageHelper = null;
        }
        if (mChromeView != null) {
            if (mTitle == null) mTitle = mChromeView.getTitle();
            mTabHost.unregisterForContextMenu(mChromeView);
            mChromeView.destroy();
            mChromeView = null;
        }
        if (destroyNativeTab) {
            if (mNativeTab != 0) {
                nativeDestroy(mNativeTab);
                mNativeTab = 0;
            }
        }
        if (mInfoBarContainer != null) {
            mInfoBarContainer.destroy();
            mInfoBarContainer = null;
        }
        mTabCrashedInBackground = false;
    }

    public ChromeView getView() {
        return mChromeView;
    }

    ChromeViewClient getViewClient() {
        return mChromeViewClient;
    }

    /**
     * Returns a Bitmap with a snapshot of contents of this tab.
     */
    public Bitmap generateFullsizeBitmap() {
        if (mChromeView == null)
            return null;
        int w = mChromeView.getWidth();
        int h = mChromeView.getHeight();
        if (w == 0 || h == 0)
            return null;
        try {
            return mChromeView.getBitmap(w, h, Bitmap.Config.RGB_565);
        } catch (OutOfMemoryError e) {
            Log.w(TAG, "OutOfMemoryError thrown while trying to fetch a tab bitmap.", e);
            ChromeNotificationCenter.broadcastImmediateNotification(
                    ChromeNotificationCenter.OUT_OF_MEMORY);
            return null;
        }
    }

    @Override
    public Bitmap getBitmap() {
        Bitmap b = null;
        try {
            b = mChromeView.getBitmap();
        } catch (OutOfMemoryError e) {
            Log.w(TAG, "OutOfMemoryError thrown while trying to fetch a tab bitmap.", e);
            ChromeNotificationCenter.broadcastImmediateNotification(
                    ChromeNotificationCenter.OUT_OF_MEMORY);
        }
        if (b != null) {
            // Since a thumbnail is available, it might as well be used to
            // update the history thumbnail.
            if (canUpdateHistoryThumbnail()) {
                updateHistoryThumbnail(b);
            }
        }
        return b;
    }

    private boolean canUpdateHistoryThumbnail() {
        // The samePage(...) check is to work around the following scenario: The
        // user types in a url/clicks on a link and quickly changes tabs. For a
        // brief period of time, progress will still be reported as 100% even
        // the the URL has changed. To work around this, keep track of the last
        // page a 100% load completion was received for and check against it
        // when determining if the current page is really 100%.
        return isReady()
                && !getUrl().startsWith(CHROME_SCHEME)
                && getProgress() == 100
                && URLUtilities.samePage(mLastLoadedUrl, getUrl());
    }

    private void updateHistoryThumbnail(Bitmap bitmap) {
        if (isIncognito()) {
            return;
        }
        // TODO(yusufo): It will probably be faster and more efficient on resources to do this on
        // the native side, but the thumbnail_generator code has to be refactored a bit to allow
        // creating a downsized version of a bitmap progressively.
        if (bitmap.getWidth() != mThumbnailWidth
                || bitmap.getHeight() != mThumbnailHeight
                || bitmap.getConfig() != Config.ARGB_8888) {
            try {
                int[] dim = new int[] {
                        bitmap.getWidth(), bitmap.getHeight()
                };
                // If the thumbnail size is small compared to the bitmap size downsize in
                // two stages. This makes the final quality better.
                float scale = Math.max(
                        (float) mThumbnailWidth / dim[0],
                        (float) mThumbnailHeight / dim[1]);
                int adjustedWidth = (scale < 1) ?
                        mThumbnailWidth * (int) (1 / Math.sqrt((double) scale)) : mThumbnailWidth;
                int adjustedHeight = (scale < 1) ?
                        mThumbnailHeight * (int) (1 / Math.sqrt((double) scale)) : mThumbnailHeight;
                scale = MathUtils.scaleToFitTargetSize(dim, adjustedWidth, adjustedHeight);
                // Horizontally center the source bitmap in the final result.
                float leftOffset = (adjustedWidth - dim[0]) / 2.0f / scale;
                Bitmap tmpBitmap = Bitmap.createBitmap(adjustedWidth,
                        adjustedHeight, Config.ARGB_8888);
                Canvas c = new Canvas(tmpBitmap);
                c.scale(scale, scale);
                c.drawBitmap(bitmap, leftOffset, 0, new Paint(Paint.FILTER_BITMAP_FLAG));
                if (scale < 1) {
                    tmpBitmap = Bitmap.createScaledBitmap(tmpBitmap,
                            mThumbnailWidth, mThumbnailHeight, true);
                }
                updateThumbnail(tmpBitmap);
                tmpBitmap.recycle();
            } catch (OutOfMemoryError ex) {
                Log.w(TAG, "OutOfMemoryError while updating the history thumbnail.");
                ChromeNotificationCenter.broadcastImmediateNotification(
                        ChromeNotificationCenter.OUT_OF_MEMORY);
            }
        } else {
            updateThumbnail(bitmap);
        }
    }

    private void initHistoryThumbnailSize(Activity activity) {
        int w, h;
        if (ChromeActivity.isTabletUi(activity)) {
            w = THUMBNAIL_W_TABLET;
            h = THUMBNAIL_H_TABLET;
        } else {
            w = THUMBNAIL_W_PHONE;
            h = THUMBNAIL_H_PHONE;
        }
        DisplayMetrics dm = new DisplayMetrics();
        activity.getWindowManager().getDefaultDisplay().getMetrics(dm);
        switch (dm.densityDpi) {
            case DisplayMetrics.DENSITY_TV:
                w = w * 4 / 3;
                h = h * 4 / 3;
                break;
            case DisplayMetrics.DENSITY_HIGH:
                w = w * 3 / 2;
                h = h * 3 / 2;
                break;
            case DisplayMetrics.DENSITY_XHIGH:
                w = w * 2;
                h = h * 2;
        }
        mThumbnailWidth = w;
        mThumbnailHeight = h;
    }

    void show(Activity activity) {
        show(activity, null);
    }

    /**
     * show() is called when we may need to recreate a ChromeView (e.g. if it
     * was purged).
     *
     * @param bitmap the thumbnail image to prime the new tab with. Can be null.
     */
    void show(Activity activity, Bitmap primeBitmap) {
        TraceEvent.begin();
        if (mChromeView == null) {
            restoreState(activity);
        }
        mSavedState = null;
        showInternal(primeBitmap);
        TraceEvent.end();
    }

    /**
     * @param bitmap the thumbnail image to prime the new tab with. Can be null.
     */
    private void showInternal(Bitmap primeBitmap) {
        mIsHidden = false;

        if (mTabCrashedInBackground) {
            reload();
            mTabCrashedInBackground = false;
        }

        mChromeView.usePrimeBitmap(primeBitmap);
        mLastShownTimestamp = System.currentTimeMillis();
        mChromeView.onShow();
        // If the page is still loading, notify of load progress so the progress
        // bar shows right
        // away (otherwise it would not show until the renderer notifies of new
        // progress being
        // made).
        if (getProgress() < 100 && !isShowingInterstitialPage())
            notifyLoadProgress();
    }

    LocationBar getLocationBar(final Activity activity) {
        // TODO: This needs to be refactored, this is ugly
        if (activity instanceof Main) {
            return ((Main) activity).mToolbar.getDelegate().getLocationBar();
        }
        return null;
    }

    void hide() {
        mIsHidden = true;
        mChromeView.onHide();
    }

    public void setSnapshotId(String snapshotId) {
        mSnapshotId = snapshotId;
        // Asynchronously query for the current state of the snapshot document
        Intent queryStateIntent =
                SnapshotArchiveManager.createQueryStateIntent(mActivity, snapshotId);
        mActivity.startService(queryStateIntent);
    }

    /**
     * Updates the UI with the correct state.
     *
     * @param snapshotId the snapshot ID that must match the tab.
     * @param snapshotUri the URI of the offline copy, if any.
     * @param snapshotViewableState the current state of the offline copy.
     * @return true if this tab handled the query result.
     */
    public boolean snapshotStateQueryResult(String snapshotId, Uri snapshotUri,
            SnapshotViewableState snapshotViewableState) {
        if (mSnapshotId == null || !mSnapshotId.equals(snapshotId)) {
            return false;
        }
        switch (snapshotViewableState) {
            case READY:
                snapshotStateReady(snapshotUri);
                break;
            case DOWNLOADING:
                snapshotStateDownloading();
                break;
            case ERROR:
                snapshotStateError();
                break;
            case UNKNOWN:
                snapshotStateNeverReady();
                break;
        }
        return true;
    }

    private void snapshotStateReady(Uri snapshotUri) {
        // Dismiss the downloading infobar if there is one.
        if (mSnapshotDownloadingInfoBar != null) {
            mSnapshotDownloadingInfoBar.dismiss();
        }
        // Show the download succeeded infobar.
        SnapshotDownloadSuceededInfobar infoBar = new SnapshotDownloadSuceededInfobar(snapshotUri);
        infoBar.setExpireOnNavigation(false);
        addInfoBar(infoBar);
        // The snapshot ID is only used for showing the infobars. Now that we
        // have shown the final
        // infobar, it is not needed anymore. Nulling it also prevents potential
        // other snapshot
        // query results from triggering the infobar again.
        mSnapshotId = null;
    }

    private void snapshotStateDownloading() {
        // Show the downloading infobar only once.
        if (mSnapshotDownloadingInfoBar == null) {
            mSnapshotDownloadingInfoBar = new SnapshotDownloadingInfobar(
                    new InfoBarDismissedListener() {
                        @Override
                        public void onInfoBarDismissed(InfoBar infoBar) {
                            mSnapshotDownloadingInfoBar = null;
                        }
                    });
            // TODO(jcivelli): http://b/5689432 we should find a way to have the
            // infobar expirable.
            // (there is a race with the navigation causing them to go away).
            mSnapshotDownloadingInfoBar.setExpireOnNavigation(false);
            addInfoBar(mSnapshotDownloadingInfoBar);
        }
    }

    private void logTabCrashed() {
        if (!mIsHidden) {
            // Renderer crashed in foreground.
            UmaSessionStats.logRendererCrash();
        }
    }

    private void snapshotStateError() {
        if (mSnapshotDownloadingInfoBar != null) {
            mSnapshotDownloadingInfoBar.dismiss();
        }
        if (mSnapshotDownloadErrorInfoBar == null) {
            String errorMessage = mContext.getString(R.string.snapshot_download_failed_infobar);
            mSnapshotDownloadErrorInfoBar = MessageInfoBar.createWarningInfoBar(
                    new InfoBarDismissedListener() {
                        @Override
                        public void onInfoBarDismissed(InfoBar infoBar) {
                            mSnapshotDownloadErrorInfoBar = null;
                        }
                    }, errorMessage);
            addInfoBar(mSnapshotDownloadErrorInfoBar);
        }
    }

    private void snapshotStateNeverReady() {
        if (mSnapshotDownloadingInfoBar != null) {
            mSnapshotDownloadingInfoBar.dismiss();
        }
    }

    private void notifyPageLoad(int notification) {
        Bundle b = new Bundle();
        b.putInt("tabId", mId);
        // When the render view dies while loading some contents, we get a
        // notification but we have no view. Note that if important, we could
        // still retrieve the URL via the native tab (by accessing the
        // navigation controller).
        b.putString("url", (getView() != null) ? getView().getUrl() : "");
        ChromeNotificationCenter.broadcastNotification(notification, b);
    }

    private void notifyPageLoadFailed(int errorCode) {
        Bundle b = new Bundle();
        b.putInt("tabId", mId);
        b.putInt("errorCode", errorCode);
        ChromeNotificationCenter.broadcastNotification(
                ChromeNotificationCenter.PAGE_LOAD_FAILED, b);
    }

    private void notifyLoadProgress() {
        Bundle b = new Bundle();
        b.putInt("tabId", mId);
        b.putInt("progress", getProgress());
        broadcastNotification(ChromeNotificationCenter.LOAD_PROGRESS_CHANGED, b);
    }

    private void notifyTabCrashed() {
        Bundle b = new Bundle();
        b.putInt("tabId", mId);
        broadcastNotification(ChromeNotificationCenter.TAB_CRASHED, b);
    }

    private void notifyTabPrefetchCommitted() {
        Bundle b = new Bundle();
        b.putInt("tabId", mId);
        broadcastNotification(ChromeNotificationCenter.TAB_PREFETCH_COMMITTED, b);
    }

    private void notifyPageTitleChanged() {
        Bundle b = new Bundle();
        b.putInt("tabId", mId);
        b.putString("title", mTitle);
        broadcastNotification(ChromeNotificationCenter.PAGE_TITLE_CHANGED, b);
    }

    private void notifyPageUrlChanged() {
        Bundle b = new Bundle();
        b.putInt("tabId", mId);
        broadcastNotification(ChromeNotificationCenter.PAGE_URL_CHANGED, b);
    }

    private void notifyEvaluateJavaScriptResultReceived(int id, String jsonResult) {
        Bundle b = new Bundle();
        b.putInt("tabId", mId);
        b.putInt("resultId", id);
        b.putString("result", jsonResult);
        broadcastNotification(ChromeNotificationCenter.JAVA_SCRIPT_EVAL_RECEIVED, b);
    }

    private void notifyFaviconHasChanged() {
        Bundle b = new Bundle();
        b.putInt("tabId", mId);
        broadcastNotification(ChromeNotificationCenter.FAVICON_CHANGED, b);
    }

    private void notifyUrlStarredChanged(boolean starred) {
        Bundle b = new Bundle();
        b.putInt("tabId", mId);
        b.putBoolean("starred", starred);
        broadcastNotification(ChromeNotificationCenter.URL_STARRED_CHANGED, b);
    }

    private void notifyContextualActionBarStateChanged(boolean shown) {
        Bundle b = new Bundle();
        b.putInt("tabId", mId);
        b.putBoolean("shown", shown);
        broadcastNotification(ChromeNotificationCenter.TAB_CONTEXTUAL_ACTION_BAR_STATE_CHANGED, b);
    }

    @VisibleForTesting
    public void setDisableContentIntentsForTests(boolean disable) {
        mDisableContentIntentsForTests = disable;
    }

    private void notifyInterstitialShown() {
        Bundle b = new Bundle();
        b.putInt("tabId", mId);
        broadcastNotification(ChromeNotificationCenter.INTERSTITIAL_PAGE_SHOWN, b);
    }

    static class TabState {
        public long lastShownTimestamp;
        public byte[] state;
        public int parentId;
        public String openerAppId;
        public boolean isIncognito;
    }

    /** Returns an opaque "state" object that can be persisted by saveState. */
    public Object getState() {
        if (mChromeView != null) {
            mSavedState = nativeGetStateAsByteArray(mNativeTab, mChromeView);
        }

        if (mSavedState == null) {
            return null;
        }

        TabState tabState = new TabState();
        tabState.state = mSavedState;
        tabState.lastShownTimestamp = mLastShownTimestamp;
        tabState.parentId = getParentId();
        tabState.openerAppId = getAppAssociatedWith();
        return tabState;
    }

    /**
     * Writes a "state" object obtained from getState to disk. Unlike getState,
     * this method may be called either on the UI thread or on a background
     * thread.
     */
    public static void saveState(int id, Object state, Activity activity, boolean encrypted)
            throws IOException {
        if (state == null || ((TabState) state).state == null) {
            return;
        }
        FileOutputStream foutput = activity.openFileOutput(
                getFilename(encrypted, id), Context.MODE_PRIVATE);
        OutputStream output;
        Cipher cipher;
        if (encrypted && (cipher = getCipher(Cipher.ENCRYPT_MODE)) != null) {
            output = new CipherOutputStream(foutput, cipher);
        } else {
            output = foutput;
        }
        saveStateInternal(output, state, encrypted);
    }

    /**
     * Deletes the persistant state file for a particular {@link Tab}.
     * @param id The numerical id that represents the {@link Tab}.
     * @param activity The {@link Activity} this state file should be saved under.
     * @param encrypted Whether or not the {@link Tab} state was encrypted.
     */
    public static void deleteStateFile(int id, Activity activity, boolean encrypted) {
        activity.deleteFile(getFilename(encrypted, id));
    }

    private static Key getKey() {
        if (mKey == null) {
            triggerIncognitoKeyGeneration();
            try {
                mKey = sIncognitoKeyGenerator.get();
                sIncognitoKeyGenerator = null;
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            } catch (ExecutionException e) {
                throw new RuntimeException(e);
            }
        }
        return mKey;
    }

    /**
     * Generates the incognito encryption key in a background thread (if
     * necessary).
     */
    private static synchronized void triggerIncognitoKeyGeneration() {
        if (mKey != null || sIncognitoKeyGenerator != null)
            return;

        sIncognitoKeyGenerator = new FutureTask<SecretKey>(new Callable<SecretKey>() {
            @Override
            public SecretKey call() throws Exception {
                KeyGenerator kgen = KeyGenerator.getInstance("AES");
                SecureRandom sr = SecureRandom.getInstance("SHA1PRNG");
                // Do not call setSeed here, this is not appropriate for secure
                // use. By default SecureRandom will generate an initial seed
                // using an internal entropy source.
                kgen.init(128, sr);
                return kgen.generateKey();
            }
        });
        AsyncTask.THREAD_POOL_EXECUTOR.execute(sIncognitoKeyGenerator);
    }

    private static Cipher getCipher(int opmode) {
        try {
            Cipher cipher = Cipher.getInstance("AES");
            cipher.init(opmode, getKey());
            return cipher;
        } catch (NoSuchAlgorithmException e) {
            Log.w("cipher", "Error in the cipher alogrithm", e);
            return null;
        } catch (InvalidKeyException e) {
            Log.w("cipher", "Wrong key to use in cipher", e);
            return null;
        } catch (NoSuchPaddingException e) {
            Log.w("cipher", "Error in creating cipher instance", e);
            return null;
        }
    }

    private static void saveStateInternal(OutputStream output, Object state, boolean encrypted)
            throws IOException {
        TabState tabState = (TabState) state;
        DataOutputStream stream = new DataOutputStream(output);
        try {
            if (encrypted) {
                stream.writeLong(KEY_CHECKER);
            }
            stream.writeLong(tabState.lastShownTimestamp);
            stream.writeInt(tabState.state.length);
            stream.write(tabState.state);
            stream.writeInt(tabState.parentId);
            stream.writeUTF(tabState.openerAppId != null ? tabState.openerAppId : "");
        } finally {
            StreamUtils.closeQuietly(stream);
        }
    }

    public static String getFilename(boolean incognito, int id) {
        return incognito ? SAVED_STATE_FILE_PREFIX_INCOGNITO + id : SAVED_STATE_FILE_PREFIX + id;
    }

    private static InputStream openStateFile(Activity activity, int id, boolean encrypted) {
        try {
            return new BufferedInputStream(activity.openFileInput(getFilename(encrypted, id)));
        } catch (FileNotFoundException e) {
            return null;
        }
    }

    public static TabState readState(int id, Activity activity) throws IOException {
        InputStream finput = openStateFile(activity, id, false);
        if (finput != null) {
            return readState(finput, false);
        } else {
            // Then try to find the encrypted version.
            finput = openStateFile(activity, id, true);
            if (finput != null) {
                return readState(finput, true);
            }
        }
        return null;
    }

    public static TabState readState(InputStream input, boolean encrypted) throws IOException {
        Cipher cipher;
        if (encrypted && (cipher = getCipher(Cipher.DECRYPT_MODE)) != null) {
            input = new CipherInputStream(input, cipher);
        }
        return readStateInternal(input, encrypted);
    }

    private static TabState readStateInternal(InputStream input,
            boolean encrypted) throws IOException {
        DataInputStream stream = new DataInputStream(input);
        try {
            if (encrypted && stream.readLong() != KEY_CHECKER) {
                // Got the wrong key, skip the file
                return null;
            }
            TabState tabState = new TabState();
            tabState.lastShownTimestamp = stream.readLong();
            int size = stream.readInt();
            tabState.state = new byte[size];
            stream.readFully(tabState.state);
            tabState.parentId = stream.readInt();
            try {
                tabState.openerAppId = stream.readUTF();
                if ("".equals(tabState.openerAppId))
                    tabState.openerAppId = null;
            } catch (EOFException eof) {
                // Could happen if reading a version of a TabState that does not include the app id.
                Log.w(TAG, "Failed to read opener app id state from tab state");
            }
            tabState.isIncognito = encrypted;
            return tabState;
        } finally {
            stream.close();
        }
    }

    public void deleteState(Activity activity) {
        deleteStateFile(mId, activity, mIncognito);
    }

    public static int restoreState(byte[] data) {
        return nativeRestoreStateFromByteArray(data);
    }

    public TabLaunchType getLaunchType() {
        return mLaunchType;
    }

    public int getParentId() {
        return mParentId;
    }

    public boolean parentIsIncognito() {
        return mParentIsIncognito;
    }

    public void setParentIsIncognito() {
        mParentIsIncognito = true;
    }

    public void addInfoBar(InfoBar infoBar) {
        mInfoBarContainer.addInfoBar(infoBar);
    }

    public void removeInfoBar(InfoBar infoBar) {
        mInfoBarContainer.removeInfoBar(infoBar);
    }

    /**
     * @return The current list of visible info bars on this tab.
     */
    public List<InfoBar> getInfoBars() {
        if (mInfoBarContainer == null) return null;
        return mInfoBarContainer.getInfoBars();
    }

    /**
     * Causes this tab to navigate to the specified URL. Note that it is
     * important to pass a correct pageTransition as it is used for ranking URLs
     * in the history so the omnibox can report suggestions correctly.
     *
     * @param url the url to navigate to.
     * @param pageTransition the transition that caused this page to be opened
     *            see ChromeView PAGE_TRANSITION_* constants.
     */
    public void loadUrl(String url, int pageTransition) {
        if (mNativeTab != 0) {
            nativeLoadUrl(mNativeTab, mChromeView, url, pageTransition);
        }
    }

    public void loadUrl(String url) {
        loadUrl(url, ChromeView.PAGE_TRANSITION_TYPED);
    }

    /**
     * @return Whether or not this tab is currently blocking any pop-ups.
     */
    public boolean isTabBlockingPopups() {
        return nativeGetPopupBlockedCount(mChromeView) > 0;
    }

    /**
     * Injects the passed JavaScript code in the current page and evaluates it.
     * The result is returned as part of a JAVA_SCRIPT_EVAL_RECEIVED broadcast
     * notification.
     *
     * @return an id that is returned along with the notification.
     */
    public int evaluateJavaScript(String script) {
        return mChromeView.evaluateJavaScript(script);
    }

    /**
     * Sets the native window id to which the tab belongs. Primarily used for
     * session restore.
     *
     * @param mWindowId
     */
    public void setWindowId(int windowId) {
        mWindowId = windowId;
        nativeSetWindowId(mChromeView, mWindowId);
    }

    /**
     * Returns the the id of that tab that opened this tab. This is similar to
     * the parent id, but can be reset on tab open and close events.
     */
    public int getOpenerId() {
        return mOpenerTabId;
    }

    /**
     * Set the id of that tab that opened this tab.
     *
     * @param opener The new value.
     * @see Tab#getOpenerId
     */
    public void setOpenerId(int opener) {
        mOpenerTabId = opener;
    }

    public int getRenderProcessPid() {
        if (isSavedAndViewDestroyed())
            return INVALID_RENDER_PROCESS_PID;
        return nativeGetRenderProcessPid(mChromeView);
    }

    public int getRenderProcessPrivateSizeKBytes() {
        if (isSavedAndViewDestroyed())
            return 0;
        return nativeGetRenderProcessPrivateSizeKBytes(mChromeView);
    }

    public void purgeRenderProcessNativeMemory() {
        if (isSavedAndViewDestroyed())
            return;
        nativePurgeRenderProcessNativeMemory(mChromeView);
    }

    /**
     * @return true if the tab is currently showing an interstitial page, such
     *         as a bad HTTPS page.
     */
    public boolean isShowingInterstitialPage() {
        return mChromeView == null ? false : nativeIsShowingInterstitialPage(mChromeView);
    }

    /**
     * Sets the value of the client parameter sent when a Google search is performed.
     */
    public static void setSearchClient(String client) {
        nativeSetSearchClient(client);
    }

    /**
     * Sets the value of the rlz parameter sent when a Google search is performed.
     */
    public static void setStaticRlz(String rlz) {
        nativeSetStaticRlz(rlz);
    }

    @VisibleForTesting
    public void showInterstitialPage(String url, String htmlContent) {
        nativeShowInterstitialPage(mChromeView, url, htmlContent);
    }

    private native int nativeInit();

    private native void nativeDestroy(int nativeTab);

    private native void nativeCreateHistoricalTab(ChromeView view, int tab_index);

    private static native void nativeCreateHistoricalTabFromState(byte[] state, int tab_index);

    private native byte[] nativeGetStateAsByteArray(int nativeTab, ChromeView view);

    private static native int nativeRestoreStateFromByteArray(byte[] state);

    private native boolean nativeIsInitialNavigation(ChromeView view);

    private native void nativeSaveTabIdForSessionSync(int nativeTab, ChromeView view);

    private native void nativeSetWindowId(ChromeView view, int mWindowId);

    private native void nativeLaunchBlockedPopups(ChromeView view);

    private native int nativeGetPopupBlockedCount(ChromeView view);

    private native String nativeGetTabContentDisplayHost(ChromeView view);

    private native int nativeGetRenderProcessPid(ChromeView view);

    private native int nativeGetRenderProcessPrivateSizeKBytes(ChromeView view);

    private native void nativePurgeRenderProcessNativeMemory(ChromeView view);

    private native void nativeShowInterstitialPage(ChromeView view, String url, String htmlContent);

    private native boolean nativeIsShowingInterstitialPage(ChromeView view);

    private native void nativeUpdateThumbnail(int nativeTab, ChromeView view, Bitmap bitmap);

    private native boolean nativeShouldUpdateThumbnail(ChromeView view, String url);

    private native long nativeGetBookmarkId(ChromeView view);

    private native void nativeLoadUrl(int nativeTab,
            ChromeView view, String url, int Pagetransition);

    private static native void nativeSetSearchClient(String client);

    private static native void nativeSetStaticRlz(String rlz);

    @CalledByNative
    public void onFirstSearch() {
        RevenueStats.onSearch(mContext);
    }
    /**
     * Saves the incognito encryption key in Chrome's bundle.
     *
     * @param outState The data bundle that get saved in RAM throughout Chrome's
     *            life span.
     */
    public static void saveEncryptionKey(Bundle outState) {
        // If the key has not yet been generated, do not spend time generating
        // one unnecessarily.
        if (mKey == null)
            return;

        byte[] wrappedKey = mKey.getEncoded();
        if (wrappedKey != null) {
            outState.putByteArray("incognito_key", wrappedKey);
        }
    }

    /**
     * Restores the incognito encryption key from Chrome's bundle.
     *
     * @param savedInstanceState Chrom's data bundle.
     * @return true if the key has been restored.
     */
    public static boolean restoreEncryptionKey(Bundle savedInstanceState) {
        byte[] wrappedKey = savedInstanceState.getByteArray("incognito_key");
        if (wrappedKey != null) {
            try {
                mKey = new SecretKeySpec(wrappedKey, "AES");
                return true;
            } catch (IllegalArgumentException e) {
                Log.w("cipher", "Error the key data is empty", e);
            }
        }
        return false;
    }

    public interface TabHost {
        // Returns true if the host activity supports intent filters so that
        // Context.startActivity() can start the Activity.
        public boolean supportIntentFilters();

        public Tab createTabWithNativeContents(int nativeWebContents, int parentId, String appId);

        public void openNewTab(String url, int parentId, boolean incognito);

        public void closeTab(Tab tab);

        public void registerForContextMenu(View view);

        public void unregisterForContextMenu(View view);

        public boolean overrideScroll(float deltaX, float deltaY, float currX, float currY);
    }

    /**
     * @return A potential fallback texture id to use when trying to draw this
     *         tab.
     */
    public int getFallbackTextureId() {
        return (!isIncognito() && getUrl().startsWith(ChromeViewListAdapter.NTP_URL)) ?
                NTP_TAB_ID : INVALID_TAB_ID;
    }

    @Override
    public TextureView getTextureView() {
        ChromeView view = getView();
        if (view != null) {
            for (int i = 0; i < view.getChildCount(); ++i) {
                View child = view.getChildAt(i);
                if (child instanceof TextureView) {
                    return (TextureView) child;
                }
            }
        }
        return null;
    }

    /**
     * @return Whether or not the tab represents a New Tab Page.
     */
    public boolean isNTP() {
        return getUrl().startsWith(ChromeViewListAdapter.NTP_URL);
    }

    @Override
    public boolean isReady() {
        ChromeView view = getView();
        return view != null && view.isReady();
    }

    @Override
    public boolean isTextureViewAvailable() {
        ChromeView view = getView();
        return view != null && view.isAvailable();
    }

    @Override
    public int getWidth() {
        ChromeView view = getView();
        return view == null ? 0 : view.getWidth();
    }

    @Override
    public int getHeight() {
        ChromeView view = getView();
        return view == null ? 0 : view.getHeight();
    }

    @Override
    public boolean useTextureView() {
        ChromeView view = getView();
        return view == null ? false : !view.hasLargeOverlay();
    }

    /**
     * @return The id of the application associated with that tab (null if not
     *         associated with an app).
     * @see #associateWithApp(String) for more information.
     */
    protected String getAppAssociatedWith() {
        return mAppAssociatedWith;
    }

    /**
     * Associates this tab with the external app with the specified id. Once a
     * tab is associated with an app, it is reused when a new page is opened
     * from that app. (unless the user typed in the location bar or in the page,
     * in which case the tab is dissociated from any app)
     *
     * @param appId The ID of application associated with the tab.
     */
    protected void associateWithApp(String appId) {
        mAppAssociatedWith = appId;
        // Start listening for user actions, as they dissociate the Tab from its
        // opener application.
        final LocationBar locationBar = getLocationBar(mActivity);
        locationBar.addTextChangeListener(new LocationBar.TextChangeListener() {
            @Override
            public void locationBarTextChanged(String text) {
                mAppAssociatedWith = null;
                locationBar.removeTextChangeListener(this);
            }
        });
    }

    /**
     * Returns true if the page currently is a snapshot of a page, that is a
     * MHTML file.
     */
    public boolean isShowingSnapshot() {
        String url = getUrl().toLowerCase();
        return url.startsWith("file://") && url.endsWith(".mht");
    }

    /**
     * Request that this tab receive focus. Currently, this function requests
     * focus for the {@link ChromeView}.
     *
     * @param hideKeyboard Whether this function should close the keyboard if it
     *            is open.
     */
    public void requestFocus(boolean hideKeyboard) {
        if (!hideKeyboard) {
            LocationBar locationBar = getLocationBar(mActivity);
            if (locationBar.hasFocus()) {
                return;
            }
        }

        ChromeView view = getView();
        if (view != null) {
            view.requestFocus();
        }
    }

    private boolean handleSnapshotOrPrintJobUrl(final String url) {
        // See if there is a snapshot or print job attached
        final String snapshotId = url.startsWith(SNAPSHOT_PREFIX) ?
                url.substring(SNAPSHOT_PREFIX.length()) : null;
        final String printJobId = url.startsWith(PRINT_JOB_PREFIX) ?
                url.substring(PRINT_JOB_PREFIX.length()) : null;
        if (snapshotId == null && printJobId == null) {
            return false;
        }

        new AsyncTask<Void, Void, SnapshotDocument>() {
            @Override
            protected SnapshotDocument doInBackground(Void... params) {
                Set<SnapshotDocument> docs = null;
                if (snapshotId != null) {
                    docs = SnapshotArchiveManager
                            .getDocumentsWithSnapshotId(mActivity, snapshotId);
                } else {
                    assert printJobId != null;
                    docs = SnapshotArchiveManager
                            .getDocumentsWithJobId(mActivity, printJobId);
                }
                if (docs.isEmpty()) {
                    return null;
                } else {
                    return docs.iterator().next();
                }
            }

            @Override
            protected void onPostExecute(SnapshotDocument result) {
                if (result == null) {
                    return;
                }
                if (result.getSnapshotId() != null) {
                    // Always display the page URL first. The infobar should pop
                    // up to let the
                    // user open the downloaded snapshot if desired.
                    loadUrl(result.getPageUri().toString(), ChromeView.PAGE_TRANSITION_LINK);
                    setSnapshotId(result.getSnapshotId());
                } else if (result.getJobId() != null &&
                        SnapshotArchiveManager.getSnapshotViewableState(result) ==
                        SnapshotViewableState.READY) {
                    SnapshotArchiveManager.openDownloadedFileAsNewTaskActivity(
                            result, mActivity, false);
                }
            }
        }.execute();
        return true;
    }

    private boolean shouldUpdateThumbnail() {
        return nativeShouldUpdateThumbnail(mChromeView, getUrl());
    }

    private void updateThumbnail(Bitmap bitmap) {
        nativeUpdateThumbnail(mNativeTab, mChromeView, bitmap);
        Bundle bundle = new Bundle();
        bundle.putString("url", getUrl());
        broadcastNotification(ChromeNotificationCenter.THUMBNAIL_UPDATED, bundle);
    }

    /**
     * @return The ID of the bookmark associated with the current URL (or -1 if no such bookmark
     *             exists).
     */
    public long getBookmarkId() {
        return nativeGetBookmarkId(mChromeView);
    }

    /**
     * @return The background color of the tab.
     */
    public int getBackgroundColor() {
        return getView() != null ? getView().getBackgroundColor() : Color.WHITE;
    }
}
