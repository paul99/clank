// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_CHROME_NATIVE_PREFERENCES_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_CHROME_NATIVE_PREFERENCES_H_
#pragma once

#include "content/browser/android/jni_helper.h"
#include "chrome/browser/autofill/personal_data_manager.h"
#include "chrome/browser/autofill/personal_data_manager_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

/**
 * PersonalDataManager Observer to let the java side know when autofill
 * profile/credit card create/edit/delete has changed.
 */
class AutofillPersonalDataManagerObserver : public PersonalDataManagerObserver {
 public:
  AutofillPersonalDataManagerObserver(JNIEnv* env, jobject obj);
  virtual void OnPersonalDataChanged() OVERRIDE;
  void DestroyPersonalDataManagerObserver(JNIEnv* env, jobject obj);

 private:
  virtual ~AutofillPersonalDataManagerObserver();
  JavaObjectWeakGlobalRef weak_chrome_native_preferences_;
};

/**
 * TemplateURLServiceLoadedObserver Observer to let the java side know when template url service
 * has completed loading.
 */
class TemplateURLServiceLoadedObserver : public content::NotificationObserver {
 public:
  TemplateURLServiceLoadedObserver(JNIEnv* env, jobject obj);
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;
  void DestroyTemplateURLServiceLoadedObserver(JNIEnv* env, jobject obj);

 private:
  virtual ~TemplateURLServiceLoadedObserver();
  JavaObjectWeakGlobalRef weak_chrome_native_preferences_;
  content::NotificationRegistrar registrar_;
};

bool RegisterChromeNativePreferences(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_CHROME_NATIVE_PREFERENCES_H_
