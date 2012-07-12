// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

/**
 * Various debugging/profiling tools related to {@link ChromeView}
 */
public class ChromeViewDebug {

    /**
     * Start profiling the update speed. You must call {@link #stopFpsProfiling}
     * to stop profiling.
     * @param chromeView The ChromeView.
     */
    public static void startFpsProfiling(ChromeView chromeView) {
        chromeView.startFpsProfiling();
    }

    /**
     * Stop profiling the update speed.
     * @param chromeView The ChromeView.
     * @return The ChromeView update speed in fps.
     */
    public static float stopFpsProfiling(ChromeView chromeView) {
        return chromeView.stopFpsProfiling();
    }

    /**
     * Fling the ChromeView from the current position.
     * @param chromeView The ChromeView.
     * @param x Fling touch starting position
     * @param y Fling touch starting position
     * @param velocityX Initial velocity of the fling (X) measured in pixels per second.
     * @param velocityY Initial velocity of the fling (Y) measured in pixels per second.
     */
    public static void fling(ChromeView chromeView, int x, int y, int velocityX, int velocityY) {
        chromeView.fling(x, y, velocityX, velocityY);
    }

    /**
     * Start pinch zoom. You must call {@link #pinchEnd} to stop.
     * @param chromeView The ChromeView
     */
    public static void pinchBegin(ChromeView chromeView) {
        chromeView.pinchBegin();
    }

    /**
     * Stop pinch zoom.
     * @param chromeView The ChromeView
     */
    public static void pinchEnd(ChromeView chromeView) {
        chromeView.pinchEnd();
    }

    /**
     * Send a pinch zoom event. This must be called inside
     * {@link #pinchBegin} and {@link #pinchEnd}.
     * @param chromeView The ChromeView.
     * @param delta The ratio of the new magnification level over the current
     *            magnification level.
     * @param anchorX The magnification anchor (X) in the current view
     *            coordinate.
     * @param anchorY The magnification anchor (Y) in the current view
     *            coordinate.
     */
    public static void pinchBy(ChromeView chromeView, float delta, int anchorX, int anchorY) {
        chromeView.pinchBy(delta, anchorX, anchorY);
    }

    /**
     * @param chromeView the ChromeView.
     * @return The most recently updated version of the complete content width.
     */
    public static int getContentWidth(ChromeView chromeView) {
        return chromeView.computeHorizontalScrollRange();
    }

    /**
     * @param chromeView the ChromeView.
     * @return The most recently updated version of the complete content height.
     */
    public static int getContentHeight(ChromeView chromeView) {
        return chromeView.computeVerticalScrollRange();
    }

    /**
     * @param chromeView the ChromeView.
     */
    public static void hideContextualActionBar(ChromeView chromeView) {
        chromeView.hideSelectActionBar();
    }
}
