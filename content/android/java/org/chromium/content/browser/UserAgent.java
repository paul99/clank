// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.content.res.Configuration;
import android.os.Build;

import org.chromium.base.CalledByNative;

/**
 * This class provides the methods called from native.
 */
class UserAgent {
    // TODO: Keep this up to date
    private static final String PREVIOUS_VERSION = "4.0.3";

    @CalledByNative
    static String getUserAgentOSInfo() {
        String osInfo = "";
        final String version = Build.VERSION.RELEASE;
        if (version.length() > 0) {
            if (Character.isDigit(version.charAt(0))) {
                // Release is a version, eg "3.1"
                osInfo += version;
            } else {
                // Release is a codename, eg "Honeycomb"
                // In this case, use the previous release's version
                osInfo += PREVIOUS_VERSION;
            }
        } else {
            // default to "1.0"
            osInfo += "1.0";
        }
        osInfo += ";";

        if ("REL".equals(Build.VERSION.CODENAME)) {
            final String model = Build.MODEL;
            if (model.length() > 0) {
                osInfo += " " + model;
            }
        }
        final String id = Build.ID;
        if (id.length() > 0) {
            osInfo += " Build/" + id;
        }

        return osInfo;
    }

    @CalledByNative
    static String getUserAgentMobile(Context appContext) {
        return BrowserProcessMain.isTabletUi(appContext) ? "" : "Mobile";

        // TODO (dtrainor): Investigate why this is returning the wrong string for certain builds...
        // for now use the code above.
        /*
        return mAppContext.getResources().getText(
                com.android.internal.R.string.web_user_agent_target_content).toString().trim();
        */
    }
}
