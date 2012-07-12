// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome.thumbnail;

import android.graphics.Bitmap;

import java.nio.ByteBuffer;

/**
 * Methods for encoding ETC1 textures.
 * <p>
 * This implementation may be used instead of android.opengl.ETC1 in order to
 * support directly encoding a Bitmap without copying.
 */
public class ETC1 {

    /**
     * Return the width of the encoded image data.
     */
    public static native int nativeGetEncodedWidth(Bitmap bitmap);

    /**
     * Return the height of the encoded image data.
     */
    public static native int nativeGetEncodedHeight(Bitmap bitmap);

    /**
     * Return the size of the encoded image data (does not include size of PKM header).
     */
    public static native int nativeGetEncodedDataSize(Bitmap bitmap);

    /**
     * Encode an entire image.
     * @param bitmap The Bitmap to be encoded. Must be in ARGB_8888 format.
     * @param out A native order direct buffer of the encoded data.
     * Must be the the size of the encoded representation of the bitmap (as
     * obtained by nativeGetEncodedDataSize).
     * @return Non zero if there was an error.
     */
    public static native int nativeEncodeImage(Bitmap bitmap, ByteBuffer out);
}
