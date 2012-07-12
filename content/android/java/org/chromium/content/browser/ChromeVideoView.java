// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.MediaController;
import android.widget.MediaController.MediaPlayerControl;

import java.lang.ref.WeakReference;

import org.chromium.base.CalledByNative;

public class ChromeVideoView extends ChromeFullscreenView implements MediaPlayerControl,
        SurfaceHolder.Callback, View.OnTouchListener, View.OnKeyListener {

    private static final String TAG = "ChromeVideoView";

    /* Do not change these values without updating their counterparts
     * in include/media/mediaplayer.h!
     */
    private static final int MEDIA_NOP = 0; // interface test message
    private static final int MEDIA_PREPARED = 1;
    private static final int MEDIA_PLAYBACK_COMPLETE = 2;
    private static final int MEDIA_BUFFERING_UPDATE = 3;
    private static final int MEDIA_SEEK_COMPLETE = 4;
    private static final int MEDIA_SET_VIDEO_SIZE = 5;
    private static final int MEDIA_ERROR = 100;
    private static final int MEDIA_INFO = 200;

    // Type needs to be kept in sync with surface_texture_peer.h.
    private static final int SET_VIDEO_SURFACE_TEXTURE = 1;

    /* Do not change these values without updating their counterparts
     * in include/media/mediaplayer.h!
     */
    /** Unspecified media player error.
     * @see android.media.MediaPlayer.OnErrorListener
     */
    public static final int MEDIA_ERROR_UNKNOWN = 1;

    /** Media server died. In this case, the application must release the
     * MediaPlayer object and instantiate a new one.
     */
    public static final int MEDIA_ERROR_SERVER_DIED = 100;

    /** The video is streamed and its container is not valid for progressive
     * playback i.e the video's index (e.g moov atom) is not at the start of the
     * file.
     */
    public static final int MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200;

    // all possible internal states
    private static final int STATE_ERROR              = -1;
    private static final int STATE_IDLE               = 0;
    private static final int STATE_PREPARING          = 1;
    private static final int STATE_PREPARED           = 2;
    private static final int STATE_PLAYING            = 3;
    private static final int STATE_PAUSED             = 4;
    private static final int STATE_PLAYBACK_COMPLETED = 5;

    private final Handler mEventHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
            case MEDIA_PREPARED:
                // We should not reach here as the mediaplayer is already
                // prepared in the renderer process
                return;
            case MEDIA_PLAYBACK_COMPLETE:
                onCompletion();
                return;
            case MEDIA_BUFFERING_UPDATE:
                onBufferingUpdate(msg.arg1);
                return;
            case MEDIA_SEEK_COMPLETE:
                return;
            case MEDIA_SET_VIDEO_SIZE:
                onVideoSizeChanged(msg.arg1, msg.arg2);
                return;
            case MEDIA_ERROR:
                onError(msg.arg1, msg.arg2);
                return;
            case MEDIA_INFO:
                return;
            case MEDIA_NOP: // interface test message - ignore
                break;
            default:
                Log.e(TAG, "Unknown message type " + msg.what);
                return;
            }
        }
    };
    private SurfaceHolder mSurfaceHolder = null;
    private int mVideoWidth;
    private int mVideoHeight;
    private int mCurrentBufferPercentage;
    private int mDuration;
    private MediaController mMediaController = null;
    private boolean mCanPause;
    private boolean mCanSeekBack;
    private boolean mCanSeekForward;
    private boolean mHasMediaMetadata = false;

    // Native pointer to C++ ChromeVideoView object which will be set by nativeInit()
    private int mNativeChromeVideoView = 0;

    // webkit should have prepared the media
    private int mCurrentState = STATE_IDLE;

    // Strings for displaying media player errors
    // TODO (qinmin): we will remove these variables when chromview and
    // the main app combine
    static String mPlaybackErrorText;
    static String mUnknownErrorText;
    static String mErrorButton;
    static String mErrorTitle;

    // This view will contain the video.
    private VideoSurfaceView mVideoSurfaceView;

    private class VideoSurfaceView extends SurfaceView {

        public VideoSurfaceView(Context context) {
            super(context);
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            int width = getDefaultSize(mVideoWidth, widthMeasureSpec);
            int height = getDefaultSize(mVideoHeight, heightMeasureSpec);
            if (mVideoWidth > 0 && mVideoHeight > 0) {
                if ( mVideoWidth * height  > width * mVideoHeight ) {
                    height = width * mVideoHeight / mVideoWidth;
                } else if ( mVideoWidth * height  < width * mVideoHeight ) {
                    width = height * mVideoWidth / mVideoHeight;
                }
            }
            setMeasuredDimension(width, height);
        }
    }

    public ChromeVideoView(Context context) {
        super(context);
    }

    public ChromeVideoView(Context context, int nativeChromeVideoView) {
        super(context);

        mNativeChromeVideoView = nativeChromeVideoView;
        /*
         * Passing a weak reference of this object to native for callbacks
         */
        nativeInit(mNativeChromeVideoView, new WeakReference<ChromeVideoView>(this));

        mCurrentBufferPercentage = 0;
        mVideoSurfaceView = new VideoSurfaceView(context);
        mCurrentState = isPlaying() ? STATE_PLAYING : STATE_PAUSED;
    }

    @Override
    void showFullscreenView() {
        this.addView(mVideoSurfaceView,
                new FrameLayout.LayoutParams(
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        ViewGroup.LayoutParams.WRAP_CONTENT,
                        Gravity.CENTER));
        mVideoSurfaceView.setOnKeyListener(this);
        mVideoSurfaceView.setOnTouchListener(this);
        mVideoSurfaceView.getHolder().addCallback(this);
        mVideoSurfaceView.getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        mVideoSurfaceView.setFocusable(true);
        mVideoSurfaceView.setFocusableInTouchMode(true);
        mVideoSurfaceView.requestFocus();
    }

    @Override
    void removeFullscreenView() {
        this.removeView(mVideoSurfaceView);
        mVideoSurfaceView = null;
    }

    @CalledByNative
    public void updateMediaMetadata(
            int videoWidth,
            int videoHeight,
            int duration,
            boolean canPause,
            boolean canSeekBack,
            boolean canSeekForward) {
        mVideoWidth = videoWidth;
        mVideoHeight = videoHeight;
        mDuration = duration;
        mCanPause = canPause;
        mCanSeekBack = canSeekBack;
        mCanSeekForward = canSeekForward;
        mHasMediaMetadata = true;

        if (mMediaController != null) {
            mMediaController.setEnabled(true);
            // If paused , should show the controller for ever.
            if (isPlaying())
                mMediaController.show();
            else
                mMediaController.show(0);
        }

        if (mVideoWidth != 0 && mVideoHeight != 0) {
            // This will trigger the onMeasure to get the display size right.
            mVideoSurfaceView.getHolder().setFixedSize(mVideoWidth, mVideoHeight);
        }
    }

    public void destroyNativeView() {
        if (mNativeChromeVideoView != 0) {
            mNativeChromeVideoView = 0;
        }
    }

    public static void registerMediaPlayerErrorText(String playbackErrorText,
            String unknownErrorText, String errorButton, String errorTitle) {
        mPlaybackErrorText = playbackErrorText;
        mUnknownErrorText = unknownErrorText;
        mErrorButton = errorButton;
        mErrorTitle = errorTitle;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        mVideoSurfaceView.setFocusable(true);
        mVideoSurfaceView.setFocusableInTouchMode(true);
        if (isInPlaybackState() && mMediaController != null) {
            if (mMediaController.isShowing()) {
                // ensure the controller will get repositioned later
                mMediaController.hide();
            }
            mMediaController.show();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        mSurfaceHolder = holder;
        openVideo();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        mSurfaceHolder = null;
        if (mNativeChromeVideoView != 0) {
            nativeExitFullscreen(mNativeChromeVideoView, true);
            destroyNativeView();
        }
        if (mMediaController != null) {
            mMediaController.hide();
            mMediaController = null;
        }
    }

    public void setMediaController(MediaController controller) {
        if (mMediaController != null) {
            mMediaController.hide();
        }
        mMediaController = controller;
        attachMediaController();
    }

    private void attachMediaController() {
        if (mMediaController != null) {
            mMediaController.setMediaPlayer(this);
            mMediaController.setAnchorView(mVideoSurfaceView);
            mMediaController.setEnabled(mHasMediaMetadata);
        }
    }

    @CalledByNative
    public void openVideo() {
        if (mSurfaceHolder != null) {
            if (mNativeChromeVideoView != 0) {
                nativeUpdateMediaMetadata(mNativeChromeVideoView);
            }
            mCurrentBufferPercentage = 0;
            if (mNativeChromeVideoView != 0) {
                int renderHandle = nativeGetRenderHandle(mNativeChromeVideoView);
                if (renderHandle == 0) {
                    nativeSetSurface(mNativeChromeVideoView,
                            mSurfaceHolder.getSurface(),
                            nativeGetRouteId(mNativeChromeVideoView),
                            nativeGetPlayerId(mNativeChromeVideoView));
                    return;
                }
                ISandboxedProcessService service =
                    SandboxedProcessLauncher.getSandboxedService(renderHandle);
                if (service == null) {
                    Log.e(TAG, "Unable to get SandboxedProcessService from pid.");
                    return;
                }
                try {
                    service.setSurface(
                            SET_VIDEO_SURFACE_TEXTURE,
                            mSurfaceHolder.getSurface(),
                            nativeGetRouteId(mNativeChromeVideoView),
                            nativeGetPlayerId(mNativeChromeVideoView));
                } catch (RemoteException e) {
                    Log.e(TAG, "Unable to call setSurfaceTexture: " + e);
                    return;
                }
            }
            requestLayout();
            invalidate();
            setMediaController(new MediaController(getChromeActivity()));

            if (mMediaController != null) {
                mMediaController.show();
            }
        }
    }

    private Handler getEventHandler() {
        return mEventHandler;
    }

    private void onVideoSizeChanged(int width, int height) {
        mVideoWidth = width;
        mVideoHeight = height;
        if (mVideoWidth != 0 && mVideoHeight != 0) {
            mVideoSurfaceView.getHolder().setFixedSize(mVideoWidth, mVideoHeight);
        }
    }

    private void onBufferingUpdate(int percent) {
        mCurrentBufferPercentage = percent;
    }

    private void onCompletion() {
        mCurrentState = STATE_PLAYBACK_COMPLETED;
        if (mMediaController != null) {
            mMediaController.hide();
        }
    }

    private void onError(int framework_err, int impl_err) {
        Log.d(TAG, "Error: " + framework_err + "," + impl_err);
        if (mCurrentState == STATE_ERROR || mCurrentState == STATE_PLAYBACK_COMPLETED) {
            return;
        }

        mCurrentState = STATE_ERROR;
        if (mMediaController != null) {
            mMediaController.hide();
        }

        /* Pop up an error dialog so the user knows that
         * something bad has happened. Only try and pop up the dialog
         * if we're attached to a window. When we're going away and no
         * longer have a window, don't bother showing the user an error.
         *
         * TODO(qinmin): we need to review whehther this Dialog is ok with
         * the rest of the Clank UI elements
         */
        if (getWindowToken() != null) {
            String message;

            if (framework_err == MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK) {
                message = mPlaybackErrorText;
            } else {
                message = mUnknownErrorText;
            }

            new AlertDialog.Builder(getContext())
                .setTitle(mErrorTitle)
                .setMessage(message)
                .setPositiveButton(mErrorButton,
                        new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        /* Inform that the video is over.
                         */
                        onCompletion();
                    }
                })
                .setCancelable(false)
                .show();
        }
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        if (isInPlaybackState() && mMediaController != null &&
                event.getAction() == MotionEvent.ACTION_DOWN) {
            toggleMediaControlsVisiblity();
        }
        return true;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent ev) {
        if (isInPlaybackState() && mMediaController != null) {
            toggleMediaControlsVisiblity();
        }
        return false;
    }

    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        boolean isKeyCodeSupported = keyCode != KeyEvent.KEYCODE_BACK &&
                                     keyCode != KeyEvent.KEYCODE_VOLUME_UP &&
                                     keyCode != KeyEvent.KEYCODE_VOLUME_DOWN &&
                                     keyCode != KeyEvent.KEYCODE_VOLUME_MUTE &&
                                     keyCode != KeyEvent.KEYCODE_CALL &&
                                     keyCode != KeyEvent.KEYCODE_MENU &&
                                     keyCode != KeyEvent.KEYCODE_SEARCH &&
                                     keyCode != KeyEvent.KEYCODE_ENDCALL;
        if (isInPlaybackState() && isKeyCodeSupported && mMediaController != null) {
            if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK ||
                    keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE) {
                if (mCurrentState == STATE_PLAYING) {
                    pause();
                    mMediaController.show();
                } else {
                    start();
                    mMediaController.hide();
                }
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_MEDIA_PLAY) {
                if (isPlaying()) {
                    start();
                    mMediaController.hide();
                }
                return true;
            } else if (keyCode == KeyEvent.KEYCODE_MEDIA_STOP
                    || keyCode == KeyEvent.KEYCODE_MEDIA_PAUSE) {
                if (isPlaying()) {
                    pause();
                    mMediaController.show();
                }
                return true;
            } else {
                toggleMediaControlsVisiblity();
            }
        } else if (keyCode == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_UP) {
            if (mNativeChromeVideoView != 0) {
                nativeExitFullscreen(mNativeChromeVideoView, false);
                destroyNativeView();
            }
            return true;
        } else if (keyCode == KeyEvent.KEYCODE_MENU || keyCode == KeyEvent.KEYCODE_SEARCH) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @CalledByNative
    public static void destroyFullscreenView() {
        exitFullScreen();
    }

    private void toggleMediaControlsVisiblity() {
        if (mMediaController.isShowing()) {
            mMediaController.hide();
        } else {
            mMediaController.show();
        }
    }

    private boolean isInPlaybackState() {
        return (mCurrentState != STATE_ERROR &&
                mCurrentState != STATE_IDLE &&
                mCurrentState != STATE_PREPARING);
    }

    /**
     * Called from native code when an interesting event happens. This method
     * just uses the EventHandler system to post the event back to the java
     * thread. We use a weak reference to the original ChromeVideoView object
     * so that the object can be garbage collected by JVM while the native
     * code is unaffected by this.
     *
     */
    @CalledByNative
    private static void postEventFromNative(Object chromeVideoViewRef,
            int what, int arg1, int arg2) {
        ChromeVideoView videoView =
            (ChromeVideoView)((WeakReference)chromeVideoViewRef).get();
        if (videoView == null) {
            return;
        }
        if (null != videoView.getEventHandler()) {
            Message m = videoView.getEventHandler().obtainMessage(
                    what, arg1, arg2);
            videoView.getEventHandler().sendMessage(m);
        }
    }

    public void start() {
        if (isInPlaybackState()) {
            if (mNativeChromeVideoView != 0) {
                nativePlay(mNativeChromeVideoView);
            }
            mCurrentState = STATE_PLAYING;
        }
    }

    public void pause() {
        if (isInPlaybackState()) {
            if (isPlaying()) {
                if (mNativeChromeVideoView != 0) {
                    nativePause(mNativeChromeVideoView);
                }
                mCurrentState = STATE_PAUSED;
            }
        }
    }

    // cache duration as mDuration for faster access
    public int getDuration() {
        if (isInPlaybackState()) {
            if (mDuration > 0) {
                return mDuration;
            }
            if (mNativeChromeVideoView != 0) {
                mDuration = nativeGetDuration(mNativeChromeVideoView);
            } else {
                mDuration = 0;
            }
            return mDuration;
        }
        mDuration = -1;
        return mDuration;
    }

    public int getCurrentPosition() {
        if (isInPlaybackState() && mNativeChromeVideoView != 0) {
            return nativeGetCurrentPosition(mNativeChromeVideoView);
        }
        return 0;
    }

    public void seekTo(int msec) {
        if (mNativeChromeVideoView != 0) {
            nativeSeekTo(mNativeChromeVideoView, msec);
        }
    }

    public boolean isPlaying() {
        return mNativeChromeVideoView != 0 && nativeIsPlaying(mNativeChromeVideoView);
    }

    public int getBufferPercentage() {
        return mCurrentBufferPercentage;
    }
    public boolean canPause() {
        return mCanPause;
    }

    public boolean canSeekBackward() {
        return mCanSeekBack;
    }

    public boolean canSeekForward() {
        return mCanSeekForward;
    }

    @CalledByNative
    public static ChromeVideoView createFullScreenView(int nativeChromeVideoView) {
        if (getChromeActivity() != null) {
            ChromeVideoView videoView = new ChromeVideoView(getChromeActivity(),
                    nativeChromeVideoView);
            showFullScreen(videoView);
            return videoView;
        }
        return null;
    }

    private native void nativeExitFullscreen(int nativeChromeVideoView, boolean relaseMediaPlayer);
    private native int nativeGetCurrentPosition(int nativeChromeVideoView);
    private native int nativeGetDuration(int nativeChromeVideoView);
    private native void nativeUpdateMediaMetadata(int nativeChromeVideoView);
    private native int nativeGetVideoWidth(int nativeChromeVideoView);
    private native int nativeGetVideoHeight(int nativeChromeVideoView);
    private native int nativeGetPlayerId(int nativeChromeVideoView);
    private native int nativeGetRouteId(int nativeChromeVideoView);
    private native int nativeGetRenderHandle(int nativeChromeVideoView);
    private native void nativeInit(int nativeChromeVideoView, Object chromeVideoView);
    private native boolean nativeIsPlaying(int nativeChromeVideoView);
    private native void nativePause(int nativeChromeVideoView);
    private native void nativePlay(int nativeChromeVideoView);
    private native void nativeSeekTo(int nativeChromeVideoView, int msec);
    private native void nativeSetSurface(int nativeChromeVideoView,
            Surface surface, int routeId, int playerId);
}
