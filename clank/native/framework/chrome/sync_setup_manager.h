// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_SYNC_SETUP_MANAGER_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_SYNC_SETUP_MANAGER_H_

#include <jni.h>

#include "base/compiler_specific.h"
#include "chrome/browser/sync/profile_sync_service_observer.h"
#include "content/browser/android/jni_helper.h"

class Profile;
class ProfileSyncService;

// The native part of the java SyncSetupManager class.
// There should only be one instance of this class whose lifecycle is managed
// from the Java side.
class SyncSetupManager : public ProfileSyncServiceObserver {
 public:
  SyncSetupManager(JNIEnv* env, jobject obj);
  void Destroy(JNIEnv*env, jobject obj);

  void TokenAvailable(JNIEnv*, jobject, jstring username, jstring auth_token);

  // Called from Java when the user manually enables sync
  void EnableSync(JNIEnv* env, jobject obj);

  // Called from Java when the user manually disables sync
  void DisableSync(JNIEnv* env, jobject obj);

  // Called from Java when the user signs in to Chrome.
  void SignInSync(JNIEnv* env, jobject obj, jstring username);

  // Called from Java when the user signs out of Chrome
  void SignOutSync(JNIEnv* env, jobject obj);

  // Returns a string version of browser_sync::SyncBackendHost::StatusSummary
  base::android::ScopedJavaLocalRef<jstring> QuerySyncStatusSummary(
      JNIEnv* env, jobject obj);

  // Called from the Java sync tests in order to ensure that a unique machine tag is used by sync.
  void SetMachineTagForTest(JNIEnv* env, jobject obj, jlong tag);

  // Returns true if the sync backend is initialized.
  jboolean IsSyncInitialized(JNIEnv* env, jobject obj);

  // Returns true if the sync code needs a decryption passphrase.
  jboolean IsPassphraseRequiredForDecryption(JNIEnv* env, jobject obj);

  // Returns true if the sync code needs a custom decryption passphrase.
  // Can not be called if the sync backend is not initialized.
  jboolean IsUsingSecondaryPassphrase(JNIEnv* env, jobject obj);

  // Returns the integer value corresponding to the current auth error state
  // (GoogleServiceAuthError.State).
  jint GetAuthError(JNIEnv* env, jobject obj);

  // ProfileSyncServiceObserver:
  virtual void OnStateChanged() OVERRIDE;
 private:
  virtual ~SyncSetupManager();
  // Add and remove observers to profile sync service.
  void AddObserver();
  void RemoveObserver();
  void InvalidateAuthToken();

  Profile* profile_;
  ProfileSyncService* sync_service_;
  // Java-side sync manager object.
  JavaObjectWeakGlobalRef weak_java_sync_setup_manager_;

  DISALLOW_COPY_AND_ASSIGN(SyncSetupManager);
};

// Registers the SyncSetupManager's native methods through JNI.
bool RegisterSyncSetupManager(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_SYNC_SETUP_MANAGER_H_
