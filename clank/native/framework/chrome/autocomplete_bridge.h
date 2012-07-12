// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_AUTOCOMPLETE_BRIDGE_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_AUTOCOMPLETE_BRIDGE_H_

#include <string>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/autocomplete/autocomplete_controller_delegate.h"
#include "content/browser/android/jni_helper.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"

class AutocompleteController;
class AutocompleteResult;
class ChromeView;
class Profile;

// The native part of the Java AutocompleteBridge class.
class AutocompleteBridge : public AutocompleteControllerDelegate {
 public:
  AutocompleteBridge(JNIEnv* env, jobject obj, Profile* profile);
  void Destroy(JNIEnv* env, jobject obj);

  // Called when the currently shown ChromeView has changed.
  void ChromeViewChanged(JNIEnv* env, jobject, jobject chrome_view);

  // Autocomplete method
  void Start(JNIEnv* env,
             jobject obj,
             jstring text,
             jstring desired_tld,
             bool prevent_inline_autocomplete,
             bool prefer_keyword,
             bool allow_exact_keyword_match,
             bool synchronous_only);

  // Autocomplete method
  void Stop(JNIEnv* env, jobject obj, bool clear_result);

  // Attempts to fully qualify an URL from an inputed search query, |jquery|.
  // If the query does not appear to be a URL, this will return NULL.
  static jstring QualifyPartialURLQuery(
      JNIEnv* env, jclass clazz, jstring jquery);

 private:
  virtual ~AutocompleteBridge();
  void InitJNI(JNIEnv* env, jobject obj);

  // AutocompleteControllerDelegate method:
  virtual void OnResultChanged(bool default_match_changed) OVERRIDE;

  // Notifies the Java LocationBar that suggestions were received based on the
  // text the user typed in last.
  void NotifySuggestionsReceived(
      const AutocompleteResult& autocomplete_result);

  // A weak ref to the profile associated with |autocomplete_controller_|.
  Profile* profile_;

  scoped_ptr<AutocompleteController> autocomplete_controller_;

  JavaObjectWeakGlobalRef weak_java_autocomplete_bridge_;

  base::android::ScopedJavaGlobalRef<jclass> omnibox_suggestion_class_;

  jmethodID omnibox_suggestion_constructor_;

  DISALLOW_COPY_AND_ASSIGN(AutocompleteBridge);
};

// Registers the LocationBar native method.
bool RegisterAutocompleteBridge(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_AUTOCOMPLETE_BRIDGE_H_
