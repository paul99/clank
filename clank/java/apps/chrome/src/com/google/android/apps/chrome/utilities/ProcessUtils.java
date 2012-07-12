// Copyright 2012 Google Inc. All Rights Reserved.

package com.google.android.apps.chrome.utilities;

import org.chromium.content.browser.ChromeView;

/**
 * A set of utility methods related to the various chrome processes.
 */
public class ProcessUtils {
    // To prevent this class from being instantiated
    private ProcessUtils() {
    }

    /**
     * Suspend/Suspend webkit timers in all renderers.
     * @param suspend true if timers should be suspended, false if to be Suspendd.
     */
    public static void toggleWebKitSharedTimers(boolean suspend) {
        nativeToggleWebKitSharedTimers(suspend);
    }

    /**
     * We have keep-alives enabled for network connections as without it some routers will
     * kill the connection, causing web pages to hang. This call closes such
     * idle-but-kept-alive connections.
     * @param viewforProfile the currently active ChromeView from which the user
     *     profile can be obtained for closing the idle connections.
     */
    public static void closeIdleConnections(ChromeView viewForProfile) {
        nativeCloseIdleConnections(viewForProfile);
    }

    private static native void nativeToggleWebKitSharedTimers(boolean suspend);
    private static native void nativeCloseIdleConnections(ChromeView viewForProfile);
}
