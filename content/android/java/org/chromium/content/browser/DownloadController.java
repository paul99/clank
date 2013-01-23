// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// See DownloadListener.h

package org.chromium.content.browser;

import java.io.File;
import java.util.Map;
import java.util.TreeMap;

import android.app.DownloadManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Environment;
import android.util.Log;

import org.chromium.base.CalledByNative;
import org.chromium.content.browser.legacy.DownloadListener;

class DownloadController {
    private static final String LOGTAG = "DownloadController";
    private static DownloadController sInstance;

    /**
     * The permission to download files without any system notification being shown.
     * The permission is defined by com.android.providers.downloads.DownloadProvider
     */
    private static final String PERMISSION_NO_NOTIFICATION =
            "android.permission.DOWNLOAD_WITHOUT_NOTIFICATION";

    public static DownloadController getInstance() {
        if (sInstance == null) {
            sInstance = new DownloadController();
        }
        return sInstance;
    }

    private Context mContext;

    private DownloadController() {
        mIdOptionMap = new TreeMap<Integer, Options>();
        nativeInit();
    }

    private static DownloadListener listenerFromView(ChromeView view) {
        return view.downloadListener();
    }

    private static DownloadListener2 listener2FromView(ChromeView view) {
        return view.downloadListener2();
    }

    private class Options implements DownloadListener2.HttpPostDownloadOptions {
        private DownloadListener2 mListener;
        private int mDownloadId;
        private boolean mShouldDownload;
        private CharSequence mDescription;
        private String mPath;
        private int mNotificationVisibility;
        private boolean mIsDone;

        public Options(int downloadId, DownloadListener2 listener) {
            this.mDownloadId = downloadId;
            this.mListener = listener;
            mShouldDownload = false;
            mDescription = "";
            mPath = "";
            mIsDone = false;
            mNotificationVisibility = DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED;
        }

        @Override
        public synchronized void setShouldDownload(boolean shouldDownload) {
            assert !mIsDone;
            this.mShouldDownload = shouldDownload;
        }

        @Override
        public synchronized void setDescription(CharSequence description) {
            mDescription = description;
        }

        @Override
        public synchronized void setDestinationInExternalPublicDir(
                String dirType, String subPath) {
            File file = Environment.getExternalStoragePublicDirectory(dirType);
            if (file.exists()) {
                if (!file.isDirectory()) {
                    throw new IllegalStateException(file.getAbsolutePath() +
                            " already exists and is not a directory");
                }
            } else {
                if (!file.mkdir()) {
                    throw new IllegalStateException("Unable to create directory: "+
                            file.getAbsolutePath());
                }
            }
            mPath = new File(file, subPath).getPath();
        }

        @Override
        public synchronized void setNotificationVisibility(int visibility) {
            mNotificationVisibility = visibility;
        }

        @Override
        public synchronized void done() {
            assert !mIsDone;
            mIsDone = true;

            // Check visibility permissions.
            if (mContext.checkCallingOrSelfPermission(PERMISSION_NO_NOTIFICATION)
                    == PackageManager.PERMISSION_GRANTED) {
                enforceAllowedValues("NotificationVisibility", mNotificationVisibility,
                        DownloadManager.Request.VISIBILITY_VISIBLE,
                        DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED,
                        DownloadManager.Request.VISIBILITY_HIDDEN);
            } else {
                enforceAllowedValues("NotificationVisibility", mNotificationVisibility,
                        DownloadManager.Request.VISIBILITY_VISIBLE,
                        DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
            }

            DownloadController.getInstance().nativePostDownloadOptionsReady(
                    mDownloadId, mShouldDownload, mPath);
        }

        public DownloadListener2 getListener() {
            return mListener;
        }

        public String getDescription() {
            return mDescription.toString();
        }

        public boolean shouldShowProgressNotifications() {
            return mNotificationVisibility == DownloadManager.Request.VISIBILITY_VISIBLE ||
                   mNotificationVisibility ==
                       DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED;
        }

        public boolean shouldShowFinalNotification() {
            return mNotificationVisibility ==
                DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED;
        }

        public boolean shouldInsert() {
            return mNotificationVisibility == DownloadManager.Request.VISIBILITY_VISIBLE ||
                   mNotificationVisibility ==
                       DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED;
        }

        // Mostly copied from com.android.providers.downloads.DownloadProvider.
        private void enforceAllowedValues(String valueDescription, Object value,
                Object... allowedValues) {
            if (value != null) {
                for (Object allowedValue : allowedValues) {
                    if (value.equals(allowedValue)) {
                        return;
                    }
                }
            }
            throw new SecurityException("Invalid value for " + valueDescription + ": " + value);
        }
    }

    private Map<Integer, Options> mIdOptionMap;

    public void setContext(Context context) {
        mContext = context;
    }

    // methods called by native code

    @CalledByNative
    public void newHttpGetDownload(ChromeView view, String url,
            String userAgent, String contentDisposition, String mimetype,
            String cookie, String referer, long contentLength) {
        DownloadListener2 listener2 = listener2FromView(view);

        if (listener2 != null) {
            listener2.requestHttpGetDownload(url, userAgent,
                    contentDisposition, mimetype, cookie, referer, contentLength);
            return;
        }

        DownloadListener listener = listenerFromView(view);
        if (listener != null) {
            listener.onDownloadStart(url, userAgent, contentDisposition,
                    mimetype, contentLength);
        }
    }

    @CalledByNative
    public void newHttpPostDownload(ChromeView view, int downloadId,
            String url, String contentDisposition, String mimetype,
            long contentLength) {
        DownloadListener2 listener = listener2FromView(view);
        if (listener == null) {
            nativePostDownloadOptionsReady(downloadId, false, "");
            return;
        }

        Options options = new Options(downloadId, listener);
        mIdOptionMap.put(downloadId, options);

        listener.onNewHttpPostDownload(url, contentDisposition, mimetype,
                contentLength, options);
    }

    @CalledByNative
    public void httpPostDownloadUpdatesStart() {
        Log.i(LOGTAG, "httpPostDownloadUpdatesStart");
    }

    @CalledByNative
    public void httpPostDownloadUpdate(int downloadId, String url,
            String contentDisposition, String mimetype, String path,
            long contentLength, long bytes_so_far) {
        Options options = mIdOptionMap.get(downloadId);
        if (options.shouldShowProgressNotifications()) {
          // TODO(boliu): Implement/hook up download progress notification UI here.
          Log.i(LOGTAG, "Download Progress: " + bytes_so_far + "/" + contentLength);
        }
    }

    @CalledByNative
    public void httpPostDownloadUpdatesFinish() {
        Log.i(LOGTAG, "httpPostDownloadUpdatesFinish");
    }

    @CalledByNative
    public void onHttpPostDownloadCompleted(int downloadId, String url,
            String contentDisposition, String mimetype, String path,
            long contentLength, boolean successful) {
        Options options = mIdOptionMap.remove(downloadId);
        options.getListener().onHttpPostDownloadCompleted(url,
                contentDisposition, mimetype, path, contentLength, successful);

        if (options.shouldInsert()) {
            if (successful && contentLength > 0) {
                String title = (new File(path)).getName();
                String description = options.getDescription();
                boolean isMediaScannerScannable = true;
                boolean showNotification = options.shouldShowFinalNotification();
                DownloadManager manager =
                    (DownloadManager) mContext.getSystemService(Context.DOWNLOAD_SERVICE);
                manager.addCompletedDownload(title, description, isMediaScannerScannable,
                        mimetype, path, contentLength, showNotification);
                Log.w(LOGTAG, "Download finished: " + path);
            } else {
                // TODO(boliu): Properly handle a failed download. Ideally DownloadManager would
                // allow a failed download to be added.
                Log.w(LOGTAG, "Download failed: " + path);
            }
        }
    }

    // native methods
    private native void nativeInit();
    private native void nativePostDownloadOptionsReady(int donwloadId,
            boolean shouldDownload, String path);
}
