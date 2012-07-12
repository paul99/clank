// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

import org.chromium.base.CalledByNative;
import org.chromium.content.browser.TraceEvent;

import java.io.File;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;
import java.text.SimpleDateFormat;

/**
 * Controller for Chrome's profiling feature.
 *
 * We don't have any UI per se. Just call startProfiling() to start and
 * stopProfiling() to stop. We'll report progress to the user with Toasts.
 *
 * If the host application registers this class's BroadcastReceiver, you can
 * also start and stop the profiler with a broadcast intent, as follows:
 * <ul>
 * <li>To start profiling: am broadcast -a com.google.android.apps.chrome.GPU_PROFILER_START
 * <li>Add "-e file /foo/bar/xyzzy" to log profiling data to a specific file.
 * <li>To stop profiling: am broadcast -a com.google.android.apps.chrome.GPU_PROFILER_STOP
 * </ul>
 */
public class GpuProfiler {

    private static final String TAG = "GpuProfiler";

    private static final String ACTION_START = "com.google.android.apps.chrome.GPU_PROFILER_START";
    private static final String ACTION_STOP = "com.google.android.apps.chrome.GPU_PROFILER_STOP";
    private static final String FILE_EXTRA = "file";

    private final Context mContext;
    private final ProfilerBroadcastReceiver mBroadcastReceiver = new ProfilerBroadcastReceiver();
    private final ProfilerIntentFilter mIntentFilter = new ProfilerIntentFilter();
    private boolean mIsProfiling;

    // We might not want to always show toasts when we start the profiler, especially if
    // showing the toast impacts performance.  This gives us the chance to disable them.
    private boolean mShowToasts = true;

    private String mFilename;

    public GpuProfiler(Context context) {
        mContext = context;
    }

    /**
     * Get a BroadcastReceiver that can handle profiler intents.
     */
    public BroadcastReceiver getBroadcastReceiver() {
        return mBroadcastReceiver;
    }

    /**
     * Get an IntentFilter for profiler intents.
     */
    public IntentFilter getIntentFilter() {
        return mIntentFilter;
    }

    /**
     * Register a BroadcastReceiver in the given context.
     */
    public void registerReceiver(Context context) {
        context.registerReceiver(getBroadcastReceiver(), getIntentFilter());
    }

    /**
     * Unregister the GPU BroadcastReceiver in the given context.
     * @param context
     */
    public void unregisterReceiver(Context context) {
        context.unregisterReceiver(getBroadcastReceiver());
    }

    /**
     * Returns true if we're currently profiling.
     */
    public boolean isProfiling() {
        return mIsProfiling;
    }

    /**
     * Returns the path of the current output file. Null if isProfiling() false.
     */
    public String getOutputPath() {
        return mFilename;
    }

    /**
     * Start profiling to a new file in the Downloads directory.
     *
     * Calls #startProfiling(String) with a new timestamped filename.
     *
     * @param showToasts Whether or not we want to show toasts during this profiling session.
     * When we are timing the profile run we might not want to incur extra draw overhead of showing
     * notifications about the profiling system.
     *
     */
    public boolean startProfiling(boolean showToasts) {
        mShowToasts = showToasts;
        String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state)) {
            logAndToastError(
                    mContext.getString(R.string.profiler_no_storage_toast));
            return false;
        }

        // Generate a hopefully-unique filename using the UTC timestamp.
        // (Not a huge problem if it isn't unique, we'll just append more data.)
        SimpleDateFormat formatter = new SimpleDateFormat(
                "yyyy-MM-dd-HHmmss", Locale.US);
        formatter.setTimeZone(TimeZone.getTimeZone("UTC"));
        File dir = Environment.getExternalStoragePublicDirectory(
                Environment.DIRECTORY_DOWNLOADS);
        File file = new File(
                dir, "chrome-profile-results-" + formatter.format(new Date()));

        return startProfiling(file.getPath(), showToasts);
    }

    /**
     * Start profiling to the specified file. Returns true on success.
     *
     * Only one GpuProfiler can be running at the same time. If another profiler
     * is running when this method is called, it will be cancelled. If this
     * profiler is already running, this method does nothing and returns false.
     *
     * @param filename The name of the file to output the profile data to.
     * @param showToasts Whether or not we want to show toasts during this profiling session.
     * When we are timing the profile run we might not want to incur extra draw overhead of showing
     * notifications about the profiling system.
     */
    public boolean startProfiling(String filename, boolean showToasts) {
        mShowToasts = showToasts;
        if (isProfiling()) {
            // Don't need a toast because this shouldn't happen via the UI.
            Log.e(TAG, "Received startProfiling, but we're already profiling");
            return false;
        }
        // Lazy initialize the native side, to allow construction before the library is loaded.
        if (mNativeGpuProfiler == 0) {
            mNativeGpuProfiler = nativeInit();
        }
        if (!nativeStartProfiling(mNativeGpuProfiler, filename)) {
            logAndToastError(mContext.getString(R.string.profiler_error_toast));
            return false;
        }

        logAndToastInfo(mContext.getString(R.string.profiler_started_toast));
        TraceEvent.setEnabledToMatchNative();
        mFilename = filename;
        mIsProfiling = true;
        return true;
    }

    /**
     * Stop profiling. This won't take effect until Chrome has flushed its file.
     */
    public void stopProfiling() {
        if (isProfiling()) {
            nativeStopProfiling(mNativeGpuProfiler);
        }
    }

    /**
     * Called by native code when the profiler's output file is closed.
     */
    @CalledByNative
    protected void onProfilingStopped() {
        if (!isProfiling()) {
            // Don't need a toast because this shouldn't happen via the UI.
            Log.e(TAG, "Received onProfilingStopped, but we aren't profiling");
            return;
        }

        logAndToastInfo(
                mContext.getString(R.string.profiler_stopped_toast, mFilename));
        TraceEvent.setEnabledToMatchNative();
        mIsProfiling = false;
        mFilename = null;
    }

    @Override
    protected void finalize() {
        if (mNativeGpuProfiler != 0) {
            nativeDestroy(mNativeGpuProfiler);
            mNativeGpuProfiler = 0;
        }
    }

    void logAndToastError(String str) {
        Log.e(TAG, str);
        if (mShowToasts) Toast.makeText(mContext, str, Toast.LENGTH_SHORT).show();
    }

    void logAndToastInfo(String str) {
        Log.i(TAG, str);
        if (mShowToasts) Toast.makeText(mContext, str, Toast.LENGTH_SHORT).show();
    }

    private static class ProfilerIntentFilter extends IntentFilter {
        ProfilerIntentFilter() {
            addAction(ACTION_START);
            addAction(ACTION_STOP);
        }
    }

    class ProfilerBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (ACTION_START.equals(intent.getAction())) {
                String filename = intent.getStringExtra(FILE_EXTRA);
                if (filename != null) {
                    startProfiling(filename, true);
                } else {
                    startProfiling(true);
                }
            } else if (ACTION_STOP.equals(intent.getAction())) {
                stopProfiling();
            } else {
                Log.e(TAG, "Unexpected intent: " + intent);
            }
        }
    }

    private int mNativeGpuProfiler;
    private native int nativeInit();
    private native void nativeDestroy(int nativeGpuProfiler);
    private native boolean nativeStartProfiling(int nativeGpuProfiler, String filename);
    private native void nativeStopProfiling(int nativeGpuProfiler);
}
