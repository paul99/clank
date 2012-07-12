// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_ANDROID_SYNC_SETUP_FLOW_HANDLER_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_ANDROID_SYNC_SETUP_FLOW_HANDLER_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "chrome/browser/sync/sync_setup_flow_handler.h"
#include "content/browser/android/jni_helper.h"

class ProfileSyncService;
class SyncSetupFlow;

// Provides an interface between the Android sync setup UI and the sync engine.
// Many of the wizard states (such as those related to GAIA) should never be
// encountered on Android and so will not be implemented.
class AndroidSyncSetupFlowHandler : public SyncSetupFlowHandler {
 public:
  // Constructor. Automatically attaches this handler to the SyncSetupWizard and
  // the passed java-side |adapter|.
  AndroidSyncSetupFlowHandler(JNIEnv* env,
                              jobject adapter);
  virtual ~AndroidSyncSetupFlowHandler();

  // Called from Java to kick off sync setup, using the passed |username| and
  // |auth_token| for authentication.
  void SyncSetupStarted(JNIEnv* env, jobject obj, jstring username,
                        jstring auth_token);
  // Called from Java when the associated |adapter| is shutdown.
  void SyncSetupEnded(JNIEnv* env, jobject obj);

  // Called from Java to configure the sync engine.
  void ConfigureSync(JNIEnv* env, jobject obj, jboolean sync_everything,
                     jboolean sync_bookmarks, jboolean sync_typed_urls,
                     jboolean sync_sessions, jboolean encrypt_all,
                     jstring decryption_passphrase, jstring updated_passphrase,
                     jboolean updated_passphrase_is_gaia);

  // Called from Java to provide the user's passphrase in response to
  // ShowPassphraseEntry().
  void SetDecryptionPassphrase(JNIEnv* env, jobject obj, jstring passphrase);

  // Implementation of SyncSetupFlowHandler.
  virtual void ShowOAuthLogin() OVERRIDE;
  virtual void ShowGaiaLogin(const DictionaryValue& args) OVERRIDE;
  virtual void ShowGaiaSuccessAndClose() OVERRIDE;
  virtual void ShowGaiaSuccessAndSettingUp() OVERRIDE;
  virtual void ShowConfigure(const DictionaryValue& args) OVERRIDE;
  virtual void ShowPassphraseEntry(const DictionaryValue& args) OVERRIDE;
  virtual void ShowSettingUp() OVERRIDE;
  virtual void ShowSetupDone(const string16& user) OVERRIDE;
  virtual void SetFlow(SyncSetupFlow* flow) OVERRIDE;
  virtual void Focus() OVERRIDE;

 private:
  // Invoked via a Callback to notify the java layer that we're done with
  // sync setup.
  void QueuedShowSetupDone();

  // We can't immediately process ShowSetupDone() invocations, because they can
  // result in this object getting freed in the middle of a running callstack.
  // We use a WeakPtrFactory to queue up invocations to ourselves
  // to handle ShowSetupDone() when we know there are no active calls in
  // progress.
  base::WeakPtrFactory<AndroidSyncSetupFlowHandler> method_factory_;

  ProfileSyncService* service_;

  // Flow object we are interacting with (this is the object we notify when the
  // UI is closed so we can cancel any sync setup in progress).
  SyncSetupFlow* flow_;

  // Encapsulation of the java-side adapter object.
  JavaObjectWeakGlobalRef weak_java_sync_setup_adapter_;
};

// register this classes' native methods through jni.
bool RegisterSyncSetupFlowHandler(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_ANDROID_SYNC_SETUP_FLOW_HANDLER_H_
