// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/web_settings.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "content/browser/android/chrome_view.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/prefs/prefs_tab_helper.h"
#include "chrome/common/pref_names.h"
#include "jni/web_settings_jni.h"

using base::android::GetClass;
using base::android::GetFieldID;
using base::android::GetMethodIDFromClassName;
using base::android::ConvertJavaStringToUTF8;
using base::android::ScopedJavaLocalRef;

struct WebSettings::FieldIds {
  FieldIds() { }

  FieldIds(JNIEnv* env) {
    const char* kStringClassName = "Ljava/lang/String;";

    // FIXME: we should be using a new GetFieldIDFromClassName() with caching.
    ScopedJavaLocalRef<jclass> clazz(
        GetClass(env, "org/chromium/content/browser/WebSettings"));
    standard_fond_family =
        GetFieldID(env, clazz, "mStandardFontFamily", kStringClassName);
    fixed_font_family =
        GetFieldID(env, clazz, "mFixedFontFamily", kStringClassName);
    sans_serif_font_family =
        GetFieldID(env, clazz, "mSansSerifFontFamily", kStringClassName);
    serif_font_family =
        GetFieldID(env, clazz, "mSerifFontFamily", kStringClassName);
    cursive_font_family =
        GetFieldID(env, clazz, "mCursiveFontFamily", kStringClassName);
    fantasy_font_family =
        GetFieldID(env, clazz, "mFantasyFontFamily", kStringClassName);
    default_text_encoding =
        GetFieldID(env, clazz, "mDefaultTextEncoding", kStringClassName);
    minimum_font_size = GetFieldID(env, clazz, "mMinimumFontSize", "I");
    minimum_logical_font_size =
        GetFieldID(env, clazz, "mMinimumLogicalFontSize", "I");
    default_font_size = GetFieldID(env, clazz, "mDefaultFontSize", "I");
    default_fixed_font_size =
        GetFieldID(env, clazz, "mDefaultFixedFontSize", "I");
    load_images_automatically =
        GetFieldID(env, clazz, "mLoadsImagesAutomatically", "Z");
    java_script_enabled =
        GetFieldID(env, clazz, "mJavaScriptEnabled", "Z");
    java_script_can_open_windows_automatically =
        GetFieldID(env, clazz, "mJavaScriptCanOpenWindowsAutomatically", "Z");
  }

  // Field ids
  jfieldID standard_fond_family;
  jfieldID fixed_font_family;
  jfieldID sans_serif_font_family;
  jfieldID serif_font_family;
  jfieldID cursive_font_family;
  jfieldID fantasy_font_family;
  jfieldID default_text_encoding;
  jfieldID minimum_font_size;
  jfieldID minimum_logical_font_size;
  jfieldID default_font_size;
  jfieldID default_fixed_font_size;
  jfieldID load_images_automatically;
  jfieldID java_script_enabled;
  jfieldID java_script_can_open_windows_automatically;
};

WebSettings::WebSettings(JNIEnv* env, jobject obj) {
}

WebSettings::~WebSettings() {
}

void WebSettings::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

void WebSettings::InitializeFieldIds(JNIEnv* env) {
  field_ids_.reset(new FieldIds(env));
}

void WebSettings::Sync(JNIEnv* env, jobject obj, jint nativeChromeView) {
  if (!field_ids_.get()) {
    InitializeFieldIds(env);
  }
  // Note on speed. One may think that an approach that reads field values via
  // JNI is ineffective and should not be used. Please keep in mind that in the
  // legacy WebView the whole Sync method took <1ms on Xoom, and no one is
  // expected to modify settings in performance-critical code.
  ChromeView* chrome_view = reinterpret_cast<ChromeView*>(nativeChromeView);
  PrefService* per_tab_prefs =
      chrome_view->tab_contents_wrapper()->prefs_tab_helper()->per_tab_prefs();

  ScopedJavaLocalRef<jstring> str(
      env, static_cast<jstring>(
          env->GetObjectField(obj, field_ids_->standard_fond_family)));
  per_tab_prefs->SetString(prefs::kWebKitStandardFontFamily,
                           ConvertJavaStringToUTF8(str));

  str.Reset(
      env, static_cast<jstring>(
          env->GetObjectField(obj, field_ids_->fixed_font_family)));
  per_tab_prefs->SetString(prefs::kWebKitFixedFontFamily,
                           ConvertJavaStringToUTF8(str));

  str.Reset(
      env, static_cast<jstring>(
          env->GetObjectField(obj, field_ids_->sans_serif_font_family)));
  per_tab_prefs->SetString(prefs::kWebKitSansSerifFontFamily,
                           ConvertJavaStringToUTF8(str));

  str.Reset(
      env, static_cast<jstring>(
          env->GetObjectField(obj, field_ids_->serif_font_family)));
  per_tab_prefs->SetString(prefs::kWebKitSerifFontFamily,
                           ConvertJavaStringToUTF8(str));

  str.Reset(
      env, static_cast<jstring>(
          env->GetObjectField(obj, field_ids_->cursive_font_family)));
  per_tab_prefs->SetString(prefs::kWebKitCursiveFontFamily,
                           ConvertJavaStringToUTF8(str));

  str.Reset(
      env, static_cast<jstring>(
          env->GetObjectField(obj, field_ids_->fantasy_font_family)));
  per_tab_prefs->SetString(prefs::kWebKitFantasyFontFamily,
                           ConvertJavaStringToUTF8(str));

  str.Reset(
      env, static_cast<jstring>(
          env->GetObjectField(obj, field_ids_->default_text_encoding)));
  per_tab_prefs->SetString(prefs::kDefaultCharset,
                            ConvertJavaStringToUTF8(str));

  jint size = env->GetIntField(obj, field_ids_->minimum_font_size);
  per_tab_prefs->SetInteger(prefs::kWebKitMinimumFontSize, size);

  size = env->GetIntField(obj, field_ids_->minimum_logical_font_size);
  per_tab_prefs->SetInteger(prefs::kWebKitMinimumLogicalFontSize, size);

  size = env->GetIntField(obj, field_ids_->default_font_size);
  per_tab_prefs->SetInteger(prefs::kWebKitDefaultFontSize, size);

  size = env->GetIntField(obj, field_ids_->default_fixed_font_size);
  per_tab_prefs->SetInteger(prefs::kWebKitDefaultFixedFontSize, size);

  jboolean flag = env->GetBooleanField(obj, field_ids_->load_images_automatically);
  per_tab_prefs->SetBoolean(prefs::kWebKitLoadsImagesAutomatically, flag);
  // Using kWebKitLoadsImagesAutomatically alone isn't enough, as it doesn't
  // affect Chrome's resource loaders.
  per_tab_prefs->SetBoolean(prefs::kWebKitImagesEnabled, flag);

  flag = env->GetBooleanField(obj, field_ids_->java_script_enabled);
  per_tab_prefs->SetBoolean(prefs::kWebKitJavascriptEnabled, flag);

  flag = env->GetBooleanField(obj, field_ids_->java_script_can_open_windows_automatically);
  per_tab_prefs->SetBoolean(
      prefs::kWebKitJavascriptCanOpenWindowsAutomatically, flag);

  flag = Java_WebSettings_getPluginsDisabled(env, obj);
  per_tab_prefs->SetBoolean(prefs::kWebKitPluginsEnabled, !flag);
}

static jint Init(JNIEnv* env, jobject obj) {
  WebSettings* web_settings = new WebSettings(env, obj);
  return reinterpret_cast<jint>(web_settings);
}

// Register native methods.

bool RegisterWebSettings(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
