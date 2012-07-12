// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;

import org.chromium.base.CalledByNative;

/**
 * Provides the bindings required for native code to obtain version information to display
 * in the about:version page.  Also provides a way for java code to determine whether Clank
 * was built as an official build.
 */
public class BrowserVersion {
    @CalledByNative
    public static String getApplicationLabel(Context context) {
        try {
            PackageManager packageManager = context.getPackageManager();
            ApplicationInfo appInfo = packageManager.getApplicationInfo(context.getPackageName(),
                    PackageManager.GET_META_DATA);
            CharSequence label = packageManager.getApplicationLabel(appInfo);
            return  label != null ? label.toString() : "";
        } catch (NameNotFoundException e) {
            return "";
        }
    }

    @CalledByNative
    public static String getAppVersionName(Context context) {
        try {
            PackageManager packageManager = context.getPackageManager();
            PackageInfo packageInfo = packageManager.getPackageInfo(context.getPackageName(),
                    PackageManager.GET_META_DATA);
            return packageInfo.versionName;
        } catch (NameNotFoundException e) {
            return "";
        }
    }

    @CalledByNative
    public static String getAppVersionCode(Context context) {
        try {
            PackageManager packageManager = context.getPackageManager();
            PackageInfo packageInfo = packageManager.getPackageInfo(context.getPackageName(),
                    PackageManager.GET_META_DATA);
            return String.valueOf(packageInfo.versionCode);
        } catch (NameNotFoundException e) {
            return "";
        }
    }

    /**
     * Check if the browser was built as an "official" build.
     */
    public static boolean isOfficialBuild() {
        return nativeIsOfficialBuild();
    }

    /**
     * Only native code can see the OFFICIAL_BUILD flag; check it from there.  This function is
     * not handled by initialize() and is not available early in startup (before the native
     * library has loaded).  Calling it before that point will result in an exception.
     *
     * TODO(dfalcantara): If/when we start using ProGuard, replace this JNI call with method
     *                    reflection: http://b/issue?id=5364886
     */
    private static native boolean nativeIsOfficialBuild();

    /**
     * Get the HTML for the terms of service to be displayed at first run.
     */
    public static String getTermsOfServiceHtml() {
      return nativeGetTermsOfServiceHtml();
    }

    /**
     * Get the HTML for the terms of service to be displayed at first run.
     */
    public static String getTermsOfServiceString() {
      return nativeGetTermsOfServiceString();
    }

    /**
     * The terms of service are a native resource.
     */
    private static native String nativeGetTermsOfServiceHtml();
    private static native String nativeGetTermsOfServiceString();
}
