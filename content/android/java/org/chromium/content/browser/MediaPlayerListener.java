// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.media.MediaPlayer;

class MediaPlayerListener implements
    MediaPlayer.OnPreparedListener,
    MediaPlayer.OnCompletionListener,
    MediaPlayer.OnBufferingUpdateListener,
    MediaPlayer.OnSeekCompleteListener,
    MediaPlayer.OnVideoSizeChangedListener,
    MediaPlayer.OnErrorListener {
    // Used to determine the class instance to dispatch the native call to.
    private int mNativeMediaPlayerListener = 0;

    MediaPlayerListener(int nativeMediaPlayerListener) {
        mNativeMediaPlayerListener = nativeMediaPlayerListener;
    }

    @Override
    public boolean onError(MediaPlayer mp, int what, int extra) {
        nativeOnMediaError(mNativeMediaPlayerListener, what);
        return true;
    }

    @Override
    public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
        nativeOnVideoSizeChanged(mNativeMediaPlayerListener, width, height);
     }

    @Override
    public void onSeekComplete(MediaPlayer mp) {
        nativeOnSeekComplete(mNativeMediaPlayerListener);
     }

    @Override
    public void onBufferingUpdate(MediaPlayer mp, int percent) {
        nativeOnBufferingUpdate(mNativeMediaPlayerListener, percent);
     }

    @Override
    public void onCompletion(MediaPlayer mp) {
        nativeOnPlaybackComplete(mNativeMediaPlayerListener);
     }

    @Override
    public void onPrepared(MediaPlayer mp) {
        nativeOnMediaPrepared(mNativeMediaPlayerListener);
    }

    private native void nativeOnMediaError(
            int nativeMediaPlayerListener,
            int errorType);

    private native void nativeOnVideoSizeChanged(
            int nativeMediaPlayerListener,
            int width, int height);

    private native void nativeOnBufferingUpdate(
            int nativeMediaPlayerListener,
            int percent);

    private native void nativeOnMediaPrepared(int nativeMediaPlayerListener);

    private native void nativeOnPlaybackComplete(int nativeMediaPlayerListener);

    private native void nativeOnSeekComplete(int nativeMediaPlayerListener);
}
