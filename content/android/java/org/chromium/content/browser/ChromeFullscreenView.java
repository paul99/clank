// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;

public class ChromeFullscreenView extends FrameLayout {

    private Surface mSurface = null;

    private static Activity mChromeActivity;
    private static FrameLayout mRootLayout;
    private static ViewGroup mContentContainer;
    private static ViewGroup mControlContainer;

    // There are can be at most 1 fullscreen video
    // TODO(qinmin): will change this once  we move the creation of this class
    // to the host application
    private static ChromeFullscreenView mFullscreenView = null;

    public ChromeFullscreenView(Context context) {
        super(context);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        return true;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_UP) {
            exitFullScreen();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    public static Activity getChromeActivity() {
        return mChromeActivity;
    }

    public static void registerChromeActivity(Activity activity, FrameLayout rootLayout,
            ViewGroup controlContainer, ViewGroup contentContainer) {
        mChromeActivity = activity;
        mRootLayout = rootLayout;
        mControlContainer = controlContainer;
        mContentContainer = contentContainer;
    }

    // Show the content of the fullscreen view, Child class can override this
    // for its own implementation.
    void showFullscreenView() {
    }

    // remove the content of the fullscreen view, Child class can override this
    // for its own implementation.
    void removeFullscreenView() {
    }

    public static void showFullScreen(ChromeFullscreenView fullscreenView) {
        if (mFullscreenView != null) {
            return;
        }

        mFullscreenView = fullscreenView;

        // TODO(qinmin): Hiding the content container will sometimes deadlock the
        // render thread. Need to figure this out.
        // mContentContainer.setVisibility(View.GONE);
        // mControlContainer.setVisibility(View.GONE);

        mChromeActivity.getWindow().setFlags(
                WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        mChromeActivity.getWindow().addContentView(fullscreenView,
                new FrameLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        Gravity.CENTER));

        fullscreenView.setBackgroundColor(Color.BLACK);
        mFullscreenView.showFullscreenView();
        fullscreenView.setVisibility(View.VISIBLE);
    }

    public static ChromeFullscreenView getFullscreenView() {
        return mFullscreenView;
    }

    public static void exitFullScreen() {
        mChromeActivity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        if (mFullscreenView != null) {
            mFullscreenView.removeFullscreenView();
            mFullscreenView.setVisibility(View.GONE);
            mRootLayout.removeView(mFullscreenView);
        }

        // TODO(qinmin): Uncomment this when we figure out why hiding the content container
        // will deadlock the render thread.
        // mContentContainer.setVisibility(View.VISIBLE);
        // mControlContainer.setVisibility(View.VISIBLE);

        mFullscreenView = null;
    }
}
