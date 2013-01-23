// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.view.MotionEvent;

class PendingMotionEvent {
    private MotionEvent mEvent;
    private long mTimeoutTime;
    private int mOfferedToNative;

    public PendingMotionEvent(MotionEvent event, long timeout, int offeredToNative) {
        mEvent = event;
        mTimeoutTime = timeout;
        mOfferedToNative = offeredToNative;
    }

    public MotionEvent event() {
        return mEvent;
    }

    public long timeoutTime() {
        return mTimeoutTime;
    }

    public int offeredToNative() {
        return mOfferedToNative;
    }
}
