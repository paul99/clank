// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome;

import com.google.common.annotations.VisibleForTesting;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;

import com.google.android.apps.chrome.preferences.ChromePreferenceManager;

import org.chromium.base.CalledByNative;

/**
 * Triggers updates to the underlying network state in native Chrome
 */
public class NetworkConnectivityReceiver extends BroadcastReceiver {

    private static final String TAG = "NetworkConnectivityReceiver";

    private static final String ACTION_CONNECTIVITY_CHANGE =
            "android.net.conn.CONNECTIVITY_CHANGE";

    private final NetworkConnectivityIntentFilter mIntentFilter =
            new NetworkConnectivityIntentFilter();

    @Override
    public void onReceive(Context context, Intent intent) {
        ChromePreferenceManager.getInstance(context).checkAllowPrefetch();

        if (intent.getBooleanExtra(ConnectivityManager.EXTRA_NO_CONNECTIVITY, false)) {
            Log.d(TAG, "Network connectivity changed, no connectivity.");
            nativeUpdateConnectivity(false);
        } else {
            ConnectivityManager manager = (ConnectivityManager)
                    context.getSystemService(Context.CONNECTIVITY_SERVICE);
            boolean isConnected = false;
            for (NetworkInfo info: manager.getAllNetworkInfo()) {
                if (info.isConnected()) {
                    isConnected = true;
                    break;
                }
            }
            Log.d(TAG, "Network connectivity changed, status is: " + isConnected);
            nativeUpdateConnectivity(isConnected);
        }
    }

    /**
     * Register a BroadcastReceiver in the given context.
     */
    public void registerReceiver(Context context) {
        context.registerReceiver(this, mIntentFilter);
    }

    /**
     * Unregister the BroadcastReceiver in the given context.
     * @param context
     */
    public void unregisterReceiver(Context context) {
        context.unregisterReceiver(this);
    }

    private static class NetworkConnectivityIntentFilter extends IntentFilter {
        NetworkConnectivityIntentFilter() {
            addAction(ACTION_CONNECTIVITY_CHANGE);
        }
    }

    private static native void nativeUpdateConnectivity(boolean isConnected);

    /**
     * A helper class which is used to create a native listener of the NetworkChangeNotifier
     * for testing.
     */
    @VisibleForTesting
    public static class NetworkChangeNotifierObserver {
        private boolean mReceivedNotification = false;

        /** Native ChangeNotifierListener which will be set by nativeInit(). */
        private int mNativeNotifierObserver = 0;

        public NetworkChangeNotifierObserver() {
            mNativeNotifierObserver = nativeInit();
        }

        public boolean hasReceivedNotification() {
            return mReceivedNotification;
        }

        @CalledByNative("NetworkChangeNotifierObserver")
        private void setReceivedNotification() {
            mReceivedNotification = true;
        }

        public void destroy() {
            if (mNativeNotifierObserver != 0) {
                nativeDestroy(mNativeNotifierObserver);
                mNativeNotifierObserver = 0;
            }
        }

        @NativeCall("NetworkChangeNotifierObserver")
        private native int nativeInit();
        @NativeCall("NetworkChangeNotifierObserver")
        private native void nativeDestroy(int nativeNetworkChangeNotifierObserver);
    }

    @VisibleForTesting
    public static NetworkChangeNotifierObserver createNetworkChangeNotifierObserverForTest() {
        return new NetworkChangeNotifierObserver();
    }
}
