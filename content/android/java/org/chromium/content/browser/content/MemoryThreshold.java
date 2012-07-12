// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.content;

import android.app.ActivityManager;
import android.content.Context;

import org.chromium.base.CalledByNative;

/**
 * This class provide memory threshold used by native.
 */
class MemoryThreshold {
    @CalledByNative
    static int getMemoryThresholdMB(Context appContext) {
        return ((ActivityManager) appContext.getSystemService(Context.ACTIVITY_SERVICE))
                .getLargeMemoryClass();
    }
}
