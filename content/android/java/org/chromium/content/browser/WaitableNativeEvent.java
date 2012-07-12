// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

// Helper class to block on asynchronous events in the native code.
class WaitableNativeEvent {
    public final int mNativeInstance;

    WaitableNativeEvent() {
        mNativeInstance = nativeInit();
    }

    // Returns 'true' if the event was signaled.
    public boolean WaitOnce(int timeoutInSeconds) {
        assert mNativeInstance != 0;
        boolean result = nativeWait(mNativeInstance, timeoutInSeconds);
        nativeDestroy(mNativeInstance);
        return result;
    }

    private native int nativeInit();
    private native void nativeDestroy(int nativeWaitableNativeEvent);
    private native boolean nativeWait(int nativeWaitableNativeEvent, int timeoutInSeconds);
}
