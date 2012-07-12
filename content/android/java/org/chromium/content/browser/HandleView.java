// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.SystemClock;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.PopupWindow;
import android.widget.TextView;

/*
 * This class is based on HandleView, the private inner class of
 * android.widget.TextView
 * TODO(olilan): refactor and share with Android code if possible
 * TODO(olilan): restore insertion point and pastepopup code commented out below
 */
class HandleView extends View {
    private Drawable mDrawable;
    private final PopupWindow mContainer;
    private int mPositionX;
    private int mPositionY;
    private final CursorController mController;
    private boolean mIsDragging;
    private float mTouchToWindowOffsetX;
    private float mTouchToWindowOffsetY;
    private float mHotspotX;
    private float mHotspotY;
    private int mLineOffsetY;
    private int mHeight;
    private int mLastParentX;
    private int mLastParentY;
    private float mDownPositionX, mDownPositionY;
    private int mContainerPositionX, mContainerPositionY;
    private long mTouchTimer;
    private boolean mIsInsertionHandle = false;
    private Runnable mLongPressCallback;

    private View mParent;
    private InsertionHandleController.PastePopupMenu mPastePopupWindow;

    int mTextSelectHandleLeftRes;
    int mTextSelectHandleRightRes;
    int mTextSelectHandleRes;

    Drawable mSelectHandleLeft;
    Drawable mSelectHandleRight;
    Drawable mSelectHandleCenter;

    final int[] mTempCoords = new int[2];
    Rect mTempRect;

    private static final int STATE_IDLE = 0;
    private static final int STATE_FADEIN = 1;
    private int mAnimationState = STATE_IDLE;
    private long mAnimationStart;
    private long mAnimationDuration;

    public static final int LEFT = 0;
    public static final int CENTER = 1;
    public static final int RIGHT = 2;

    // Number of dips to subtract from the handle's y position to give a suitable
    // y coordinate for the corresponding text position. This is to compensate for the fact
    // that the handle position is at the base of the line of text.
    private static final float LINE_OFFSET_Y_DIP = 5.0f;

    public static final int[] TEXT_VIEW_HANDLE_ATTRS = {
        android.R.attr.textSelectHandleLeft,
        android.R.attr.textSelectHandle,
        android.R.attr.textSelectHandleRight,
    };

    public HandleView(CursorController controller, int pos, View parent) {
        super(parent.getContext());
        Context context = parent.getContext();
        mParent = parent;
        mController = controller;
        mContainer = new PopupWindow(context, null, android.R.attr.textSelectHandleWindowStyle);
        mContainer.setSplitTouchEnabled(true);
        mContainer.setClippingEnabled(false);

        TypedArray a = context.obtainStyledAttributes(TEXT_VIEW_HANDLE_ATTRS);
        mTextSelectHandleLeftRes = a.getResourceId(a.getIndex(LEFT), 0);
        mTextSelectHandleRes = a.getResourceId(a.getIndex(CENTER), 0);
        mTextSelectHandleRightRes = a.getResourceId(a.getIndex(RIGHT), 0);
        a.recycle();

        setOrientation(pos);

        // Convert line offset dips to pixels.
        mLineOffsetY = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP,
                LINE_OFFSET_Y_DIP, context.getResources().getDisplayMetrics());
    }


    public void setOrientation(int pos) {
        int handleWidth;
        switch (pos) {
        case LEFT: {
            if (mSelectHandleLeft == null) {
                mSelectHandleLeft = getContext().getResources().getDrawable(
                        mTextSelectHandleLeftRes);
            }
            mDrawable = mSelectHandleLeft;
            handleWidth = mDrawable.getIntrinsicWidth();
            mHotspotX = (handleWidth * 3) / 4;
            break;
        }

        case RIGHT: {
            if (mSelectHandleRight == null) {
                mSelectHandleRight = getContext().getResources().getDrawable(
                        mTextSelectHandleRightRes);
            }
            mDrawable = mSelectHandleRight;
            handleWidth = mDrawable.getIntrinsicWidth();
            mHotspotX = handleWidth / 4;
            break;
        }

        case CENTER:
        default: {
            if (mSelectHandleCenter == null) {
                mSelectHandleCenter = getContext().getResources().getDrawable(
                        mTextSelectHandleRes);
            }
            mDrawable = mSelectHandleCenter;
            handleWidth = mDrawable.getIntrinsicWidth();
            mHotspotX = handleWidth / 2;
            mIsInsertionHandle = true;
            break;
        }
        }

        final int handleHeight = mDrawable.getIntrinsicHeight();
        mHotspotY = 0;
        mHeight = handleHeight;
        invalidate();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        setMeasuredDimension(mDrawable.getIntrinsicWidth(),
                mDrawable.getIntrinsicHeight());
    }

    public void show() {
        if (!isPositionVisible()) {
            hide();
            return;
        }
        mContainer.setContentView(this);
        final int[] coords = mTempCoords;
        mParent.getLocationInWindow(coords);
        mContainerPositionX = coords[0] + mPositionX;
        mContainerPositionY = coords[1] + mPositionY;
        mContainer.showAtLocation(mParent, 0, mContainerPositionX, mContainerPositionY);

        // Hide paste view when handle is moved on screen.
        if (mPastePopupWindow != null) {
            mPastePopupWindow.hide();
        }
    }

    public void hide() {
        mIsDragging = false;
        mContainer.dismiss();
        if (mPastePopupWindow != null) {
            mPastePopupWindow.hide();
        }
    }

    public boolean isShowing() {
        return mContainer.isShowing();
    }

    private boolean isPositionVisible() {
        // Always show a dragging handle.
        if (mIsDragging) {
            return true;
        }

        final int left = 0;
        final int right = mParent.getWidth();
        final int top = 0;
        final int bottom = mParent.getHeight();

        if (mTempRect == null) {
            mTempRect = new Rect();
        }
        final Rect clip = mTempRect;
        clip.left = left;
        clip.top = top;
        clip.right = right;
        clip.bottom = bottom;

        final ViewParent parent = mParent.getParent();
        if (parent == null || !parent.getChildVisibleRect(mParent, clip, null)) {
            return false;
        }

        final int[] coords = mTempCoords;
        mParent.getLocationInWindow(coords);
        final int posX = coords[0] + mPositionX + (int) mHotspotX;
        final int posY = coords[1] + mPositionY + (int) mHotspotY;

        return posX >= clip.left && posX <= clip.right &&
                posY >= clip.top && posY <= clip.bottom;
    }

    public void moveTo(int x, int y) {
        mPositionX = x;
        mPositionY = y;
        if (isPositionVisible()) {
            int[] coords = null;
            if (mContainer.isShowing()) {
                coords = mTempCoords;
                mParent.getLocationInWindow(coords);
                final int containerPositionX = coords[0] + mPositionX;
                final int containerPositionY = coords[1] + mPositionY;

                if (containerPositionX != mContainerPositionX ||
                    containerPositionY != mContainerPositionY) {
                    mContainerPositionX = containerPositionX;
                    mContainerPositionY = containerPositionY;

                    mContainer.update(mContainerPositionX, mContainerPositionY,
                            getRight() - getLeft(), getBottom() - getTop());

                    // Hide paste popup window as soon as a scroll occurs.
                    if (mPastePopupWindow != null) {
                        mPastePopupWindow.hide();
                    }
                }
            } else {
                show();
            }

            if (mIsDragging) {
                if (coords == null) {
                    coords = mTempCoords;
                    mParent.getLocationInWindow(coords);
                }
                if (coords[0] != mLastParentX || coords[1] != mLastParentY) {
                    mTouchToWindowOffsetX += coords[0] - mLastParentX;
                    mTouchToWindowOffsetY += coords[1] - mLastParentY;
                    mLastParentX = coords[0];
                    mLastParentY = coords[1];
                }
                // Hide paste popup window as soon as the handle is dragged.
                if (mPastePopupWindow != null) {
                    mPastePopupWindow.hide();
                }
            }
        } else {
            hide();
        }
    }

    @Override
    protected void onDraw(Canvas c) {
        switch (mAnimationState) {
        case STATE_IDLE:
            mDrawable.setAlpha(255);
            break;
        case STATE_FADEIN:
            float alpha = (float)(System.currentTimeMillis() - mAnimationStart) / mAnimationDuration;
            if (alpha >= 1.0f) {
                alpha = 1.0f;
                mAnimationState = STATE_IDLE;
            }
            mDrawable.setAlpha((int)(255*alpha));
            break;
        }
        if (mAnimationState != STATE_IDLE)
            invalidate();

        mDrawable.setBounds(0, 0, getRight() - getLeft(), getBottom() - getTop());
        mDrawable.draw(c);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        switch (ev.getActionMasked()) {
            case MotionEvent.ACTION_DOWN: {
                mDownPositionX = ev.getRawX();
                mDownPositionY = ev.getRawY();
                mTouchToWindowOffsetX = mDownPositionX - mPositionX;
                mTouchToWindowOffsetY = mDownPositionY - mPositionY;
                final int[] coords = mTempCoords;
                mParent.getLocationInWindow(coords);
                mLastParentX = coords[0];
                mLastParentY = coords[1];
                mIsDragging = true;
                mController.beforeStartUpdatingPosition(this);
                mTouchTimer = SystemClock.uptimeMillis();
                break;
            }

            case MotionEvent.ACTION_MOVE: {
                updatePosition(ev.getRawX(), ev.getRawY());
                break;
            }

            case MotionEvent.ACTION_UP:
                if (mIsInsertionHandle) {
                    long delay = SystemClock.uptimeMillis() - mTouchTimer;
                    if (delay < ViewConfiguration.getTapTimeout()) {
                        if (mPastePopupWindow != null && mPastePopupWindow.isShowing()) {
                            // Tapping on the handle dismisses the displayed paste view,
                            mPastePopupWindow.hide();
                        } else {
                            showPastePopupWindow();
                        }
                    }
                }
                mIsDragging = false;
                break;

            case MotionEvent.ACTION_CANCEL:
                mIsDragging = false;
                break;
        }
        return true;
    }

    public boolean isDragging() {
        return mIsDragging;
    }

    /**
     * @return Returns the x position of the handle
     */
    public int getPositionX() {
        return mPositionX;
    }

    /**
     * @return Returns the y position of the handle
     */
    public int getPositionY() {
        return mPositionY;
    }

    private void updatePosition(float rawX, float rawY) {
        final float newPosX = rawX - mTouchToWindowOffsetX + mHotspotX;
        final float newPosY = rawY - mTouchToWindowOffsetY + mHotspotY - mLineOffsetY;

        mController.updatePosition(this, Math.round(newPosX), Math.round(newPosY));
    }

    void positionAt(int x, int y) {
        moveTo((int)(x - mHotspotX), (int)(y - mHotspotY));
    }

    // Returns the x coordinate of the position that the handle appears to be pointing to.
    public int getAdjustedPositionX() {
        return (int) (mPositionX + mHotspotX);
    }

    // Returns the y coordinate of the position that the handle appears to be pointing to.
    public int getAdjustedPositionY() {
        return (int) (mPositionY + mHotspotY);
    }

    // Returns a suitable y coordinate for the text position corresponding to the handle.
    // As the handle points to a position on the base of the line of text, this method
    // returns a coordinate a small number of pixels higher (i.e. a slightly smaller number)
    // than getAdjustedPositionY.
    public int getLineAdjustedPositionY() {
        return (int) (mPositionY + mHotspotY - mLineOffsetY);
    }

    public Drawable getDrawable() {
        return mDrawable;
    }

    void showPastePopupWindow() {
        InsertionHandleController ihc = (InsertionHandleController) mController;
        if (mIsInsertionHandle && ihc.canPaste()) {
            if (mPastePopupWindow == null) {
                // Lazy initialization: create when actually shown only.
                mPastePopupWindow = ihc.new PastePopupMenu();
            }
            mPastePopupWindow.show();
        }
    }

    public void startFadein() {
        mAnimationState = STATE_FADEIN;
        mAnimationStart = System.currentTimeMillis();
        mAnimationDuration = 100;
        invalidate();
    }
}
