// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_LOCATION_BAR_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_LOCATION_BAR_H_

#include <string>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "content/browser/android/jni_helper.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"

class ChromeView;
class Profile;

// The native part of the Java LocationBar class.
// Note that there should only be one instance of this class whose lifecycle
// is managed from the Java side.
class LocationBar : public content::NotificationObserver {
 public:
  // TODO(jcivelli): this constructor should not take a profile, it should get
  //                 it from the current tab (and update the profile on the
  //                 autocomplete_controller_ when the user switches tab).
  LocationBar(JNIEnv* env, jobject obj, Profile* profile);
  void Destroy(JNIEnv* env, jobject obj);

  // Called by the Java code when the user clicked on the security button.
  void OnSecurityButtonClicked(JNIEnv*, jobject, jobject context);

  // Called to get the SSL security level of current tab.
  int GetSecurityLevel(JNIEnv*, jobject);

 private:
  virtual ~LocationBar();

  // NotificationObserver method:
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  void OnSSLVisibleStateChanged(const content::NotificationSource& source,
                                const content::NotificationDetails& details);

  void OnUrlsStarred(const content::NotificationSource& source,
                     const content::NotificationDetails& details);

  JavaObjectWeakGlobalRef weak_java_location_bar_;

  content::NotificationRegistrar notification_registrar_;

  DISALLOW_COPY_AND_ASSIGN(LocationBar);
};

// Registers the LocationBar native method.
bool RegisterLocationBar(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_LOCATION_BAR_H_
