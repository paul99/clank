// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.app.ActivityManager;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.SurfaceTexture;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Surface;

import java.util.Locale;

import org.chromium.base.CalledByNative;

class BrowserProcessMain {

    private static final String TAG = "BrowserProcessMain";

    // Prevents initializing the process more than once.
    private static boolean sInitialized = false;

    // Computes the actual max renderer processes used.
    private static int normalizeMaxRendererProcesses(Context context, int maxRendererProcesses) {
        if (maxRendererProcesses == MAX_RENDERERS_AUTOMATIC) {
            // We use the device's memory class to decide the maximum renderer
            // processes. For the baseline devices the memory class is 16 and we will
            // limit it to one render process. For the devices with memory class 24,
            // we allow two render processes.
            ActivityManager am = (ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
            maxRendererProcesses = Math.max(((am.getMemoryClass() - 8) / 8), 1);
        }
        if (maxRendererProcesses > MAX_RENDERERS_LIMIT) {
            Log.w(TAG, "Excessive maxRendererProcesses value: " + maxRendererProcesses);
            return MAX_RENDERERS_LIMIT;
        }
        return Math.max(0, maxRendererProcesses);
    }

    // Automatically decide the number of renderer processes to use based on device memory class.
    static final int MAX_RENDERERS_AUTOMATIC = -1;
    // Use single-process mode that runs the renderer on a separate thread in the main application.
    static final int MAX_RENDERERS_SINGLE_PROCESS = 0;
    // Cap on the maximum number of renderer processes that can be requested.
    static final int MAX_RENDERERS_LIMIT = 3;  // TODO(tedbo): Raise limit

    /**
     * Initialize the process as a ChromeView host. This must be called from the main UI thread.
     * This should be called by the ChromeView constructor to prepare this process for ChromeView
     * use outside of the browser. In the case where ChromeView is used in the browser then
     * initBrowserProcess() should already have been called and this is a no-op.
     *
     * @param context Context used to obtain the application context.
     * @param maxRendererProcesses See ChromeView.enableMultiProcess().
     */
    static void initChromeViewProcess(Context context, int maxRendererProcesses) {
        genericChromiumProcessInit(context, maxRendererProcesses, false);
    }

    /**
     * Initialize the platform browser process. This must be called from the main UI thread before
     * accessing ChromeView in order to treat this as a browser process.
     *
     * @param context Context used to obtain the application context.
     * @param maxRendererProcesses See ChromeView.enableMultiProcess().
     */
    static void initChromiumBrowserProcess(Context context, int maxRendererProcesses) {
        genericChromiumProcessInit(context, maxRendererProcesses, true);
    }

    /**
     * Shared implementation for the initXxx methods.
     * @param context Context used to obtain the application context
     * @param maxRendererProcesses See ChromeView.enableMultiProcess()
     * @param hostIsChrome pass true if running as the system browser process.
     */
    private static void genericChromiumProcessInit(Context context, int maxRendererProcesses,
            boolean hostIsChrome) {
        if (sInitialized) {
            return;
        }
        sInitialized = true;

        // Normally Main.java will have kicked this off asynchronously for Chrome. But
        // other ChromeView apps like tests also need them so we make sure we've
        // extracted resources here. We can still make it a little async (wait until
        // the library is loaded).
        ResourceExtractor resourceExtractor = ResourceExtractor.get(context);
        resourceExtractor.startExtractingResources();

        // Normally Main.java will have already loaded the library asynchronously, we only
        // need to load it here if we arrived via another flow, e.g. bookmark access & sync setup.
        LibraryLoader.loadAndInitSync();

        Context appContext = context.getApplicationContext();

        // This block is inside genericChromiumProcessInit() instead
        // of initChromiumBrowserProcess() to make sure we do it once.
        // In here it is protected with the sInitialized.
        if (hostIsChrome) {
            if (nativeIsOfficialBuild() ||
                    CommandLine.getInstance().hasSwitch(CommandLine.ADD_OFFICIAL_COMMAND_LINE)) {
                Resources res = context.getResources();
                int id = ChromeView.getResourceId("R.array.official_command_line");
                try {
                    String[] switches = res.getStringArray(id);
                    CommandLine.getInstance().appendSwitchesAndArguments(switches);
                } catch (Resources.NotFoundException e) {
                    // Do nothing.  It is fine to have no command line
                    // additions for an official build.
                }
            }
        }

        int maxRenderers = normalizeMaxRendererProcesses(appContext, maxRendererProcesses);
        Log.i(TAG, "Initializing chromium process, renderers=" + maxRenderers +
                " hostIsChrome=" + hostIsChrome);

        // Now we really need to have the resources ready.
        resourceExtractor.waitForCompletion();

        nativeInitBrowserProcess(appContext, maxRenderers, hostIsChrome,
                isTabletUi(context), getPlugins(context));
    }

    /**
     * Called from native code to set up the peer surface texture producer in
     * another process.
     *
     * @param pid Process handle of the sandboxed process to share the
     *            SurfaceTexture with.
     * @param type The type of process that the SurfaceTexture is for.
     * @param st The SurfaceTexture object to share with the sandboxed process.
     * @param primaryID Used to route the call to the correct client instance.
     * @param secondaryID Used to route the call to the correct client instance.
     */
    @SuppressWarnings("unused")
    @CalledByNative
    static void establishSurfaceTexturePeer(int pid, int type, SurfaceTexture st, int primaryID,
            int secondaryID) {
        Surface surface = new Surface(st);
        SandboxedProcessLauncher.establishSurfacePeer(pid, type, surface, primaryID, secondaryID);

        // We need to explicitly release the native resource of our newly created surface
        // or the Surface class will print a warning message to the log in its finalizer.
        // This should be ok to do since our caller is responsible for retaining a
        // reference to the SurfaceTexture that is being sent across processes and the
        // receiving end should have retained a reference before the binder call finished.
        surface.release();
    }

    /**
     * @param context The application {@link Context}.
     * @return Whether or not the device should be given a tablet UI.
     */
    protected static boolean isTabletUi(Context context) {
        // TODO(joth): This duplicates ChromeActivity.isTabletUi, and probably shouldn't
        // live in framework (it's actually answering 'should we use chrome tablet UI?').
        return context.getResources().getConfiguration().smallestScreenWidthDp >= 600;
    }

    @CalledByNative
    private static String getDefaultLocale() {
        Locale locale = Locale.getDefault();
        return locale.getLanguage() + "-" + locale.getCountry();
    }

    private static String getPlugins(final Context context) {
        return PepperPluginManager.getPlugins(context);
    }

    // Prepare the process for browser/ChromeView use. See browser_process_main.{h,cc}.
    private static native void nativeInitBrowserProcess(Context applicationContext,
            int maxRenderProcesses, boolean hostIsChrome, boolean isTablet,
            String plugin_descriptor);

    // Is this an official build of Chrome?  Only native code knows
    // for sure.  Official build knowledge is needed very early in
    // process startup.
    private static native boolean nativeIsOfficialBuild();
}
