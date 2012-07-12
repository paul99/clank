// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.view.View;

/*
 * This class is based on SelectionModifierCursorController, the private inner
 * class of android.widget.TextView
 * TODO(olilan): refactor and share with Android code if possible
 */
abstract class SelectionHandleController implements CursorController {

    // The following constants match the ones in base/i18n/rtl.h.
    private static final int TEXT_DIRECTION_RTL = 1;
    private static final int TEXT_DIRECTION_LTR = 2;

    /** The cursor controller images, lazily created when shown. */
    private HandleView mStartHandle, mEndHandle;

    /** Whether handles should show automatically when text is selected. */
    private boolean mAllowAutomaticShowing = true;

    /** Whether selection anchors are active. */
    private boolean mIsShowing;

    private View mParent;

    private int mFixedHandleX;
    private int mFixedHandleY;

    SelectionHandleController(View parent) {
        mParent = parent;
    }

    /** Automatically show selection anchors when text is selected. */
    public void allowAutomaticShowing() {
        mAllowAutomaticShowing = true;
    }

    /** Hide selection anchors, and don't automatically show them. */
    public void hideAndDisallowAutomaticShowing() {
        hide();
        mAllowAutomaticShowing = false;
    }

    @Override
    public boolean isShowing() {
        return mIsShowing;
    }

    @Override
    public void hide() {
        if (mIsShowing) {
            if (mStartHandle != null) mStartHandle.hide();
            if (mEndHandle != null) mEndHandle.hide();
            mIsShowing = false;
        }
    }

    public void cancelFadeOutAnimation() {
        hide();
    }

    /**
     * Updates the selection for a movement of the given handle (which
     * should be the start handle or end handle) to coordinates x,y.
     * Note that this will not actually result in the handle moving to (x,y):
     * selectBetweenCoordinates(x1,y1,x2,y2) will trigger the selection and set the
     * actual coordinates later via showHandlesAt.
     */
    @Override
    public void updatePosition(HandleView handle, int x, int y) {
        selectBetweenCoordinates(mFixedHandleX, mFixedHandleY, x, y);
    }

    @Override
    public void beforeStartUpdatingPosition(HandleView handle) {
        HandleView fixedHandle = (handle == mStartHandle) ? mEndHandle : mStartHandle;
        mFixedHandleX = fixedHandle.getAdjustedPositionX();
        mFixedHandleY = fixedHandle.getLineAdjustedPositionY();
    }

    /**
     * The concrete implementation must trigger a selection between the given
     * coordinates and (possibly asynchronously) set the actual handle positions
     * after the selection is made via showHandlesAt(x1,y1,x2,y2).
     * @param x1
     * @param y1
     * @param x2
     * @param y2
     */
    public abstract void selectBetweenCoordinates(int x1, int y1, int x2, int y2);

    /**
     * @return true iff this controller is currently used to move the selection start.
     */
    public boolean isSelectionStartDragged() {
        return mStartHandle != null && mStartHandle.isDragging();
    }

    /**
     * @return true iff this controller is currently being used to drag either the selection start or end.
     */
    public boolean isDragging() {
        return (mStartHandle != null && mStartHandle.isDragging()) ||
               (mEndHandle != null && mEndHandle.isDragging());
    }

    @Override
    public void onTouchModeChanged(boolean isInTouchMode) {
        if (!isInTouchMode) {
            hide();
        }
    }

    @Override
    public void onDetached() {}

    /**
     * Moves the start handle so that it points at the given coordinates.
     * @param x
     * @param y
     */
    public void setStartHandlePosition(int x, int y) {
        mStartHandle.positionAt(x, y);
    }

    /**
     * Moves the end handle so that it points at the given coordinates.
     * @param x
     * @param y
     */
    public void setEndHandlePosition(int x, int y) {
        mEndHandle.positionAt(x, y);
    }

    /**
     * Moves the start handle by x pixels horizontally and y pixels vertically.
     * @param x
     * @param y
     */
    public void moveStartHandleBy(int x, int y) {
        mStartHandle.moveTo(mStartHandle.getPositionX() + x, mStartHandle.getPositionY() + y);
    }

    /**
     * Moves the end handle by x pixels horizontally and y pixels vertically.
     * @param x
     * @param y
     */
    public void moveEndHandleBy(int x, int y) {
        mEndHandle.moveTo(mEndHandle.getPositionX() + x, mEndHandle.getPositionY() + y);
    }

    public void onSelectionChanged(int x1, int y1, int dir1, int x2, int y2, int dir2) {
        if (mAllowAutomaticShowing) {
            showHandlesAt(x1, y1, dir1, x2, y2, dir2);
        }
    }

    /**
     * Sets both start and end position and show the handles.
     * Note: this method does not trigger a selection, see
     * selectBetweenCoordinates()
     *
     * @param x1
     * @param y1
     * @param x2
     * @param y2
     */
    public void showHandlesAt(int x1, int y1, int dir1, int x2, int y2, int dir2) {
        createHandlesIfNeeded(dir1, dir2);
        setStartHandlePosition(x1, y1);
        setEndHandlePosition(x2, y2);
        showHandlesIfNeeded();
    }

    private void createHandlesIfNeeded(int start_dir, int end_dir) {
        if (mStartHandle == null) {
            mStartHandle = new HandleView(this,
                start_dir == TEXT_DIRECTION_LTR ? HandleView.LEFT : HandleView.RIGHT, mParent);
        }
        if (mEndHandle == null) {
            mEndHandle = new HandleView(this,
                end_dir == TEXT_DIRECTION_LTR ? HandleView.RIGHT : HandleView.LEFT, mParent);
        }
    }

    private void showHandlesIfNeeded() {
        if (!mIsShowing) {
            mIsShowing = true;
            mStartHandle.show();
            mEndHandle.show();
        }
    }

    public boolean isHiddenTemporarily() {
        if (mStartHandle != null) return mStartHandle.getVisibility() == View.INVISIBLE;
        return false;
    }

    public void hideTemporarily() {
        if (mStartHandle != null) mStartHandle.setVisibility(View.INVISIBLE);
        if (mEndHandle != null) mEndHandle.setVisibility(View.INVISIBLE);
    }

    public void undoHideTemporarily() {
        if (mStartHandle != null) {
            mStartHandle.setVisibility(View.VISIBLE);
            mStartHandle.startFadein();
        }
        if (mEndHandle != null) {
            mEndHandle.setVisibility(View.VISIBLE);
            mEndHandle.startFadein();
        }
    }
}
