// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.view.ViewTreeObserver;

 /**
 * A CursorController instance can be used to control a cursor in the text.
 * This class is based on the private inner interface CursorController in
 * android.widget.TextView
 * TODO(olilan): refactor and share with Android code if possible
 */
interface CursorController extends ViewTreeObserver.OnTouchModeChangeListener {

    /**
     * Hide the cursor controller from screen.
     */
    public void hide();

    /**
     * @return true if the CursorController is currently visible
     */
    public boolean isShowing();

    /**
     * Called when the handle is about to start updating its position.
     * @param handle
     */
    public void beforeStartUpdatingPosition(HandleView handle);

    /**
     * Update the controller's position.
     */
    public void updatePosition(HandleView handle, int x, int y);

    /**
     * Called when the view is detached from window. Perform house keeping task, such as
     * stopping Runnable thread that would otherwise keep a reference on the context, thus
     * preventing the activity to be recycled.
     */
    public void onDetached();
}