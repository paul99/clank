// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.graphics.SurfaceTexture;

class SurfaceTextureListener implements SurfaceTexture.OnFrameAvailableListener {
    // Used to determine the class instance to dispatch the native call to.
    private int mNativeSurfaceTextureListener = 0;

    SurfaceTextureListener(int nativeSurfaceTextureListener) {
        assert nativeSurfaceTextureListener != 0;
        mNativeSurfaceTextureListener = nativeSurfaceTextureListener;
    }

    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        nativeFrameAvailable(mNativeSurfaceTextureListener);
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            nativeDestroy(mNativeSurfaceTextureListener);
        } finally {
            super.finalize();
        }
    }

    private native void nativeFrameAvailable(int nativeSurfaceTextureListener);
    private native void nativeDestroy(int nativeSurfaceTextureListener);
}
