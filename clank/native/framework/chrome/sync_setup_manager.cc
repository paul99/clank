// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/sync_setup_manager.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/logging.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/signin/token_service.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/signin_manager.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/sync/glue/session_model_associator.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/syncable/model_type.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/net/gaia/gaia_constants.h"
#include "chrome/common/net/gaia/google_service_auth_error.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/notification_service.h"
#include "google/cacheinvalidation/include/types.h"
#include "google/cacheinvalidation/v2/types.pb.h"
#include "jni/sync_setup_manager_jni.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using content::BrowserThread;

namespace {
const char kSyncDisabledStatus[] = "OFFLINE_DISABLED";

// Must be called on the UI thread.
static void SendNudgeNotification(const std::string& str_object_id,
                                  int64 version,
                                  const std::string& payload) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Reconstruct the Invalidation from its constituent parts. It had to be broken up to get
  // through Android Intent messaging.
  invalidation::ObjectId object_id(ipc::invalidation::ObjectSource::CHROME_SYNC, str_object_id);
  invalidation::Invalidation invalidation(object_id, version, payload);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_SYNC_INCOMING_NOTIFICATION,
      content::NotificationService::AllSources(),
      content::Details<invalidation::Invalidation>(&invalidation));
}

// The SyncSetupManager::mNativeSyncSetupManager field.
SyncSetupManager* g_sync_setup_manager = NULL;
}  // namespace

SyncSetupManager::SyncSetupManager(JNIEnv* env, jobject obj)
    : weak_java_sync_setup_manager_(env, obj) {
  profile_ = NULL;
  sync_service_ = NULL;
  if (g_browser_process == NULL ||
      g_browser_process->profile_manager() == NULL) {
    LOG(ERROR) << "Browser process or profile manager not initialized";
    return;
  }

  profile_ = g_browser_process->profile_manager()->GetDefaultProfile();
  if (profile_ == NULL) {
    LOG(ERROR) << "Sync Init: Profile not found.";
    return;
  }

  sync_service_ = profile_->GetProfileSyncService();
  AddObserver();
}

void SyncSetupManager::AddObserver() {
  if (sync_service_ == NULL) {
    LOG(ERROR) << "Sync service not found.";
    return;
  }
  if (!sync_service_->HasObserver(this)) {
    sync_service_->AddObserver(this);
  }

  std::string signed_in_username =
      profile_->GetPrefs()->GetString(prefs::kGoogleServicesUsername);
  if (!signed_in_username.empty()) {
    // If the user is logged in, see if he has a valid token - if not, fetch
    // a new one.
    if (!profile_->GetTokenService()->HasTokenForService(
            GaiaConstants::kSyncService) ||
        (sync_service_->GetAuthError().state() ==
         GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS)) {
      VLOG(2) << "Trying to update token for user " << signed_in_username;
      InvalidateAuthToken();
    }
  }
}

void SyncSetupManager::RemoveObserver() {
  if (sync_service_ == NULL) {
    LOG(ERROR) << "Sync service not found.";
    return;
  }
  if (sync_service_->HasObserver(this)) {
    sync_service_->RemoveObserver(this);
  }
}

SyncSetupManager::~SyncSetupManager() {
  RemoveObserver();
  g_sync_setup_manager = NULL;
}

void SyncSetupManager::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

void SyncSetupManager::OnStateChanged() {
  // Check for auth errors.
  const GoogleServiceAuthError& auth_error = sync_service_->GetAuthError();
  if (auth_error.state() == GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS) {
      VLOG(2) << "Updating auth token.";
      InvalidateAuthToken();
  }

  // Notify the java world that our sync state has changed.
  JNIEnv* env = AttachCurrentThread();
  Java_SyncSetupManager_syncStateChanged(
      env, weak_java_sync_setup_manager_.get(env).obj());
}

void SyncSetupManager::TokenAvailable(JNIEnv* env, jobject, jstring username,
    jstring auth_token) {
  std::string token = ConvertJavaStringToUTF8(env, auth_token);
  profile_->GetTokenService()->OnIssueAuthTokenSuccess(
      GaiaConstants::kSyncService, token);
}

void SyncSetupManager::EnableSync(JNIEnv* env, jobject) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(sync_service_);
  // Don't need to do anything if we're already enabled.
  if (profile_->GetPrefs()->GetBoolean(prefs::kSyncSuppressStart)) {
    // We clear the sync setup completed flag to be able to enable sync in the
    // customization preference screen. It will automatically be set again when
    // the sync backend is initialized, and this will trigger an event that
    // enables the customization screen.
    sync_service_->ClearSyncSetupCompleted();
    // This auto starts the sync engine.
    sync_service_->UnsuppressAndStart();
  } else {
    VLOG(2) << "Ignoring call to EnableSync() because sync is already enabled";
  }
}

void SyncSetupManager::DisableSync(JNIEnv* env, jobject) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(sync_service_);
  sync_service_->StopAndSuppress();
}

void SyncSetupManager::SignInSync(JNIEnv* env, jobject, jstring username) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(sync_service_);
  // If sync is already running, just exit.
  if (sync_service_->AreCredentialsAvailable() &&
      sync_service_->HasSyncSetupCompleted()) {
    LOG(ERROR) << "Ignoring signin request - user is already signed in to sync";
    return;
  }

  // Set the currently-signed-in username, fetch an auth token if necessary,
  // and enable sync.
  std::string name = ConvertJavaStringToUTF8(env, username);
  profile_->GetPrefs()->SetString(prefs::kGoogleServicesUsername, name);
  if (!sync_service_->AreCredentialsAvailable()) {
    // No credentials available - request an auth token.
    // If fetching the auth token is successful, this will cause
    // ProfileSyncService to start sync when it receives
    // NOTIFICATION_TOKEN_AVAILABLE.
    VLOG(2) << "Fetching auth token for " << name;
    InvalidateAuthToken();
  }

  // Enable sync (if we don't have credentials yet, this will enable sync but
  // will not start it up - sync will start once credentials arrive).
  sync_service_->UnsuppressAndStart();
}

void SyncSetupManager::SignOutSync(JNIEnv* env, jobject) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(sync_service_);
  DCHECK(profile_);
  sync_service_->DisableForUser();

  // Clear the tokens.
  SigninManagerFactory::GetForProfile(profile_)->SignOut();

  // Need to clear suppress start flag manually
  PrefService* pref_service = profile_->GetPrefs();
  pref_service->SetBoolean(prefs::kSyncSuppressStart, false);
}

ScopedJavaLocalRef<jstring> SyncSetupManager::QuerySyncStatusSummary(
    JNIEnv* env, jobject) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(sync_service_);
  DCHECK(profile_);
  browser_sync::SyncBackendHost::StatusSummary status_summary =
      sync_service_->QuerySyncStatusSummary();
  std::string status;

  // Detect if sync is disabled vs signed out
  if (status_summary ==
      browser_sync::SyncBackendHost::Status::OFFLINE_UNUSABLE &&
      profile_->GetPrefs()->GetBoolean(prefs::kSyncSuppressStart)) {
    status = kSyncDisabledStatus;
  } else {
    status = ProfileSyncService::BuildSyncStatusSummaryText(status_summary);
  }

  return ConvertUTF8ToJavaString(env, status);
}

void SyncSetupManager::SetMachineTagForTest(JNIEnv* env, jobject obj, jlong tag) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  browser_sync::SessionModelAssociator::SetMachineTagForTest(tag);
}

jint SyncSetupManager::GetAuthError(JNIEnv* env, jobject) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(sync_service_);
  return sync_service_->GetAuthError().state();
}

jboolean SyncSetupManager::IsSyncInitialized(JNIEnv* env, jobject) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(sync_service_);
  return sync_service_->sync_initialized();
}

jboolean SyncSetupManager::IsPassphraseRequiredForDecryption(JNIEnv* env, jobject) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(sync_service_);
  return sync_service_->IsPassphraseRequiredForDecryption();
}

jboolean SyncSetupManager::IsUsingSecondaryPassphrase(JNIEnv* env, jobject) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(sync_service_);
  return sync_service_->IsUsingSecondaryPassphrase();
}

void SyncSetupManager::InvalidateAuthToken() {
  // Get the token from token-db. If there's no token yet, this must be the
  // the first time the user is signing in so we don't need to invalidate
  // anything.
  TokenService* token_service = profile_->GetTokenService();
  std::string invalid_token;
  if (token_service->HasTokenForService(GaiaConstants::kSyncService)) {
    invalid_token = token_service->GetTokenForService(
        GaiaConstants::kSyncService);
  }
  const std::string& sync_username =
      profile_->GetPrefs()->GetString(prefs::kGoogleServicesUsername);
  // Call into java to invalidate the current token and get a new one.
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> j_sync_username =
      ConvertUTF8ToJavaString(env, sync_username);
  ScopedJavaLocalRef<jstring> j_invalid_token =
      ConvertUTF8ToJavaString(env, invalid_token);
  Java_SyncSetupManager_getNewAuthToken(
      env, weak_java_sync_setup_manager_.get(env).obj(),
      j_sync_username.obj(), j_invalid_token.obj());
}

static void NudgeSyncer(JNIEnv* env,
                        jclass clazz,
                        jstring objectId,
                        jlong version,
                        jstring payload) {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    SendNudgeNotification(ConvertJavaStringToUTF8(env, objectId), version,
                          ConvertJavaStringToUTF8(env, payload));
  } else {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
        base::Bind(&SendNudgeNotification,
                            ConvertJavaStringToUTF8(env, objectId), version,
                            ConvertJavaStringToUTF8(env, payload)));
  }
}

static int Init(JNIEnv* env, jobject obj) {
  // We should have only one SyncSetupManager instance.
  DCHECK(!g_sync_setup_manager);
  g_sync_setup_manager = new SyncSetupManager(env, obj);
  return reinterpret_cast<jint>(g_sync_setup_manager);
}

// Register native methods

bool RegisterSyncSetupManager(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
