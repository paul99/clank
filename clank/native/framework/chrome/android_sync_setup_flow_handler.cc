// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/android_sync_setup_flow_handler.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/logging.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/signin/token_service.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/sync_setup_flow.h"
#include "chrome/browser/sync/sync_setup_wizard.h"
#include "chrome/browser/sync/syncable/model_type.h"
#include "chrome/browser/sync/util/oauth.h"
#include "chrome/common/net/gaia/gaia_constants.h"
#include "chrome/common/pref_names.h"
#include "jni/sync_setup_flow_adapter_jni.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ConvertJavaStringToUTF8;

AndroidSyncSetupFlowHandler::AndroidSyncSetupFlowHandler(
    JNIEnv* env, jobject obj)
    : method_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)),
      flow_(NULL),
      weak_java_sync_setup_adapter_(env, obj) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  DCHECK(profile);
  service_ = profile->GetProfileSyncService();
  DCHECK(service_);
}

AndroidSyncSetupFlowHandler::~AndroidSyncSetupFlowHandler() {
  // Detach ourselves from the SyncSetupFlow. This will cancel any setup in
  // progress.
  if (flow_)
    flow_->OnDialogClosed(std::string());
  // Make sure OnDialogClosed() has disassociated us from the flow.
  DCHECK(!flow_);
  method_factory_.InvalidateWeakPtrs();
}

void AndroidSyncSetupFlowHandler::ShowOAuthLogin() {  // megamerge: unwritten
  NOTIMPLEMENTED();
}

// None of the Gaia-related states should ever be encountered (since we don't
// prompt for login credentials).
void AndroidSyncSetupFlowHandler::ShowGaiaLogin(const DictionaryValue& args) {
  // This can be called either as part of initial setup (when we step through
  // the GAIA_LOGIN state) or if an error occurs while contacting the server.
  int error_code = 0;
  if (args.GetInteger("error", &error_code) && error_code != 0) {
    JNIEnv* env = AttachCurrentThread();
    Java_SyncSetupFlowAdapter_notifyError(
        env, weak_java_sync_setup_adapter_.get(env).obj(),
        error_code);
  }
}

void AndroidSyncSetupFlowHandler::ShowGaiaSuccessAndClose() {
  NOTREACHED();
}

void AndroidSyncSetupFlowHandler::ShowGaiaSuccessAndSettingUp() {
  // Do nothing here - this is just a transient state we get to as part of
  // initialization.
}

// Helper function to extract a boolean value from a DictionaryValue and return
// it as a jboolean.
static jboolean GetJBooleanValue(const std::string& key,
                                 const DictionaryValue& args) {
  bool value = false;
  CHECK(args.GetBoolean(key, &value));
  return value;
}

static jboolean GetJBooleanValueWithDefault(const std::string& key,
                                            const DictionaryValue& args,
                                            jboolean defaultValue) {
  return args.HasKey(key) ? GetJBooleanValue(key, args) : defaultValue;
}

void AndroidSyncSetupFlowHandler::ShowConfigure(const DictionaryValue& args) {
  // Unfortunately this API is geared around passing parameters via JSON to
  // a javascript backend (name-value pairs), so we have to extract them
  // manually to pass them to java.
  JNIEnv* env = AttachCurrentThread();

  Java_SyncSetupFlowAdapter_notifyShowConfigure(
      env, weak_java_sync_setup_adapter_.get(env).obj(),
      GetJBooleanValue("syncAllDataTypes", args),
      GetJBooleanValue("syncBookmarks", args),
      GetJBooleanValue("syncTypedUrls", args),
      GetJBooleanValue("syncSessions", args),
      GetJBooleanValue("encryptAllData", args),
      GetJBooleanValueWithDefault("passphrase_setting_rejected", args, false),
      // TODO(atwilson): Initialize the needsPassphrase parameter from the
      // passed args when the upstream code starts passing this.
      service_->IsPassphraseRequiredForDecryption(),
      !GetJBooleanValue("usePassphrase", args),
      service_->passphrase_required_reason() != sync_api::REASON_ENCRYPTION);
}

void AndroidSyncSetupFlowHandler::ShowPassphraseEntry(
    const DictionaryValue& args) {
  // Should only be called if the passphrase the user entered was incorrect.
  // TODO(atwilson): Remove this when http://crbug.com/90786 is fixed since this
  // is just working around that issue (SyncSetupFlow should directly call
  // ShowConfigure() itself).
  DCHECK(service_->IsPassphraseRequiredForDecryption());
  ShowConfigure(args);
}

void AndroidSyncSetupFlowHandler::ShowSettingUp() {
  // We don't show the "setting up" dialog on Android - we just display
  // our "waiting" UI as soon as the user clicks the button, so we ignore this
  // notification.
}

void AndroidSyncSetupFlowHandler::ShowSetupDone(const string16& user) {
  // Notify the java layer that we've finished setting up sync. We do this via
  // a task so we allow the current callstack to complete.
  MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(
          &AndroidSyncSetupFlowHandler::QueuedShowSetupDone, method_factory_.GetWeakPtr()));
}

void AndroidSyncSetupFlowHandler::QueuedShowSetupDone() {
  JNIEnv* env = AttachCurrentThread();
  Java_SyncSetupFlowAdapter_notifySetupDone(
      env, weak_java_sync_setup_adapter_.get(env).obj());
}

void AndroidSyncSetupFlowHandler::SetFlow(SyncSetupFlow* flow) {
  DCHECK(!flow_ || !flow);
  flow_ = flow;
}

void AndroidSyncSetupFlowHandler::Focus() {
  // No need to do anything here - the UI is always already on top.
}

// ----------------------------------------------------------------------------
// Native JNI methods
// ----------------------------------------------------------------------------

// Called by SyncSetupFlowAdapter on the Java side to notify us when the UI has
// been closed.
void AndroidSyncSetupFlowHandler::SyncSetupEnded(JNIEnv* env, jobject obj) {
  delete this;
}

// Called by SyncSetupFlowAdapter to kick off sync setup.
void AndroidSyncSetupFlowHandler::SyncSetupStarted(
    JNIEnv* env, jobject obj, jstring username, jstring auth_token) {
  // If the backend is already initialized, jump right to the CONFIGURE screen,
  // otherwise sit in the GAIA_SUCCESS/SETTING_UP state until the backend
  // finishes initialization.
  bool sync_initialized = service_->sync_initialized();
  if (sync_initialized) {
    service_->get_wizard().Step(SyncSetupWizard::CONFIGURE);
  } else {
    // Need to step through both GAIA_LOGIN and GAIA_SUCCESS state, because
    // SyncSetupFlow won't let us just start out in GAIA_SUCCESS state.
    service_->get_wizard().Step(SyncSetupWizard::GAIA_LOGIN);
    service_->get_wizard().Step(SyncSetupWizard::GAIA_SUCCESS);
  }
  CHECK(service_->get_wizard().AttachSyncSetupHandler(this));
  DCHECK(flow_);

  if (!service_->AreCredentialsAvailable()) {
    // No credentials available yet - provide some, so the backend can
    // initialize.
    Profile* profile =
        g_browser_process->profile_manager()->GetDefaultProfile();
    DCHECK(profile);
    profile->GetPrefs()->SetString(prefs::kGoogleServicesUsername,
                                   ConvertJavaStringToUTF8(env, username));
    profile->GetTokenService()->Initialize(GaiaConstants::kChromeSource,
                                           profile);
    std::string token = ConvertJavaStringToUTF8(env, auth_token);
    // OnIssueAuthTokenSuccess will send out a notification to the sync
    // service that will cause the sync backend to initialize, which will
    // cause our ShowConfigure() callback to be invoked.
    profile->GetTokenService()->OnIssueAuthTokenSuccess(
        browser_sync::SyncServiceName(), token);
    service_->UnsuppressAndStart();
    DCHECK(service_->AreCredentialsAvailable());
  } else {
    if (!sync_initialized) {
      // The backend was not initialized, but we had credentials.
      VLOG(1) << "The backend was not initialized, but we had credentials.";
    }
  }
}


// Called by SyncSetupFlowAdapter on the Java side to pass off the configuration
// settings the user has chosen.
void AndroidSyncSetupFlowHandler::ConfigureSync(
    JNIEnv* env, jobject obj, jboolean sync_everything, jboolean sync_bookmarks,
    jboolean sync_typed_urls, jboolean sync_sessions, jboolean encrypt_all,
    jstring decryption_passphrase, jstring updated_passphrase,
    jboolean updated_passphrase_is_gaia) {
  DCHECK(flow_);
  SyncConfiguration configuration;
  configuration.sync_everything = sync_everything;
  configuration.encrypt_all = encrypt_all;
  configuration.set_secondary_passphrase = false;

  if (!sync_everything) {
    if (sync_bookmarks)
      configuration.data_types.Put(syncable::BOOKMARKS);
    if (sync_typed_urls)
      configuration.data_types.Put(syncable::TYPED_URLS);
    if (sync_sessions)
      configuration.data_types.Put(syncable::SESSIONS);
  }

  // If the caller passed in a decryption passphrase, pass it along.
  if (decryption_passphrase) {
    if (service_->IsUsingSecondaryPassphrase()) {
      configuration.set_secondary_passphrase = true;
      configuration.secondary_passphrase = ConvertJavaStringToUTF8(
          env, decryption_passphrase);
    } else {
      configuration.set_gaia_passphrase = true;
      configuration.gaia_passphrase = ConvertJavaStringToUTF8(
          env, decryption_passphrase);
    }
  }

  // If the caller passed in an updated passphrase (enabling encryption), set
  // it now.
  if (updated_passphrase) {
    if (updated_passphrase_is_gaia) {
      // Can't encrypt with gaia passphrase if we're already encrypted with
      // a secondary passphrase. Also can't encrypt with a gaia passphrase if
      // we're also trying to set a GAIA decryption passphrase.
      DCHECK(!service_->IsUsingSecondaryPassphrase());
      DCHECK(configuration.gaia_passphrase.empty());
      configuration.set_gaia_passphrase = true;
      configuration.gaia_passphrase = ConvertJavaStringToUTF8(
          env, updated_passphrase);
    } else {
      // If the caller passed a secondary passphrase, then turn on secondary
      // passphrase encryption.
      configuration.set_secondary_passphrase = true;
      configuration.secondary_passphrase = ConvertJavaStringToUTF8(
          env, updated_passphrase);
    }
  }
  flow_->OnUserConfigured(configuration);
}


void AndroidSyncSetupFlowHandler::SetDecryptionPassphrase(
    JNIEnv* env, jobject obj, jstring passphrase) {
  flow_->OnPassphraseEntry(ConvertJavaStringToUTF8(env, passphrase));
}

static jint Create(JNIEnv* env, jobject obj) {
  // Create the SyncSetupFlowHandler to interface with the setup wizard.
  AndroidSyncSetupFlowHandler* handler =
      new AndroidSyncSetupFlowHandler(env, obj);
  return reinterpret_cast<jint>(handler);
}

bool RegisterSyncSetupFlowHandler(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
