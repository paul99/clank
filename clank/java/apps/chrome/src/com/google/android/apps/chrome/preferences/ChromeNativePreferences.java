// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome.preferences;

import org.chromium.base.AccessedByNative;
import org.chromium.base.CalledByNative;

import org.chromium.content.browser.CommandLine;
import org.chromium.content.browser.RemoteDebuggingController;
import org.chromium.content.browser.ThreadUtils;

import android.util.Log;

import java.util.ArrayList;
import java.util.HashMap;

/**
 * ChromeNativePreferences manages the native preferences. ChromeNativePreferences is a
 * singleton class. The fields of the preferences are initialized based on its native counterpart.
 * The caller can modify each preference individually or batch them together and then synchronize
 * them with the native using {@link ChromeNativePreferences#update}.
 */
public final class ChromeNativePreferences {

    // Keys for autofill content dictionaries.  Other than GUID, these
    // MUST match static strings defined in chrome_native_preferences.cc
    // (e.g. kAutofillName).
    public static final String AUTOFILL_GUID = "guid";
    public static final String AUTOFILL_NAME = "name";
    public static final String AUTOFILL_COMPANY = "company";
    public static final String AUTOFILL_ADDR1 = "addr1";
    public static final String AUTOFILL_ADDR2 = "addr2";
    public static final String AUTOFILL_CITY = "city";
    public static final String AUTOFILL_STATE = "state";
    public static final String AUTOFILL_ZIP = "zip";
    public static final String AUTOFILL_COUNTRY = "country";
    public static final String AUTOFILL_PHONE = "phone";
    public static final String AUTOFILL_EMAIL = "email";
    public static final String AUTOFILL_PREVIEW = "preview";
    public static final String AUTOFILL_OBFUSCATED = "obfuscated";
    public static final String AUTOFILL_NUMBER = "number";
    public static final String AUTOFILL_MONTH = "month";
    public static final String AUTOFILL_YEAR = "year";

    // Keys for name/password dictionaries.
    // MUST match static strings defined in chrome_native_preferences.cc
    public static final String PASSWORD_LIST_URL = "url";
    public static final String PASSWORD_LIST_NAME = "name";
    public static final String PASSWORD_LIST_PASSWORD = "password";

    // Does not need sync with native; used to pass the password id
    // into a new activity.
    public static final String PASSWORD_LIST_ID = "id";

    // Keys for popup exception dictionaries
    // MUST match static strings defined in chrome_native_preferences.cc
    public static final String EXCEPTION_DISPLAY_PATTERN = "displayPattern";
    public static final String EXCEPTION_SETTING = "setting";

    // Keys for localized search engines.
    // MUST match static strings defined in chrome_native_preferences.cc
    public static final String SEARCH_ENGINE_ID = "searchEngineId";
    public static final String SEARCH_ENGINE_SHORT_NAME = "searchEngineShortName";
    public static final String SEARCH_ENGINE_KEYWORD = "searchEngineKeyword";
    public static final String SEARCH_ENGINE_URL = "searchEngineUrl";

    // Does not need sync with native; used for the popup settings check
    public static final String EXCEPTION_SETTING_ALLOW = "allow";
    public static final String EXCEPTION_SETTING_BLOCK = "block";
    public static final String EXCEPTION_SETTING_DEFAULT = "default";
    private static ChromeNativePreferences sPrefs;

    /** Native AutofillPersonalDataManagerObserver pointer which will be set by
     * nativeInitAutofillPersonalDataManagerObserver(). */
    private int mNativeAutofillPersonalDataManagerObserver = 0;

    /** Native TemplateURLServiceLoadedObserver pointer which will be set by
     * nativeInitTemplateURLServiceLoadedObserver(). */
    private int mNativeTemplateURLServiceLoadedObserver = 0;

    @AccessedByNative
    private int mSearchEngine;
    @AccessedByNative
    private boolean mAcceptCookiesEnabled;
    @AccessedByNative
    private boolean mRememberPasswordsEnabled;
    @AccessedByNative
    private boolean mAutofillEnabled;
    @AccessedByNative
    private boolean mAllowLocationEnabled;
    @AccessedByNative
    private boolean mJavaScriptEnabled;
    @AccessedByNative
    private String mSyncAccountName;
    @AccessedByNative
    private boolean mSyncSuppressStart;
    @AccessedByNative
    private boolean mSyncSetupCompleted;
    @AccessedByNative
    private boolean mSyncEverything;
    @AccessedByNative
    private boolean mSyncBookmarks;
    @AccessedByNative
    private boolean mSyncTypedUrls;
    @AccessedByNative
    private boolean mSyncSessions;
    @AccessedByNative
    private float mFontScaleFactor;
    @AccessedByNative
    private boolean mForceEnableZoom;
    @AccessedByNative
    private int mMinimumFontSize;
    @AccessedByNative
    private boolean mRemoteDebuggingEnabled;
    @AccessedByNative
    private boolean mAllowPopupsEnabled;
    // Initialized by native code
    @AccessedByNative
    private boolean mAutologinEnabled;
    @AccessedByNative
    private boolean mSearchSuggestEnabled;
    @AccessedByNative
    private boolean mNetworkPredictionEnabled;
    @AccessedByNative
    private boolean mResolveNavigationErrorEnabled;

    @AccessedByNative
    private String mUserAgent;
    private static String sExecutablePathValue;
    private static String sProfilePathValue;

    private static String sDefaultCountryCodeAtInstall = "";

    // Object to notify when "clear browsing data" completes.
    private OnClearBrowsingDataListener mClearBrowsingDataListener;
    // Object to notify when "autofill data update" completes.
    private onAutoFillDataUpdatedListener mAutoFillDataUpdatedListener;
    //Object to notify templateURLService is done loading.
    private ArrayList<TemplateURLServiceLoadedListener> mTemplateURLServiceLoadedListeners;

    private static final String LOG_TAG = "ChromeNativePreferences";

    private ChromeNativePreferences() {
        update();
    }

    public static synchronized ChromeNativePreferences getInstance() {
        if (sPrefs == null) {
            sPrefs = new ChromeNativePreferences();
        }
        return sPrefs;
    }

    /**
     * Get the default country code set during install.
     * @return country code.
     */
    public static String getDefaultCountryCodeAtInstall() {
        return sDefaultCountryCodeAtInstall;
    }

    /**
     * Set the default country code during install.
     * @param country code
     */
    public static void setDefaultCountryCodeAtInstall(String countryCode) {
        if (CommandLine.getInstance().hasSwitch(CommandLine.DEFAULT_COUNTRY_CODE_AT_INSTALL)) {
            sDefaultCountryCodeAtInstall = CommandLine.getInstance().getSwitchValue(
                    CommandLine.DEFAULT_COUNTRY_CODE_AT_INSTALL);
        } else {
            sDefaultCountryCodeAtInstall = countryCode;
        }
    }

    /**
     * Update Java version of the ChromeNativePreferences based on its native counterpart.
     * Must be called on UI Thread.
     */
    public void update() {
        ThreadUtils.assertOnUiThread();
        if (nativeGetCountryCodeAtInstall().isEmpty()) {
            nativeSetCountryCodeAtInstall(sDefaultCountryCodeAtInstall);
        }
        nativeGet();
        RemoteDebuggingController.getInstance().updateUserPreferenceState(mRemoteDebuggingEnabled);
    }

    /**
     * Return the About Chrome value for executable path.
     */
    public String getExecutablePathValue() {
        return sExecutablePathValue;
    }

    /**
     * Return the About Chrome value for profile path.
     */
    public String getProfilePathValue() {
        return sProfilePathValue;
    }

    /**
     * Set the About Chrome value for executable path.
     */
    @CalledByNative
    public static void setExecutablePathValue(String pathValue) {
        sExecutablePathValue = pathValue;
    }

    /**
     * Set the About Chrome value for profile path.
     */
    @CalledByNative
    public static void setProfilePathValue(String pathValue) {
        sProfilePathValue = pathValue;
    }

    /**
     * @return the mSearchEngine
     */
    public int getSearchEngine() {
        return mSearchEngine;
    }

    /**
     * @return the mAcceptCookiesEnabled
     */
    public boolean isAcceptCookiesEnabled() {
        return mAcceptCookiesEnabled;
    }

    /**
     * @return the mRememberPasswordsEnabled
     */
    public boolean isRememberPasswordsEnabled() {
        return mRememberPasswordsEnabled;
    }

    /**
     * @return the mAutofillEnabled
     */
    public boolean isAutofillEnabled() {
        return mAutofillEnabled;
    }

    /**
     * @return the mAllowLocationEnabled
     */
    public boolean isAllowLocationEnabled() {
        return mAllowLocationEnabled;
    }

    /**
     * @return the font scale factor
     */
    public float getFontScaleFactor() {
        return mFontScaleFactor;
    }

    /**
     * @return the force enable zoom setting
     */
    public boolean getForceEnableZoom() {
        return mForceEnableZoom;
    }

    /**
     * @return the mFontSize
     */
    public int getMinimumFontSize() {
        return mMinimumFontSize;
    }

    public boolean isRemoteDebuggingEnabled() {
        return mRemoteDebuggingEnabled;
    }

    /**
     * Returns true if JavaScript is enabled. It may return the temporary value set by
     * {@link #setJavaScriptEnabled}. The default is true.
     */
    public boolean javaScriptEnabled() {
        return mJavaScriptEnabled;
    }

    /**
     * Returns true if sync setup has completed. This is set internally by the
     * chrome sync code, so there is no setter.
     */
    public boolean syncSetupCompleted() {
        return mSyncSetupCompleted;
    }

    /**
     * Returns true if we should suppress starting up sync or displaying the
     * sync setup UI (because the user chose "Don't Sync" or disabled sync via
     * the dashboard).
     */
    public boolean syncSuppressStart() {
        return mSyncSuppressStart;
    }

    /**
     * Sets the preference that controlls whether the sync engine starts on startup.
     */
    public void setSyncSuppressStart(boolean suppress) {
        mSyncSuppressStart = suppress;
        nativeSetSyncSuppressStart(mSyncSuppressStart);
    }

    /**
     * Enable or disable JavaScript. If "immediately" is true, the change is applied right away.
     * Otherwise the change will be applied when {@link #sync} is called.
     */
    public void setJavaScriptEnabled(boolean enabled) {
        mJavaScriptEnabled = enabled;
        nativeSetJavaScriptEnabled(mJavaScriptEnabled);
    }

    /**
     * Returns the current account username associated with sync.
     */
    public String getSyncAccountName() {
        return mSyncAccountName;
    }

    /**
     * @return Whether or not all data types are being synced.
     */
    public boolean isSyncEverythingEnabled() {
        return mSyncEverything;
    }

    /**
     * @return Whether or not bookmarks are being synced.
     */
    public boolean isSyncBookmarksEnabled() {
        return mSyncBookmarks;
    }

    /**
     * @return Whether or not bookmarks are being synced.
     */
    public boolean isSyncTypedUrlsEnabled() {
        return mSyncTypedUrls;
    }

    /**
     * @return Whether or not sessions are being synced.
     */
    public boolean isSyncSessionEnabled() {
        return mSyncSessions;
    }

    /**
     * Returns the current user agent. It may return the temporary value set by
     * {@link #setUserAgent}.
     */
    public String userAgent() {
        return mUserAgent;
    }

    /**
     * Set the current user agent. If "ua" is empty, reset to the default user agent. If
     * "immediately" is true, the change is applied right away. Otherwise the change will be
     * applied when {@link #sync} is called.
     */
    public void setUserAgent(String ua, boolean immediately) {
        mUserAgent = ua;
        if (immediately) {
          nativeSetUserAgent(mUserAgent);
        }
    }

    /**
     * Enable or disable x-auto-login
     */
    public void setAutologinEnabled(boolean autologinEnabled) {
        nativeSetAutologinEnabled(autologinEnabled);
        mAutologinEnabled = autologinEnabled;
    }

    /**
     * Return true if x-auto-login is enabled, false otherwise.
     */
    public boolean isAutologinEnabled() {
        return mAutologinEnabled;
    }

    /**
     * Return whether Search Suggest is enabled.
     */
    public boolean isSearchSuggestEnabled() {
        return mSearchSuggestEnabled;
    }

    /**
     * Sets whether search suggest should be enabled.
     */
    public void setSearchSuggestEnabled(boolean enabled) {
        mSearchSuggestEnabled = enabled;
        nativeSetSearchSuggestEnabled(enabled);
    }

    /**
     * Return whether Network Predictions is enabled.
     */
    public boolean isNetworkPredictionEnabled() {
        return mNetworkPredictionEnabled;
    }

    /**
     * Sets whether network predictions should be enabled.
     */
    public void setNetworkPredictionEnabled(boolean enabled) {
        mNetworkPredictionEnabled = enabled;
        nativeSetNetworkPredictionEnabled(enabled);
    }

    /**
     * Return whether the web service to resolve navigation error is enabled.
     */
    public boolean isResolveNavigationErrorEnabled() {
        return mResolveNavigationErrorEnabled;
    }

    /**
     * Sets whether the web service to resolve navigation error should be enabled.
     */
    public void setResolveNavigationErrorEnabled(boolean enabled) {
        mResolveNavigationErrorEnabled = enabled;
        nativeSetResolveNavigationErrorEnabled(enabled);
    }

    // A "get saved passwords" query is async.  When a pref pane wants
    // data it can register and unregister itself with
    // startPasswordListRequest and stopPasswordListRequest.
    public interface PasswordListObserver {
        public void passwordListAvailable(int count);
        public void passwordExceptionListAvailable(int count);
    }

    public static void startPasswordListRequest(PasswordListObserver owner) {
        nativeStartPasswordListRequest(owner);
    }
    public static void stopPasswordListRequest() {
        nativeStopPasswordListRequest();
    }
    public static HashMap<String,String> getSavedNamePassword(int index) {
        return nativeGetSavedNamePassword(index);
    }
    public static HashMap<String,String> getSavedPasswordException(int index) {
        return nativeGetSavedPasswordException(index);
    }
    public static void removeSavedNamePassword(int index) {
        nativeRemoveSavedNamePassword(index);
    }
    public static void removeSavedPasswordException(int index) {
        nativeRemoveSavedPasswordException(index);
    }

    public String[] getAutofillProfileGUIDs() {
        return nativeGetAutofillProfileGUIDs();
    }

    public HashMap<String,String> getAutofillProfile(String guid) {
        return nativeGetAutofillProfile(guid);
    }

    // If creating a new profile (|GUID| is ""), returns the new GUID.
    // Else returns |GUID|.
    public String setAutofillProfile(String guid, HashMap profile) {
        return nativeSetAutofillProfile(guid, profile);
    }

    public void deleteAutofillProfile(String guid) {
        nativeDeleteAutofillProfile(guid);
    }

    public String[] getAutofillCreditCardGUIDs() {
        return nativeGetAutofillCreditCardGUIDs();
    }

    public HashMap<String,String> getAutofillCreditCard(String guid) {
        return nativeGetAutofillCreditCard(guid);
    }

    // If creating a new profile (|GUID| is ""), returns the new GUID.
    // Else returns |GUID|.
    public String setAutofillCreditCard(String guid, HashMap profile) {
        return nativeSetAutofillCreditCard(guid, profile);
    }

    public void deleteAutofillCreditCard(String guid) {
        nativeDeleteAutofillCreditCard(guid);
    }

    public interface OnClearBrowsingDataListener {
        public abstract void onBrowsingDataCleared();
    }

    // Clear the specified types of browsing data asynchronously.
    // |listener| is an object to be notified when clearing completes.
    // It can be null, but many operations (e.g. navigation) are
    // ill-advised while browsing data is being cleared.
    public void clearBrowsingData(OnClearBrowsingDataListener listener,
            boolean history, boolean cache, boolean cookies_and_site_data,
            boolean passwords, boolean formData) {
        mClearBrowsingDataListener = listener;
        nativeClearBrowsingData(
            history, cache, cookies_and_site_data, passwords, formData);
    }

    // Called from native.
    private void browsingDataCleared() {
        if (mClearBrowsingDataListener != null) {
            mClearBrowsingDataListener.onBrowsingDataCleared();
            mClearBrowsingDataListener = null;
        }
    }

    public void setSearchEngine(int selectedIndex) {
        mSearchEngine = selectedIndex;
        nativeSetSearchEngine(selectedIndex);
    }

    public void setAllowCookiesEnabled(boolean allow) {
        mAcceptCookiesEnabled = allow;
        nativeSetAllowCookiesEnabled(allow);
    }

    public void setAutoFillEnabled(boolean enabled) {
        mAutofillEnabled = enabled;
        nativeSetAutoFillEnabled(enabled);
    }

    public void setRememberPasswordsEnabled(boolean allow) {
        mRememberPasswordsEnabled = allow;
        nativeSetRememberPasswordsEnabled(allow);
    }

    public void setAllowLocationEnabled(boolean allow) {
        mAllowLocationEnabled = allow;
        nativeSetAllowLocationEnabled(allow);
    }

    public void setFontScaleFactor(float scale) {
        mFontScaleFactor = scale;
        nativeSetFontScaleFactor(scale);
    }

    public void setForceEnableZoom(boolean enabled) {
        mForceEnableZoom = enabled;
        nativeSetForceEnableZoom(enabled);
    }

    public void setMinimumFontSize(int selectedSize) {
        mMinimumFontSize = selectedSize;
        nativeSetMinimumFontSize(selectedSize);
    }

    public void setRemoteDebuggingEnabled(boolean enabled) {
        mRemoteDebuggingEnabled = enabled;
        RemoteDebuggingController.getInstance().updateUserPreferenceState(enabled);
        nativeSetRemoteDebuggingEnabled(enabled);
    }

    /**
     * Return the setting if popups are enabled
     */
    public boolean popupsEnabled() {
        return mAllowPopupsEnabled;
    }

    /**
     * Sets the preferences on whether to enable/disable popups
     *
     * @param allow attribute to enable/disable popups
     */
    public void setAllowPopupsEnabled(boolean allow) {
        mAllowPopupsEnabled = allow;
        nativeSetAllowPopupsEnabled(allow);
    }

    /**
     * Adds/Edit a popup exception
     *
     * @param pattern attribute for the popup exception pattern
     * @param allow attribute to specify whether to allow or block pattern
     */
    public void setPopupException(String pattern, boolean allow) {
        nativeSetPopupException(pattern, allow);
    }

    /**
     * Removes a popup exception
     *
     * @param pattern attribute for the popup exception pattern
     */
    public void removePopupException(String pattern) {
        nativeRemovePopupException(pattern);
    }

    /**
     * get all the currently saved popup exceptions
     *
     * @return HashMap array of all the exceptions and their settings
     */
    public HashMap<String,String>[] getPopupExceptions() {
        return nativeGetPopupExceptions();
    }

    /**
     * Called from native then autofill profile/credit card create/edit/delete is complete
     */
    private void autofillDataUpdated() {
        if (mAutoFillDataUpdatedListener != null) {
            mAutoFillDataUpdatedListener.onAutoFillDataUpdated();
        }
    }

    /**
     * Register the AutoFillDataUpdatedListener and native
     * AutofillPersonalDataManagerObserver pointer will be set
     */
    public void registerAutoFillDataUpdatedListener(onAutoFillDataUpdatedListener listener) {
        if (mAutoFillDataUpdatedListener != null) {
            Log.w(LOG_TAG , "mAutoFillDataUpdatedListener set again before getting destroyed");
        }
        mAutoFillDataUpdatedListener = listener;
        Log.e(LOG_TAG, "jrg: Init()");
        mNativeAutofillPersonalDataManagerObserver = nativeInitAutofillPersonalDataManagerObserver();
    }

    /**
     * Unregister the AutoFillDataUpdatedListener and native PersonalDataManagerObserver pointer
     * will be destroyed
     */
    public void unregisterAutoFillDataUpdatedListener() {
        mAutoFillDataUpdatedListener = null;
        if (mNativeAutofillPersonalDataManagerObserver != 0) {
            nativeDestroyPersonalDataManagerObserver(mNativeAutofillPersonalDataManagerObserver);
            mNativeAutofillPersonalDataManagerObserver = 0;
        }
    }

    /**
     * This listener will be notified when autofill profile/credit card create/edit/delete
     * is complete
     */
    public interface onAutoFillDataUpdatedListener {
        public abstract void onAutoFillDataUpdated();
    }

    /**
     * Search engine preference is updated from native.
     */
    public void updateSearchEngineFromNative() {
        nativeUpdateSearchEngineInJava();
    }

    /**
     * Called from native when template URL service is fone loading.
     */
    private void templateURLServiceLoaded() {
        if (mTemplateURLServiceLoadedListeners != null) {
            for (TemplateURLServiceLoadedListener t : mTemplateURLServiceLoadedListeners) {
                t.onTemplateURLServiceLoaded();
            }
        }
    }

    /**
     * Register the TemplateURLServiceLoadedListener and native TemplateURLServiceLoadedObserver
     * pointer will be set.
     */
    public void registerTemplateURLServiceLoadedListener(
            TemplateURLServiceLoadedListener listener) {
        if (mTemplateURLServiceLoadedListeners == null) {
            mTemplateURLServiceLoadedListeners = new ArrayList<TemplateURLServiceLoadedListener>();
        }
        if (mTemplateURLServiceLoadedListeners.contains(listener)) {
            Log.w(LOG_TAG , "mTemplateURLServiceLoadedListener set again before getting destroyed");
            assert false;
        }
        mTemplateURLServiceLoadedListeners.add(listener);
        if (mNativeTemplateURLServiceLoadedObserver == 0) {
            mNativeTemplateURLServiceLoadedObserver = nativeInitTemplateURLServiceLoadedObserver();
        }
    }

    /**
     * Unregister the TemplateURLServiceLoadedListener and native TemplateURLServiceLoadedObserver
     * pointer will be destroyed.
     */
    public void unregisterTemplateURLServiceLoadedListener(
            TemplateURLServiceLoadedListener listener) {
        assert (mTemplateURLServiceLoadedListeners != null);
        assert (mTemplateURLServiceLoadedListeners.contains(listener));
        mTemplateURLServiceLoadedListeners.remove(listener);
        if (mTemplateURLServiceLoadedListeners.size() == 0) {
            mTemplateURLServiceLoadedListeners = null;
        }
        if (mTemplateURLServiceLoadedListeners == null &&
                mNativeTemplateURLServiceLoadedObserver != 0) {
            nativeDestroyTemplateURLServiceLoadedObserver(mNativeTemplateURLServiceLoadedObserver);
            mNativeTemplateURLServiceLoadedObserver = 0;
        }
    }

    /**
     * This listener will be notified when template url service is done laoding.
     */
    public interface TemplateURLServiceLoadedListener {
        public abstract void onTemplateURLServiceLoaded();
    }

    /**
     * Returns whether the template url service is loaded.
     */
    public boolean isTemplateURLServiceLoaded() {
        return nativeIsTemplateURLServiceLoaded();
    }

    /**
     * get the setting for a pop up exception pattern
     *
     * @return String setting of the exception
     */
    public String getPopupExceptionSettingFromPattern(String pattern) {
        String setting = ChromeNativePreferences.EXCEPTION_SETTING_DEFAULT;
        for (HashMap<String,String> exception : getPopupExceptions()) {
            if (exception.get(ChromeNativePreferences.EXCEPTION_DISPLAY_PATTERN).equals(pattern)) {
                setting = exception.get(ChromeNativePreferences.EXCEPTION_SETTING);
                break;
            }
        }
        return setting;
    }

    /**
     * Get all the version strings from native.
     * @return hash map of all the strings.
     */
    public HashMap<String,String> getAboutVersionStrings() {
        return nativeGetAboutVersionStrings();
    }

    /**
     * Set executable and profile path values needed for about chrome.
     */
    public void setPathValuesForAboutChrome() {
        if (sExecutablePathValue == null && sProfilePathValue == null) {
            nativeSetPathValuesForAboutChrome();
        }
    }

    /**
     * Get the hash map of the localized search engines.
     * @return hash map of the search engines.
     */
    public HashMap<String,String>[] getLocalizedSearchEngines() {
        return nativeGetLocalizedSearchEngines();
    }

    private native void nativeSetSyncSuppressStart(boolean enabled);
    private native boolean nativeIsTemplateURLServiceLoaded();
    private native int nativeInitAutofillPersonalDataManagerObserver();
    private native int nativeInitTemplateURLServiceLoadedObserver();
    private native void nativeDestroyTemplateURLServiceLoadedObserver(
            int nativeTemplateURLServiceLoadedObserver);
    private native void nativeDestroyPersonalDataManagerObserver(
            int nativeAutofillPersonalDataManagerObserver);
    private native void nativeGet();
    private native void nativeUpdateSearchEngineInJava();
    private native void nativeSetJavaScriptEnabled(boolean enabled);
    private native void nativeSetUserAgent(String ua);
    private static native void nativeStartPasswordListRequest(PasswordListObserver owner);
    private static native void nativeStopPasswordListRequest();
    private static native HashMap<String,String> nativeGetSavedNamePassword(int index);
    private static native HashMap<String,String> nativeGetSavedPasswordException(int index);
    private static native void nativeRemoveSavedNamePassword(int index);
    private static native void nativeRemoveSavedPasswordException(int index);

    private native String[] nativeGetAutofillProfileGUIDs();
    private native HashMap<String,String> nativeGetAutofillProfile(String guid);
    private native String nativeSetAutofillProfile(String guid, HashMap profile);
    private native void nativeDeleteAutofillProfile(String guid);
    private native String[] nativeGetAutofillCreditCardGUIDs();
    private native HashMap<String,String> nativeGetAutofillCreditCard(String guid);
    private native String nativeSetAutofillCreditCard(String guid, HashMap profile);
    private native void nativeDeleteAutofillCreditCard(String guid);

    private native void nativeClearBrowsingData(boolean history, boolean cache,
            boolean cookies_and_site_data, boolean passwords, boolean formData);
    private native void nativeSetSearchEngine(int selectedIndex);
    private native void nativeSetAllowCookiesEnabled(boolean allow);
    private native void nativeSetAutoFillEnabled(boolean enabled);
    private native void nativeSetRememberPasswordsEnabled(boolean allow);
    private native void nativeSetAllowLocationEnabled(boolean allow);
    private native void nativeSetFontScaleFactor(float scale);
    private native void nativeSetForceEnableZoom(boolean enabled);
    private native void nativeSetMinimumFontSize(int fontSize);
    private native void nativeSetAllowPopupsEnabled(boolean allow);
    private native void nativeSetPopupException(String pattern, boolean allow);
    private native void nativeRemovePopupException(String pattern);
    private native HashMap<String,String>[] nativeGetPopupExceptions();
    private native void nativeSetAutologinEnabled(boolean autologinEnabled);
    private native HashMap<String,String> nativeGetAboutVersionStrings();
    private native void nativeSetPathValuesForAboutChrome();
    private native void nativeSetCountryCodeAtInstall(String countryCode);
    private native String nativeGetCountryCodeAtInstall();
    private native HashMap<String,String>[] nativeGetLocalizedSearchEngines();
    private native void nativeSetSearchSuggestEnabled(boolean enabled);
    private native void nativeSetNetworkPredictionEnabled(boolean enabled);
    private native void nativeSetResolveNavigationErrorEnabled(boolean enabled);
    private native void nativeSetRemoteDebuggingEnabled(boolean enabled);
}
