// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.accessibility.AccessibilityNodeInfo;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Manages the AccessibilityNodeInfo interface differences between JB and ICS.  Properly finds the
 * JB interfaces using reflection.  Most of these functions are NOOPs and variables aren't useful
 * if {@link AccessibilityInjectorAPIAdapter#init()} fails.
 */
class AccessibilityInjectorAPIAdapter {
    private static final int MIN_SDK_API_VERSION_SUPPORTED = 16;

    private static AtomicBoolean sApisSupported;
    private static Method sView_performAccessibilityAction;
    private static Method sAccessibilityNodeInfo_setMovementGranularities;

    public static int MOVEMENT_GRANULARITY_CHARACTER = 0;
    public static int MOVEMENT_GRANULARITY_WORD = 0;
    public static int MOVEMENT_GRANULARITY_LINE = 0;
    public static int MOVEMENT_GRANULARITY_PARAGRAPH = 0;
    public static int MOVEMENT_GRANULARITY_PAGE = 0;
    public static int ACTION_NEXT_AT_MOVEMENT_GRANULARITY = 0;
    public static int ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY = 0;
    public static int ACTION_NEXT_HTML_ELEMENT = 0;
    public static int ACTION_PREVIOUS_HTML_ELEMENT = 0;
    public static int ACTION_CLICK = 0;
    public static String ACTION_ARGUMENT_MOVEMENT_GRANULARITY_INT = "";
    public static String ACTION_ARGUMENT_HTML_ELEMENT_STRING = "";

    /**
     * Initializes the {@link AccessibilityInjectorAPIAdapter} class by attempting to find all of
     * the required methods and member variables required for use.  This method can be called
     * multiple times, it will only initialize once, but will always return whether or not these
     * methods and member variables should be used.
     *
     * @return Whether or not finding the JB APIs was successful.
     */
    public static boolean init() {
        if (sApisSupported != null) return sApisSupported.get();

        sApisSupported = new AtomicBoolean(false);

        if (Build.VERSION.SDK_INT < MIN_SDK_API_VERSION_SUPPORTED) return sApisSupported.get();

        // We need to call a protected internal method added in JB.  Since this is not part of the
        // SDK, we allow this to fail and not mark the api as unsupported, but we will not support
        // all of the gesture types on JB if this method is moved by an android manufacturer.
        try {
            sView_performAccessibilityAction = View.class.getDeclaredMethod(
                    "performAccessibilityActionInternal", int.class, Bundle.class);
            sView_performAccessibilityAction.setAccessible(true);
        } catch (NoSuchMethodException e) {
            /* Ignore. */
        } catch (SecurityException e) {
            /* Ignore. */
        } catch (IllegalArgumentException e) {
            /* Ignore. */
        }

        try {
            sAccessibilityNodeInfo_setMovementGranularities = AccessibilityNodeInfo.class.
                    getDeclaredMethod("setMovementGranularities", int.class);

            MOVEMENT_GRANULARITY_CHARACTER = AccessibilityNodeInfo.class.
                    getDeclaredField("MOVEMENT_GRANULARITY_CHARACTER").getInt(null);

            MOVEMENT_GRANULARITY_WORD = AccessibilityNodeInfo.class.
                    getDeclaredField("MOVEMENT_GRANULARITY_WORD").getInt(null);

            MOVEMENT_GRANULARITY_LINE = AccessibilityNodeInfo.class.
                    getDeclaredField("MOVEMENT_GRANULARITY_LINE").getInt(null);

            MOVEMENT_GRANULARITY_PARAGRAPH = AccessibilityNodeInfo.class.
                    getDeclaredField("MOVEMENT_GRANULARITY_PARAGRAPH").getInt(null);

            MOVEMENT_GRANULARITY_PAGE = AccessibilityNodeInfo.class.
                    getDeclaredField("MOVEMENT_GRANULARITY_PAGE").getInt(null);

            ACTION_NEXT_AT_MOVEMENT_GRANULARITY = AccessibilityNodeInfo.class.getDeclaredField(
                    "ACTION_NEXT_AT_MOVEMENT_GRANULARITY").getInt(null);

            ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY = AccessibilityNodeInfo.class.getDeclaredField(
                    "ACTION_PREVIOUS_AT_MOVEMENT_GRANULARITY").getInt(null);

            ACTION_NEXT_HTML_ELEMENT = AccessibilityNodeInfo.class.getDeclaredField(
                    "ACTION_NEXT_HTML_ELEMENT").getInt(null);

            ACTION_PREVIOUS_HTML_ELEMENT = AccessibilityNodeInfo.class.getDeclaredField(
                    "ACTION_PREVIOUS_HTML_ELEMENT").getInt(null);

            ACTION_CLICK = AccessibilityNodeInfo.class.getDeclaredField(
                    "ACTION_CLICK").getInt(null);

            ACTION_ARGUMENT_MOVEMENT_GRANULARITY_INT =
                    (String) AccessibilityNodeInfo.class.getDeclaredField(
                            "ACTION_ARGUMENT_MOVEMENT_GRANULARITY_INT").get(null);

            ACTION_ARGUMENT_HTML_ELEMENT_STRING = (String) AccessibilityNodeInfo.class.
                    getDeclaredField("ACTION_ARGUMENT_HTML_ELEMENT_STRING").get(null);

            sApisSupported.set(true);
        } catch (NoSuchMethodException e) {
            /* Ignore. */
        } catch (SecurityException e) {
            /* Ignore. */
        } catch (IllegalArgumentException e) {
            /* Ignore. */
        } catch (IllegalAccessException e) {
            /* Ignore. */
        } catch (NoSuchFieldException e) {
            /* Ignore. */
        }

        return sApisSupported.get();
    }

    /**
     * Calls {@link View#performAccessibilityAction} on {@code clazz}.  This only happens if a call
     * to {@link #init()} is successful.
     *
     * @param clazz  The {@link View} to call {@link View#performAccessibilityAction} on.
     * @param action The action to perform.
     * @param bundle The {@link Bundle} with relevant action data.
     * @return       Whether or not the action was successfully performed.
     * @see          View#performAccessibilityAction
     */
    public static boolean View_performAccessibilityAction(View clazz,
            int action, Bundle bundle) {
        if (!init()) return false;

        if (sView_performAccessibilityAction == null) return false;

        try {
            return (Boolean) sView_performAccessibilityAction.invoke(clazz, action, bundle);
        } catch (IllegalAccessException ex) {
            /* Ignore. */
        } catch (IllegalArgumentException e) {
            /* Ignore. */
        } catch (InvocationTargetException e) {
            /* Ignore. */
        }

        return false;
    }

    /**
     * Calls {@link AccessibilityNodeInfo#setMovementGranularities} on {@code clazz}.  This only
     * happens if a call to {@link #init()} is successful.
     *
     * @param clazz         The {@link AccessibilityNodeInfo} to call
     *                      {@link AccessibilityNodeInfo#setMovementGranularities} on.
     * @param granularities The granularity of movements that should be supported.
     * @see                 AccessibilityNodeInfo#setMovementGranularities
     */
    public static void AccessibilityNodeInfo_setMovementGranularities(AccessibilityNodeInfo clazz,
            int granularities) {
        if (!init()) return;

        try {
            sAccessibilityNodeInfo_setMovementGranularities.invoke(clazz, granularities);
        } catch (IllegalAccessException ex) {
            /* Ignore. */
        } catch (IllegalArgumentException e) {
            /* Ignore. */
        } catch (InvocationTargetException e) {
            /* Ignore. */
        }
    }
}