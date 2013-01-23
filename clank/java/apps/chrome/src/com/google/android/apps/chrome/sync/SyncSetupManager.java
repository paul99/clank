// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome.sync;

import android.accounts.Account;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Looper;
import android.preference.PreferenceManager;
import android.util.Log;

import com.google.android.apps.chrome.AccountManagerHelper;
import com.google.android.apps.chrome.ChromeNotificationCenter;
import com.google.android.apps.chrome.preferences.ChromeNativePreferences;
import com.google.android.apps.chrome.services.GoogleServicesNotificationController;
import com.google.android.apps.chrome.snapshot.IntentServiceWithWakeLock;
import com.google.android.apps.chrome.snapshot.SnapshotArchiveManager;
import com.google.android.apps.chrome.snapshot.SnapshotListenerManager;
import com.google.android.apps.chrome.snapshot.SnapshotSettings;
import com.google.android.apps.chrome.sync.Debug;

import org.chromium.content.browser.ActivityStatus;
import org.chromium.base.CalledByNative;

import com.google.common.annotations.VisibleForTesting;

import java.util.LinkedList;
import java.util.List;

/**
 *  Starts and monitors various sync related tasks.
 *   - monitor profile sync service for auth errors.
 *   - add listeners to AccountManager
 *   - start Tango service if sync setup is completed
 *
 *  It is intended to be an application level object (not tied to any activity),
 *  as we want to monitor native side sync service even when no activity is running.
 *
 *  The object should be created on the main thread.
 *
 *  TODO(jgreenwald): Rename this to something more appropriate, such as GoogleServiceManager
 */
public class SyncSetupManager {

    public interface SyncStateChangedListener {
        // Invoked when the underlying sync status has changed.
        public void syncStateChanged();
    }

    private static final String TAG = "SyncSetupManager";

    // Set to 'true' to enable debug messages.
    private static final boolean DEBUG = Debug.DEBUG;

    static final String AUTH_TOKEN_TYPE_SYNC = "chromiumsync";

    private static final String ACCOUNTS_CHANGED_PREFS_KEY = "prefs_sync_accounts_changed";

    private static final String SYNC_WANTED_STATE_PREFS_KEY = "prefs_sync_wanted_state";

    private static final String SYNC_PREVIOUS_MASTER_STATE_PREFS_KEY =
            "prefs_sync_master_previous_state";

    @VisibleForTesting
    protected final Context mContext;

    private final GoogleServicesNotificationController mGoogleServicesNotificationController;

    // Native SyncSetupManager object.
    private int mNativeSyncSetupManager;

    private final Handler mUiHandler;

    private final SyncStatusHelper mSyncStatusHelper;

    private final List<SyncStateChangedListener> mListeners =
            new LinkedList<SyncStateChangedListener>();

    /**
     * This is called pretty early in our application. Avoid any blocking operations here.
     */
    public SyncSetupManager(Context context) {
        // We should store the application context, as we outlive any activity which may create us.
        mContext = context.getApplicationContext();

        // This may cause us to create ProfileSyncService even if sync has not
        // been set up, but ProfileSyncService::Startup() wont be called until
        // credentials are available.
        mNativeSyncSetupManager = nativeInit();

        // We are created on the Ui thread.
        mUiHandler = new Handler();

        // Register listener to monitor Account manager for changes.
        mSyncStatusHelper = SyncStatusHelper.get(context);
        mSyncStatusHelper.registerObserver(new SyncSettingsObserver());

        // Register listeners for things that may require to Android notifications
        mGoogleServicesNotificationController =
                GoogleServicesNotificationController.createNewInstance(mContext, this,
                        mSyncStatusHelper);
        addSyncStateChangedListener(mGoogleServicesNotificationController);
        SnapshotListenerManager.addListener(mGoogleServicesNotificationController);

        // Register listener that disables sync when sync is disabled from Google Dashboard
        addSyncStateChangedListener(new SyncDisabler(this, mSyncStatusHelper));

        // Enable/disable sync based on Android settings.
        applyAndroidSyncStateOnUiThread();

        // Add a listener which stops our cache invalidation services when we are paused.
        ActivityStatus.getInstance().registerListener(
                new ChromeSyncInvalidationListenerController(mContext));
    }

    /**
     * Currently, destroy is never called as we are never destroyed.
     */
    public void destroy() {
        nativeDestroy(mNativeSyncSetupManager);
        mNativeSyncSetupManager = 0;
    }

    public void onMainActivityResume() {
        boolean accountsChanged = checkAndClearAccountsChangedPref(mContext);
        validateSyncSettings(accountsChanged);
    }

    public void validateSyncSettings(boolean accountsChanged) {
        Account syncAccount = mSyncStatusHelper.getSignedInUser();
        if (syncAccount == null) {
            // Sync not signed in. Nothing to do.
            return;
        }

        // Always check for account deleted.
        if (!accountExists(syncAccount)) {
            Log.i(TAG, "Signed in account has been deleted. Signing out of Chrome.");
            signOut();
            return;
        }

        if (mSyncStatusHelper.isSyncEnabled(syncAccount)) {
            if (syncSetupCompleted()) {
                if (accountsChanged) {
                    // Sync is signed in and account exists. Credentials may have changed, nudge
                    // the syncer if sync is enabled.
                    requestSyncFromNativeChrome("", 0, "");
                }
            } else {
                // We should have set up sync but for some reason it's not enabled. Tell the sync
                // engine to start.
                syncSignIn(syncAccount.name);
            }
        }
    }

    private boolean accountExists(Account account) {
        Account[] accounts = AccountManagerHelper.get(mContext).getGoogleAccounts();
        for (Account a : accounts) {
            if (a.equals(account)) {
                return true;
            }
        }
        return false;
    }

    public void signOut() {
        // Disable all enabled features
        Account currentlySignedInUser = mSyncStatusHelper.getSignedInUser();
        if (DEBUG) {
          Log.d(TAG, "Signing user out of Chrome: " + currentlySignedInUser);
        }
        SyncStates.SyncStatesBuilder states = SyncStates.create();
        if (mSyncStatusHelper.isSyncEnabled(currentlySignedInUser)) {
            states.sync(false);
        }
        if (SnapshotSettings.isEnabled(mContext)) {
            states.sendToDevice(false);
        }
        if (isAutoLoginEnabled(mContext)) {
            states.autoLogin(false);
        }
        setStates(states.build(), currentlySignedInUser);
        // Clear currently logged in user
        mSyncStatusHelper.clearSignedInUser();
        // Also completely sign out the native sync, clearing all sync state
        nativeSignOutSync(mNativeSyncSetupManager);
    }

    /**
     * Signs in to sync.
     */
    public void syncSignIn(String account) {
        nativeSignInSync(mNativeSyncSetupManager, account);
        // Notify listeners right away that the sync state has changed (native side does not do
        // this)
        syncStateChanged();
    }

    private boolean isStateChangeValid(Account account, SyncStates states) {
        if (DEBUG) {
          StringBuilder sb = new StringBuilder();
          if (states.hasSync()) {
              sb.append("sync = ").append(states.isSyncEnabled());
          }
          if (states.hasWantedSyncState()) {
              sb.append(", wantedSyncState = ").append(states.isWantedSyncStateEnabled());
          }
          if (states.hasSendToDevice()) {
              sb.append(", sendToDevice = ").append(states.isSendToDeviceEnabled());
          }
          if (states.hasAutoLoginSet()) {
              sb.append(", autoLogin = ").append(states.isAutoLoginEnabled());
          }
          Log.d(TAG, "setState: " + sb.toString());
        }
        if (account == null) {
            if (DEBUG) {
              Log.d(TAG, "No account supplied. Not changing states");
            }
            return false;
        }
        return true;
    }

    public void setStates(final SyncStates states, final Account account) {
        if (!isStateChangeValid(account, states)) {
            return;
        }
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                if (states.hasSync()) {
                    setSyncState(account, states);
                }
                if (states.hasSendToDevice()) {
                    setSendToDeviceState(states.isSendToDeviceEnabled());
                }
                if (states.hasAutoLoginSet()) {
                    setAutologinState(states.isAutoLoginEnabled());
                }
            }
        };
        runOnUiThread(runnable);
    }

    private void runOnUiThread(Runnable runnable) {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            runnable.run();
        } else {
            mUiHandler.post(runnable);
        }
    }

    private void applyAndroidSyncStateOnUiThread() {
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                Account account = mSyncStatusHelper.getSignedInUser();
                if (account == null) {
                    Log.v(TAG, "Ignoring pref change because user is not signed in to Chrome");
                    return;
                }
                boolean inSyncState = mSyncStatusHelper.isSyncEnabledForChrome(account);
                boolean inMasterSyncState = mSyncStatusHelper.isMasterSyncAutomaticallyEnabled();
                boolean inPreviousMasterSyncState = getPreviousMasterSyncState();
                boolean inWantedSyncState = getWantedSyncState(account);
                SyncStates newSyncStates =
                        SyncStateCalculator.calculateNewSyncStates(account, inSyncState,
                                inMasterSyncState, inPreviousMasterSyncState, inWantedSyncState);
                setStates(newSyncStates, account);
            }
        };
        runOnUiThread(runnable);
    }

    private void setAutologinState(boolean enabled) {
        ChromeNativePreferences.getInstance().setAutologinEnabled(enabled);
        if (!enabled) {
            ChromeNotificationCenter.broadcastNotification(
                    ChromeNotificationCenter.AUTO_LOGIN_DISABLED);
        }
    }

    private void setSyncState(Account account, SyncStates states) {
        storeSyncStatePreferences(account, states);
        if (states.isSyncEnabled()) {
            if (mSyncStatusHelper.isMasterSyncAutomaticallyEnabled()) {
                if (DEBUG) {
                  Log.d(TAG, "Enabling sync");
                }
                ChromeSyncInvalidationListener.startChromeSyncInvalidationListener(mContext);
                nativeEnableSync(mNativeSyncSetupManager);
                mSyncStatusHelper.enableAndroidSync(account);
            } else {
                if (DEBUG) {
                  Log.d(TAG, "Unable to enable sync, since master sync is disabled. "
                             + "Displaying error notification.");
                }
                mGoogleServicesNotificationController.
                        displayAndroidMasterSyncDisabledNotification();
            }
        } else {
            if (DEBUG) {
              Log.d(TAG, "Disabling sync");
            }
            ChromeSyncInvalidationListener.stopChromeSyncInvalidationListener(mContext);
            nativeDisableSync(mNativeSyncSetupManager);
            mSyncStatusHelper.disableAndroidSync(account);
        }
        syncStateChanged();
    }

    private void storeSyncStatePreferences(Account account, SyncStates states) {
        boolean wantedSyncState = states.hasWantedSyncState() ?
                states.isWantedSyncStateEnabled() : states.isSyncEnabled();
        if (DEBUG) {
          Log.d(TAG, "Setting wanted sync state to " + wantedSyncState +
                (states.hasWantedSyncState() ? " (from Android settings)" : ""));
        }

        SharedPreferences.Editor editor =
                PreferenceManager.getDefaultSharedPreferences(mContext).edit();
        editor.putBoolean(SYNC_WANTED_STATE_PREFS_KEY, wantedSyncState);

        if (states.hasMasterSyncState()) {
            if (DEBUG) {
              Log.d(TAG, "Setting previous master sync state to " +
                    states.isMasterSyncStateEnabled());
            }
            editor.putBoolean(SYNC_PREVIOUS_MASTER_STATE_PREFS_KEY,
                    states.isMasterSyncStateEnabled());
        }
        editor.apply();
    }

    private void setSendToDeviceState(boolean desiredPrintState) {
        if (SnapshotSettings.isEnabled(mContext) != desiredPrintState) {
            Intent intent = SnapshotArchiveManager.createSetEnabledIntent(mContext,
                    desiredPrintState);
            IntentServiceWithWakeLock.startServiceWithWakeLock(mContext, intent);
        }
    }

    static void requestSyncFromNativeChrome(String objectId, long version, String payload) {
        nativeNudgeSyncer(objectId, version, payload);
    }

    /**
     * Must be run on the main thread since it is contacting the sync engine directly

     * @return the current sync status
     */
    @VisibleForTesting
    public String querySyncStatus() {
        assert (Looper.getMainLooper() == Looper.myLooper());
        assert mNativeSyncSetupManager != 0;
        return nativeQuerySyncStatusSummary(mNativeSyncSetupManager);
    }

    /**
     * Sets the the machine tag used by sync to a unique value and overrides the default value
     * which is a static value derived from the device's android ID.
     * @param tag Numeric value used by sync to initialize its machine tag.
     */
    @VisibleForTesting
    public void setMachineTagForTest(long tag) {
        assert (Looper.getMainLooper() == Looper.myLooper());
        assert mNativeSyncSetupManager != 0;
        nativeSetMachineTagForTest(mNativeSyncSetupManager, tag);
    }

    /**
     * Requests a new auth token from the AccountManager. Invalidates the old token
     * if |invalidAuthToken| is not empty.
     */
    @CalledByNative
    public void getNewAuthToken(final String username, final String invalidAuthToken) {
        if (DEBUG) {
          Log.d(TAG, "Handling request for auth token from sync engine");
        }
        if (username == null) {
            Log.e(TAG, "username is null");
            return;
        }

        final AccountManagerHelper accountManagerHelper = AccountManagerHelper.get(mContext);
        final Account account = accountManagerHelper.getAccountFromName(username);
        if (account == null) {
            Log.e(TAG, "Account not found for provided username.");
            return;
        }

        // Since this is blocking, do it in the background.
        new AsyncTask<Void, Void, String>() {

            @Override
            public String doInBackground(Void... params) {
                // Invalidate our old auth token and fetch a new one.
                return accountManagerHelper.getNewAuthToken(
                        account, invalidAuthToken, AUTH_TOKEN_TYPE_SYNC);
            }

            @Override
            public void onPostExecute(String authToken) {
                if (authToken == null) {
                    // TODO(sync): Need to hook LOGIN_ACCOUNTS_CHANGED_ACTION (http://b/5354713).
                    if (DEBUG) {
                      Log.d(TAG, "Auth token for sync was null.");
                    }
                } else {
                    if (mNativeSyncSetupManager != 0) {
                        if (DEBUG) {
                          Log.d(TAG, "Successfully retrieved sync auth token.");
                        }
                        nativeTokenAvailable(mNativeSyncSetupManager, username, authToken);
                    } else {
                        Log.e(TAG, "Native sync setup manager not valid.");
                    }
                }
            }
        }.execute();
    }

    /**
     * Checks if a password or a passphrase is required for decryption of sync data.
     *
     * Returns NONE if the state is unavailable, or decryption passphrase/password is not required.
     *
     * @return the enum describing the decryption passphrase type required
     */
    public SyncDecryptionPassphraseType getSyncDecryptionPassphraseType() {
        // ProfileSyncService::IsUsingSecondaryPassphrase() (which is called by
        // SyncSetupManager::IsUsingSecondaryPassphrase() requires the sync backend to be
        // initialized, and that happens just after OnPassphraseRequired(). Therefore, we need to
        // guard that call with a a check of the sync backend since we can not be sure which
        // passphrase type we should tell the user we need.
        // This is tracked in:
        // http://code.google.com/p/chromium/issues/detail?id=108127
        if (nativeIsSyncInitialized(mNativeSyncSetupManager) &&
                nativeIsPassphraseRequiredForDecryption(mNativeSyncSetupManager)) {
            if (nativeIsUsingSecondaryPassphrase(mNativeSyncSetupManager)) {
                return SyncDecryptionPassphraseType.CUSTOM_PASSPHRASE;
            } else {
                return SyncDecryptionPassphraseType.GOOGLE_ACCOUNT_PASSWORD;
            }
        }
        return SyncDecryptionPassphraseType.NONE;
    }

    public GoogleServiceAuthError.State getAuthError() {
        int authErrorCode = nativeGetAuthError(mNativeSyncSetupManager);
        return GoogleServiceAuthError.State.fromCode(authErrorCode);
    }

    public boolean syncSetupCompleted() {
        // Make sure the syncSetupCompleted() pref is up-to-date.
        ChromeNativePreferences.getInstance().update();
        return ChromeNativePreferences.getInstance().syncSetupCompleted();
    }

    public GoogleServicesNotificationController getGoogleServicesNotificationController() {
        return mGoogleServicesNotificationController;
    }

    public void addSyncStateChangedListener(SyncStateChangedListener listener) {
        mListeners.add(listener);
    }

    public void removeSyncStateChangedListener(SyncStateChangedListener listener) {
        mListeners.remove(listener);
    }

    /**
     * Called when the state of the native sync engine has changed, so various
     * UI elements can update themselves.
     */
    @CalledByNative
    public void syncStateChanged() {
        for (SyncStateChangedListener listener : mListeners) {
            listener.syncStateChanged();
        }
    }

    private class SyncSettingsObserver extends SyncStatusHelper.SyncSettingsChangedObserver {

        @Override
        protected void syncSettingsChanged() {
            applyAndroidSyncStateOnUiThread();
        }
    }

    static boolean isAutoLoginEnabled(Context context) {
        if (!SyncStatusHelper.get(context).isSignedIn()) {
            return false;
        }
        ChromeNativePreferences preferences = ChromeNativePreferences.getInstance();
        preferences.update();
        return preferences.isAutologinEnabled();
    }

    /**
     * Returns the user's wanted sync automatically state for Chrome.
     *
     * If this has never been set, it returns the current sync state flag.
     */
    private boolean getWantedSyncState(Account account) {
        return PreferenceManager.getDefaultSharedPreferences(mContext)
                .getBoolean(SYNC_WANTED_STATE_PREFS_KEY,
                        mSyncStatusHelper.isSyncEnabledForChrome(account));
    }

    /**
     * Returns the previous master sync automatically state.
     *
     * If this has never been set, it returns the current master sync automatically flag.
     */
    private boolean getPreviousMasterSyncState() {
        return PreferenceManager.getDefaultSharedPreferences(mContext)
                .getBoolean(SYNC_PREVIOUS_MASTER_STATE_PREFS_KEY,
                        mSyncStatusHelper.isMasterSyncAutomaticallyEnabled());
    }

    /**
     * Sets the ACCOUNTS_CHANGED_PREFS_KEY to true.
     */
    public static void markAccountsChangedPref(Context context) {
        // The process may go away as soon as we return from onReceive but Android makes sure
        // that in-flight disk writes from apply() complete before changing component states.
        PreferenceManager.getDefaultSharedPreferences(context)
                .edit().putBoolean(ACCOUNTS_CHANGED_PREFS_KEY, true).apply();
    }

    private boolean checkAndClearAccountsChangedPref(Context context) {
        if (PreferenceManager.getDefaultSharedPreferences(context)
                .getBoolean(ACCOUNTS_CHANGED_PREFS_KEY, false)) {
            // Clear the value in prefs.
            PreferenceManager.getDefaultSharedPreferences(context)
                    .edit().putBoolean(ACCOUNTS_CHANGED_PREFS_KEY, true).apply();
            return true;
        } else {
            return false;
        }
    }

    // Native methods
    private static native void nativeNudgeSyncer(String objectId, long version, String payload);
    private native int nativeInit();
    private native void nativeDestroy(int nativeSyncSetupManager);
    private native void nativeEnableSync(int nativeSyncSetupManager);
    private native void nativeDisableSync(int nativeSyncSetupManager);
    private native void nativeSignInSync(int nativeSyncSetupManager, String username);
    private native void nativeSignOutSync(int nativeSyncSetupManager);
    private native void nativeTokenAvailable(int nativeSyncSetupManager, String username,
                                             String authToken);
    private native void nativeSetMachineTagForTest(int nativeSyncSetupManager, long tag);
    private native String nativeQuerySyncStatusSummary(int nativeSyncSetupManager);
    private native int nativeGetAuthError(int nativeSyncSetupManager);
    private native boolean nativeIsSyncInitialized(int nativeSyncSetupManager);
    private native boolean nativeIsPassphraseRequiredForDecryption(int nativeSyncSetupManager);
    private native boolean nativeIsUsingSecondaryPassphrase(int nativeSyncSetupManager);
}
