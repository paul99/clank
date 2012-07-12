// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome.uma;

import android.content.ComponentCallbacks;
import android.content.Context;
import android.content.res.Configuration;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Message;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.util.Log;

import com.google.android.apps.chrome.ChromeNotificationCenter;
import com.google.android.apps.chrome.ChromeNotificationCenter.ChromeNotificationHandler;
import com.google.android.apps.chrome.ChromeViewListAdapter;
import com.google.android.apps.chrome.R;
import com.google.android.apps.chrome.Tab;
import com.google.android.apps.chrome.TabModel;
import com.google.android.apps.chrome.UmaRecordAction;
import com.google.android.apps.chrome.preferences.ChromePreferenceManager;

import org.chromium.base.CalledByNative;
import org.chromium.content.browser.ActivityStatus;
import org.chromium.content.browser.ChromeView;

/**
 * Mainly sets up session stats for chrome. A session is defined as the duration when the
 * application is in the foreground.  Also used to communicate information between Clank
 * and the framework's MetricService.
 */
public class UmaSessionStats {
    private static final String TAG = UmaSessionStats.class.getSimpleName();
    private static int mNativeUmaSessionStats = 0;
    // List adapater is needed to get the count of open tabs. We want to log the number of open
    // tabs on every page load.
    private ChromeViewListAdapter mChromeViewListAdapter = null;

    private Context mContext = null;
    private ComponentCallbacks mComponentCallbacks;

    private boolean mKeyboardConnected = false;

    public UmaSessionStats() {
        mNotificationHandler = new NotificationHandler();
    }

    // Notifications we care about.
    private static final int[] NOTIFICATIONS = {
        ChromeNotificationCenter.PAGE_LOAD_FINISHED,
    };

    private class NotificationHandler extends ChromeNotificationHandler {
        @Override
        public void handleMessage(Message m) {
            // Ignore the notification if no session is active.
            if (mChromeViewListAdapter == null) {
                Log.e(TAG, "Ignoring notification: ChromeViewListAdapter is null");
                return;
            }

            switch (m.what) {
                case ChromeNotificationCenter.PAGE_LOAD_FINISHED:
                    recordPageLoadStats(m.getData().getInt("tabId"));
                    break;
                default:
                    break;
            }
        }
    }

    private ChromeNotificationHandler mNotificationHandler = null;

    private void recordPageLoadStats(int tabId) {
        Tab tab = mChromeViewListAdapter.getTabById(tabId);
        ChromeView chromeView = tab == null ? null : tab.getView();
        boolean isDesktopUserAgent =
            (chromeView == null) ? false : chromeView.getUseDesktopUserAgent();
        UmaRecordAction.pageLoaded(
                getTabCountFromModel(mChromeViewListAdapter.getModel(false)),
                getTabCountFromModel(mChromeViewListAdapter.getModel(true)),
                mKeyboardConnected, isDesktopUserAgent);
    }

    private int getTabCountFromModel(TabModel model) {
        return model == null ? 0 : model.getCount();
    }

    /**
     * Starts a new session for logging.
     */
    public void startNewSession(Context context, ChromeViewListAdapter listAdapter) {
        // Lazily create the native object and the notification handler. These objects are never
        // destroyed.
        if (mNativeUmaSessionStats == 0) {
            mNativeUmaSessionStats = nativeInit();
        }

        mComponentCallbacks = new ComponentCallbacks() {
            @Override
            public void onLowMemory() {
                // Not required
            }

            @Override
            public void onConfigurationChanged(Configuration newConfig) {
                mKeyboardConnected = newConfig.keyboard != Configuration.KEYBOARD_NOKEYS;
            }
        };
        mContext = context;
        mContext.registerComponentCallbacks(mComponentCallbacks);
        mKeyboardConnected = mContext.getResources().getConfiguration().
                keyboard != Configuration.KEYBOARD_NOKEYS;

        mChromeViewListAdapter = listAdapter;
        ChromeNotificationCenter.registerForNotifications(NOTIFICATIONS, mNotificationHandler);
        nativeUmaResumeSession(mNativeUmaSessionStats);
    }

    /**
     * Logs the current session.
     */
    public void logAndEndSession() {
        // Unregister for notifications before doing anything else.
        ChromeNotificationCenter.unregisterForNotifications(NOTIFICATIONS, mNotificationHandler);
        mContext.unregisterComponentCallbacks(mComponentCallbacks);
        mChromeViewListAdapter = null;
        nativeUmaEndSession(mNativeUmaSessionStats);
    }

    public static void logRendererCrash() {
        nativeLogRendererCrash(ActivityStatus.getInstance().isPaused());
    }

    /**
     * Checks if we may upload logs on the current network connection.
     */
    @CalledByNative
    public static boolean mayUploadOnCurrentConnection(Context context) {
        String defaultStr = context.getString(R.string.crash_dump_never_upload_value);

        String alwaysUpStr = context.getString(R.string.crash_dump_always_upload_value);
        String wifiUpStr = context.getString(R.string.crash_dump_only_with_wifi_value);

        // Default to never uploading to avoid uploading when we shouldn't.
        String crashUploadCondition = PreferenceManager.getDefaultSharedPreferences(context)
            .getString(ChromePreferenceManager.PREF_CRASH_DUMP_UPLOAD, defaultStr);

        // Avoid uploading stats if we're not allowed to.
        ConnectivityManager connManager =
            (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo activeInf = connManager.getActiveNetworkInfo();
        NetworkInfo wifiNetInf = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);

        return (crashUploadCondition.equals(alwaysUpStr)
                    && activeInf != null
                    && activeInf.isConnected()) ||
                (crashUploadCondition.equals(wifiUpStr)
                    && wifiNetInf != null
                    && wifiNetInf.isConnected());
    }

    /**
     * Updates the state of the MetricsService to account for the user's preferences.
     */
    public void updateMetricsServiceState(Context context) {
        String neverUpStr = context.getString(R.string.crash_dump_never_upload_value);

        // Default to never uploading to avoid logging when we shouldn't be.
        String crashUploadCondition = PreferenceManager.getDefaultSharedPreferences(context)
            .getString(ChromePreferenceManager.PREF_CRASH_DUMP_UPLOAD, neverUpStr);
        boolean mayRecordStats = !crashUploadCondition.equals(neverUpStr);

        // Re-start the MetricsService with the given parameters.
        nativeUpdateMetricsServiceState(mayRecordStats);
    }

    private native int nativeInit();
    private native void nativeUpdateMetricsServiceState(boolean mayRecord);
    private native void nativeUmaResumeSession(int nativeUmaSessionStats);
    private native void nativeUmaEndSession(int nativeUmaSessionStats);
    private static native void nativeLogRendererCrash(boolean isPaused);
}
