// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_SRC_INSTANT_TAB_H_
#define CLANK_NATIVE_FRAMEWORK_SRC_INSTANT_TAB_H_

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/android/jni_helper.h"

class InstantLoader;
class TabContentsWrapper;

// C++ side of a Java InstantTab.
// An InstantTab is the same as a Tab but has the capacity to prefetch.
class InstantTab {
 public:
  InstantTab(JNIEnv* env,
             jobject obj,
             jobject to_replace,
             jobject my_chrome_view);
  ~InstantTab();

  void Destroy(JNIEnv* env, jobject obj);
  bool PrefetchUrl(JNIEnv* env, jobject obj, jstring jurl, jint page_transition);
  jboolean CommitPrefetch(JNIEnv* env, jobject obj);
  jboolean PrefetchCanceled(JNIEnv* env, jobject obj);

  // Called from our instant loader delegate to tell the ChromeView
  // which TabContents it now refers to.  The ChromeView places a
  // pointer to itself within a TabContentsViewAndroid; if the pointer
  // is not there, certain ops which need to walk up to java
  // (e.g. infobars) won't work.
  void TabContentsCreated(TabContentsWrapper* tab_contents_wrapper);

  // Called from our instant loader delegate when prefetching fails.
  void DiscardPrefetch();

 private:
  class InstantLoaderDelegateImpl;

  // Instanciates an InstantLoader if none is available.
  void InitInstantLoader();

  bool committed_;  // Have we committed this prefetch?

  // Whether the current prerender has been canceled (when the page returned 403
  // for example).
  bool canceled_;

  JavaObjectWeakGlobalRef java_instant_tab_;
  JavaObjectWeakGlobalRef to_replace_jchrome_view_;
  JavaObjectWeakGlobalRef my_jchrome_view_;

  scoped_ptr<InstantLoader> instant_loader_;
  scoped_ptr<InstantLoaderDelegateImpl> instant_loader_delegate_;

  // A method factory used to delay discarding the InstantLoader.
  base::WeakPtrFactory<InstantTab> weak_instant_method_factory_;
};

bool RegisterInstantTab(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_SRC_INSTANT_TAB_H_
