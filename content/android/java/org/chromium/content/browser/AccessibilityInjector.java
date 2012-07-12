// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.content.Context;
import android.os.Bundle;
import android.os.SystemClock;
import android.os.Vibrator;
import android.provider.Settings;
import android.speech.tts.TextToSpeech;
import android.view.View;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;

import org.apache.http.NameValuePair;
import org.apache.http.client.utils.URLEncodedUtils;
import org.json.JSONException;
import org.json.JSONObject;

import java.lang.reflect.Field;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Handles injecting accessibility Javascript and related Javascript -> Java APIs.
 */
class AccessibilityInjector {
    // The ChromeView this injector is responsible for managing.
    private ChromeView mChromeView;

    // The Java objects that are exposed to JavaScript
    private TextToSpeech mTextToSpeech;
    private VibratorWrapper mVibrator;
    private CallbackHandler mCallback;

    // Lazily loaded helper objects.
    private AccessibilityManager mAccessibilityManager;
    private JSONObject mAccessibilityJSONObject;

    // Whether or not we should be injecting the script.
    private boolean mInjectedScriptEnabled;
    private boolean mScriptInjected;

    // constants for determining script injection strategy
    private static final int ACCESSIBILITY_SCRIPT_INJECTION_UNDEFINED = -1;
    private static final int ACCESSIBILITY_SCRIPT_INJECTION_OPTED_OUT = 0;
    private static final int ACCESSIBILITY_SCRIPT_INJECTION_PROVIDED = 1;
    private static final String ALIAS_ACCESSIBILITY_JS_INTERFACE = "accessibility";
    private static final String ALIAS_ACCESSIBILITY_JS_INTERFACE_2 = "accessibility2";
    private static final String ALIAS_TRAVERSAL_JS_INTERFACE = "accessibilityTraversal";

    // Template for JavaScript that injects a screen-reader.
    private static final String ACCESSIBILITY_SCREEN_READER_URL =
            "https://ssl.gstatic.com/accessibility/javascript/android/clankvox.js";

    private static final String ACCESSIBILITY_SCREEN_READER_JAVASCRIPT_TEMPLATE =
            "(function() {" +
            "    var chooser = document.createElement('script');" +
            "    chooser.type = 'text/javascript';" +
            "    chooser.src = '%1s';" +
            "    document.getElementsByTagName('head')[0].appendChild(chooser);" +
            "  })();";

    // JavaScript call to turn ChromeVox on or off.
    private static final String TOGGLE_CHROME_VOX_JAVASCRIPT =
            "(function() {" +
            "    if (typeof cvox !== 'undefined') {" +
            "        cvox.ChromeVox.host.activateOrDeactivateChromeVox(%1s);" +
            "    }" +
            "  })();";

    // Template for JavaScript that performs AndroidVox actions.
    private static final String ACCESSIBILITY_ANDROIDVOX_TEMPLATE =
            "cvox.AndroidVox.performAction('%1s')";

    /**
     * Creates an instance of the AccessibilityInjector based on {@code chromeView}.
     * @param chromeView The ChromeView that this AccessibilityInjector manages.
     */
    public AccessibilityInjector(ChromeView chromeView) {
        mChromeView = chromeView;
    }

    /**
     * Injects a <script> tag into the current web site that pulls in the ChromeVox script for
     * accessibility support.  Only injects if accessibility is turned on by
     * {@link AccessibilityManager#isEnabled()}, accessibility script injection is turned on, and
     * javascript is enabled on this page.
     *
     * @see AccessibilityManager#isEnabled()
     */
    public void injectAccessibilityScriptIntoPage() {
        if (!accessibilityIsAvailable()) return;

        int axsParameterValue = getAxsUrlParameterValue();
        if (axsParameterValue == ACCESSIBILITY_SCRIPT_INJECTION_UNDEFINED) {
            try {
                Field field = Settings.Secure.class.getField("ACCESSIBILITY_SCRIPT_INJECTION");
                field.setAccessible(true);
                String ACCESSIBILITY_SCRIPT_INJECTION = (String) field.get(null);

                boolean onDeviceScriptInjectionEnabled = (Settings.Secure.getInt(
                        mChromeView.getContext().getContentResolver(),
                        ACCESSIBILITY_SCRIPT_INJECTION, 0) == 1);
                String js = getScreenReaderInjectingJs();

                if (onDeviceScriptInjectionEnabled && js != null && mChromeView.isAlive()) {
                    addOrRemoveAccessibilityApisIfNecessary();
                    mChromeView.evaluateJavaScript(js);
                    mInjectedScriptEnabled = true;
                    mScriptInjected = true;
                }
            } catch (NoSuchFieldException ex) {
            } catch (IllegalArgumentException ex) {
            } catch (IllegalAccessException ex) {
            }
        }
    }

    /**
     * Handles adding or removing accessibility related Java objects ({@link TextToSpeech} and
     * {@link Vibrator}) interfaces from Javascript.  This method should be called at a time when it
     * is safe to add or remove these interfaces, specifically when the {@link ChromeView} is first
     * initialized or right before the {@link ChromeView} is about to navigate to a URL or reload.
     * <p>
     * If this method is called at other times, the interfaces might not be correctly removed,
     * meaning that Javascript can still access these Java objects that may have been already
     * shut down.
     */
    public void addOrRemoveAccessibilityApisIfNecessary() {
        if (accessibilityIsAvailable()) {
            Context context = mChromeView.getContext();
            if (context != null) {
                // Enabled, we should try to add if we have to.
                if (mTextToSpeech == null) {
                    mTextToSpeech = new TextToSpeech(context, null, null);
                    mChromeView.addJavascriptInterface(mTextToSpeech,
                            ALIAS_ACCESSIBILITY_JS_INTERFACE);
                }

                if (mVibrator == null) {
                    mVibrator = new VibratorWrapper(context);
                    mChromeView.addJavascriptInterface(mVibrator,
                            ALIAS_ACCESSIBILITY_JS_INTERFACE_2);
                }

                if (mCallback == null) {
                    mCallback = new CallbackHandler(ALIAS_TRAVERSAL_JS_INTERFACE);
                    mChromeView.addJavascriptInterface(mCallback, ALIAS_TRAVERSAL_JS_INTERFACE);
                }
            }
        } else {
            // Disabled, we should try to remove if we have to.
            if (mTextToSpeech != null) {
                mChromeView.removeJavascriptInterface(ALIAS_ACCESSIBILITY_JS_INTERFACE);
                mTextToSpeech.stop();
                mTextToSpeech.shutdown();
                mTextToSpeech = null;
            }

            if (mVibrator != null) {
                mChromeView.removeJavascriptInterface(ALIAS_ACCESSIBILITY_JS_INTERFACE_2);
                mVibrator.cancel();
                mVibrator = null;
            }

            if (mCallback != null) {
                mChromeView.removeJavascriptInterface(ALIAS_TRAVERSAL_JS_INTERFACE);
                mCallback = null;
            }
        }
    }

    /**
     * Performs the specified accessibility action.
     *
     * @param action The identifier of the action to perform.
     * @param arguments The action arguments, or {@code null} if no arguments.
     * @return {@code true} if the action was successful.
     * @see View#performAccessibilityAction(int, Bundle)
     */
    public boolean performAccessibilityAction(int action, Bundle arguments) {
        if (!accessibilityIsAvailable() || !mChromeView.isAlive() ||
                !mInjectedScriptEnabled || !mScriptInjected) {
            return false;
        }

        return sendActionToAndroidVox(action, arguments);
    }

    /**
     * Notifies this handler that a page load has started, which means we should mark the
     * accessibility script as not being injected.  This way we can properly ignore incoming
     * accessibility gesture events.
     */
    public void onPageLoadStarted() {
        mScriptInjected = false;
    }

    /**
     * Initializes an {@link AccessibilityNodeAction} with the actions and movement granularity
     * levels supported by this {@link AccessibilityInjector}.
     * <p>
     * If an action identifier is added in this method, this {@link AccessibilityInjector} should
     * also return {@code true} from {@link #supportsAccessibilityAction(int)}.
     * </p>
     *
     * @param info The info to initialize.
     * @see View#onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo)
     */
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        if (!AccessibilityInjectorAPIAdapter.init()) return;
        AccessibilityInjectorAPIAdapter.AccessibilityNodeInfo_setMovementGranularities(info,
                AccessibilityInjectorAPIAdapter.MOVEMENT_GRANULARITY_CHARACTER |
                AccessibilityInjectorAPIAdapter.MOVEMENT_GRANULARITY_WORD |
                AccessibilityInjectorAPIAdapter.MOVEMENT_GRANULARITY_LINE |
                AccessibilityInjectorAPIAdapter.MOVEMENT_GRANULARITY_PARAGRAPH |
                AccessibilityInjectorAPIAdapter.MOVEMENT_GRANULARITY_PAGE);
        info.addAction(AccessibilityInjectorAPIAdapter.ACTION_NEXT_AT_MOVEMENT_GRANULARITY);
        info.addAction(AccessibilityInjectorAPIAdapter.ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY);
        info.addAction(AccessibilityInjectorAPIAdapter.ACTION_NEXT_HTML_ELEMENT);
        info.addAction(AccessibilityInjectorAPIAdapter.ACTION_PREVIOUS_HTML_ELEMENT);
        info.addAction(AccessibilityInjectorAPIAdapter.ACTION_CLICK);
        info.setClickable(true);
    }

    /**
     * Returns {@code true} if this {@link AccessibilityInjector} should handle the specified
     * action.
     *
     * @param action An accessibility action identifier.
     * @return {@code true} if this {@link AccessibilityInjector} should handle the specified
     *         action.
     */
    public boolean supportsAccessibilityAction(int action) {
        if (!AccessibilityInjectorAPIAdapter.init()) return false;
        if (action == AccessibilityInjectorAPIAdapter.ACTION_NEXT_AT_MOVEMENT_GRANULARITY ||
                action == AccessibilityInjectorAPIAdapter.ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY ||
                action == AccessibilityInjectorAPIAdapter.ACTION_NEXT_HTML_ELEMENT ||
                action == AccessibilityInjectorAPIAdapter.ACTION_PREVIOUS_HTML_ELEMENT ||
                action == AccessibilityInjectorAPIAdapter.ACTION_CLICK) {
            return true;
        }

        return false;
    }

    /**
     * Stop any notifications that are currently going on (e.g. Text-to-Speech).
     */
    public void stopCurrentNotifications() {
        if (mChromeView.isAlive()) {
            if (mTextToSpeech != null) mTextToSpeech.stop();
            if (mVibrator != null) mVibrator.cancel();
        }
    }

    /**
     * Checks whether or not touch to explore is enabled on the system.
     */
    public boolean accessibilityIsAvailable() {
        return getAccessibilityManager().isEnabled() && mChromeView.getJavaScriptEnabled();
    }

    /**
     * Sets whether or not the script is enabled.  If the script is disabled, we also stop any
     * we output that is occurring.
     * @param enabled Whether or not to enable the script.
     */
    public void setScriptEnabled(boolean enabled) {
        if (!accessibilityIsAvailable() || mInjectedScriptEnabled == enabled) return;

        mInjectedScriptEnabled = enabled;
        if (mChromeView.isAlive()) {
            String js = String.format(TOGGLE_CHROME_VOX_JAVASCRIPT, Boolean.toString(
                    mInjectedScriptEnabled));
            mChromeView.evaluateJavaScript(js);

            if (!mInjectedScriptEnabled) {
                // Stop any TTS/Vibration right now.
                stopCurrentNotifications();
            }
        }
    }

    private int getAxsUrlParameterValue() {
        if (mChromeView.getUrl() == null) return ACCESSIBILITY_SCRIPT_INJECTION_UNDEFINED;

        try {
            List<NameValuePair> params = URLEncodedUtils.parse(new URI(mChromeView.getUrl()),
                    null);

            for (NameValuePair param : params) {
                if ("axs".equals(param.getName())) {
                    return Integer.parseInt(param.getValue());
                }
            }
        } catch (URISyntaxException ex) {
        } catch (NumberFormatException ex) {
        } catch (IllegalArgumentException ex) {
        }

        return ACCESSIBILITY_SCRIPT_INJECTION_UNDEFINED;
    }

    private String getScreenReaderInjectingJs() {
        return String.format(ACCESSIBILITY_SCREEN_READER_JAVASCRIPT_TEMPLATE,
                ACCESSIBILITY_SCREEN_READER_URL);
    }

    private AccessibilityManager getAccessibilityManager() {
        if (mAccessibilityManager == null) {
            mAccessibilityManager = (AccessibilityManager) mChromeView.getContext().
                    getSystemService(Context.ACCESSIBILITY_SERVICE);
        }

        return mAccessibilityManager;
    }

    /**
     * Packs an accessibility action into a JSON object and sends it to AndroidVox.
     *
     * @param action The action identifier.
     * @param arguments The action arguments, if applicable.
     * @return The result of the action.
     */
    private boolean sendActionToAndroidVox(int action, Bundle arguments) {
        if (!AccessibilityInjectorAPIAdapter.init()) return false;
        if (mCallback == null) return false;
        if (mAccessibilityJSONObject == null) {
            mAccessibilityJSONObject = new JSONObject();
        } else {
            // Remove all keys from the object.
            final Iterator<?> keys = mAccessibilityJSONObject.keys();
            while (keys.hasNext()) {
                keys.next();
                keys.remove();
            }
        }

        try {
            mAccessibilityJSONObject.accumulate("action", action);
            if (arguments != null) {
                if (action == AccessibilityInjectorAPIAdapter.ACTION_NEXT_AT_MOVEMENT_GRANULARITY ||
                        action == AccessibilityInjectorAPIAdapter.
                        ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY) {
                    final int granularity = arguments.getInt(AccessibilityInjectorAPIAdapter.
                            ACTION_ARGUMENT_MOVEMENT_GRANULARITY_INT);
                    mAccessibilityJSONObject.accumulate("granularity", granularity);
                } else if (action == AccessibilityInjectorAPIAdapter.ACTION_NEXT_HTML_ELEMENT ||
                        action == AccessibilityInjectorAPIAdapter.ACTION_PREVIOUS_HTML_ELEMENT) {
                    final String element = arguments.getString(
                            AccessibilityInjectorAPIAdapter.ACTION_ARGUMENT_HTML_ELEMENT_STRING);
                    mAccessibilityJSONObject.accumulate("element", element);
                }
            }
        } catch (JSONException ex) {
            return false;
        }

        final String jsonString = mAccessibilityJSONObject.toString();
        final String jsCode = String.format(ACCESSIBILITY_ANDROIDVOX_TEMPLATE, jsonString);
        return mCallback.performAction(mChromeView, jsCode);
    }

    /**
     * Used to protect how long JavaScript can vibrate for.  This isn't a good comprehensive
     * protection, just used to cover mistakes and protect against long vibrate durations/repeats.
     */
    private static class VibratorWrapper {
        private static final long MAX_VIBRATE_DURATION_MS = 5000;

        private Vibrator mVibrator;

        public VibratorWrapper(Context context) {
            mVibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        }

        @SuppressWarnings("unused")
        public boolean hasVibrator() {
            return mVibrator.hasVibrator();
        }

        @SuppressWarnings("unused")
        public void vibrate(long milliseconds) {
            milliseconds = Math.min(milliseconds, MAX_VIBRATE_DURATION_MS);
            mVibrator.vibrate(milliseconds);
        }

        @SuppressWarnings("unused")
        public void vibrate(long[] pattern, int repeat) {
            for (int i = 0; i < pattern.length; ++i) {
                pattern[i] = Math.min(pattern[i], MAX_VIBRATE_DURATION_MS);
            }

            repeat = -1;

            mVibrator.vibrate(pattern, repeat);
        }

        @SuppressWarnings("unused")
        public void cancel() {
            mVibrator.cancel();
        }
    }

    private static class CallbackHandler {
        private static final String JAVASCRIPT_ACTION_TEMPLATE =
                "(function() { %s.onResult(%d, %s); })()";

        // Time in milliseconds to wait for a result before failing.
        private static final long RESULT_TIMEOUT = 5000;

        private final AtomicInteger mResultIdCounter = new AtomicInteger();
        private final Object mResultLock = new Object();
        private final String mInterfaceName;

        private boolean mResult = false;
        private long mResultId = -1;

        private CallbackHandler(String interfaceName) {
            mInterfaceName = interfaceName;
        }

        /**
         * Performs an action and attempts to wait for a result.
         *
         * @param chromeView The ChromeView to perform the action on.
         * @param code Javascript code that evaluates to a result.
         * @return The result of the action.
         */
        private boolean performAction(ChromeView chromeView, String code) {
            final int resultId = mResultIdCounter.getAndIncrement();
            final String js = String.format(JAVASCRIPT_ACTION_TEMPLATE, mInterfaceName, resultId,
                    code);
            chromeView.evaluateJavaScript(js);

            return getResultAndClear(resultId);
        }

        /**
         * Gets the result of a request to perform an accessibility action.
         *
         * @param resultId The result id to match the result with the request.
         * @return The result of the request.
         */
        private boolean getResultAndClear(int resultId) {
            synchronized (mResultLock) {
                final boolean success = waitForResultTimedLocked(resultId);
                final boolean result = success ? mResult : false;
                clearResultLocked();
                return result;
            }
        }

        /**
         * Clears the result state.
         */
        private void clearResultLocked() {
            mResultId = -1;
            mResult = false;
        }

        /**
         * Waits up to a given bound for a result of a request and returns it.
         *
         * @param resultId The result id to match the result with the request.
         * @return Whether the result was received.
         */
        private boolean waitForResultTimedLocked(int resultId) {
            long waitTimeMillis = RESULT_TIMEOUT;
            final long startTimeMillis = SystemClock.uptimeMillis();
            while (true) {
                try {
                    if (mResultId == resultId) return true;
                    if (mResultId > resultId) return false;
                    final long elapsedTimeMillis = SystemClock.uptimeMillis() - startTimeMillis;
                    waitTimeMillis = RESULT_TIMEOUT - elapsedTimeMillis;
                    if (waitTimeMillis <= 0) return false;
                    mResultLock.wait(waitTimeMillis);
                } catch (InterruptedException ie) {
                    /* ignore */
                }
            }
        }

        /**
         * Callback exposed to JavaScript.  Handles returning the result of a
         * request to a waiting (or potentially timed out) thread.
         *
         * @param id The result id of the request as a {@link String}.
         * @param result The result o fa request as a {@link String}.
         */
        @SuppressWarnings("unused")
        public void onResult(String id, String result) {
            final long resultId;

            try {
                resultId = Long.parseLong(id);
            } catch (NumberFormatException e) {
                return;
            }

            synchronized (mResultLock) {
                if (resultId > mResultId) {
                    mResult = Boolean.parseBoolean(result);
                    mResultId = resultId;
                }
                mResultLock.notifyAll();
            }
        }
    }
}
