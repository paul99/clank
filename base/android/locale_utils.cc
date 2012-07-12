// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/locale_utils.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/string_util.h"
#include "unicode/uloc.h"

namespace base {
namespace android {

namespace {

std::string GetLocaleComponent(
    const std::string& locale,
    int32_t (*uloc_func)(const char*, char*, int32_t, UErrorCode*),
    int32_t max_capacity) {
  std::string result;
  UErrorCode error = U_ZERO_ERROR;
  int32_t actual_length = uloc_func(locale.c_str(),
                                    WriteInto(&result, max_capacity),
                                    max_capacity,
                                    &error);
  DCHECK(U_SUCCESS(error));
  DCHECK(actual_length < max_capacity);
  result.resize(actual_length);
  return result;
}

ScopedJavaLocalRef<jobject> NewJavaLocale(
    JNIEnv* env,
    ScopedJavaLocalRef<jclass> locale_class,
    jmethodID constructor_id,
    const std::string& locale) {
  std::string language = GetLocaleComponent(
      locale, uloc_getLanguage, ULOC_LANG_CAPACITY);
  std::string country = GetLocaleComponent(
      locale, uloc_getCountry, ULOC_COUNTRY_CAPACITY);
  std::string variant = GetLocaleComponent(
      locale, uloc_getVariant, ULOC_FULLNAME_CAPACITY);
  return ScopedJavaLocalRef<jobject>(
      env, env->NewObject(
          locale_class.obj(), constructor_id,
          ConvertUTF8ToJavaString(env, language).obj(),
          ConvertUTF8ToJavaString(env, country).obj(),
          ConvertUTF8ToJavaString(env, variant).obj()));
}

string16 GetDisplayScript(const std::string& locale,
                          const std::string& display_locale) {
  const int kBufferSize = 1024;
  string16 result;
  UErrorCode error = U_ZERO_ERROR;
  int32_t actual_length = uloc_getDisplayScript(locale.c_str(),
                                                display_locale.c_str(),
                                                WriteInto(&result, kBufferSize),
                                                kBufferSize,
                                                &error);
  DCHECK(U_SUCCESS(error));
  DCHECK(actual_length < kBufferSize);
  result.resize(actual_length);
  return result;
}

}  // namespace

string16 GetDisplayNameForLocale(const std::string& locale,
                                 const std::string& display_locale) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jclass> locale_class = GetClass(env, "java/util/Locale");
  jmethodID constructor_id = GetMethodID(
      env, locale_class, "<init>",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

  // GetDisplayNameForLocale() in l10n_utils.cc converts zh-CN and zh-TW into
  // zh-Hans and zh-Hant respectively expecting to get better display name for
  // Chinese locales. However, the Java Locale class doesn't support scripts,
  // so we get the display names of the scripts later from ICU.
  // We have display names of Hans and Hant in our version of ICU data.
  bool is_zh = locale == "zh-Hans" || locale == "zh-Hant";
  ScopedJavaLocalRef<jobject> java_locale = NewJavaLocale(
      env, locale_class, constructor_id, is_zh ? "zh" : locale);
  ScopedJavaLocalRef<jobject> java_display_locale = NewJavaLocale(
      env, locale_class, constructor_id, display_locale);

  jmethodID method_id = GetMethodID(env, locale_class, "getDisplayName",
                                    "(Ljava/util/Locale;)Ljava/lang/String;");
  ScopedJavaLocalRef<jstring> java_result(
      env,
      static_cast<jstring>(env->CallObjectMethod(java_locale.obj(), method_id,
                                                 java_display_locale.obj())));
  string16 result = ConvertJavaStringToUTF16(java_result);

  if (is_zh) {
    result.append(1, ' ');
    result.append(1, '(');
    result.append(GetDisplayScript(locale, display_locale));
    result.append(1, ')');
  }

  return result;
}

}  // namespace android
}  // namespace base
