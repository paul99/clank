// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

// Controls DevTools remote debugging server state.
public class RemoteDebuggingController {

    private static RemoteDebuggingController sInstance;

    private boolean mUserPreferenceEnabled;

    private static final String TAG = "RemoteDebuggingController";

    public static RemoteDebuggingController getInstance() {
        if (sInstance == null) {
            sInstance = new RemoteDebuggingController();
        }
        return sInstance;
    }

    public void updateUserPreferenceState(boolean enabled) {
        if (mUserPreferenceEnabled != enabled) {
            mUserPreferenceEnabled = enabled;
            updateRemoteDebuggingState();
        }
    }

    void updateRemoteDebuggingState() {
        if (mUserPreferenceEnabled != nativeIsRemoteDebuggingEnabled()) {
            if (mUserPreferenceEnabled) {
                nativeStartRemoteDebugging();
            } else {
                nativeStopRemoteDebugging();
            }
        }
    }

    private RemoteDebuggingController() {
        mUserPreferenceEnabled = nativeIsRemoteDebuggingEnabled();
    }

    private static native void nativeStartRemoteDebugging();
    private static native void nativeStopRemoteDebugging();
    private static native boolean nativeIsRemoteDebuggingEnabled();
}
