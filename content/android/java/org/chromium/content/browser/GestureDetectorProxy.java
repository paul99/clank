// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.view.ViewConfiguration;
import android.view.MotionEvent;

import org.chromium.content.browser.third_party.GestureDetector;
import org.chromium.content.browser.third_party.GestureDetector.OnGestureListener;

import java.util.Iterator;

// This class sits in between the system GestureDetector and ChromeView in order
// to control long press timers.
// For instance, we may receive a DOWN then UP, so we may need to cancel the
// timer before the UP completes its roundtrip from WebKit.
class GestureDetectorProxy {
    private GestureDetector mGestureDetector;
    private OnGestureListener mListener;
    private MotionEvent mCurrentDownEvent;
    private Handler mHandler;
    private int mTouchSlopSquare;
    private boolean mInLongPress;

    // The following are used when touch events are offered to native, and not for
    // anything relating to the GestureDetector.
    // True iff a touch_move has exceeded the touch slop distance.
    private boolean mMoveConfirmed;
    // Coordinates of the start of a touch event (i.e. the touch_down).
    private int mTouchInitialX;
    private int mTouchInitialY;

    private static final int LONG_PRESS = 2;

    private static final int LONGPRESS_TIMEOUT = ViewConfiguration.getLongPressTimeout();
    private static final int TAP_TIMEOUT = ViewConfiguration.getTapTimeout();

    GestureDetectorProxy(Context context, GestureDetector gestureDetector,
            OnGestureListener listener) {
        mListener = listener;
        mGestureDetector = gestureDetector;
        mGestureDetector.setIsLongpressEnabled(false);
        mHandler = new LongPressHandler();
        final ViewConfiguration configuration = ViewConfiguration.get(context);
        int touchSlop = configuration.getScaledTouchSlop();
        mTouchSlopSquare = touchSlop * touchSlop;
    }

    private class LongPressHandler extends Handler {
        LongPressHandler() {
            super();
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case LONG_PRESS:
                dispatchLongPress();
                break;
            default:
                throw new RuntimeException("Unknown message " + msg); //never
            }
        }
    }

    // Delegates to the system GestureDetector, and initiates a LONG_PRESS timer
    // if needed.
    boolean onTouchEvent(MotionEvent ev) {
        if (ev.getAction() == MotionEvent.ACTION_DOWN) {
            if (mCurrentDownEvent != null) {
                mCurrentDownEvent.recycle();
            }
            mCurrentDownEvent = MotionEvent.obtain(ev);
            mHandler.sendEmptyMessageAtTime(LONG_PRESS, mCurrentDownEvent.getDownTime()
                    + TAP_TIMEOUT + LONGPRESS_TIMEOUT);
            mInLongPress = false;
            return mGestureDetector.onTouchEvent(ev);
        }
        return mGestureDetector.onTouchEvent(ev);
    }

    // True only if it's the start of a new stream (ACTION_DOWN), or a
    // continuation of the current stream.
    boolean canHandle(MotionEvent ev) {
        return ev.getAction() == MotionEvent.ACTION_DOWN ||
            (mCurrentDownEvent != null &&
             mCurrentDownEvent.getDownTime() == ev.getDownTime());
    }

    // Cancel LONG_PRESS timers.
    void cancelLongPressIfNeeded(MotionEvent ev) {
        if (!hasPendingMessage() ||
            mCurrentDownEvent == null || ev.getDownTime() != mCurrentDownEvent.getDownTime()) {
            return;
        }
        final int action = ev.getAction();
        final float y = ev.getY();
        final float x = ev.getX();
        switch (action & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_MOVE:
                final int deltaX = (int) (x - mCurrentDownEvent.getX());
                final int deltaY = (int) (y - mCurrentDownEvent.getY());
                int distance = (deltaX * deltaX) + (deltaY * deltaY);
                if (distance > mTouchSlopSquare) {
                    mInLongPress = false;
                    mHandler.removeMessages(LONG_PRESS);
                }
                break;
            case MotionEvent.ACTION_UP:
                if (mCurrentDownEvent.getDownTime() + TAP_TIMEOUT + LONGPRESS_TIMEOUT >
                    ev.getEventTime()) {
                    mInLongPress = false;
                    mHandler.removeMessages(LONG_PRESS);
                }
                break;
        }
    }

    // Given a stream of pending events, cancel the LONG_PRESS timer if appropriate.
    void cancelLongPressIfNeeded(Iterator<PendingMotionEvent> pendingEvents) {
        if (mCurrentDownEvent == null)
            return;
        if (!mHandler.hasMessages(LONG_PRESS))
            return;
        long currentDownTime = mCurrentDownEvent.getDownTime();
        while (pendingEvents.hasNext()) {
            MotionEvent pending = pendingEvents.next().event();
            if (pending.getDownTime() != currentDownTime) {
                break;
            }
            cancelLongPressIfNeeded(pending);
        }
    }

    void cancelLongPress() {
        if (mHandler.hasMessages(LONG_PRESS)) {
            mHandler.removeMessages(LONG_PRESS);
        }
    }

    // Used this to check if a onSingleTapUp is part of a long press event.
    boolean isInLongPress() {
        return mInLongPress;
    }

    private void dispatchLongPress() {
        mInLongPress = true;
        mListener.onLongPress(mCurrentDownEvent);
    }

    protected boolean hasPendingMessage() {
        return mHandler.hasMessages(LONG_PRESS);
    }

    void onOfferTouchEventToNative(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            mMoveConfirmed = false;
            mTouchInitialX = Math.round(event.getX());
            mTouchInitialY = Math.round(event.getY());
        }
    }

    boolean confirmOfferMoveEventToNative(MotionEvent event) {
        if (!mMoveConfirmed) {
            int deltaX = Math.round(event.getX()) - mTouchInitialX;
            int deltaY = Math.round(event.getY()) - mTouchInitialY;
            if (deltaX * deltaX + deltaY * deltaY >= mTouchSlopSquare) {
                mMoveConfirmed = true;
            }
        }
        return mMoveConfirmed;
    }
}
