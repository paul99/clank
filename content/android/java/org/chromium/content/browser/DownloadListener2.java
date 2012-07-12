// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

public interface DownloadListener2 {
    /**
     * Notify the host application that a file should be downloaded. Replaces
     * onDownloadStart from DownloadListener.
     * @param url The full url to the content that should be downloaded
     * @param userAgent the user agent to be used for the download.
     * @param contentDisposition Content-disposition http header, if
     *                           present.
     * @param mimetype The mimetype of the content reported by the server
     * @param cookie The cookie
     * @param contentLength The file size reported by the server
     */
    public abstract void requestHttpGetDownload(String url, String userAgent,
            String contentDisposition, String mimetype, String cookie,
            long contentLength);

    /**
     * A callback class used to set Http POST download options. DownloadListener
     * can set options from a different thread. When finished, done should be
     * called. After calling done, the setters can no longer be called.
     */
    public interface HttpPostDownloadOptions {
        /**
         * Set if the Http POST download should proceed or be cancelled.
         * @param shouldDownload true if continue downloading, false if cancel.
         */
        public void setShouldDownload(boolean shouldDownload);

        /**
         * Set the description in the notification. Only used when notifications
         * are shown for this download.
         */
        public void setDescription(CharSequence description);

        /**
         * Set the local destination for the downloaded file to a path within the public external
         * storage directory (as returned by
         * {@link Environment#getExternalStoragePublicDirectory(String)}.
         *<p>
         * The downloaded file is not scanned by MediaScanner.
         *
         * @param dirType the directory type to pass to
         *        {@link Environment#getExternalStoragePublicDirectory(String)}
         * @param subPath the path within the external directory, including the destination filename
         */
        public void setDestinationInExternalPublicDir(String dirType, String subPath);

        /**
         * Control whether a system notification is posted while this download is running
         * or when it is completed.
         *<p>
         * It can take the following values:
         * {@link android.app.DownloadManager.Request#VISIBILITY_HIDDEN},
         * {@link android.app.DownloadManager.Request#VISIBILITY_VISIBLE},
         * {@link android.app.DownloadManager.Request#VISIBILITY_VISIBLE_NOTIFY_COMPLETED}.
         *<p>
         * If set to {@link #VISIBILITY_HIDDEN}, this requires the permission
         * android.permission.DOWNLOAD_WITHOUT_NOTIFICATION.
         *
         * @param visibility the visibility setting value
         */
        public void setNotificationVisibility(int visibility);

        /**
         * Finalize these options. Nothing will happen until this method is
         * called, including continuing or cancelling the download. This
         * method can only be called once, after which this object becomes
         * immutable.
         */
        public void done();
    }

    /**
     * Notify the host application of a new Http POST download. Http POST
     * requests cannot be repeated so host application cannot handle the
     * download. Instead, options can be set about the download and
     * DownloadListener will be notified of updates to the download.
     *
     * @param url The full url to the content that should be downloaded
     * @param contentDisposition Content-disposition http header, if
     *                           present.
     * @param mimetype The mimetype of the content reported by the server
     * @param contentLength The file size reported by the server
     * @param callback Callback for the result path to allow application
     *                 to set the options. Methods on the callback can be
     *                 called from any thread.
     */
    public abstract void onNewHttpPostDownload(String url,
            String contentDisposition, String mimetype, long contentLength,
            HttpPostDownloadOptions callback);

    /**
     * Notify the host application that a POST download is finished.
     * @param url The full url to the content that should be downloaded
     * @param contentDisposition Content-disposition http header, if
     *                           present.
     * @param mimetype The mimetype of the content reported by the server
     * @param path Path of the downloaded file.
     * @param contentLength The file size reported by the server
     * @param successful Whether the download succeeded
     */
    public abstract void onHttpPostDownloadCompleted(String url,
            String contentDisposition, String mimetype, String path,
            long contentLength, boolean successful);
}
