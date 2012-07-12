// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.os.Handler;
import android.os.Message;

import org.chromium.base.AccessedByNative;
import org.chromium.base.CalledByNative;

/**
 * Manages settings state for a WebView. When a WebView is first created, it
 * obtains a set of default settings. These default settings will be returned
 * from any getter call. A WebSettings object obtained from
 * WebView.getSettings() is tied to the life of the WebView. If a WebView has
 * been destroyed, any method call on WebSettings will throw an
 * IllegalStateException.
 */
public class WebSettings extends org.chromium.content.browser.legacy.WebSettings {
    // This class must be created on the UI thread. Afterwards, it can be
    // used from any thread. Internally, the class uses a message queue
    // to call native code on the UI thread only.

    private int mNativeWebSettings = 0;

    private ChromeView mWebView;
    private int mNativeChromeView = 0;

    // A flag to avoid sending superfluous synchronization messages.
    private boolean mIsSyncMessagePending = false;
    // Custom handler that queues messages to call native code on the UI thread.
    private final EventHandler mEventHandler;

    private static final int MINIMUM_FONT_SIZE = 1;
    private static final int MAXIMUM_FONT_SIZE = 72;

    // Private settings so we don't have to go into native code to
    // retrieve the values. After setXXX, sendSyncMessage() needs to be called.
    //
    // TODO(mnaganov): populate with the complete set of legacy WebView settings.

    @AccessedByNative
    private String mStandardFontFamily = "sans-serif";
    private String mFixedFontFamily = "monospace";
    private String mSansSerifFontFamily = "sans-serif";
    private String mSerifFontFamily = "serif";
    private String mCursiveFontFamily = "cursive";
    private String mFantasyFontFamily = "fantasy";
    // FIXME: Should be obtained from Android. Problem: it is hidden.
    private String mDefaultTextEncoding = "Latin-1";
    private int mMinimumFontSize = 8;
    private int mMinimumLogicalFontSize = 8;
    private int mDefaultFontSize = 16;
    private int mDefaultFixedFontSize = 13;
    private boolean mLoadsImagesAutomatically = true;
    private boolean mJavaScriptEnabled = false;
    private boolean mJavaScriptCanOpenWindowsAutomatically = false;
    private PluginState mPluginState = PluginState.OFF;

    // Class to handle messages to be processed on the UI thread.
    private class EventHandler {
        // Actual UI thread handler
        private Handler mHandler;

        EventHandler() {
            mHandler = new Handler() {
                @Override
                public void handleMessage(Message msg) {
                    synchronized (WebSettings.this) {
                        nativeSync(mNativeWebSettings, mNativeChromeView);
                        mIsSyncMessagePending = false;
                    }
                }
            };
        }

        private synchronized void sendSyncMessage() {
            mHandler.sendMessage(Message.obtain());
        }
    }

    /**
     * Package constructor to prevent clients from creating a new settings
     * instance. Must be called on the UI thread.
     */
    WebSettings(ChromeView webView, int nativeChromeView) {
        mWebView = webView;
        mNativeChromeView = nativeChromeView;
        mNativeWebSettings = nativeInit();
        assert mNativeWebSettings != 0;

        mEventHandler = new EventHandler();
        nativeSync(mNativeWebSettings, nativeChromeView);
    }

    /**
     * Destroys the native side of the WebSettings. This WebSettings object
     * cannot be used after this method has been called. Should only be called
     * when related ChromeView is destroyed.
     */
    void destroy() {
        nativeDestroy(mNativeWebSettings);
        mNativeWebSettings = 0;
        mNativeChromeView = 0;
    }

    /**
     * Set the standard font family name.
     * @param font A font family name.
     */
    public synchronized void setStandardFontFamily(String font) {
        if (!mStandardFontFamily.equals(font)) {
            mStandardFontFamily = font;
            sendSyncMessage();
        }
    }

    /**
     * Get the standard font family name. The default is "sans-serif".
     * @return The standard font family name as a string.
     */
    public synchronized String getStandardFontFamily() {
        return mStandardFontFamily;
    }

    /**
     * Set the fixed font family name.
     * @param font A font family name.
     */
    public synchronized void setFixedFontFamily(String font) {
        if (!mFixedFontFamily.equals(font)) {
            mFixedFontFamily = font;
            sendSyncMessage();
        }
    }

    /**
     * Get the fixed font family name. The default is "monospace".
     * @return The fixed font family name as a string.
     */
    public synchronized String getFixedFontFamily() {
        return mFixedFontFamily;
    }

    /**
     * Set the sans-serif font family name.
     * @param font A font family name.
     */
    public synchronized void setSansSerifFontFamily(String font) {
        if (!mSansSerifFontFamily.equals(font)) {
            mSansSerifFontFamily = font;
            sendSyncMessage();
        }
    }

    /**
     * Get the sans-serif font family name.
     * @return The sans-serif font family name as a string.
     */
    public synchronized String getSansSerifFontFamily() {
        return mSansSerifFontFamily;
    }

    /**
     * Set the serif font family name. The default is "sans-serif".
     * @param font A font family name.
     */
    public synchronized void setSerifFontFamily(String font) {
        if (!mSerifFontFamily.equals(font)) {
            mSerifFontFamily = font;
            sendSyncMessage();
        }
    }

    /**
     * Get the serif font family name. The default is "serif".
     * @return The serif font family name as a string.
     */
    public synchronized String getSerifFontFamily() {
        return mSerifFontFamily;
    }

    /**
     * Set the cursive font family name.
     * @param font A font family name.
     */
    public synchronized void setCursiveFontFamily(String font) {
        if (!mCursiveFontFamily.equals(font)) {
            mCursiveFontFamily = font;
            sendSyncMessage();
        }
    }

    /**
     * Get the cursive font family name. The default is "cursive".
     * @return The cursive font family name as a string.
     */
    public synchronized String getCursiveFontFamily() {
        return mCursiveFontFamily;
    }

    /**
     * Set the fantasy font family name.
     * @param font A font family name.
     */
    public synchronized void setFantasyFontFamily(String font) {
        if (!mFantasyFontFamily.equals(font)) {
            mFantasyFontFamily = font;
            sendSyncMessage();
        }
    }

    /**
     * Get the fantasy font family name. The default is "fantasy".
     * @return The fantasy font family name as a string.
     */
    public synchronized String getFantasyFontFamily() {
        return mFantasyFontFamily;
    }

    /**
     * Set the minimum font size.
     * @param size A non-negative integer between 1 and 72.
     * Any number outside the specified range will be pinned.
     */
    public synchronized void setMinimumFontSize(int size) {
        size = clipFontSize(size);
        if (mMinimumFontSize != size) {
            mMinimumFontSize = size;
            sendSyncMessage();
        }
    }

    /**
     * Get the minimum font size. The default is 8.
     * @return A non-negative integer between 1 and 72.
     */
    public synchronized int getMinimumFontSize() {
        return mMinimumFontSize;
    }

    /**
     * Set the minimum logical font size.
     * @param size A non-negative integer between 1 and 72.
     * Any number outside the specified range will be pinned.
     */
    public synchronized void setMinimumLogicalFontSize(int size) {
        size = clipFontSize(size);
        if (mMinimumLogicalFontSize != size) {
            mMinimumLogicalFontSize = size;
            sendSyncMessage();
        }
    }

    /**
     * Get the minimum logical font size. The default is 8.
     * @return A non-negative integer between 1 and 72.
     */
    public synchronized int getMinimumLogicalFontSize() {
        return mMinimumLogicalFontSize;
    }

    /**
     * Set the default font size.
     * @param size A non-negative integer between 1 and 72.
     * Any number outside the specified range will be pinned.
     */
    public synchronized void setDefaultFontSize(int size) {
        size = clipFontSize(size);
        if (mDefaultFontSize != size) {
            mDefaultFontSize = size;
            sendSyncMessage();
        }
    }

    /**
     * Get the default font size. The default is 16.
     * @return A non-negative integer between 1 and 72.
     */
    public synchronized int getDefaultFontSize() {
        return mDefaultFontSize;
    }

    /**
     * Set the default fixed font size.
     * @param size A non-negative integer between 1 and 72.
     * Any number outside the specified range will be pinned.
     */
    public synchronized void setDefaultFixedFontSize(int size) {
        size = clipFontSize(size);
        if (mDefaultFixedFontSize != size) {
            mDefaultFixedFontSize = size;
            sendSyncMessage();
        }
    }

    /**
     * Get the default fixed font size. The default is 16.
     * @return A non-negative integer between 1 and 72.
     */
    public synchronized int getDefaultFixedFontSize() {
        return mDefaultFixedFontSize;
    }

    /**
     * Tell the WebView to enable JavaScript execution.
     *
     * @param flag True if the WebView should execute JavaScript.
     */
    public synchronized void setJavaScriptEnabled(boolean flag) {
        if (mJavaScriptEnabled != flag) {
            mJavaScriptEnabled = flag;
            sendSyncMessage();
        }
    }

    /**
     * Tell the WebView to load image resources automatically.
     * @param flag True if the WebView should load images automatically.
     */
    public synchronized void setLoadsImagesAutomatically(boolean flag) {
        if (mLoadsImagesAutomatically != flag) {
            mLoadsImagesAutomatically = flag;
            sendSyncMessage();
        }
    }

    /**
     * Return true if the WebView will load image resources automatically.
     * The default is true.
     * @return True if the WebView loads images automatically.
     */
    public synchronized boolean getLoadsImagesAutomatically() {
        return mLoadsImagesAutomatically;
    }

    /**
     * Return true if JavaScript is enabled. <b>Note: The default is false.</b>
     *
     * @return True if JavaScript is enabled.
     */
    public synchronized boolean getJavaScriptEnabled() {
        return mJavaScriptEnabled;
    }

    /**
     * Tell the WebView to enable plugins.
     * @param flag True if the WebView should load plugins.
     * @deprecated This method has been deprecated in favor of
     *             {@link #setPluginState}
     */
    @Deprecated
    public synchronized void setPluginsEnabled(boolean flag) {
        setPluginState(flag ? PluginState.ON : PluginState.OFF);
    }

    /**
     * Tell the WebView to enable, disable, or have plugins on demand. On
     * demand mode means that if a plugin exists that can handle the embedded
     * content, a placeholder icon will be shown instead of the plugin. When
     * the placeholder is clicked, the plugin will be enabled.
     * @param state One of the PluginState values.
     */
    public synchronized void setPluginState(PluginState state) {
        if (mPluginState != state) {
            mPluginState = state;
            sendSyncMessage();
        }
    }

    /**
     * Return true if plugins are enabled.
     * @return True if plugins are enabled.
     * @deprecated This method has been replaced by {@link #getPluginState}
     */
    @Deprecated
    public synchronized boolean getPluginsEnabled() {
        return mPluginState == PluginState.ON;
    }

    /**
     * Return true if plugins are disabled.
     * @return True if plugins are disabled.
     * @hide
     */
    @CalledByNative
    private synchronized boolean getPluginsDisabled() {
        return mPluginState == PluginState.OFF;
    }

    /**
     * Return the current plugin state.
     * @return A value corresponding to the enum PluginState.
     */
    public synchronized PluginState getPluginState() {
        return mPluginState;
    }


    /**
     * Tell javascript to open windows automatically. This applies to the
     * javascript function window.open().
     * @param flag True if javascript can open windows automatically.
     */
    public synchronized void setJavaScriptCanOpenWindowsAutomatically(
            boolean flag) {
        if (mJavaScriptCanOpenWindowsAutomatically != flag) {
            mJavaScriptCanOpenWindowsAutomatically = flag;
            sendSyncMessage();
        }
    }

    /**
     * Return true if javascript can open windows automatically. The default
     * is false.
     * @return True if javascript can open windows automatically during
     *         window.open().
     */
    public synchronized boolean getJavaScriptCanOpenWindowsAutomatically() {
        return mJavaScriptCanOpenWindowsAutomatically;
    }

    /**
     * Set the default text encoding name to use when decoding html pages.
     * @param encoding The text encoding name.
     */
    public synchronized void setDefaultTextEncodingName(String encoding) {
        if (!mDefaultTextEncoding.equals(encoding)) {
            mDefaultTextEncoding = encoding;
            sendSyncMessage();
        }
    }

    /**
     * Get the default text encoding name. The default is "Latin-1".
     * @return The default text encoding name as a string.
     */
    public synchronized String getDefaultTextEncodingName() {
        return mDefaultTextEncoding;
    }

    private int clipFontSize(int size) {
        if (size < MINIMUM_FONT_SIZE) {
            return MINIMUM_FONT_SIZE;
        } else if (size > MAXIMUM_FONT_SIZE) {
            return MAXIMUM_FONT_SIZE;
        }
        return size;
    }

    /* Send a synchronization message to handle syncing the native settings. */
    private synchronized void sendSyncMessage() {
        // Only post if a sync is not pending
        if (!mIsSyncMessagePending) {
            mIsSyncMessagePending = true;
            mEventHandler.sendSyncMessage();
        }
    }

    // Initialize the WebSettings native side.
    private native int nativeInit();

    private native void nativeDestroy(int nativeWebSettings);

    // Synchronize the native and java settings.
    private native void nativeSync(int nativeWebSettings, int nativeChromeView);
}
