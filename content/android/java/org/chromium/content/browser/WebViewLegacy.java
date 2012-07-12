// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.view.View;

import org.chromium.content.browser.legacy.PluginList;

/**
 * This class contains implementations of all methods from the legacy WebView which are supported
 * only to maintain backwards compatibility.
 */
class WebViewLegacy {
    private ChromeView mChromeView;

    WebViewLegacy(ChromeView chromeView) {
        mChromeView = chromeView;
    }

    /**
     * Return the list of currently loaded plugins.
     * @return The list of currently loaded plugins.
     */
    public static PluginList getPluginList() {
        // This method was used for Gears, which has been deprecated. It has
        // done nothing since it was deprecated.
        return null;
    }

    public void refreshPlugins(boolean reloadOpenPages) {
        // This method was used for Gears, which has been deprecated. It has
        // done nothing since it was deprecated.
    }

    /**
     * Pause all layout, parsing, and JavaScript timers for all WebViews. This
     * applies to all WebViews in this process, not just this WebView. This can
     * be useful if the application has been paused.
     */
    public void pauseTimers() {
        mChromeView.onActivityPause();
    }

    /**
     * Resume all layout, parsing, and JavaScript timers for all WebViews. This
     * will resume dispatching all timers.
     */
    public void resumeTimers() {
        mChromeView.onActivityResume();
    }

    /**
     * Call this to pause any extra processing associated with this WebView and
     * its associated DOM, plugins, JavaScript etc. For example, if the WebView
     * is taken offscreen, this could be called to reduce unnecessary CPU or
     * network traffic. When the WebView is again "active", call onResume().
     *
     * Note that this differs from pauseTimers(), which affects all WebViews.
     */
    public void onPause() {
        mChromeView.onHide();
    }

    /**
     * Call this to resume a WebView after a previous call to onPause().
     */
    public void onResume() {
        mChromeView.onShow();
    }

    /**
     * Find all instances of searchString on the page and highlight them.
     *
     * @param searchString String to find.
     * @return The number of occurrences of the searchString that were found.
     */
    public int findAll(String searchString) {
        return mChromeView.findAllLegacy(searchString);
    }

    /**
     * Highlight and scroll to the next occurrence of the string last passed to
     * {@link ChromeView#findAll(String) findAll()}. The search wraps around the
     * page infinitely. Must be preceded by a call to
     * {@link ChromeView#findAll(String) findAll()} that has not been cancelled
     * by a subsequent call to {@link ChromeView#clearMatches() clearMatches()}.
     *
     * @see ChromeView#findAll(String)
     * @see ChromeView#clearMatches()
     * @param forwards Whether to search forwards (true) or backwards (false)
     */
    public void findNext(boolean forwards) {
        mChromeView.findNextLegacy(forwards);
    }

    /**
     * Clear the highlighting surrounding text matches created by
     * {@link ChromeView#findAll(String) findAll()}.
     *
     * @see ChromeView#findAll(String)
     * @see ChromeView#findNext(boolean)
     */
    // Note that this just calls the asynchronous version. That is fine because
    // clearMatches doesn't trigger any additional actions that other methods
    // can rely on. It simply clears all the highlights on the page.
    public void clearMatches() {
        mChromeView.stopFinding(ChromeView.FindSelectionAction.CLEAR_SELECTION);
    }

    /**
     * Return true if the page can navigate back or forward in the page history
     * the given number of steps. Negative steps means going backwards, positive
     * steps means going forwards.
     *
     * @param steps The number of steps to check against in the page history.
     */
    public boolean canGoBackOrForward(int steps) {
        return mChromeView.canGoBackOrForwardLegacy(steps);
    }

    /**
     * Go to the history item that is the number of steps away from the current
     * item. Negative steps means going backwards, positive steps means going
     * forwards. If steps is beyond available history, nothing happens.
     *
     * @param steps The number of steps to take back or forward in the history.
     */
    public void goBackOrForward(int steps) {
        mChromeView.goBackOrForwardLegacy(steps);
    }

    /**
     * Called to inform the WebView that a new child is added to a parent view.
     * This method no longer needs to be called on the WebView and is a no-op.
     */
    // This is to implement ViewGroup.OnHierarchyChangeListener
    public void onChildViewAdded(View parent, View child) {
    }

    /**
     * Called to inform the WebView that a child is removed from a parent view.
     * This method no longer needs to be called on the WebView and is a no-op.
     */
    // This is to implement ViewGroup.OnHierarchyChangeListener
    public void onChildViewRemoved(View p, View child) {
    }

    /**
     * Called to inform the WebView about the focus change in the view tree.
     * This method no longer needs to be called on the WebView and is a no-op.
     */
    // This is to implement ViewTreeObserver.OnGlobalFocusChangeListener
    public void onGlobalFocusChanged(View oldFocus, View newFocus) {
    }

    /**
     * Load the given data into the WebView using a 'data' scheme URL. Content
     * loaded in this way does not have the ability to load content from the
     * network.
     * <p>
     * If the value of the encoding parameter is 'base64', then the data must be
     * encoded as base64. Otherwise, the data must use ASCII encoding for octets
     * inside the range of safe URL characters and use the standard %xx hex
     * encoding of URLs for octets outside that range.
     *
     * @param data A String of data in the given encoding.
     * @param mimeType The MIMEType of the data, e.g. 'text/html'.
     * @param encoding The encoding of the data.
     */
    public void loadData(String data, String mimeType, String encoding) {
        StringBuilder dataUrl = new StringBuilder("data:");
        dataUrl.append(mimeType);
        if ("base64".equals(encoding)) {
            dataUrl.append(";base64");
        }
        dataUrl.append(",");
        dataUrl.append(data);
        mChromeView.loadUrlWithoutUrlSanitization(dataUrl.toString());
    }

    /**
     * Get the height in pixels of the visible portion of the title bar.
     *
     * @return The height in pixels of the visible portion of the title bar.
     */
    public int getVisibleTitleHeight() {
        // The implementation in the legacy WebView is such that we always
        // return 0 in a WebView manipulated through only the public API.
        return 0;
    }

    // TODO: Add implementations of all deprecated methods.
}
