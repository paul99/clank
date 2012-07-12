// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome.partnerbookmarks;

import android.content.Context;
import android.content.pm.ApplicationInfo;

/**
 * The Java counterpart for the C++ partner bookmarks shim.
 * Responsible for:
 * - checking if we need to fetch the partner bookmarks,
 * - kicking off the async fetching of the partner bookmarks,
 * - pushing the partner bookmarks to the C++ side,
 * - reporting that all partner bookmarks were read to the C++ side.
 */
public class PartnerBookmarksShim {

    private static final String TAG = "PartnerBookmarksShim";

    private Context mContext;
    // JNI c++ pointer
    private int mNativePartnerBookmarksShim = 0;

    /**
     * Initializes the shim with the given context object and attached to C++ side.
     * @param context   A Context object.
     */
    public void Initialize(Context context) {
        mContext = context;
        mNativePartnerBookmarksShim = nativeInit();
    }

    /** Waits till the async reader finishes. */
    public void WaitForCompletion() {
        // TODO(aruslan): http://b/6397072 Enable wait handling/blocking
    }

    /** Destroys the native elements and shuts down the C++ shim. */
    public void Destroy() {
        WaitForCompletion();
        nativeDestroy(mNativePartnerBookmarksShim);
        mNativePartnerBookmarksShim = 0;
    }

    /** Checks if we need to fetch the Partner bookmarks and kicks the reading off */
    public void KickOffReading() {
        if ((mContext.getApplicationInfo().flags & ApplicationInfo.FLAG_SYSTEM) == 0) {
           nativePartnerBookmarksCreationComplete(mNativePartnerBookmarksShim);
           return;
        }
        WaitForCompletion();
        PartnerBookmarksReader reader = new PartnerBookmarksReader(mContext);
        reader.readBookmarks(
                new PartnerBookmarksReader.OnBookmarksReadListener() {
                    @Override
                    public void onBookmarksRead() {
                        nativePartnerBookmarksCreationComplete(mNativePartnerBookmarksShim);
                    }
                }, new PartnerBookmarksReader.BookmarkPushCallback() {
                    @Override
                    public long onBookmarkPush(String url, String title,
                            boolean isFolder, long parentId,
                            byte[] favicon, byte[] touchicon) {
                        return AddPartnerBookmark(url, title,
                                isFolder, parentId, favicon, touchicon);
                    }
                });
    }

    /**
     * Pushes the partner bookmark to the native shim.
     * @param url       The URL.
     * @param title     The title.
     * @param isFolder  True if this is a partner folder.
     * @param parentId  NATIVE parent id.
     * @param favicon   .PNG in a blob, used if no touchicon is set.
     * @param touchicon .PNG in a blob.
     * @return          NATIVE id of the created bookmark.
     */
    private long AddPartnerBookmark(String url, String title,
            boolean isFolder, long parentId, byte[] favicon, byte[] touchicon) {
        return nativeAddPartnerBookmark(mNativePartnerBookmarksShim, url, title,
                isFolder, parentId, favicon, touchicon);
    }

    // JNI
    private native int nativeInit();
    private native void nativeDestroy(int nativePartnerBookmarksShim);
    private native long nativeAddPartnerBookmark(int nativePartnerBookmarksShim,
            String url, String title, boolean isFolder, long parentId,
            byte[] favicon, byte[] touchicon);
    private native void nativePartnerBookmarksCreationComplete(int nativePartnerBookmarksShim);
}
