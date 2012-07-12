// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_INFOBAR_ANDROID_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_INFOBAR_ANDROID_H_
#pragma once

#include <map>

#include "base/android/scoped_java_ref.h"
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "content/browser/android/jni_helper.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class InfoBar;
class InfoBarDelegate;
class TabContentsWrapper;

namespace content {
class WebContents;
}

// This is the native side of InfoBarContainer.java.
// It observes infobar notifications from the chromium side and dispatches to
// the java side for handling the UI, and also provides an API over jni to
// notify back the chromium side of user actions.
class InfoBarContainerAndroid : public content::NotificationObserver {
 public:
  InfoBarContainerAndroid(JNIEnv* env,
                          jobject infobar_container,
                          content::WebContents* web_contents);
  void Destroy(JNIEnv* env, jobject obj);

  content::WebContents* web_contents() const { return web_contents_; }

 private:
  virtual ~InfoBarContainerAndroid();

  // Overridden from NotificationObserver:
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Adds an InfoBar for the specified delegate, in response to a notification
  // from the selected TabContents.
  void AddInfoBar(InfoBarDelegate* delegate);

  // Type-specific helpers for AddInfoBar().
  void AddConfirmInfoBar(InfoBarDelegate* delegate);
  void AddAutoLoginInfoBar(InfoBarDelegate* delegate);

  // Removes an InfoBar for the specified delegate, in response to a
  // notification from the selected TabContents.
  void RemoveInfoBar(InfoBarDelegate* delegate);

  // Registers the infobar notifications for |tab_contents|.
  void RegisterTabContentsNotifications(content::WebContents* tab_contents);

  // Unregisters the infobar notifications for |tab_contents|.
  void UnregisterTabContentsNotifications(content::WebContents* tab_contents);

  // Remove all pending infobars from |tab_contents|.
  void RemovePendingInfoBars(TabContentsWrapper* tab_contents_wrapper);

  // Creates all existing infobars for the |tab_contents|.
  void UpdateInfoBars(TabContentsWrapper* tab_contents_wrapper);

  // We received a notification that the tab contents is being replaced,
  // cleanup any pending infobar.
  void OnTabContentsReplaced(content::WebContents* old_tab_contents,
                             content::WebContents* new_tab_contents);

  // The tab contents we are associated with.
  content::WebContents* web_contents_;

  // We're owned by the java infobar, need to use a weak ref so it can destroy
  // us.
  JavaObjectWeakGlobalRef weak_java_infobar_container_;

  content::NotificationRegistrar registrar_;

  typedef std::map<InfoBarDelegate*, InfoBar*> DelegateToInfoBarMap;
  DelegateToInfoBarMap delegate_to_info_bar_map_;

  DISALLOW_COPY_AND_ASSIGN(InfoBarContainerAndroid);
};

// This class is forward-declared in infobar_delegate.h. It provides
// a bridge between cross-platform InfoBarDelegate and the platform-specific
// code (in our case, implemented by InfoBarContainer.java).
class InfoBar {
 public:
  explicit InfoBar(InfoBarDelegate* delegate);
  ~InfoBar();

  void set_java_infobar(const base::android::JavaRef<jobject>& java_info_bar);
  void set_infobar_container(InfoBarContainerAndroid* infobar_container);

  void OnConfirmClicked(JNIEnv* env, jobject obj, bool confirm);
  base::android::ScopedJavaLocalRef<jstring> GetAutoLoginMessage(
      JNIEnv* env, jobject obj, jstring jaccount);
  void OnAutoLogin(JNIEnv* env, jobject obj, jstring url);
  void OnInfoBarClosed(JNIEnv* env, jobject obj);
  // |remove_from_tab_contents_wrapper| should be set to true when the closing
  // of the infobar is initiated from Java.
  void Close(bool remove_from_tab_contents_wrapper);

 private:
  InfoBarContainerAndroid* container_;
  // The InfoBar's delegate.
  InfoBarDelegate* delegate_;
  base::android::ScopedJavaGlobalRef<jobject> java_info_bar_;
  // True when the infobar is being closed.
  bool closing_;

  DISALLOW_COPY_AND_ASSIGN(InfoBar);
};

// Register the InfoBarContainer's native methods through jni
bool RegisterInfoBarContainer(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_INFOBAR_ANDROID_H_
