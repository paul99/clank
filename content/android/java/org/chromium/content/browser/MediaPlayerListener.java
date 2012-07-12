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
    MediaPlayer.OnErrorListener,
    MediaPlayer.OnInfoListener {
    // Used to determine the class instance to dispatch the native call to.
    private int mNativeMediaPlayerBridge = 0;

    /* Do not change these values without updating their counterparts
     * in include/media/mediaplayer.h!
     * TODO(tedbo): These should be shared with ChromeVideoView.java
     */
    private static final int MEDIA_NOP = 0; // interface test message
    private static final int MEDIA_PREPARED = 1;
    private static final int MEDIA_PLAYBACK_COMPLETE = 2;
    private static final int MEDIA_BUFFERING_UPDATE = 3;
    private static final int MEDIA_SEEK_COMPLETE = 4;
    private static final int MEDIA_SET_VIDEO_SIZE = 5;
    private static final int MEDIA_ERROR = 100;
    private static final int MEDIA_INFO = 200;

    MediaPlayerListener(int nativeMediaPlayerBridge) {
        mNativeMediaPlayerBridge = nativeMediaPlayerBridge;
    }

    @Override
    public boolean onInfo(MediaPlayer mp, int what, int extra) {
        nativeNotify(mNativeMediaPlayerBridge, MEDIA_INFO, what, extra);
        return true;
    }

    @Override
    public boolean onError(MediaPlayer mp, int what, int extra) {
        nativeNotify(mNativeMediaPlayerBridge, MEDIA_ERROR, what, extra);
        return true;
    }

    @Override
    public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
        nativeNotify(mNativeMediaPlayerBridge, MEDIA_SET_VIDEO_SIZE, width, height);
     }

    @Override
    public void onSeekComplete(MediaPlayer mp) {
        nativeNotify(mNativeMediaPlayerBridge, MEDIA_SEEK_COMPLETE, 0, 0);
     }

    @Override
    public void onBufferingUpdate(MediaPlayer mp, int percent) {
        nativeNotify(mNativeMediaPlayerBridge, MEDIA_BUFFERING_UPDATE, percent, 0);
     }

    @Override
    public void onCompletion(MediaPlayer mp) {
        nativeNotify(mNativeMediaPlayerBridge, MEDIA_PLAYBACK_COMPLETE, 0, 0);
     }

    @Override
    public void onPrepared(MediaPlayer mp) {
        nativeNotify(mNativeMediaPlayerBridge, MEDIA_PREPARED, 0, 0);
    }

    private native void nativeNotify(int mediaPlayerBridge, int msg, int ext1, int ext2);
}
