// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/autocomplete_bridge.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/string16.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/autocomplete/autocomplete.h"
#include "chrome/browser/autocomplete/autocomplete_classifier.h"
#include "chrome/browser/autocomplete/autocomplete_match.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/toolbar/toolbar_model.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "clank/native/framework/chrome/page_info_viewer.h"
#include "content/browser/android/chrome_view.h"
#include "jni/autocomplete_controller_jni.h"
#include "net/base/registry_controlled_domain.h"
#include "ui/base/resource/resource_bundle.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ClearException;
using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertUTF16ToJavaString;
using base::android::GetClass;
using base::android::GetMethodID;

namespace {

const char* const kOmniboxSuggestionClassName =
    "com/google/android/apps/chrome/OmniboxSuggestion";

}  // namespace

AutocompleteBridge::AutocompleteBridge(JNIEnv* env, jobject obj, Profile* profile)
    : profile_(profile),
      autocomplete_controller_(new AutocompleteController(profile, this)),
      weak_java_autocomplete_bridge_(env, obj) {
  InitJNI(env, obj);
}

AutocompleteBridge::~AutocompleteBridge() {
}

void AutocompleteBridge::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

void AutocompleteBridge::InitJNI(JNIEnv* env, jobject obj) {
  omnibox_suggestion_class_.Reset(GetClass(env, kOmniboxSuggestionClassName));
  omnibox_suggestion_constructor_ =
      GetMethodID(env, omnibox_suggestion_class_, "<init>",
                  "(IIILjava/lang/String;Ljava/lang/String;"
                  "Ljava/lang/String;Z)V");
}

// AutocompleteControllerDelegate implementation:
void AutocompleteBridge::OnResultChanged(bool default_match_changed) {
  if (autocomplete_controller_ != NULL)
    NotifySuggestionsReceived(autocomplete_controller_->result());
}

void AutocompleteBridge::Start(JNIEnv* env,
                               jobject obj,
                               jstring j_text,
                               jstring j_desired_tld,
                               bool prevent_inline_autocomplete,
                               bool prefer_keyword,
                               bool allow_exact_keyword_match,
                               bool synchronous_only) {
  if (autocomplete_controller_ == NULL)
    return;

  string16 desired_tld;
  if (j_desired_tld != NULL)
    desired_tld = ConvertJavaStringToUTF16(env, j_desired_tld);
  string16 text = ConvertJavaStringToUTF16(env, j_text);
  AutocompleteInput::MatchesRequested matches = AutocompleteInput::ALL_MATCHES;
  if (synchronous_only)
    matches = AutocompleteInput::SYNCHRONOUS_MATCHES;
  autocomplete_controller_->Start(
      text, desired_tld, prevent_inline_autocomplete, prefer_keyword,
      allow_exact_keyword_match, matches);
}

void AutocompleteBridge::ChromeViewChanged(JNIEnv* env,
                                           jobject obj,
                                           jobject view) {
  if (view == NULL) {
    profile_ = g_browser_process->profile_manager()->GetDefaultProfile();
    autocomplete_controller_.reset();
  } else {
    ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
    DCHECK(chrome_view);
    Profile* currentProfile = chrome_view->GetProfile();
    if (!currentProfile->IsSameProfile(profile_) ||
        currentProfile->IsOffTheRecord() != profile_->IsOffTheRecord() ||
        autocomplete_controller_ == NULL) {
      autocomplete_controller_.reset(
          new AutocompleteController(currentProfile, this));
      profile_ = currentProfile;
    }
  }
}

void AutocompleteBridge::Stop(JNIEnv* env,
                              jobject obj,
                              bool clear_results) {
  if (autocomplete_controller_ != NULL)
    autocomplete_controller_->Stop(clear_results);
}

void AutocompleteBridge::NotifySuggestionsReceived(
    const AutocompleteResult& autocomplete_result) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobjectArray> suggestions(env,
      env->NewObjectArray(autocomplete_result.size(),
                          omnibox_suggestion_class_.obj(), NULL));
  for (size_t i = 0; i < autocomplete_result.size(); ++i) {
    const AutocompleteMatch& match = autocomplete_result.match_at(i);
    ScopedJavaLocalRef<jstring> contents =
        ConvertUTF16ToJavaString(env, match.contents);
    ScopedJavaLocalRef<jstring> description =
        ConvertUTF16ToJavaString(env, match.description);
    ScopedJavaLocalRef<jstring> url =
        ConvertUTF8ToJavaString(env, match.destination_url.spec());
    ScopedJavaLocalRef<jobject> suggestion(env,
        env->NewObject(omnibox_suggestion_class_.obj(),
                       omnibox_suggestion_constructor_,
                       match.type, match.relevance,
                       match.transition,
                       contents.obj(), description.obj(), url.obj(),
                       match.starred));
    env->SetObjectArrayElement(suggestions.obj(), i, suggestion.obj());
    CheckException(env);
  }

  // Get the inline-autocomplete text.
  const AutocompleteResult::const_iterator default_match(
      autocomplete_result.default_match());
  string16 inline_autocomplete_text;
  if (default_match != autocomplete_result.end()) {
    if ((default_match->inline_autocomplete_offset != string16::npos) &&
          (default_match->inline_autocomplete_offset <
           default_match->fill_into_edit.length())) {
      inline_autocomplete_text =
            default_match->fill_into_edit.substr(
                default_match->inline_autocomplete_offset);
    }
  }
  ScopedJavaLocalRef<jstring> inline_text =
      ConvertUTF16ToJavaString(env, inline_autocomplete_text);
  Java_AutocompleteController_onSuggestionsReceived(env,
      weak_java_autocomplete_bridge_.get(env).obj(), suggestions.obj(),
      inline_text.obj());

  if (ClearException(env))
    DLOG(ERROR) << "Exception while notifying of suggestions";
}

static int Init(JNIEnv* env, jobject obj, jobject view) {
  Profile* profile;
  if (view == NULL) {
    profile = g_browser_process->profile_manager()->GetDefaultProfile();
  } else {
    ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
    DCHECK(chrome_view);
    profile = chrome_view->GetProfile();
  }
  DCHECK(profile);
  AutocompleteBridge* native_bridge = new AutocompleteBridge(env, obj, profile);
  return reinterpret_cast<jint>(native_bridge);
}

static jstring QualifyPartialURLQuery(
    JNIEnv* env, jclass clazz, jstring jquery) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  if (!profile)
    return NULL;
  AutocompleteMatch match;
  string16 query_string(
      base::android::ConvertJavaStringToUTF16(env, jquery));
  profile->GetAutocompleteClassifier()->Classify(
      query_string, string16(), false, false, &match, NULL);
  if (!match.destination_url.is_valid())
    return NULL;

  // Only return a URL if the match is a URL type.
  if (match.type != AutocompleteMatch::URL_WHAT_YOU_TYPED &&
      match.type != AutocompleteMatch::HISTORY_URL &&
      match.type != AutocompleteMatch::NAVSUGGEST)
    return NULL;

  // As we are returning to Java, it is fine to call Release().
  return base::android::ConvertUTF8ToJavaString(
      env, match.destination_url.spec()).Release();
}

// Register native methods
bool RegisterAutocompleteBridge(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
