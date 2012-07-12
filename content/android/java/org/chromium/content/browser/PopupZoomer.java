// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Path.Direction;
import android.graphics.PointF;
import android.graphics.PorterDuff.Mode;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region.Op;
import android.graphics.drawable.NinePatchDrawable;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.Interpolator;
import android.view.animation.OvershootInterpolator;

/*
 * PopupZoomer is used for the on-demand link zooming popup.
 * It handles manipulation of the canvas and touch events to display
 * the on-demand zoom magnifier.
 */
class PopupZoomer extends View {
    // The padding between the edges of the view and the popup.
    // Note that there is a mirror constant in chrome/renderer/render_view.cc
    // which should be kept in sync if this is changed.
    private static final int ZOOM_BOUNDS_MARGIN = 25;
    // Time it takes for the animation to finish in ms.
    private static final long ANIMATION_TIME = 300;

    public static interface OnTapListener {
        public boolean onSingleTap(View v, MotionEvent event);
        public boolean onLongPress(View v, MotionEvent event);
    }
    private OnTapListener mOnTapListener = null;

    // ID of resource used for drawing the frame around the zooming popup.
    private static int sOverlayResourceId;
    // Cached drawable used to frame the zooming popup.
    // TODO(tonyg): This should be marked purgeable so that if the system wants
    // to recover this memory, we can just reload it from the resource ID next
    // time it is needed. See http://b/5052865.
    private static NinePatchDrawable sOverlayNinePatchDrawable;
    // The padding used for drawing the overlay around the content, instead of directly
    // above it.
    private static Rect sOverlayPadding;
    // The radius of the overlay bubble, used for rounding the bitmap to draw underneath it.
    private static float sOverlayCornerRadius;

    private Interpolator mShowInterpolator = new OvershootInterpolator();
    private Interpolator mHideInterpolator = new ReverseInterpolator(mShowInterpolator);

    private boolean mAnimating = false;
    private boolean mShowing = false;
    private long mAnimationStartTime = 0;
    // The time that was left for the outwards animation to finish.
    // This is used in the case that the zoomer is cancelled while it is still
    // animating outwards, to avoid having it jump to full size then animate closed.
    private long mTimeLeft = 0;

    private RectF mViewClipRect;

    private Rect mTargetBounds;

    // How far to shift the canvas after all zooming is done,
    // to keep it inside the bounds of the view.
    private float mShiftX = 0, mShiftY = 0;
    // The magnification factor of the popup.
    private float mScale = 1.0f;
    // The bounds that are being grown.  The extrusion values are how
    // far the area extends from the touch point.
    private RectF mClipRect;
    private float mLeftExtrusion, mTopExtrusion, mRightExtrusion, mBottomExtrusion;
    // The last touch point, where the animation will start from.
    private PointF mTouch = new PointF();

    private Bitmap mZoomedBitmap;

    private float mScrollX, mScrollY;
    private float mMinScrollX, mMaxScrollX;
    private float mMinScrollY, mMaxScrollY;

    private GestureDetector mGestureDetector;

    public static void setOverlayResourceId(int id) {
        sOverlayResourceId = id;
        sOverlayNinePatchDrawable = null;
    }

    public static void setOverlayCornerRadius(float r) {
        sOverlayCornerRadius = r;
    }

    /**
     * Gets the drawable that should be used to frame the zooming popup, loading
     * it from the resource bundle if not already cached.
     */
    private NinePatchDrawable getOverlayNinePatchDrawable() {
        assert sOverlayResourceId != 0;
        if (sOverlayNinePatchDrawable == null) {
            sOverlayNinePatchDrawable =
                (NinePatchDrawable)getContext().getResources().getDrawable(sOverlayResourceId);
            sOverlayPadding = new Rect();
            sOverlayNinePatchDrawable.getPadding(sOverlayPadding);
        }
        return sOverlayNinePatchDrawable;
    }

    // Copied from android.util.MathUtils.
    // TODO(wangxianzhu): Should use MathUtils.constrain() once it becomes public.
    private static float constrain(float amount, float low, float high) {
        return amount < low ? low : (amount > high ? high : amount);
    }

    private static int constrain(int amount, int low, int high) {
        return amount < low ? low : (amount > high ? high : amount);
    }

    public PopupZoomer(Context c) {
        super(c);

        setVisibility(INVISIBLE);
        setFocusable(true);
        setFocusableInTouchMode(true);

        GestureDetector.SimpleOnGestureListener listener =
            new GestureDetector.SimpleOnGestureListener() {
                @Override
                public boolean onScroll(MotionEvent e1, MotionEvent e2,
                        float distanceX, float distanceY) {
                    if (mAnimating) return true;

                    float x = e1.getX();
                    float y = e1.getY();
                    if (!isTouchOutsideArea(x, y)) {
                        scroll(distanceX, distanceY);
                    } else {
                        hide(true);
                    }
                    return true;
                }

                @Override
                public boolean onSingleTapUp(MotionEvent e) {
                    return handleTapOrPress(e, false);
                }

                @Override
                public void onLongPress(MotionEvent e) {
                    handleTapOrPress(e, true);
                }

                private boolean handleTapOrPress(MotionEvent e, boolean isLongPress) {
                    if (mAnimating) return true;

                    float x = e.getX();
                    float y = e.getY();
                    if (isTouchOutsideArea(x, y)) {
                        hide(true);
                    } else if (mOnTapListener != null) {
                        PointF converted = convertTouchPoint(e.getX(), e.getY());
                        MotionEvent event = MotionEvent.obtainNoHistory(e);
                        event.setLocation(converted.x, converted.y);
                        if (isLongPress) {
                            mOnTapListener.onLongPress(PopupZoomer.this, event);
                        } else {
                            mOnTapListener.onSingleTap(PopupZoomer.this, event);
                        }
                        hide(true);
                    }
                    return true;
                }
        };
        mGestureDetector = new GestureDetector(c, listener);
    }

    public void setOnTapListener(OnTapListener listener) {
        mOnTapListener = listener;
    }

    public void setBitmap(Bitmap b) {
        if (mZoomedBitmap != null) {
            mZoomedBitmap.recycle();
            mZoomedBitmap = null;
        }
        mZoomedBitmap = b;
        // Round the corners of the bitmap so it doesn't stick out
        // around the overlay.
        Canvas c = new Canvas(mZoomedBitmap);
        Path p = new Path();
        RectF canvasRect = new RectF(0, 0, c.getWidth(), c.getHeight());
        p.addRoundRect(canvasRect,
                       sOverlayCornerRadius,
                       sOverlayCornerRadius,
                       Direction.CCW);
        c.clipPath(p, Op.XOR);
        Paint clearPaint = new Paint();
        clearPaint.setXfermode(new PorterDuffXfermode(Mode.SRC));
        clearPaint.setColor(Color.TRANSPARENT);
        c.drawPaint(clearPaint);
    }

    private boolean scroll(float x, float y) {
        mScrollX = constrain(mScrollX - x, mMinScrollX, mMaxScrollX);
        mScrollY = constrain(mScrollY - y, mMinScrollY, mMaxScrollY);
        invalidate();
        return true;
    }

    private void startAnimation(boolean show) {
        mAnimating = true;
        mShowing = show;
        mTimeLeft = 0;
        if (show) {
            setVisibility(VISIBLE);
            initDimensions();
        } else {
            long endTime = mAnimationStartTime + ANIMATION_TIME;
            mTimeLeft = endTime - System.currentTimeMillis();
            if (mTimeLeft < 0) mTimeLeft = 0;
        }
        mAnimationStartTime = System.currentTimeMillis();
        invalidate();
    }

    public void hideImmediately() {
        mAnimating = false;
        mShowing = false;
        mTimeLeft = 0;
        setVisibility(INVISIBLE);
        mZoomedBitmap.recycle();
        mZoomedBitmap = null;
    }

    public boolean isShowing() {
        return mShowing || mAnimating;
    }

    public void setLastTouch(float x, float y) {
        mTouch.x = x;
        mTouch.y = y;
    }

    private void setTargetBounds(Rect rect) {
        mViewClipRect = new RectF(ZOOM_BOUNDS_MARGIN,
                ZOOM_BOUNDS_MARGIN,
                getWidth() - ZOOM_BOUNDS_MARGIN,
                getHeight() - ZOOM_BOUNDS_MARGIN);
        mTargetBounds = rect;
    }

    private void initDimensions() {
        if (mTargetBounds == null ||
            mTouch == null) {
            return;
        }

        mScale = (float)mZoomedBitmap.getWidth() / mTargetBounds.width();

        float l = mTouch.x - mScale * (mTouch.x - mTargetBounds.left);
        float t = mTouch.y - mScale * (mTouch.y - mTargetBounds.top);
        float r = l + mZoomedBitmap.getWidth();
        float b = t + mZoomedBitmap.getHeight();
        mClipRect = new RectF(l, t, r, b);
        int width = getWidth();
        int height = getHeight();

        // Ensure it stays inside the bounds of the view.  First shift it around to see if it
        // can fully fit in the view, then clip it to the padding section of the view to
        // ensure no overflow.
        mShiftX = 0;
        mShiftY = 0;

        // Right now this has the happy coincidence of showing the leftmost portion
        // of a scaled up bitmap, which usually has the text in it.  When we want to support
        // RTL languages, we can conditionally switch the order of this check to push it
        // to the left instead of right.
        if (mClipRect.left < ZOOM_BOUNDS_MARGIN) {
            mShiftX = ZOOM_BOUNDS_MARGIN - mClipRect.left;
            mClipRect.left += mShiftX;
            mClipRect.right += mShiftX;
        } else if (mClipRect.right > width - ZOOM_BOUNDS_MARGIN) {
            mShiftX = (width - ZOOM_BOUNDS_MARGIN - mClipRect.right);
            mClipRect.right += mShiftX;
            mClipRect.left += mShiftX;
        }
        if (mClipRect.top < ZOOM_BOUNDS_MARGIN) {
            mShiftY = ZOOM_BOUNDS_MARGIN - mClipRect.top;
            mClipRect.top += mShiftY;
            mClipRect.bottom += mShiftY;
        } else if (mClipRect.bottom > height - ZOOM_BOUNDS_MARGIN) {
            mShiftY = height - ZOOM_BOUNDS_MARGIN - mClipRect.bottom;
            mClipRect.bottom += mShiftY;
            mClipRect.top += mShiftY;
        }

        // Allow enough scrolling to get to the entire bitmap that
        // may be clipped inside the bounds of the view.
        mMinScrollX = mMaxScrollX = mMinScrollY = mMaxScrollY = 0;
        if (mViewClipRect.right + mShiftX < mClipRect.right)
            mMinScrollX = mViewClipRect.right - mClipRect.right;
        if (mViewClipRect.left + mShiftX > mClipRect.left)
            mMaxScrollX = mViewClipRect.left - mClipRect.left;
        if (mViewClipRect.top + mShiftY > mClipRect.top)
            mMaxScrollY = mViewClipRect.top - mClipRect.top;
        if (mViewClipRect.bottom + mShiftY < mClipRect.bottom)
            mMinScrollY = mViewClipRect.bottom - mClipRect.bottom;
        mClipRect.intersect(mViewClipRect);

        mLeftExtrusion = mTouch.x - mClipRect.left;
        mRightExtrusion = mClipRect.right - mTouch.x;
        mTopExtrusion = mTouch.y - mClipRect.top;
        mBottomExtrusion = mClipRect.bottom - mTouch.y;
        float scrollWidth = mMaxScrollX - mMinScrollX;
        float scrollHeight = mMaxScrollY - mMinScrollY;
        float percentX = (mTouch.x - mTargetBounds.centerX()) / (mTargetBounds.width() / 2.f) + .5f;
        float percentY = (mTouch.y - mTargetBounds.centerY()) / (mTargetBounds.height() / 2.f) + .5f;
        mScrollX = scrollWidth * percentX * -1f;
        mScrollY = scrollHeight * percentY * -1f;
        mScrollX = constrain(mScrollX, mMinScrollX, mMaxScrollX);
        mScrollY = constrain(mScrollY, mMinScrollY, mMaxScrollY);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (!isShowing() || mZoomedBitmap == null) return;
        canvas.save();
        float time = (System.currentTimeMillis() - mAnimationStartTime + mTimeLeft) /
                ((float)ANIMATION_TIME);
        time = constrain(time, 0, 1);
        if (time >= 1) {
            mAnimating = false;
            if (!isShowing()) {
                hideImmediately();
                return;
            }
        } else {
            invalidate();
        }

        if (mShowing) {
            time = mShowInterpolator.getInterpolation(time);
        } else {
            time = mHideInterpolator.getInterpolation(time);
        }

        // Draw a faded color over the entire view to fade out the original content.
        canvas.drawARGB((int)(80 * time), 0, 0, 0);
        canvas.save();

        // Since we want the content to appear directly above its counterpart
        // we need to make sure that it starts out at exactly the same size as
        // it appears in the page.
        float scale = time * (mScale - 1.0f) / mScale + 1.0f / mScale;

        RectF rect = new RectF();
        // Since we want the content to appear directly above its counterpart on the
        // page, we need to remove the mShiftX/Y effect at the beginning of the animation.
        float unshiftX = - mShiftX * (1.0f - time) / mScale;
        float unshiftY = - mShiftY * (1.0f - time) / mScale;
        rect.left = mTouch.x - mLeftExtrusion * scale + unshiftX;
        rect.top = mTouch.y - mTopExtrusion * scale + unshiftY;
        rect.right = mTouch.x + mRightExtrusion * scale + unshiftX;
        rect.bottom = mTouch.y + mBottomExtrusion * scale + unshiftY;
        canvas.clipRect(rect);

        // Since the canvas transform APIs all pre-concat the
        // transformations, this is done in reverse order.
        // The canvas is first scaled up, then shifted the appropriate
        // amount of pixels.
        canvas.scale(scale, scale, rect.left, rect.top);
        canvas.translate(mScrollX, mScrollY);
        canvas.drawBitmap(mZoomedBitmap, rect.left, rect.top, null);
        canvas.restore();
        NinePatchDrawable overlayNineTile = getOverlayNinePatchDrawable();
        overlayNineTile.setBounds((int)rect.left - sOverlayPadding.left,
                (int)rect.top - sOverlayPadding.top,
                (int)rect.right + sOverlayPadding.right,
                (int)rect.bottom + sOverlayPadding.bottom);
        int alpha = constrain((int)(time * 255), 0, 255);
        overlayNineTile.setAlpha(alpha);
        overlayNineTile.draw(canvas);
        canvas.restore();
    }

    public void show(Rect rect){
        if (mShowing || mZoomedBitmap == null) return;

        setTargetBounds(rect);
        startAnimation(true);
    }

    public void hide(boolean animation){
        if (!mShowing)
            return;

        if (animation)
            startAnimation(false);
        else
            hideImmediately();
    }

    // Returns whether or not a point is inside the drawable area for this popup zoomer.
    private boolean isTouchOutsideArea(float x, float y) {
        return !mClipRect.contains(x, y);
    }

    @Override
    public boolean onTouchEvent (MotionEvent event) {
        mGestureDetector.onTouchEvent(event);
        return true;
    }

    private static class ReverseInterpolator implements Interpolator {
        private Interpolator mInterpolator;

        public ReverseInterpolator(Interpolator i) {
            mInterpolator = i;
        }

        @Override
        public float getInterpolation(float input) {
            input = 1.0f - input;
            if (mInterpolator == null)
                return input;
            return mInterpolator.getInterpolation(input);
        }
    }

    private PointF convertTouchPoint(float x, float y) {
        x -= mShiftX;
        y -= mShiftY;
        x = mTouch.x + (x - mTouch.x - mScrollX) / mScale;
        y = mTouch.y + (y - mTouch.y - mScrollY) / mScale;
        return new PointF(x, y);
    }
}
