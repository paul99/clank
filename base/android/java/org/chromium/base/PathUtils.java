// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import android.content.Context;
import android.os.Environment;

import java.io.File;

/**
 * This class provides the path related methods for the native library.
 */
class PathUtils {

    /**
     * @return the private directory that used to store application data.
     */
    @CalledByNative
    public static String getDataDirectory(Context appContext) {
        return appContext.getDir("chrome", Context.MODE_PRIVATE).getPath();
    }

    /**
     * @return the cache library.
     */
    @CalledByNative
    public static String getCacheDirectory(Context appContext) {
        return appContext.getCacheDir().getPath();
    }

    /**
     * @return the external storage directory.
     */
    @CalledByNative
    public static String getExternalStorageDirectory() {
        return Environment.getExternalStorageDirectory().getAbsolutePath();
    }

}
