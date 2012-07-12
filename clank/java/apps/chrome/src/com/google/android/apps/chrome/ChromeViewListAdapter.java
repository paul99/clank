// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome;

import static com.google.android.apps.chrome.ChromeNotificationCenter.broadcastImmediateNotification;
import static com.google.android.apps.chrome.ChromeNotificationCenter.broadcastNotification;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.preference.PreferenceActivity;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

import com.google.android.apps.chrome.ChromeNotificationCenter.ChromeNotificationHandler;
import com.google.android.apps.chrome.preferences.ClearBrowsingDataDialogFragment;
import com.google.android.apps.chrome.preferences.PreferenceHeaders;
import com.google.android.apps.chrome.services.GoogleServicesNotificationController;
import com.google.android.apps.chrome.snapshot.SnapshotArchiveManager;
import com.google.android.apps.chrome.snapshot.SnapshotViewableState;
import com.google.android.apps.chrome.thumbnail.ThumbnailBitmap;
import com.google.android.apps.chrome.thumbnail.ThumbnailCache;
import com.google.android.apps.chrome.utilities.MathUtils;
import com.google.android.apps.chrome.utilities.URLUtilities;
import com.google.android.apps.chrome.widgetcontroller.fullscreen.Fullscreen;

import org.chromium.base.AccessedByNative;
import org.chromium.base.CalledByNative;
import org.chromium.content.browser.ChromeView;
import org.chromium.content.browser.TraceEvent;

import com.google.common.annotations.VisibleForTesting;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * This class manages all the ChromeViews in the app.  As it manipulates views, it must be
 * instantiated and used in the UI Thread.  It acts as a TabModel which delegates all
 * TabModel methods to the active model that it contains.
 */
public class ChromeViewListAdapter extends BaseAdapter implements TabModel, TabModelSelector {
    private static final int NORMAL_TAB_MODEL_INDEX = 0;
    private static final int INCOGNITO_TAB_MODEL_INDEX = 1;

    /**
     * The number of milliseconds to delay creation of the cached NTP when receiving the
     * appropriate events.
     */
    private static final long NTP_CACHE_CREATE_DELAY_MS = 300;

    /**
     * The number of milliseconds to delay creation of the cached NTP when the app was launched
     * with a URL to load on startup. This is longer than the above because when launched with
     * a URL the expectation is for that URL to load and run immediately and we should give
     * it sufficient time. This is especially important if the loaded page is computationally
     * heavy with a lot of graphics/script such as benchmarks. When launched with a URL the
     * user will also mostly be reading/using that page for a while so we can lazily generate
     * the NTP image, as it's probably unlikely the user would need it soon in this case.
     */
    private static final long NTP_CACHE_CREATE_DELAY_URL_LAUNCH_MS = 60000;

    /**
     * The number of milliseconds from the time we receive notification from NTP that it
     * has loaded all initial resources to when we take a snapshot image. This time is to allow
     * the NTP to complete its layout and render properly.
     */
    private static final long NTP_PRIME_BITMAP_CAPTURE_DELAY_MS = 1000;

    /**
     * The application ID used for tabs opened from an application that does not specify an app ID
     * in its VIEW intent extras.
     */
    private static final String UNKNOWN_APP_ID = "com.google.android.apps.chrome.unknown_app";

    public static final String NTP_URL = "chrome://newtab/";
    public static final String BOOKMARKS_URL = "chrome://newtab/#bookmarks";
    public static final String WELCOME_URL = "chrome://welcome/";

    public static final String NTP_CACHED_TAG = "#cached_ntp";

    private Activity mActivity;

    /** Flag set when the asynchronous loading of tabs is finishjed. */
    private final AtomicBoolean mAllTabsLoaded = new AtomicBoolean(false);
    private final TabPersistentStore mTabSaver;
    private TabModel[] mModels;
    private int mActiveModelIndex = NORMAL_TAB_MODEL_INDEX;
    /** Next available id for new tabs. */
    private int mNextId = 0;
    private TabModelOrderController mOrderController;

    // According to tedchoc, use of the mPreloadWebViewContainer as a
    // container for the mCachedNtpTab is to force a draw of the
    // cached NTP when in overview mode.  This allows creation of a
    // bitmap to animate on when creating a new tab page from overview
    // mode.
    private ViewGroup mPreloadWebViewContainer;

    private boolean mInOverviewMode;

    private BroadcastReceiver mSnapshotDownloadBroadcastReceiver;

    private ThumbnailCache mThumbnailCache;

    private Fullscreen mFullscreen;

    private long mNtpCacheCreateDelayMs = NTP_CACHE_CREATE_DELAY_MS;

    @VisibleForTesting
    public static interface ClearBrowsingDataActivityStarter {
        public void startActivity();
    }

    /**
     * Used to allow delegating the preference activity creation to UI tests so they can
     * properly access it.
     */
    private ClearBrowsingDataActivityStarter mClearBrowsingDataActivityStarter;

    @VisibleForTesting
    public void setClearBrowsingDataActivityStarter(ClearBrowsingDataActivityStarter starter) {
        mClearBrowsingDataActivityStarter = starter;
    }

    private Tab mVisibleTab;

    private class ChromeTabModel implements TabModel {
        private List<Tab> mTabs;
        private int mIndex = INVALID_TAB_INDEX;
        private Tab mCachedNtpTab;
        private InstantTab mInstantTab;
        private boolean mIsIncognito = false;
        /** Native Tab pointer which will be set by nativeInit(). */
        @AccessedByNative
        private int mNativeChromeTabModel = 0;
        // The native window_id for this tab model
        private int mWindowId;

        public ChromeTabModel(boolean incognito) {
            mIsIncognito = incognito;
            mTabs = new ArrayList<Tab>();
            mNativeChromeTabModel = nativeInit();
            mWindowId = nativeGetWindowId(mNativeChromeTabModel);

            mClearBrowsingDataActivityStarter = new ClearBrowsingDataActivityStarter() {
                @Override
                public void startActivity() {
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    intent.setClassName(mActivity, PreferenceHeaders.class.getName());
                    intent.setFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT |
                            Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.putExtra(PreferenceActivity.EXTRA_SHOW_FRAGMENT,
                            ClearBrowsingDataDialogFragment.class.getName());
                    mActivity.startActivity(intent);
                }
            };
        }

        @Override
        public void openNewTab(String url, int parentId, boolean incognito) {
            TabModel model = getModel(incognito);
            model.createNewTab(url,
                               TabLaunchType.FROM_LONGPRESS,
                               getTabIndexById(parentId) + 1,
                               parentId,
                               incognito);
        }

        @Override
        @CalledByNative("ChromeTabModel")
        public boolean isIncognito() {
            return mIsIncognito;
        }

        @Override
        public void destroy() {
            for (Tab tab : mTabs) {
                tab.destroy();
            }
            if (mNativeChromeTabModel != 0) {
                nativeDestroy(mNativeChromeTabModel);
                mNativeChromeTabModel = 0;
            }
        }

        /**
         * Returns a valid new unique id.
         *
         * For security reasons TabPersistentStore might delete anything at and above mNextId.
         * If the id generation need to be changed, make sure if works nicely
         * {@link TabPersistentStore#loadState()}
         *
         * @return A valid new id.
         */
        private int getNewId() {
            return mNextId++;
        }

        private int addTab(int index, Tab tab) {
            assert(tab.isIncognito() == mIsIncognito);
            if (index < 0) {
                mTabs.add(tab);
            } else {
                mTabs.add(index, tab);
                if (index <= mIndex) {
                    mIndex++;
                }
            }
            if (getCurrentModel() != this) {
                // When adding new tabs in the background, make sure we set a valid index when the
                // first one is added.  When in the foreground, calls to setIndex will take care of
                // this.
                mIndex = Math.max(mIndex, 0);
            }
            tab.setWindowId(mWindowId);
            int newIndex = indexOf(tab);
            notifyDataSetChanged();
            return newIndex;
        }

        // Implements TabModel
        @Override
        public void moveTab(int id, int newIndex) {
            newIndex = MathUtils.clamp(newIndex, 0, mTabs.size());

            int curIndex = getTabIndexById(id);

            if (curIndex == INVALID_TAB_INDEX || curIndex == newIndex || curIndex + 1 == newIndex) {
                return;
            }

            Tab tab = mTabs.remove(curIndex);
            if (curIndex < newIndex) --newIndex;

            mTabs.add(newIndex, tab);

            if (curIndex == mIndex) {
                mIndex = newIndex;
            } else if (curIndex < mIndex && newIndex >= mIndex) {
                --mIndex;
            } else if (curIndex > mIndex && newIndex <= mIndex) {
                ++mIndex;
            }

            notifyDataSetChanged();
            notifyTabMoved(id, curIndex, newIndex, mIsIncognito);
        }

        /**
         * If we're closing the last tab for this render process we need to
         * reload the previous tab before closing this one. See
         * http://b/issue?id=4462585
         */
        private void reloadPreviousTabIfNeeded(boolean parentIsIncognito, int index) {
            Tab tab = getModel(parentIsIncognito).getTab(index);
            if (tab == null) {
                return;
            }

            ChromeView chromeView = tab.getView();
            if (chromeView != null && chromeView.needsReload()) {
                chromeView.reload();
            }
        }

        @Override
        public void closeTab(Tab tab) {
            closeTab(tab, true);
        }

        @Override
        public Tab getNextTabIfClosed(int id) {
            Tab tabToClose = getTabById(id);
            if (tabToClose == null) return null;

            Tab currentTab = getCurrentTab();
            int closingTabIndex = indexOf(tabToClose);
            Tab adjacentTab = getTab((closingTabIndex == 0) ? 1 : closingTabIndex - 1);
            Tab parentTab = getModel(tabToClose.parentIsIncognito())
                    .getTabById(tabToClose.getParentId());

            // Determine which tab to select next according to these rules:
            //   * If closing a background tab, keep the current tab selected.
            //   * Otherwise, if not in overview mode, select the parent tab if it exists.
            //   * Otherwise, select an adjacent tab if one exists.
            //   * Otherwise, if closing the last incognito tab, select the current normal tab.
            //   * Otherwise, select nothing.
            Tab nextTab = null;
            if (tabToClose != currentTab && currentTab != null) {
                nextTab = currentTab;
            } else if (parentTab != null && !mInOverviewMode) {
                nextTab = parentTab;
            } else if (adjacentTab != null) {
                nextTab = adjacentTab;
            } else if (mIsIncognito) {
                nextTab = getModel(false).getCurrentTab();
            }

            return nextTab;
        }

        /**
         * This will unregister and destroy the closing tab then select the previous
         * tab.
         */
        @Override
        public void closeTab(Tab tabToClose, boolean animate) {
            if (tabToClose == null) {
                assert false;
                return;
            }
            Tab currentTab = getCurrentTab();
            int closingTabId = tabToClose.getId();
            int closingTabIndex = indexOf(tabToClose);
            Tab adjacentTab = getTab((closingTabIndex == 0) ? 1 : closingTabIndex - 1);

            Tab nextTab = getNextTabIfClosed(closingTabId);

            // Give listeners a chance to do anything they need before the tab goes away.
            blockingNotifyTabClosing(closingTabId, animate);

            // Remove the tab from the model.
            mTabs.remove(tabToClose);
            mTabSaver.removeTabFromQueues(tabToClose);
            mThumbnailCache.remove(closingTabId);
            if (!animate) mThumbnailCache.unpinTexture(closingTabId);

            boolean nextIsIncognito = (nextTab == null) ? false : nextTab.isIncognito();
            int nextTabId = (nextTab == null) ? INVALID_TAB_ID : nextTab.getId();
            int nextTabIndex = (nextTab == null) ? INVALID_TAB_INDEX :
                    getModel(nextIsIncognito).getTabIndexById(nextTabId);

            // Send a delayed notification to trigger animations, etc.
            notifyTabClosed(closingTabId, mIsIncognito, nextTabId, nextIsIncognito);

            // Select the next tab.
            if (nextTab != currentTab) {
                // If we're leaving this model, we need to properly set mIndex on this model.
                if (nextIsIncognito != mIsIncognito) {
                    mIndex = indexOf(adjacentTab);
                }
                // Note - if this is an incognito tab being closed and the next
                // tab is "normal", we will jump the user back to the normal
                // stack. May want to consider some special UI for that.
                reloadPreviousTabIfNeeded(nextIsIncognito, nextTabIndex);
                ChromeTabModel nextModel = (ChromeTabModel) getModel(nextIsIncognito);
                nextModel.setIndex(nextTabIndex, TabSelectionType.FROM_CLOSE);
            } else {
                // The current tab hasn't changed (but its index might have) so just update mIndex.
                mIndex = nextTabIndex;
            }

            if (!mIsIncognito) {
                tabToClose.createHistoricalTab();
            }
            int renderProcessPid = Tab.INVALID_RENDER_PROCESS_PID;
            if (!tabToClose.isSavedAndViewDestroyed()) {
                renderProcessPid = tabToClose.getRenderProcessPid();
            }
            tabToClose.destroy();
            tabToClose.deleteState(mActivity);

            if (mIsIncognito && getCount() == 0) {
                ChromeView.destroyIncognitoProfile();
            }

            if (renderProcessPid != Tab.INVALID_RENDER_PROCESS_PID) {
                // If the render process hosting the closed tab has more tabs open,
                // we may have caches (v8,webcore,..) which can be purged
                // immediately to reclaim memory. Without this operation those
                // caches will remain occupied until a GC happens in future or we
                // force a GC in low memory condition.
                for (Tab tab : mTabs) {
                    if (tab.isSavedAndViewDestroyed()) continue;

                    if (tab.getRenderProcessPid() == renderProcessPid) {
                        tab.purgeRenderProcessNativeMemory();
                        break;
                    }
                }
            }
        }

        @Override
        public void closeTabByIndex(int i) {
            Tab tab = getTab(i);
            if (tab != null) {
                closeTab(tab);
            }
        }

        @Override
        public void closeAllTabs() {
            mTabSaver.cancelLoadingTabs(isIncognito());

            if (mInOverviewMode) {
                Bundle b = new Bundle();
                b.putBoolean("incognito", isIncognito());
                broadcastNotification(ChromeNotificationCenter.TABS_CLOSE_ALL_REQUEST, b);
            } else {
                while (getCount() > 0) {
                    closeTabByIndex(0);
                }
            }
        }

        // Find the tab at the specified position.
        @Override
        @CalledByNative("ChromeTabModel")
        public Tab getTab(int position) {
            if (position < 0 || position >= mTabs.size())
                return null;
            return mTabs.get(position);
        }

        // Find the tab with the specified id.
        @Override
        public Tab getTabById(int id) {
            if (id == INVALID_TAB_ID) return null;
            for (int i = 0; i < mTabs.size(); i++) {
                if (mTabs.get(i).getId() == id) {
                    return mTabs.get(i);
                }
            }
            return null;
        }

        @Override
        public Tab getTabFromView(ChromeView view) {
            if (view == null) return null;
            for (int i = 0; i < mTabs.size(); i++) {
                if (mTabs.get(i).getView() == view) {
                    return mTabs.get(i);
                }
            }
            return null;
        }

        // Returns the index of the Tab with a given id in mTabs
        @Override
        public int getTabIndexById(int id) {
            if (id == INVALID_TAB_ID) return INVALID_TAB_INDEX;
            for (int i = 0; i < mTabs.size(); i++) {
                if (mTabs.get(i).getId() == id) {
                    return i;
                }
            }
            return INVALID_TAB_INDEX;
        }

        @Override
        public int getTabIndexByUrl(String url) {
            for (int i = 0; i < mTabs.size(); i++) {
                if (mTabs.get(i).getUrl().contentEquals(url)) {
                    return i;
                }
            }
            return INVALID_TAB_INDEX;
        }

        @Override
        public Tab getCurrentTab() {
            return getTab(index());
        }

        // Index of the given tab in the order of the tab stack.
        @Override
        public int indexOf(Tab tab) {
            return mTabs.indexOf(tab);
        }

        @Override
        @CalledByNative("ChromeTabModel")
        public int index() {
            return mIndex;
        }

        @Override
        public void setIndex(int i) {
            setIndex(i, TabSelectionType.FROM_USER);
        }

        private void setIndex(int i, final TabSelectionType type) {
            Tab currentTab = getCurrentTab();
            int lastId = currentTab != null ? currentTab.getId() : TabModel.INVALID_TAB_ID;
            if (getCurrentModel() != this) {
                Tab otherModelCurrentTab = getCurrentModel().getCurrentTab();
                lastId = otherModelCurrentTab != null ? otherModelCurrentTab.getId()
                        : TabModel.INVALID_TAB_ID;
            }
            if (mIndex == i && getCurrentModel() == this) {
                if (currentTab != null) showTab(currentTab, type);
                notifyDataSetChanged();
                if (currentTab != null) {
                    notifyTabSelected(currentTab.getId(), lastId, type, mIsIncognito);
                }
                return;
            }

            TraceEvent.begin();
            if (getCurrentModel() != this) {
                selectModel(mIsIncognito);
            }

            if (mTabs.size() <= 0) {
                mIndex = INVALID_TAB_INDEX;
            } else {
                mIndex = MathUtils.clamp(i, 0, mTabs.size() - 1);
            }


            if (getCurrentTab() == null) {
                notifyDataSetChanged();
            } else {
                // Prime LRU cache in case bitmap reads show up
                // between now and when the Runnable gets executed.
                // This prevents a selected tab from flashing
                // approximate then back.
                // TODO(jrg): this shouldn't be needed.
                mThumbnailCache.getTexture(getCurrentTab().getId(), true);

                // TODO(joth): Make this even faster, Tab.show() can still block for over 50ms.
                // See http://b/5278198 and http://b/5035061
                Tab t = getCurrentTab();
                if (t == null) return;
                showTab(t, type);
                // notifyDataSetChanged() can call into
                // ChromeViewHolderTablet.handleTabChangeExternal(),
                // which will eventually move the ChromeView
                // onto the current view hieratchy (with
                // addView()).
                notifyDataSetChanged();
                if (type == TabSelectionType.FROM_USER) {
                    // We only want to record when the user actively switches to a different tab.
                    UmaRecordAction.tabSwitched();
                }
                notifyTabSelected(getCurrentTab().getId(), lastId, type, mIsIncognito);
                // Don't declare victory until this deferred task has completed.
            }
            TraceEvent.end();
        }

        private void showTab(Tab tab, final TabSelectionType type) {
            if (mVisibleTab != null && mVisibleTab == tab && !mVisibleTab.isHidden()) return;

            if (tab == null) return;

            if (mVisibleTab != null && mVisibleTab != tab
                    && !mVisibleTab.isSavedAndViewDestroyed()) {
                // TODO(dtrainor): Once we figure out why we can't grab a snapshot from the current
                // tab when we have other tabs loading from external apps remove the checks for
                // FROM_EXTERNAL_APP/FROM_NEW.
                if (type != TabSelectionType.FROM_CLOSE &&
                        (tab.getLaunchType() != TabModel.TabLaunchType.FROM_EXTERNAL_APP ||
                        type != TabSelectionType.FROM_NEW)) {
                    cacheTabBitmap(mVisibleTab);
                }
                mVisibleTab.hide();
                mTabSaver.addTabToSaveQueue(mVisibleTab);
            }

            tab.show(mActivity);
            mVisibleTab = tab;
        }

        // The number of open tabs in this model currently.
        @Override
        @CalledByNative("ChromeTabModel")
        public int getCount() {
            return mTabs.size();
        }

        @Override
        public Tab launchNTP() {
            TraceEvent.begin();
            Tab t = createNewTab(NTP_URL, TabModel.TabLaunchType.FROM_OVERVIEW);
            TraceEvent.end();
            return t;
        }

        /**
         * Creates a new tab with the given url at the end of the tab stack, and activates it
         * to bring it to the foreground.
         */
        @Override
        public Tab createNewTab(String url, TabModel.TabLaunchType type) {
            return createNewTab(url, type, INVALID_TAB_INDEX);
        }

        /**
         * Creates a new tab with the given url at the specified position in the tab stack,
         * and activates it to bring it to the foreground. The new tab is inserted before any
         * existing tab at the given position. If position is negative, the new tab is added
         * to the end of the stack. The specified position must be <= the size of the tab stack.
         *
         * If the tab is launched from a long press on a link, the tab is created as a child of
         * the current page and it is added after the current tab.
         */
        public Tab createNewTab(String url, TabModel.TabLaunchType type, int position) {
            int parentId = Tab.NO_PARENT_ID;
            boolean parentIsIncognito = mIsIncognito;
            Tab currentTab = getCurrentTab();

            if (currentTab != null && (type == TabLaunchType.FROM_LINK ||
                    type == TabLaunchType.FROM_LONGPRESS)) {
                parentId = currentTab.getId();
                parentIsIncognito = currentTab.isIncognito();
                int parentIndex = getTabIndexById(parentId);
                if (parentIndex != INVALID_TAB_INDEX) {
                    position = parentIndex + 1;
                }
            }

            return createNewTab(url, type, position, parentId, parentIsIncognito);
        }

        @Override
        public Tab createNewTab(String url, TabModel.TabLaunchType type, int position,
                int parentId, boolean parentIsIncognito) {
            boolean openInForeground = mOrderController.willOpenInForeground(type, mIsIncognito);
            int tabId = getNewId();
            int sourceTabId = getCurrentTabId();
            notifyTabCreating(tabId, sourceTabId, type, mIsIncognito, openInForeground);

            Tab t;
            boolean triggerUrlLoad = true;
            t = new Tab(tabId, this, mActivity, mIsIncognito, type, parentId);

            url = NewTabPageUtil.appendNtpSectionIfNeeded(mActivity, isIncognito(), url);

            if (parentIsIncognito) {
                t.setParentIsIncognito();
            }

            position = mOrderController.determineInsertionIndex(type, position, t);
            int newIndex = addTab(position, t);
            if (triggerUrlLoad) {
                url = URLUtilities.fixUrl(url);
                t.loadUrl(url, getTransitionType(type));
            }
            notifyTabCreated(tabId, sourceTabId, url, type, 0, 0, mIsIncognito, openInForeground);
            if (openInForeground) {
                selectModel(t.isIncognito());
                setIndex(newIndex, TabSelectionType.FROM_NEW);
            }
            return t;
        }

        @Override
        public void prefetchUrl(String url, int transition) {
            url = URLUtilities.fixUrl(url);
            if (url.startsWith("about:") || url.startsWith("chrome:")) return;
            if (mInstantTab == null) {
                mInstantTab = new InstantTab(mActivity, getCurrentTab(),
                        mIsIncognito || getCurrentTab().parentIsIncognito());
            }
            if (mInstantTab.getPrefetchedUrl() == null ||
                    !mInstantTab.getPrefetchedUrl().equals(url)) {
                mInstantTab.prefetchUrl(url, transition);
            }
        }

        @Override
        public void endPrefetch() {
            if (mInstantTab != null) {
                mInstantTab.destroy();
                mInstantTab = null;
            }
        }

        @Override
        public boolean commitPrefetchUrl(String url) {
            boolean success = false;
            if (mInstantTab != null && mInstantTab.getPrefetchedUrl() != null &&
                    mInstantTab.getPrefetchedUrl().equals(url) &&
                    mInstantTab.commitPrefetch()) {
                // The call to commitPrefetch replaces the Tab's ChromeView when successful.
                notifyDataSetChanged();  // Needed to update/redraw the current tab.
                success = true;
            }
            endPrefetch();
            return success;
        }

        @Override
        public String getPrefetchedUrl() {
            if (mInstantTab != null) {
                return mInstantTab.getPrefetchedUrl();
            }
            return null;
        }

        @Override
        public Tab launchUrl(String url, TabModel.TabLaunchType type) {
            return createNewTab(url, type);
        }

        @Override
        public Tab launchUrlFromExternalApp(String url, String appId, boolean forceNewTab) {
            boolean isLaunchedFromChrome = TextUtils.equals(appId, mActivity.getPackageName());
            if (forceNewTab && !isLaunchedFromChrome) {
                // We don't associate the tab with that app ID, as it is assumed that if the
                // application wanted to open this tab as a new tab, it probably does not want it
                // reused either.
                return launchUrl(url, TabLaunchType.FROM_EXTERNAL_APP);
            }

            if (TextUtils.isEmpty(appId)) {
                // If we have no application ID, we use a made-up one so that these tabs can be
                // reused.
                appId = UNKNOWN_APP_ID;
            }
            TabModel tabModel = mModels[NORMAL_TAB_MODEL_INDEX];
            // Let's try to find an existing tab that was started by that app.
            for (int i = 0; i < tabModel.getCount(); i++) {
                Tab tab = tabModel.getTab(i);
                if (appId.equals(tab.getAppAssociatedWith())) {
                    // We don't reuse the tab, we create a new one at the same index instead.
                    // Reusing a tab would require clearing the navigation history and clearing the
                    // contents (we would not want the previous content to show).
                    Tab newTab = createNewTab(url, TabLaunchType.FROM_EXTERNAL_APP, i,
                            Tab.NO_PARENT_ID, false);
                    newTab.associateWithApp(appId);
                    closeTab(tab, false);
                    return newTab;
                }
            }

            // No tab for that app, we'll have to create a new one.
            Tab tab = launchUrl(url, TabLaunchType.FROM_EXTERNAL_APP);
            tab.associateWithApp(appId);
            return tab;
        }

        @Override
        public Tab createTabWithNativeContents(int id, int nativeWebContents, TabLaunchType type,
                int index, int parentId, String appId) {
            boolean selectTab = type != TabLaunchType.FROM_RESTORE;
            int sourceTabId = getCurrentTabId();
            notifyTabCreating(id, sourceTabId, type, mIsIncognito, selectTab);

            int x = 0, y = 0;
            Tab currentTab = getCurrentTab();
            if (currentTab != null && currentTab.getView() != null) {
                x = currentTab.getView().getSingleTapX();
                y = currentTab.getView().getSingleTapY();
            }
            Tab t = new Tab(id, this, mActivity, mIsIncognito, nativeWebContents, type, parentId);
            if (!TextUtils.isEmpty(appId)) {
                t.associateWithApp(appId);
            }
            index = mOrderController.determineInsertionIndex(type, index, t);
            int newIndex = addTab(index, t);

            // TODO(johnme): The tab's url is often not yet available. Fix this.
            notifyTabCreated(t.getId(), sourceTabId, t.getView().getUrl(), type, x, y,
                             mIsIncognito, selectTab);
            // Restore is handling the active index by himself.
            if (selectTab) {
                setIndex(newIndex, TabModel.TabSelectionType.FROM_NEW);
            }
            return t;
        }

        @Override
        public Tab createTabWithNativeContents(int nativeWebContents, int parentId, String appId) {
            int index = getTabIndexById(parentId);

            // If we have a valid parent index increment by one so we add this tab directly after
            // the parent tab.
            if (index >= 0) index++;

            return createTabWithNativeContents(getNewId(), nativeWebContents, TabLaunchType.FROM_LINK,
                    index, parentId, appId);
        }

        @Override
        public ThumbnailCache getThumbnailCache() {
            return mThumbnailCache;
        }

        @Override
        public boolean supportIntentFilters() {
            return true;
        }

        /**
         * Caches the bitmap of the tab if necessary.
         *
         * @param tab The tab whose bitmap should be cached.
         */
        private void cacheTabBitmap(Tab tab) {
            if (ChromeActivity.isTabletUi(mActivity)
                    && ChromeView.hasHardwareAcceleration(mActivity)) {
                 return;
             }

            // For optimization purpose we do not regenerate the tab bitmap
            // in overview mode because it has already been created when
            // switching to overview (see TabStackState.show).
            if (tab != null
                    && tab.getView() != null
                    && tab.getView().getVisibility() == View.VISIBLE) {
                // Phone need 2 steps to get the tab to a thumbnail:
                // - This is super fast on phone.
                mThumbnailCache.cacheTabThumbnail(tab);
                // - This is a no-op on tablet.
                mThumbnailCache.moveViewTextureToCache(tab.getId());
            }
        }

        private Tab getTabOrCachedNtpById(int id) {
            Tab tab = getTabById(id);
            if (tab == null && mCachedNtpTab != null && mCachedNtpTab.getId() == id) {
                tab = mCachedNtpTab;
            }
            return tab;
        }

        private void createCachedNtp() {
            if (mCachedNtpTab != null ||
                    mPreloadWebViewContainer == null ||
                    mThumbnailCache.isTextureCached(NTP_TAB_ID, false, true)) {
                return;
            }

            final Runnable renderTabTask = new Runnable() {
                @Override
                public void run() {
                    if (mCachedNtpTab == null || mPreloadWebViewContainer == null) {
                        return;
                    }

                    // If the NTP cache already has been parented, then the following logic is
                    // unnecessary and has already occurred.
                    if (mCachedNtpTab.getView().getParent() != null) return;

                    mCachedNtpTab.getView().setVisibility(View.VISIBLE);
                    mPreloadWebViewContainer.setVisibility(View.VISIBLE);
                    mPreloadWebViewContainer.addView(
                            mCachedNtpTab.getView(),
                            ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.MATCH_PARENT);
                    mCachedNtpTab.show(mActivity);
                }
            };

            // Generate a cached NTP.
            mCachedNtpTab = new Tab(getNewId(), ChromeTabModel.this, mActivity, mIsIncognito,
                    TabLaunchType.FROM_OVERVIEW, Tab.NO_PARENT_ID);
            // Add a special tag to indicate that it's a request for cached NTP tab.
            // The native framework will clear the tag when processing this request.
            mCachedNtpTab.loadUrl(NTP_URL + NTP_CACHED_TAG);
            mCachedNtpTab.getView().setBackgroundColor(Color.WHITE);
            // Render the tab in a separate UI task to give other tasks
            // a chance to use the UI thread.
            new Handler().post(renderTabTask);
        }

        private void createDelayedCacheNtp(int selectedTabId) {
            // For memory reasons, don't create a cached NTP tab for incognito, since
            // otherwise we would always have 2 cached NTPs.
            if (isIncognito() || mThumbnailCache.isTextureCached(NTP_TAB_ID, false, true)) {
                // Calling get texture here to make sure we load the texture from disk if it isn't
                // in the cache already.
                mThumbnailCache.getTexture(TabModel.NTP_TAB_ID, true);
                return;
            }

            // If the first page to load was an NTP itself, we can take a snapshot of
            // that. So don't need to create a cached NTP in that case.
            if (getTabById(selectedTabId) != null &&
                    getTabById(selectedTabId).getUrl().startsWith(NTP_URL)) {
                return;
            }

            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    createCachedNtp();
                }
            }, mNtpCacheCreateDelayMs);
            // Reset the delay timeout to default for cached NTPs created in future.
            mNtpCacheCreateDelayMs = NTP_CACHE_CREATE_DELAY_MS;
        }

        private void clearCachedNtp() {
            if (mCachedNtpTab == null) {
                return;
            }
            ChromeView ntpChromeView = mCachedNtpTab.getView();
            if (ntpChromeView != null &&
                    ntpChromeView.getParent() != null &&
                    mPreloadWebViewContainer != null) {
                mPreloadWebViewContainer.removeView(ntpChromeView);
            }
            mCachedNtpTab.destroy();
            mCachedNtpTab = null;
        }

        private void clearCachedNtpAndThumbnails() {
            clearCachedNtp();
            mThumbnailCache.handleLowMemory(true);
        }

        private void onNtpResourcesLoaded(Tab tab) {
            if (isIncognito()) return;

            // Cached NTP has fully loaded with all images etc. so we can take
            // a snapshot of it and tear it down. But there is no guarantee that
            // the NTP would have been drawn yet, so generateFullsizeBitmap() may return
            // an empty bitmap if called immediately. Hence we use a delay here.
            final int tabId = tab.getId();
            new Handler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    Tab tab = getTabById(tabId);
                    if (tab == null && mCachedNtpTab != null && mCachedNtpTab.getId() == tabId) {
                        tab = mCachedNtpTab;
                    }
                    if (tab != null) {
                        Bitmap bitmap = tab.generateFullsizeBitmap();
                        if (bitmap != null) {
                            mThumbnailCache.putTexture(NTP_TAB_ID, new ThumbnailBitmap(bitmap,
                                    ThumbnailBitmap.defaultScale()));
                        }
                        if (tab == mCachedNtpTab) {
                            clearCachedNtp();
                        }
                    }
                }
            }, NTP_PRIME_BITMAP_CAPTURE_DELAY_MS);
        }

        @Override
        public void registerForContextMenu(View view) {
            ChromeViewListAdapter.this.registerForContextMenu(view);
        }

        @Override
        public void unregisterForContextMenu(View view) {
            ChromeViewListAdapter.this.unregisterForContextMenu(view);
        }

        @Override
        public boolean overrideScroll(float deltaX, float deltaY, float scrollX, float scrollY) {
            return ChromeViewListAdapter.this.overrideScroll(deltaX, deltaY, scrollX, scrollY);
        }

        @CalledByNative("ChromeTabModel")
        public void createTabWithNativeContents(int nativeWebContents) {
            if (getCurrentModel() != null) {
                // We don't provide an opener app id as this tab's opening has been triggered by
                // Chrome, not an external intent and the tab won't therefore be reused.
                getCurrentModel().createTabWithNativeContents(nativeWebContents,
                        getCurrentTab().getId(), null);
            }
        }

        @CalledByNative("ChromeTabModel")
        public void openClearBrowsingData() {
            mClearBrowsingDataActivityStarter.startActivity();
        }

        @Override
        @CalledByNative("ChromeTabModel")
        public boolean areAllTabsLoaded() {
            return mAllTabsLoaded.get();
        }

        @NativeCall("ChromeTabModel")
        private native int nativeInit();

        @NativeCall("ChromeTabModel")
        private native void nativeDestroy(int nativeTabModel);

        @NativeCall("ChromeTabModel")
        private native int nativeGetWindowId(int nativeTabModel);

        @NativeCall("ChromeTabModel")
        private native void nativeBroadcastAllTabsLoaded(int nativeTabModel);
    }

    public ChromeViewListAdapter(Activity activity, boolean startIncognito,
            boolean newInstanceLaunchedWithUrl) {
        mActiveModelIndex = startIncognito ? INCOGNITO_TAB_MODEL_INDEX : NORMAL_TAB_MODEL_INDEX;
        mActivity = activity;
        mTabSaver = new TabPersistentStore(this, mActivity);
        mOrderController = new TabModelOrderController(this);

        // If we were a new instance & launched with a URL, give a LOT of time before
        // creating a cached NTP so the newly opened page doesn't get slowed down by
        // our NTP creation.
        if (newInstanceLaunchedWithUrl) {
            mNtpCacheCreateDelayMs = NTP_CACHE_CREATE_DELAY_URL_LAUNCH_MS;
        }
    }

    private static final int[] NOTIFICATIONS = {
            ChromeNotificationCenter.OVERVIEW_MODE_SHOW_START,
            ChromeNotificationCenter.OVERVIEW_MODE_HIDE_FINISHED,
            ChromeNotificationCenter.TABS_STATE_LOADED,
            ChromeNotificationCenter.LOAD_STOPPED,
            ChromeNotificationCenter.NEW_TAB_PAGE_READY,
            ChromeNotificationCenter.TAB_CREATED,
            ChromeNotificationCenter.PAGE_URL_CHANGED,
            ChromeNotificationCenter.AUTO_LOGIN_RESULT,
            ChromeNotificationCenter.AUTO_LOGIN_DISABLED,
    };

    private ChromeNotificationHandler mNotificationHandler = new ChromeNotificationHandler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case ChromeNotificationCenter.OVERVIEW_MODE_SHOW_START:
                    handleOnOverviewModeShown();
                    break;
                case ChromeNotificationCenter.OVERVIEW_MODE_HIDE_FINISHED:
                    handleOnOverviewModeHidden();
                    break;
                case ChromeNotificationCenter.LOAD_STOPPED:
                    handleOnPageLoadStopped(msg.getData().getInt("tabId"));
                    break;
                case ChromeNotificationCenter.NEW_TAB_PAGE_READY:
                    handleNewTabPageReady(msg.getData().getInt("tabId"));
                    break;
                case ChromeNotificationCenter.TABS_STATE_LOADED:
                    if (!mAllTabsLoaded.getAndSet(true)) {
                        // This is the first time we set |mAllTabsLoaded|,
                        // so we need to broadcast.
                        ChromeTabModel model = (ChromeTabModel) getModel(false);

                        assert model != null;
                        if (model != null) {
                            model.nativeBroadcastAllTabsLoaded(model.mNativeChromeTabModel);
                        }
                    }
                    break;
                case ChromeNotificationCenter.TAB_CREATED:
                    // Purposefully fall through
                case ChromeNotificationCenter.PAGE_URL_CHANGED:
                    Tab tab = getTabById(msg.getData().getInt("tabId"));
                    if (tab != null) {
                        mThumbnailCache.invalidateIfChanged(tab);
                    }
                    break;
                case ChromeNotificationCenter.AUTO_LOGIN_RESULT:
                    Bundle b = msg.getData();
                    String accountName = b.getString("accountName");
                    String authToken = b.getString("authToken");
                    boolean success = b.getBoolean("success");
                    String result = b.getString("result");
                    handleAutoLoginResult(accountName, authToken, success, result);
                    break;
                case ChromeNotificationCenter.AUTO_LOGIN_DISABLED:
                    handleAutoLoginDisabled();
                    break;
            }
        }
    };

    private void handleNewTabPageReady(int id) {
        ChromeTabModel model = (ChromeTabModel) getModel(false);

        assert model != null;
        if (model == null) return;

        Tab tab = model.getTabOrCachedNtpById(id);

        if (tab != null) model.onNtpResourcesLoaded(tab);
    }

    private void handleOnOverviewModeHidden() {
        mInOverviewMode = false;
        if (mPreloadWebViewContainer != null) {
            mPreloadWebViewContainer.setVisibility(View.GONE);
        }
    }

    private void handleOnOverviewModeShown() {
        mInOverviewMode = true;
    }

    private void handleOnPageLoadStopped(int tabId) {
        ChromeTabModel normalModel = (ChromeTabModel) getModel(false);

        assert normalModel != null;
        if (normalModel != null) normalModel.createDelayedCacheNtp(tabId);

        for (int i = 0; i < mModels.length; i++) {
            if (mModels[i] == null) continue;
            Tab tab = mModels[i].getTabById(tabId);

            if (tab != null) {
                mTabSaver.addTabToSaveQueue(tab);
                break;
            }
        }
    }

    private void handleAutoLoginResult(String accountName, String authToken, boolean success,
            String result) {
        if (mModels == null) return;

        for (TabModel model : mModels) {
            for (int i = 0; i < model.getCount(); i++) {
                Tab tab = model.getTab(i);
                if (tab != null && tab.getInfoBarContainer() != null) {
                    tab.getInfoBarContainer().dismissAutoLogins(accountName, authToken, success,
                            result);
                }
            }
        }
    }

    private void handleAutoLoginDisabled() {
        for (TabModel model : mModels) {
            for (int i = 0; i < model.getCount(); i++) {
                Tab tab = model.getTab(i);
                if (tab != null && tab.getInfoBarContainer() != null) {
                    tab.getInfoBarContainer().dismissAutoLogins();
                }
            }
        }
    }

    void onNativeLibraryReady() {
        mThumbnailCache = new ThumbnailCache(mActivity);
        // If reads are stacking up we're sweeping real quick.
        // Allow some to get thrown away so we can keep up.
        if (ChromeActivity.isTabletUi(mActivity)) {
            mThumbnailCache.setStackedReads(false);
        }
        mModels = new TabModel[2];
        mModels[0] = new ChromeTabModel(false);
        mModels[1] = new ChromeTabModel(true);

        mSnapshotDownloadBroadcastReceiver = new SnapshotStateBroadcastReceiver();
        mActivity.registerReceiver(mSnapshotDownloadBroadcastReceiver,
                new IntentFilter(SnapshotArchiveManager.ACTION_SNAPSHOT_STATE_UPDATE));

        ChromeNotificationCenter.registerForNotifications(
                NOTIFICATIONS, mNotificationHandler);
    }

    /**
     * A broadcast receiver for snapshot state changes. This class only posts tasks to the
     * main handler.
     */
    private class SnapshotStateBroadcastReceiver extends BroadcastReceiver {

        private Handler mHandler;

        private SnapshotStateBroadcastReceiver() {
            mHandler = new Handler(Looper.getMainLooper());
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            mHandler.post(new UpdateSnapshotStateTask(intent));
        }
    }

    /**
     * Updating snapshot infobars has to be done on the main thread, and this is the task that
     * updates the UI.
     */
    private class UpdateSnapshotStateTask implements Runnable {

        private final Intent mIntent;

        public UpdateSnapshotStateTask(Intent intent) {
            mIntent = intent;
        }

        @Override
        public void run() {
            String snapshotId = mIntent.getStringExtra(
                    SnapshotArchiveManager.EXTRA_SNAPSHOT_ID);
            String snapshotUri = mIntent.getStringExtra(
                    SnapshotArchiveManager.EXTRA_SNAPSHOT_URI);
            String snapshotViewableStateString = mIntent.getStringExtra(
                    SnapshotArchiveManager.EXTRA_SNAPSHOT_VIEWABLE_STATE);
            SnapshotViewableState snapshotViewableState =
                    SnapshotViewableState.fromValue(snapshotViewableStateString);
            Uri parsedSnapshotUri = snapshotUri == null ? null : Uri.parse(snapshotUri);
            if (snapshotId != null) {
                // We only open URLs sent from another device in non incognito tabs, so it is safe
                // to ignore incognito tabs.
                boolean handled = false;
                for (int i = 0; i < mModels[0].getCount(); ++i) {
                    if (mModels[0].getTab(i).snapshotStateQueryResult(snapshotId, parsedSnapshotUri,
                            snapshotViewableState)) {
                        handled = true;
                        break;
                    }
                }
                if (!handled &&
                        mIntent.hasExtra(SnapshotArchiveManager.EXTRA_SNAPSHOT_ERROR_MESSAGE)) {
                    // Tab with offline copy has already been closed, and this was an error.
                    // We need to show an Android notification.
                    GoogleServicesNotificationController notificationController =
                            GoogleServicesNotificationController.getInstance();
                    // Using the document ID as the unique ID, with the snapshot ID hashcode as a
                    // backup.
                    int uniqueId = mIntent.getIntExtra(SnapshotArchiveManager.EXTRA_DOCUMENT_ID,
                            snapshotId.hashCode());
                    notificationController.showOneOffNotification(uniqueId,
                            mIntent.getStringExtra(
                                    SnapshotArchiveManager.EXTRA_SNAPSHOT_ERROR_MESSAGE));
                }
            }
        }
    }

    public void onActivityDestroyed() {
        destroy();
    }

    public void setFullscreenHandler(Fullscreen fullscreen) {
        mFullscreen = fullscreen;
    }

    @Override
    public boolean isIncognito() {
        return getCurrentModel() != null && getCurrentModel().isIncognito();
    }

    @Override
    public Tab createNewTab(String url, TabLaunchType type) {
        return getCurrentModel() != null ? getCurrentModel().createNewTab(url, type) : null;
    }

    @Override
    public Tab createNewTab(String url, TabLaunchType type, int position, int parentId,
            boolean parentIsIncognito) {
        return getCurrentModel() != null ? getCurrentModel().createNewTab(
                url, type, position, parentId, parentIsIncognito) : null;
    }

    @Override
    public void moveTab(int id, int newIndex) {
        if (getCurrentModel() != null) getCurrentModel().moveTab(id, newIndex);
    }

    @Override
    public void prefetchUrl(String url, int transition) {
        if (getCurrentModel() != null) getCurrentModel().prefetchUrl(url, transition);
    }

    @Override
    public void endPrefetch() {
        if (getCurrentModel() != null) getCurrentModel().endPrefetch();
    }

    @Override
    public boolean commitPrefetchUrl(String url) {
        return getCurrentModel() != null && getCurrentModel().commitPrefetchUrl(url);
    }

    @Override
    public String getPrefetchedUrl() {
        return getCurrentModel() != null ? getCurrentModel().getPrefetchedUrl() : null;
    }

    @Override
    public void closeTab(Tab tab) {
        if (getCurrentModel() != null) getCurrentModel().closeTab(tab);
    }

    @Override
    public Tab getNextTabIfClosed(int id) {
        return getCurrentModel() != null ? getCurrentModel().getNextTabIfClosed(id)
                : null;
    }

    @Override
    public void closeTab(Tab tab, boolean animate) {
        getCurrentModel().closeTab(tab, animate);
    }

    @Override
    public void closeTabByIndex(int i) {
        if (getCurrentModel() != null) getCurrentModel().closeTabByIndex(i);
    }

    @Override
    public void closeAllTabs() {
        if (getCurrentModel() != null) getCurrentModel().closeAllTabs();
    }

    @Override
    public int getCount() {
        return getCurrentModel() != null ? getCurrentModel().getCount() : 0;
    }

    @Override
    public Tab getTab(int position) {
        return getCurrentModel() != null ? getCurrentModel().getTab(position) : null;
    }

    @Override
    public Tab getTabById(int id) {
        return getCurrentModel() != null ? getCurrentModel().getTabById(id) : null;
    }

    @Override
    public Tab getTabFromView(ChromeView view) {
        return getCurrentModel() != null ? getCurrentModel().getTabFromView(view) : null;
    }

    @Override
    public int getTabIndexById(int id) {
        return getCurrentModel() != null ? getCurrentModel().getTabIndexById(id) :
                INVALID_TAB_INDEX;
    }

    @Override
    public int getTabIndexByUrl(String url) {
        return getCurrentModel() != null ? getCurrentModel().getTabIndexByUrl(url) :
                INVALID_TAB_INDEX;
    }

    @Override
    public Tab getCurrentTab() {
        return getCurrentModel() != null ? getCurrentModel().getCurrentTab() : null;
    }

    @Override
    public Tab launchUrl(String url, TabLaunchType type) {
        return getCurrentModel() != null ? getCurrentModel().launchUrl(url, type) : null;
    }

    @Override
    public Tab launchUrlFromExternalApp(String url, String appId, boolean openInNewTab) {
        // This should be called on the real tab model directly.
        assert false;
        return null;
    }

    @Override
    public Tab createTabWithNativeContents(int nativeWebContents, int parentId, String appId) {
        return getCurrentModel() != null ? getCurrentModel().createTabWithNativeContents(
                nativeWebContents, parentId, appId) : null;
    }

    @Override
    public Tab createTabWithNativeContents(int id, int nativeWebContents,
            TabLaunchType type, int index, int parentId, String appId) {
        return getCurrentModel() != null ? getCurrentModel().createTabWithNativeContents(id,
                nativeWebContents, type, index, parentId, appId) : null;
    }

    @Override
    public Tab launchNTP() {
        return getCurrentModel() != null ? getCurrentModel().launchNTP() : null;
    }

    @Override
    public int indexOf(Tab tab) {
        return getCurrentModel() != null ? getCurrentModel().indexOf(tab) : INVALID_TAB_INDEX;
    }

    @Override
    public int index() {
        return getCurrentModel() != null ? getCurrentModel().index() : INVALID_TAB_INDEX;
    }

    @Override
    public void setIndex(int i) {
        if (getCurrentModel() != null) getCurrentModel().setIndex(i);
    }

    @Override
    public ThumbnailCache getThumbnailCache() {
        return getCurrentModel() != null ? getCurrentModel().getThumbnailCache() : null;
    }

    @Override
    public boolean supportIntentFilters() {
        return getCurrentModel() != null && getCurrentModel().supportIntentFilters();
    }

    @Override
    public boolean areAllTabsLoaded() {
        return getCurrentModel() != null && getCurrentModel().areAllTabsLoaded();
    }

    // End of delegation.

    private int getCurrentTabId() {
        Tab tab = getCurrentTab();
        return (tab == null) ? INVALID_TAB_ID : tab.getId();
    }

    private void notifyTabCreating(
            int tabId, int sourceTabId, TabModel.TabLaunchType type, boolean isIncognito,
            boolean willBeSelected) {
        Bundle b = new Bundle();
        b.putInt("tabId", tabId);
        b.putInt("sourceTabId", sourceTabId);
        b.putString("type", type.name());
        b.putBoolean("incognito", isIncognito);
        b.putBoolean("willBeSelected", willBeSelected);
        broadcastImmediateNotification(ChromeNotificationCenter.TAB_CREATING, b);
    }

    private void notifyTabCreated(
            int tabId, int sourceTabId, String url, TabModel.TabLaunchType type,
            int x, int y, boolean isIncognito, boolean willBeSelected) {
        Bundle b = new Bundle();
        b.putInt("tabId", tabId);
        b.putInt("sourceTabId", sourceTabId);
        b.putString("type", type.name());
        b.putString("url", url);
        b.putInt("x", x);
        b.putInt("y", y);
        b.putBoolean("incognito", isIncognito);
        b.putBoolean("willBeSelected", willBeSelected);
        broadcastNotification(ChromeNotificationCenter.TAB_CREATED, b);
    }

    private void notifyTabSelected(
            int tabId, int prevTabId, TabModel.TabSelectionType selectionType,
            boolean isIncognito) {
        Bundle b = new Bundle();
        b.putInt("tabId", tabId);
        b.putInt("lastId", prevTabId);
        b.putString("type", selectionType.name());
        b.putBoolean("incognito", isIncognito);
        broadcastImmediateNotification(ChromeNotificationCenter.TAB_SELECTED, b);
    }

    private void blockingNotifyTabClosing(int tabId, boolean animate) {
        Bundle b = new Bundle();
        b.putInt("tabId", tabId);
        b.putBoolean("animate", animate);
        broadcastImmediateNotification(ChromeNotificationCenter.TAB_CLOSING, b);
    }

    private void notifyTabClosed(int tabId, boolean isIncognito, int nextId,
            boolean nextIncognito) {
        Bundle b = new Bundle();
        b.putInt("tabId", tabId);
        b.putBoolean("incognito", isIncognito);
        b.putInt("nextId", nextId);
        b.putBoolean("nextIncognito", nextIncognito);
        broadcastNotification(ChromeNotificationCenter.TAB_CLOSED, b);
    }

    private void blockingNotifyModelSelected() {
        Bundle b = new Bundle();
        b.putBoolean("incognito", isIncognitoSelected());
        broadcastImmediateNotification(ChromeNotificationCenter.TAB_MODEL_SELECTED, b);
    }

    private void notifyTabMoved(int id, int oldIndex, int newIndex, boolean incognito) {
        Bundle b = new Bundle();
        b.putInt("tabId", id);
        b.putInt("fromPosition", oldIndex);
        b.putInt("toPosition", newIndex);
        b.putBoolean("incognito", incognito);
        broadcastNotification(ChromeNotificationCenter.TAB_MOVED, b);
    }

    /**
     * @param container The layout container that preload web views should be
     *        rendered into (so a bitmap can be rendered before it is added to
     *        the tab stack).
     */
    public void setPreloadWebViewContainer(ViewGroup container) {
        mPreloadWebViewContainer = container;
    }

    @Override
    public Object getItem(int position) {
        return null;
    }

    @Override
    public long getItemId(int position) {
        return 0;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        return getView(position);
    }

    /**
     * Returns the currently active view in the currently active model.
     * This can be null.
     * @return The currently active view, or null.
     */
    public ChromeView getCurrentView() {
        if (getCurrentModel() == null) return null;

        Tab tab = getCurrentModel().getTab(getCurrentModel().index());
        if (tab != null) {
            return tab.getView();
        }
        return null;
    }

    private ChromeView getView(int position) {
        if (getCurrentModel() == null) return null;

        Tab t = getCurrentModel().getTab(position);
        if (t == null)
            return null;
        return t.getView();
    }

    // Implementation of TabModelSelector.
    @Override
    public TabModel getModel(boolean incognito) {
        TabModel model = null;
        if (mModels != null) model = mModels[incognito ? 1 : 0];
        return model != null ? model : EmptyTabModel.getInstance();
    }

    @Override
    public TabModel getCurrentModel() {
        TabModel model = null;
        if (mModels != null) model = mModels[mActiveModelIndex];
        return model != null ? model : EmptyTabModel.getInstance();
    }

    @Override
    public void selectModel(boolean incognito) {
        int newActiveModelIndex = incognito ? INCOGNITO_TAB_MODEL_INDEX : NORMAL_TAB_MODEL_INDEX;
        if (newActiveModelIndex != mActiveModelIndex) {
            mActiveModelIndex = newActiveModelIndex;
            // Re-select the previously selected tab in the model as it will handle re-showing the
            // tab.
            getCurrentModel().setIndex(getCurrentModel().index());
            blockingNotifyModelSelected();

            // Make the call to notifyDataSetChanged() after any delayed events
            // have had a chance to fire. Otherwise, this may result in some
            // drawing to occur before animations have a chance to work.
            new Handler().post(new Runnable() {
                @Override
                public void run() {
                    notifyDataSetChanged();
                }
            });
        }
    }

    @Override
    public int getCurrentModelIndex() {
        return mActiveModelIndex;
    }

    @Override
    public boolean isIncognitoSelected() {
        return (mActiveModelIndex == INCOGNITO_TAB_MODEL_INDEX);
    }

    // End of TabModelSelector.

    private static int getTransitionType(TabLaunchType type) {
        switch (type) {
            case FROM_LINK:
            case FROM_EXTERNAL_APP:
                return ChromeView.PAGE_TRANSITION_LINK;
            case FROM_OVERVIEW:
            case FROM_MENU:
            case FROM_LONGPRESS:
                return ChromeView.PAGE_TRANSITION_START_PAGE;
            default:
                assert false;
                return ChromeView.PAGE_TRANSITION_LINK;
        }
    }

    public void saveState() {
        mTabSaver.saveState();
    }

    public void loadState() {
        mNextId = mTabSaver.loadState();
    }

    public void clearState() {
        mTabSaver.clearState();
    }

    public void clearEncryptedState() {
        mTabSaver.clearEncryptedState();
    }

    /**
     * Return the memory used by all render processes associated with all tabs.
     */
    public int getPrivateDirtyMemoryOfRenderersKBytes() {
        ArrayList<Integer> rendererPids = new ArrayList<Integer>();
        int memoryUsedKBytes = 0;
        for (TabModel model : mModels) {
            for (int i = 0; i < model.getCount(); ++i) {
                Tab tab = model.getTab(i);
                int pid = tab.getRenderProcessPid();
                if (!rendererPids.contains(pid)) {
                    rendererPids.add(pid);
                    memoryUsedKBytes += tab.getRenderProcessPrivateSizeKBytes();
                }
            }
        }
        return memoryUsedKBytes;
    }

    // Implementation of Tab.TabHost

    @Override
    public void registerForContextMenu(View view) {
        mActivity.registerForContextMenu(view);
    }

    @Override
    public void unregisterForContextMenu(View view) {
        mActivity.unregisterForContextMenu(view);
    }

    @Override
    public boolean overrideScroll(float deltaX, float deltaY, float scrollX, float scrollY) {
        if (mFullscreen != null) {
            return mFullscreen.overrideScroll(deltaX, deltaY, scrollX, scrollY);
        }

        return false;
    }

    @Override
    public void destroy() {
        mTabSaver.destroy();
        if (mSnapshotDownloadBroadcastReceiver != null) {
            mActivity.unregisterReceiver(mSnapshotDownloadBroadcastReceiver);
        }
        ChromeNotificationCenter.unregisterForNotifications(
                NOTIFICATIONS, mNotificationHandler);
        // Looks like the activity can be destroyed before native library is ready.
        if (mModels != null) {
            for (TabModel model : mModels) {
                model.destroy();
            }
            mModels = null;
        }
    }

    @Override
    public void openNewTab(String url, int parentId, boolean incognito) {
        TabModel model = getModel(incognito);
        TabLaunchType type = TabLaunchType.FROM_LONGPRESS;
        Tab parent = model.getTabById(parentId);
        Tab newtab = model.createNewTab(url,
                                        type,
                                        model.getTabIndexById(parentId),
                                        parentId,
                                        parent.isIncognito());
        model.setIndex(model.getTabIndexById(newtab.getId()));
    }

    public void bringToFrontOrLaunchUrl(String url, String baseUrl, TabModel.TabLaunchType type) {
        boolean reusingTab = false;
        for (int i = 0; i < getCount(); ++i) {
            if (getTab(i).getUrl().startsWith(baseUrl)) {
                setIndex(i);
                reusingTab = true;
                break;
            }
        }
        if (!reusingTab) {
            launchUrl(url, type);
        }
    }

    /**
     * @return Number of restored tabs on cold startup.
     */
    int getRestoredTabCount() {
        return mTabSaver.getRestoredTabCount();
    }

    /**
     * @return The InstantTab.
     */
    @VisibleForTesting
    InstantTab getInstantTab() {
        return ((ChromeTabModel) getModel(false)).mInstantTab;
    }

    void clearCachedNtpAndThumbnails() {
        ((ChromeTabModel) getModel(false)).clearCachedNtpAndThumbnails();
    }
}
