/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.android.apps.chrome.bridge;

import android.os.Bundle;

import com.google.android.apps.chrome.CalledByNativeUnchecked;
import com.google.android.apps.chrome.ChromeNotificationCenter;
import com.google.android.apps.chrome.OmniboxSuggestion;
import com.google.android.apps.chrome.suggestionproviders.VoiceSuggestionProvider;
import com.google.android.apps.chrome.suggestionproviders.VoiceSuggestionProvider.VoiceResult;
import com.google.common.annotations.VisibleForTesting;

import org.chromium.content.browser.ChromeView;

import java.util.Arrays;
import java.util.List;

/**
 * Bridge to the native AutocompleteController
 */
public class AutocompleteController {
    private static final int MAX_DEFAULT_SUGGESTION_COUNT = 5;
    private static final int MAX_VOICE_SUGGESTION_COUNT = 3;

    private int mNativeAutocompleteBridge; // Used by native code
    private OnSuggestionsReceivedListener mListener;

    private VoiceSuggestionProvider mVoiceSuggestionProvider = new VoiceSuggestionProvider();

    public static interface OnSuggestionsReceivedListener {
        void onSuggestionsReceived(OmniboxSuggestion[] suggestions,
                String inlineAutocompleteText);
    }

    public AutocompleteController(OnSuggestionsReceivedListener listener) {
        this(null, listener);
    }

    public AutocompleteController(ChromeView chromeView,
            OnSuggestionsReceivedListener listener) {
        mNativeAutocompleteBridge = nativeInit(chromeView);
        if (mNativeAutocompleteBridge == 0) {
            throw new IllegalStateException("nativeInit() failed!");
        }
        mListener = listener;
    }

    public void destroy() {
        if (mNativeAutocompleteBridge != 0) {
            nativeDestroy(mNativeAutocompleteBridge);
            mNativeAutocompleteBridge = 0;
        }
    }

    /**
     * Resets the underlying auto complete controller based on the profile specified in
     * this chrome view.
     *
     * <p>This will implicitly stop the auto complete suggestions, so
     * {@link #start(String, boolean)} must be called again to start them flowing again.  This
     * should not be an issue as changing profiles should not normally occur while waiting on
     * omnibox suggestions.
     *
     * @param chromeView The chrome view containing the reference to the profile.
     */
    public void setProfile(ChromeView chromeView) {
        nativeChromeViewChanged(mNativeAutocompleteBridge, chromeView);
    }

    public void start(String text, boolean preventInlineAutocomplete) {
        nativeStart(mNativeAutocompleteBridge, text, null, preventInlineAutocomplete,
                false, false, false);
    }

    /**
     * Stops generating autocomplete suggestions for the currently specified text from
     * {@link #start(String, boolean)}.
     *
     * <p>
     * Calling this method with {@code false}, will result in
     * {@link #onSuggestionsReceived(OmniboxSuggestion[], String)} being called with an empty
     * result set.
     *
     * @param clear Whether to clear the most recent autocomplete results.
     */
    public void stop(boolean clear) {
        if (clear) mVoiceSuggestionProvider.clearVoiceSearchResults();

        nativeStop(mNativeAutocompleteBridge, clear);
    }

    // TODO(bulach): this should have either a throw clause, or
    // handle the exception in the java side rather than the native side.
    @CalledByNativeUnchecked
    public void onSuggestionsReceived(OmniboxSuggestion[] suggestions,
            String inlineAutocompleteText) {
        if (suggestions.length > MAX_DEFAULT_SUGGESTION_COUNT) {
            // Trim to the default amount of normal suggestions we can have.
            suggestions = Arrays.copyOf(suggestions, MAX_DEFAULT_SUGGESTION_COUNT);
        }

        // Run through new providers to get an updated list of suggestions.
        suggestions = mVoiceSuggestionProvider.addVoiceSuggestions(
                suggestions, MAX_VOICE_SUGGESTION_COUNT);

        // Notify callbacks of suggestions.
        mListener.onSuggestionsReceived(suggestions, inlineAutocompleteText);
        ChromeNotificationCenter.broadcastNotification(
                ChromeNotificationCenter.AUTOCOMPLETE_SUGGESTIONS_RECEIVED, null);
    }

    /**
     * Pass the auto complete controller a {@link Bundle} representing the results of a voice
     * recognition.
     * @param data A {@link Bundle} containing the results of a voice recognition.
     * @return The top voice match if one exists, {@code null} otherwise.
     */
    public VoiceResult onVoiceResults(Bundle data) {
        mVoiceSuggestionProvider.setVoiceResultsFromIntentBundle(data);
        List<VoiceResult> results = mVoiceSuggestionProvider.getResults();
        return (results != null && results.size() > 0) ? results.get(0) : null;
    }

    @Override
    protected void finalize() {
        destroy();
    }

    @VisibleForTesting
    protected native int nativeInit(ChromeView chromeView);
    private native void nativeDestroy(int nativeAutocompleteBridge);
    private native void nativeChromeViewChanged(int nativeAutocompleteBridge,
            ChromeView chromeView);
    private native void nativeStart(int nativeAutocompleteBridge, String text, String desired_tld,
            boolean prevent_inline_autocomplete, boolean prefer_keyword,
            boolean allow_exact_keyword_match, boolean synchronous_only);
    private native void nativeStop(int nativeAutocompleteBridge, boolean clear_results);

    /**
     * Given a search query, this will attempt to see if the query appears to be portion of a
     * properly formed URL.  If it appears to be a URL, this will return the fully qualified
     * version (i.e. including the scheme, etc...).  If the query does not appear to be a URL,
     * this will return null.
     *
     * @param query The query to be expanded into a fully qualified URL if appropriate.
     * @return The fully qualified URL or null.
     */
    public static native String nativeQualifyPartialURLQuery(String query);
}
