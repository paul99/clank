// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_TAB_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_TAB_H_

#include "base/android/scoped_java_ref.h"
#include "chrome/browser/sync/glue/synced_tab_delegate.h"
#include "content/browser/android/jni_helper.h"

namespace content {
class WebContents;
}

class Tab {
 public:
  Tab(JNIEnv* env, jobject obj);
  void Destroy(JNIEnv* env, jobject obj);
  base::android::ScopedJavaLocalRef<jbyteArray> GetStateAsByteArray(
      JNIEnv* env, jobject obj, jobject view);
  static Tab* GetNativeTab(JNIEnv* env, jobject obj);
  static content::WebContents* RestoreStateFromByteArray(void* data, int size);
  void SaveTabIdForSessionSync(JNIEnv* env, jobject obj, jobject view);

  // Determine if the thumbnail should be updated for the current url.
  static jboolean ShouldUpdateThumbnail(JNIEnv* env,
                                        jobject obj,
                                        jobject view,
                                        jstring jurl);

  // Update the thumbnail associated with this tab
  void UpdateThumbnail(JNIEnv* env, jobject obj, jobject view, jobject bitmap);

  // Fixes url if it is an about/chrome url and checks whether it should be handled without the
  // usual navigation path, only then forwards it to ChromeView's LoadUrlInternal.
  void LoadUrl(JNIEnv* env, jobject obj, jobject view, jstring jurl,
               int page_transition);


  browser_sync::SyncedTabDelegate* synced_tab_delegate() {
    return synced_tab_delegate_;
  }

  int id() {
    return tab_id_;
  }

 private:
  ~Tab();

  JavaObjectWeakGlobalRef weak_java_tab_;
  browser_sync::SyncedTabDelegate* synced_tab_delegate_; // weak ptr
  int tab_id_;
};

// register the Tab's native methods through jni
bool RegisterTab(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_TAB_H_
