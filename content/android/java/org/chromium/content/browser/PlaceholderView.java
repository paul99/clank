// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.View;

/**
 * PlaceholderView is a static snapshot of the page which is shown while the TextureView
 * contents are not available. This happens when a resize is in progress.
 */
class PlaceholderView extends View {
    private static final boolean DEBUG_SHOW_PLACEHOLDER_VIEW_ACTIVE = false;

    private Bitmap mBitmap;
    private final Paint mBgPaint = new Paint();
    private final Rect mSurfaceRect = new Rect();
    private boolean mActive = false;
    private final ChromeView mChromeView;
    private Runnable mPostHideRunnable;

    public PlaceholderView(Context context, ChromeView chromeView) {
        super(context);
        mChromeView = chromeView;
        show();
    }

    public void show() {
        stopAnimation();
        setVisibility(View.VISIBLE);
        mActive = true;
    }

    /**
     * Immediately hides this view (terminating any active animations).
     */
    // Do not call this from onAnimationEnd of any view animations as it causes an awesome stack
    // overflow as cancel() on an animation calls onAnimationEnd.
    public void hideImmediately() {
        stopAnimation();
        hideViewAndDeactivate();
    }

    private void hideViewAndDeactivate() {
        setVisibility(View.INVISIBLE);
        mActive = false;
        if (mPostHideRunnable != null) {
            mPostHideRunnable.run();
            mPostHideRunnable = null;
        }
    }

    public void hideLater() {
        mActive = false;
        post(new Runnable() {
            @Override
            public void run() {
                if (!mActive) {
                    hideImmediately();
                }
            }
        });
    }

    public void hideWithAnimation() {
        mActive = false;
        AlphaAnimation anim = new AlphaAnimation(1.0f, 0.0f);
        anim.setDuration(150);
        anim.setAnimationListener(new Animation.AnimationListener() {
            @Override
            public void onAnimationStart(Animation animation) {
            }
            @Override
            public void onAnimationRepeat(Animation animation) {
            }
            @Override
            public void onAnimationEnd(Animation animation) {
                hideViewAndDeactivate();
            }
        });
        startAnimation(anim);
    }

    private void stopAnimation() {
        if (getAnimation() != null) {
            getAnimation().cancel();
            setAnimation(null);
        }
    }

    public boolean contentCoversView() {
        return mSurfaceRect.contains(0, 0, getWidth(), getHeight());
    }

    public void setBitmap(Bitmap bitmap, int cropWidth, int cropHeight) {
        mBitmap = bitmap;
        if (bitmap != null) {
            mSurfaceRect.set(0, 0, cropWidth, cropHeight);
        } else {
            mSurfaceRect.set(0, 0, 0, 0);
        }
    }

    public void resetBitmap() {
        mBitmap = null;
        mSurfaceRect.set(0, 0, 0, 0);
    }

    public boolean isActive() {
        return mActive;
    }

    @Override
    protected void onVisibilityChanged(View changedView, int visibility) {
        if (visibility != View.VISIBLE) {
            resetBitmap();
        }
    }

    @Override
    public void onDraw(Canvas canvas) {
        mBgPaint.setColor(mChromeView.getBackgroundColor());
        // Fill right.
        canvas.drawRect(mSurfaceRect.right, 0, getWidth(), mSurfaceRect.bottom, mBgPaint);
        // Fill below.
        canvas.drawRect(0, mSurfaceRect.bottom, getWidth(), getHeight(), mBgPaint);
        // Draw snapshot.
        if (mBitmap != null) {
            canvas.drawBitmap(mBitmap, null, mSurfaceRect, mBgPaint);
            mBgPaint.setColorFilter(null);
        }

        if (DEBUG_SHOW_PLACEHOLDER_VIEW_ACTIVE) {
            mBgPaint.setColor(Color.RED);
            canvas.drawRect(10, 10, 90, 90, mBgPaint);
            mBgPaint.setColorFilter(null);
        }
    }

    protected void setPostHideRunnable(Runnable r) {
        mPostHideRunnable = r;
    }
}
