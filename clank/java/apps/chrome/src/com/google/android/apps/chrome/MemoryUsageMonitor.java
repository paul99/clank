// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Message;
import android.util.Log;

import com.google.android.apps.chrome.ChromeNotificationCenter.ChromeNotificationHandler;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;

class MemoryUsageMonitor {

    private static final String TAG = "MemoryUsageMonitor";

    /**
     * A constant to indicate that we are under severe memory pressure and should free
     * up all we can.
     */
    private static final int FREE_AS_MUCH_AS_POSSIBLE = Integer.MAX_VALUE;

    private static final int[] NOTIFICATIONS = {
            ChromeNotificationCenter.OUT_OF_MEMORY,
            ChromeNotificationCenter.TAB_CREATING,
            ChromeNotificationCenter.TAB_SELECTED,
            ChromeNotificationCenter.TAB_CLOSING,
    };

    private final ChromeNotificationHandler mNotificationHandler = new ChromeNotificationHandler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case ChromeNotificationCenter.TAB_CREATING:
                    onTabCreating(msg.getData().getInt("tabId"),
                            msg.getData().getBoolean("willBeSelected"));
                    break;
                case ChromeNotificationCenter.TAB_SELECTED:
                    onTabSelected(msg.getData().getInt("tabId"));
                    break;
                case ChromeNotificationCenter.TAB_CLOSING:
                    onTabClosing(msg.getData().getInt("tabId"));
                    break;
                case ChromeNotificationCenter.OUT_OF_MEMORY:
                    freeMemory();
                    break;
                default:
                    assert false;
            }
        }
    };

    /**
     * Interface to be implemented by users of MemoryUsageMonitor class.
     */
    public interface Delegate {
        /**
         * Returns the TabModel for normal and incognito modes.
         * @param forIncognito true if requested tab model is for incognito mode.
         * @return the requested TabModel
         */
        public TabModel getTabModel(boolean forIncognito);

        /**
         * Clears the cached NTP and thumbnails to release memory allocated for them.
         */
        public void clearCachedNtpAndThumbnails();

        /**
         * Freeze-dry the given tab, saves its state and destroys the internal instance.
         * The tab will be reloaded from the saved state if user opens it again.
         * @param tabId ID of the tab to be freeze-dried.
         */
        public void saveStateAndDestroyTab(int tabId);

        /**
         * Registers for application level notifications.
         */
        public void registerForNotifications(int[] notifications,
                ChromeNotificationHandler handler);

        /**
         * Unregisters for application level notifications.
         */
        public void unregisterForNotifications(int[] notifications,
                ChromeNotificationHandler handler);
    }

    private Delegate mDelegate;
    private ArrayList<Integer> mTabs = new ArrayList<Integer>();
    private int mMaxActiveTabs;

    /**
     * @param maxActiveTabs The maximum number of tabs that should be kept active at any time.
     * @param delegate Delegate implemented by the caller
     */
    MemoryUsageMonitor(int maxActiveTabs, Delegate delegate) {
        mMaxActiveTabs = maxActiveTabs;
        Log.i(TAG, "Max active tabs = " + mMaxActiveTabs);

        mDelegate = delegate;
        mDelegate.registerForNotifications(NOTIFICATIONS, mNotificationHandler);
    }

    void destroy() {
        mDelegate.unregisterForNotifications(NOTIFICATIONS, mNotificationHandler);
        mDelegate = null;
    }

    /**
     * Creates a Delegate that does the default operations on the given Tab model
     * during the various memory related events.
     */
    static Delegate getDefaultDelegate(final ChromeViewListAdapter model) {
        return new Delegate() {
            @Override
            public void saveStateAndDestroyTab(int tabId) {
                // The tab ids for normal and incognito are part of the same number
                // range and ChromeViewListAdapter.getTabById() only checks the currently
                // selected model. Since we may freeze tabs in either normal or incognito
                // models we need to check in both models.
                Tab tab = model.getModel(false).getTabById(tabId);
                if (tab == null) {
                    tab = model.getModel(true).getTabById(tabId);
                }
                assert tab != null;
                tab.saveStateAndDestroy();
            }

            @Override
            public TabModel getTabModel(boolean forIncognito) {
                return model.getModel(forIncognito);
            }

            @Override
            public void clearCachedNtpAndThumbnails() {
                model.clearCachedNtpAndThumbnails();
            }

            @Override
            public void registerForNotifications(int[] notifications,
                    ChromeNotificationHandler handler) {
                ChromeNotificationCenter.registerForNotifications(notifications, handler);
            }

            @Override
            public void unregisterForNotifications(int[] notifications,
                    ChromeNotificationHandler handler) {
                ChromeNotificationCenter.unregisterForNotifications(notifications, handler);
            }
        };
    }

    /**
     * Returns the default max number of tabs that we should keep active based on the
     * device's memory class.
     */
    static int getDefaultMaxActiveTabs(Context context) {
        // We use the device's memory class to decide the maximum number of tabs we allow to
        // remain loaded. For the baseline devices the memory class is 16 and we will limit it
        // to 2 tabs. For the devices with memory class 24 we allow 3 tabs and so on.
        ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        return Math.max((am.getMemoryClass() / 8), 1);
    }

    /**
     * Tries to save and destroy up to half of the current tabs to free some
     * memory, starting from tabs that are hosted by renderes consuming most
     * of memory.
     */
    private void freeMemory(int expectedToFreeKb) {
        ArrayList<Tab> tabsToSave = new ArrayList<Tab>();

        TabModel normalModel = mDelegate.getTabModel(false);
        for (int i = 0; i < normalModel.getCount(); i++) {
            if (i != normalModel.index() && !normalModel.getTab(i).isSavedAndViewDestroyed()) {
                tabsToSave.add(normalModel.getTab(i));
            }
        }
        // saveStateAndDestroy will save the tab into a byte-array which is still in mem,
        // so we are safe here to store incoginito tabs.
        TabModel incognitoModel = mDelegate.getTabModel(true);
        for (int i = 0; i < incognitoModel.getCount(); i++) {
            if (i != incognitoModel.index() &&
                    !incognitoModel.getTab(i).isSavedAndViewDestroyed()) {
                tabsToSave.add(incognitoModel.getTab(i));
            }
        }

        final Tab foregroundTab = normalModel.getCurrentTab();
        final int foregroundPid = foregroundTab != null ? foregroundTab.getRenderProcessPid() : -1;
        final HashMap<Integer, Integer> memoryUsageByRenderer = new HashMap<Integer, Integer>();
        for (int i = 0; i < tabsToSave.size(); i++) {
            Tab tab = tabsToSave.get(i);
            int pid = tab.getRenderProcessPid();
            if (!memoryUsageByRenderer.containsKey(pid)) {
                // We are using the same metric that is used in TabMemoryHandler
                // for evaluating amount of RAM occupied by all of application
                // processes.
                memoryUsageByRenderer.put(pid, tab.getRenderProcessPrivateSizeKBytes());
            }
        }
        // Sort tabs to put the ones from biggest renderer processes first,
        // except for the process of the current foreground tab. Tabs from the
        // same process are arranged by last usage time, older first.
        Collections.sort(tabsToSave, new Comparator<Tab>() {
            @Override
            public int compare(Tab tab1, Tab tab2) {
                int pid1 = tab1.getRenderProcessPid();
                int pid2 = tab2.getRenderProcessPid();
                if (pid1 != pid2) {
                    if (pid1 == foregroundPid) {
                        return 1;
                    } else if (pid2 == foregroundPid) {
                        return -1;
                    }
                    int memoryInUse1 = memoryUsageByRenderer.get(pid1);
                    int memoryInUse2 = memoryUsageByRenderer.get(pid2);
                    if (memoryInUse1 > memoryInUse2) {
                        return -1;
                    } else if (memoryInUse1 < memoryInUse2) {
                        return 1;
                    }
                } else {
                    long time1 = tab1.getLastShownTimestamp();
                    long time2 = tab2.getLastShownTimestamp();
                    if (time1 < time2) {
                        return -1;
                    } else if (time1 > time2) {
                        return 1;
                    }
                }
                return 0;
            }
        });
        // Delete tabs until we free up the expected amount.
        // Unfortunately, we can do this only by process granularity.
        int lastPid = -1;
        int lastPidPrivateMemorySizeKb = -1;
        int freedSoFarKb = 0;
        int tabIndex = 0;
        int foregroundTabId = foregroundTab != null ? foregroundTab.getId() : Tab.INVALID_TAB_ID;
        int foregroundTabParentId = foregroundTab != null ?
                foregroundTab.getParentId() : Tab.INVALID_TAB_ID;
        for (; tabIndex < tabsToSave.size(); tabIndex++) {
            Tab tab = tabsToSave.get(tabIndex);
            int pid = tab.getRenderProcessPid();
            // Don't close parent or child of the current tab.
            if (tab.getParentId() == foregroundTabId || tab.getId() == foregroundTabParentId) {
                continue;
            }
            // Tabs are grouped by process.
            if (pid != lastPid) {
                if (lastPid != -1) {
                    freedSoFarKb += lastPidPrivateMemorySizeKb;
                    if (freedSoFarKb > expectedToFreeKb) {
                        break;
                    }
                }
                lastPid = pid;
                lastPidPrivateMemorySizeKb = memoryUsageByRenderer.get(pid);
            }
            tab.saveStateAndDestroy();
        }

        if (freedSoFarKb < expectedToFreeKb) {  // Need to release more memory?
            // Purge caches and do GC for remaining pages running in background.
            // Do it once per process as caches and V8 are static.
            final HashMap<Integer, Boolean> purgedRenderers = new HashMap<Integer, Boolean>();
            purgedRenderers.put(foregroundPid, true);
            for (; tabIndex < tabsToSave.size(); tabIndex++) {
                Tab tab = tabsToSave.get(tabIndex);
                int pid = tab.getRenderProcessPid();
                if (!purgedRenderers.containsKey(pid)) {
                    tab.purgeRenderProcessNativeMemory();
                    purgedRenderers.put(pid, true);
                }
            }

            mDelegate.clearCachedNtpAndThumbnails();
        }
    }

    public void freeMemory() {
        freeMemory(FREE_AS_MUCH_AS_POSSIBLE);
    }

    private void freezeTabsIfNecessary() {
        for (int i = mTabs.size() - 1; i >= mMaxActiveTabs; --i) {
            mDelegate.saveStateAndDestroyTab(mTabs.get(i));
        }
    }

    private void onTabCreating(int tabId, boolean willBeSelected) {
        // Tab.INVALID_TAB_ID is used by the cached NTP and we should ignore it from our
        // monitoring as it is transient. We should never be called for the cached NTP
        // because ChromeViewListAdapter doesn't send a TAB_CREATING notification for it.
        assert tabId != TabModel.INVALID_TAB_ID;

        assert !mTabs.contains(tabId);

        if (willBeSelected || mTabs.size() == 0) {
            mTabs.add(0, tabId);
        } else {
            // Add it next to the current tab so it doesn't get killed until later.
            mTabs.add(1, tabId);
        }

        freezeTabsIfNecessary();
    }

    private void onTabSelected(int tabId) {
        mTabs.remove(new Integer(tabId));
        mTabs.add(0, tabId);
        freezeTabsIfNecessary();
    }

    private void onTabClosing(int tabId) {
        mTabs.remove(Integer.valueOf(tabId));
    }
}
