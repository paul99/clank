// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.net;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Proxy;
import org.chromium.base.CalledByNative;

// This class partners with native ProxyConfigServiceAndroid to listen for
// proxy change notifications from Android.
class ProxyChangeListener {
    private static final String TAG = "ProxyChangeListener";

    private ProxyChangeListener(Context context) {
        mContext = context;
    }

    @CalledByNative
    static public ProxyChangeListener create(Context context) {
        return new ProxyChangeListener(context);
    }

    @CalledByNative
    static public String getProperty(String property) {
        return System.getProperty(property);
    }

    @CalledByNative
    public synchronized void start(int nativePtr) {
        assert mNativePtr == 0;
        mNativePtr = nativePtr;
        registerReceiver();
    }

    @CalledByNative
    public synchronized void stop() {
        mNativePtr = 0;
        unregisterReceiver();
    }

    private class ProxyReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(Proxy.PROXY_CHANGE_ACTION)) {
                proxySettingsChanged();
            }
        }
    }

    private synchronized void proxySettingsChanged() {
        if (mNativePtr == 0)
            return;

        // Note that we are currently running on (e.g.) the UI thread and
        // that the C++ part will need to run the callbacks on the IO thread.
        nativeProxySettingsChanged(mNativePtr);
    }

    /**
     * See chrome/net/proxy/proxy_config_service_android.cc
     */
    private native void nativeProxySettingsChanged(
            int nativePtr /* net::ProxyConfigServiceAndroid */);

    private void registerReceiver() {
        if (mProxyReceiver != null) {
            return;
        }
        IntentFilter filter = new IntentFilter();
        filter.addAction(Proxy.PROXY_CHANGE_ACTION);
        mProxyReceiver = new ProxyReceiver();
        mContext.getApplicationContext().registerReceiver(mProxyReceiver, filter);
    }

    private void unregisterReceiver() {
        if (mProxyReceiver == null) {
            return;
        }
        mContext.unregisterReceiver(mProxyReceiver);
        mProxyReceiver = null;
    }

    private int mNativePtr;
    private Context mContext;
    private ProxyReceiver mProxyReceiver;
}