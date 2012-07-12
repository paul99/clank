// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.google.android.apps.chrome.sync;

import org.chromium.base.CalledByNative;

import com.google.android.apps.chrome.NativeCall;

import android.accounts.Account;
import android.util.Log;

import java.util.HashSet;
import java.util.Set;

/**
 * This adapter class wraps a native AndroidSyncSetupFlowHandler and exposes APIs and listener
 * callbacks to allow UI implemented in Java to implement the native sync wizard interfaces.
 */
class SyncSetupFlowAdapter {

    private static final String TAG = "ChromeSyncSetupFlowAdapter";

    public interface Listener {
        /**
         * Invoked when the sync backend is initialized and it is ready to be configured.
         * This is the signal for the listener to display the configuration UI to the user.
         * @param syncEverything True if the user is currently set to sync everything on this
         *     device.
         * @param syncTypes Set of types currently being synced.
         * @param encryptAllData True if the user has all of his data encrypted.
         * @param previousPassphraseInvalid True if the passphrase passed by a previous
         *     configureSync() call was invalid.
         * @param needPassphrase True if the sync engine needs a passphrase to decrypt data.
         * @param passphraseIsGaia True if the user is currently encrypting using his GAIA password.
         */
        public void showConfigure(boolean syncEverything,
                                  Set<ModelType> syncTypes,
                                  boolean encryptAllData,
                                  boolean previousPassphraseInvalid,
                                  boolean needPassphrase,
                                  boolean passphraseIsGaia,
                                  boolean passphraseRequiredForExternalType);

        /** Invoked when the sync setup is complete. */
        public void setupDone();

        /**
         * Invoked when an error occured while setting up sync.
         *
         * @param googleServiceAuthError The error that occured.
         */
        public void syncSetupError(GoogleServiceAuthError googleServiceAuthError);

    };

    /**
     * Reference to the native SyncSetupFlowHandler instance which is returned by
     * nativeSyncSetupStarted().
     */
    private int mNativeSyncSetupFlow;

    /** The listener we forward events to */
    private Listener mListener;

    public SyncSetupFlowAdapter(Listener l) {
        mListener = l;
        mNativeSyncSetupFlow = nativeCreate();
    }

    public void startSetup(Account account, String authToken) {
        // TODO(atwilson): Remove authToken when we add support for dynamically requesting it.
        Log.v(TAG, "Setting up sync for " + account);
        assert mNativeSyncSetupFlow != 0;
        nativeSyncSetupStarted(mNativeSyncSetupFlow, account.name, authToken);
    }

    /**
     * Invoked to configure the underlying sync engine. This should only be
     * invoked *after* a notifyShowConfigure() call has happened, as the sync
     * engine isn't ready to be configured before that.
     *
     * @param syncEverything True if the user wants to sync all types.
     * @param types Set of types to sync (ignored if syncEverything == true).
     * @param encryptAll True if the user wants to turn on encryption for all data types.
     *    Does nothing (leaves encrytion state as-is) if false.
     * @param decryptionPassphrase If non-null, sets the decryption passphrase for the client
     *    (must be provided if needPassphrase == true in previous showConfigure() call).
     * @param updatedPassphrase If non-null, this causes the user's data to be re-encrypted with the
     *    passed passphrase.
     * @param updatedPassphraseIsGaia If true, the value passed in updatedPassphrase is the
     *     user's GAIA password.
     */
    public void configureSync(boolean syncEverything, Set<ModelType> types,
                              boolean encryptAll, String decryptionPassphrase,
                              String updatedPassphrase, boolean updatedPassphraseIsGaia) {
        Log.v(TAG, "Configuring sync - syncEverything = " + syncEverything);
        assert mNativeSyncSetupFlow != 0;
        nativeConfigureSync(mNativeSyncSetupFlow, syncEverything,
                            syncEverything || types.contains(ModelType.BOOKMARK),
                            syncEverything || types.contains(ModelType.TYPED_URL),
                            syncEverything || types.contains(ModelType.SESSION),
                            encryptAll, decryptionPassphrase,
                            updatedPassphrase, updatedPassphraseIsGaia);
    }

    /**
     * Invoked when the setup process is complete so we can disconnect from the
     * native-side SyncSetupFlowHandler.
     */
    public void destroy() {
        Log.v(TAG, "Destroying native SyncSetupFlow");
        if (mNativeSyncSetupFlow != 0) {
            nativeSyncSetupEnded(mNativeSyncSetupFlow);
            mNativeSyncSetupFlow = 0;
        }
    }

    /**
     * Called from native code when the sync backend is initialized and is ready
     * for configuration. The current sync engine settings are passed as
     * parameters so the UI can be populated.
     *
     * @param syncEverything True if the user is currently set to sync everything on this device.
     * @param syncBookmarks True if the user is syncing bookmarks.
     * @param syncTypedUrls True if the user is syncing typed urls.
     * @param syncSessions True if the user is syncing tabs.
     * @param encryptAllData True if the user has all of his data encrypted.
     * @param invalidPassphrase True if the previous attempt to set the user's passphrase failed.
     * @param needPassphrase True if we need a passphrase to decrypt the user's data.
     * @param passphraseIsGaia True if the user is currently encrypting using his GAIA password.
     */
    @CalledByNative
    public void notifyShowConfigure(boolean syncEverything,
                                    boolean syncBookmarks,
                                    boolean syncTypedUrls,
                                    boolean syncSessions,
                                    boolean encryptAllData,
                                    boolean invalidPassphrase,
                                    boolean needPassphrase,
                                    boolean passphraseIsGaia,
                                    boolean passphraseRequiredForExternalType) {
        Log.v(TAG, "showConfigure()");
        assert mNativeSyncSetupFlow != 0;
        Set<ModelType> syncTypes = new HashSet<ModelType>();
        if (syncBookmarks) {
            syncTypes.add(ModelType.BOOKMARK);
        }
        if (syncTypedUrls) {
            syncTypes.add(ModelType.TYPED_URL);
        }
        if (syncSessions) {
            syncTypes.add(ModelType.SESSION);
        }
        mListener.showConfigure(syncEverything, syncTypes, encryptAllData,
                                invalidPassphrase, needPassphrase, passphraseIsGaia,
                                passphraseRequiredForExternalType);
    }

    /**
     * Called from native code when the sync setup is complete.
     */
    @CalledByNative
    public void notifySetupDone() {
        Log.v(TAG, "setupDone()");
        assert mNativeSyncSetupFlow != 0;
        mListener.setupDone();
    }

    @CalledByNative
    public void notifyError(int errorCode) {
        Log.v(TAG, "error(" + errorCode + ")");
        mListener.syncSetupError(new GoogleServiceAuthError(errorCode));
    }

    private native int nativeCreate();
    private native void nativeSyncSetupStarted(int nativeAndroidSyncSetupFlowHandler,
                                               String username, String authToken);
    private native void nativeConfigureSync(int nativeAndroidSyncSetupFlowHandler,
                                            boolean syncEverything,
                                            boolean syncBookmarks,
                                            boolean syncTypedUrls,
                                            boolean syncSessions,
                                            boolean encryptAll,
                                            String decryptionPassphrase,
                                            String updatedPassphrase,
                                            boolean updatedPassphraseIsGaia);
    private native void nativeSyncSetupEnded(int nativeAndroidSyncSetupFlowHandler);
}
