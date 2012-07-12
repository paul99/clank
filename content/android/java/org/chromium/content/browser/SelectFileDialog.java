// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.provider.MediaStore;

import java.util.Vector;

public class SelectFileDialog {
    private final int mNativeSelectFileDialog;
    private final Vector<String> mFileTypes;
    private final String mCapture;  // May be null if no capture parameter was set.

    private static final String IMAGE_TYPES = "image/*";
    private static final String VIDEO_TYPES = "video/*";
    private static final String AUDIO_TYPES = "audio/*";
    private static final String ANY_TYPES = "*/*";
    private static final String CAPTURE_CAMERA = "camera";
    private static final String CAPTURE_CAMCORDER = "camcorder";
    private static final String CAPTURE_MICROPHONE = "microphone";
    private static final String CAPTURE_FILESYSTEM = "filesystem";

    SelectFileDialog(int nativeSelectFileDialog, Vector<String> types, String capture) {
        mNativeSelectFileDialog = nativeSelectFileDialog;
        mFileTypes = types;
        mCapture = capture;
    }

    public boolean onFileSelected(ContentResolver contentResolver, Uri result) {
        boolean success = false;
        if ("file".equals(result.getScheme())) {
            nativeOnFileSelected(mNativeSelectFileDialog, result.getPath());
            success = true;
        } else if ("content".equals(result.getScheme())) {
            Cursor c = contentResolver.query(result,
                    new String[] { MediaStore.MediaColumns.DATA },
                    null, null, null);
            if (c != null) {
                if (c.getCount() == 1) {
                    c.moveToFirst();
                    String path = c.getString(0);

                    if (path != null) {
                        // Not all providers support the MediaStore.DATA column. For
                        // example, Gallery3D (com.android.gallery3d.provider) does not
                        // support it for Picasa Web Album images.
                        nativeOnFileSelected(mNativeSelectFileDialog, path);
                        success = true;
                    }
                }
                c.close();
            }
        }
        if (!success) {
            // Unsupported file type, or failure to resolve content uri to file path.
            onFileNotSelected();
        }
        return success;
    }

    public void onFileNotSelected() {
        nativeOnFileNotSelected(mNativeSelectFileDialog);
    }

    public boolean noSpecificType() {
        // We use a single Intent to decide the type of the file chooser we display
        // to the user, which means we can only give it a single type. If there
        // are multiple accept types specified, we will fallback to a generic
        // chooser (unless a capture parameter has been specified, in which case
        // we'll try to satisfy that first.
        // TODO(benm): Investigate having an Intent per accept type, and combining
        // them together in the final chooser we send to the system. This should
        // allow us to honor multiple accept types better.
        return mFileTypes.size() != 1 || mFileTypes.contains(ANY_TYPES);
    }

    public boolean shouldShowImageTypes() {
        return noSpecificType() || mFileTypes.contains(IMAGE_TYPES);
    }

    public boolean shouldShowVideoTypes() {
        return noSpecificType() || mFileTypes.contains(VIDEO_TYPES);
    }

    public boolean shouldShowAudioTypes() {
        return noSpecificType() || mFileTypes.contains(AUDIO_TYPES);
    }

    public boolean captureCamera() {
        return shouldShowImageTypes() && mCapture != null && mCapture.startsWith(CAPTURE_CAMERA);
    }

    public boolean captureCamcorder() {
        return shouldShowVideoTypes() && mCapture != null && mCapture.startsWith(CAPTURE_CAMCORDER);
    }

    public boolean captureMicrophone() {
        return shouldShowAudioTypes() && mCapture != null &&
                mCapture.startsWith(CAPTURE_MICROPHONE);
    }

    public boolean captureFilesystem() {
        return mCapture != null && mCapture.startsWith(CAPTURE_FILESYSTEM);
    }

    private native void nativeOnFileSelected(int nativeSelectFileDialogImpl,
            String filePath);
    private native void nativeOnFileNotSelected(int nativeSelectFileDialogImpl);
}
