// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_CHROME_BROWSER_PROVIDER_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_CHROME_BROWSER_PROVIDER_H_

#include "base/android/scoped_java_ref.h"
#include "base/memory/scoped_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "chrome/browser/bookmarks/base_bookmark_model_observer.h"
#include "chrome/browser/cancelable_request.h"
#include "chrome/browser/history/android_history_types.h"
#include "content/browser/android/jni_helper.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class AndroidProviderService;
class FaviconService;
class Profile;

namespace history {
class TopSites;
}

namespace sql {
class Statement;
}

// This class implements the native methods of ChromeBrowserProvider.java
class ChromeBrowserProvider : public BaseBookmarkModelObserver,
                              public content::NotificationObserver {
 public:
  ChromeBrowserProvider(JNIEnv* env, jobject obj);
  void Destroy(JNIEnv*, jobject);

  // Adds either a new bookmark or bookmark folder based on |is_folder|.  The
  // bookmark is added to the beginning of the specified parent and if the
  // parent ID is not valid (i.e. < 0) then it will be added to the bookmark
  // bar.
  jlong AddBookmark(JNIEnv* env, jobject,
                    jstring jurl,
                    jstring jtitle,
                    jboolean is_folder,
                    jlong parent_id);

  // Removes a bookmark (or folder) with the specified ID.
  void RemoveBookmark(JNIEnv*, jobject, jlong id);

  // Updates a bookmark (or folder) with the the new title and new URL.
  // The |url| field will be ignored if the bookmark node is a folder.
  // If a valid |parent_id| (>= 0) different from the currently specified
  // parent is given, the node will be moved to that folder as the first
  // child.
  void UpdateBookmark(JNIEnv* env,
                      jobject,
                      jlong id,
                      jstring url,
                      jstring title,
                      jlong parent_id);

  // The below are methods to support Android public API.
  // Bookmark and history APIs. -----------------------------------------------
  jlong AddBookmarkFromAPI(JNIEnv* env,
                           jobject obj,
                           jstring url,
                           jobject created,
                           jobject isBookmark,
                           jobject date,
                           jbyteArray favicon,
                           jstring title,
                           jobject visits,
                           jlong parent_id);

  base::android::ScopedJavaLocalRef<jobject> QueryBookmarkFromAPI(
      JNIEnv* env,
      jobject obj,
      jobjectArray projection,
      jstring selections,
      jobjectArray selection_args,
      jstring sort_order);

  jint UpdateBookmarkFromAPI(JNIEnv* env,
                             jobject obj,
                             jstring url,
                             jobject created,
                             jobject isBookmark,
                             jobject date,
                             jbyteArray favicon,
                             jstring title,
                             jobject visits,
                             jlong parent_id,
                             jstring selections,
                             jobjectArray selection_args);

  jint RemoveBookmarkFromAPI(JNIEnv* env,
                             jobject obj,
                             jstring selections,
                             jobjectArray selection_args);

  jint RemoveHistoryFromAPI(JNIEnv* env,
                            jobject obj,
                            jstring selections,
                            jobjectArray selection_args);

  jlong AddSearchTermFromAPI(JNIEnv* env,
                             jobject obj,
                             jstring search_term,
                             jobject date);

  base::android::ScopedJavaLocalRef<jobject> QuerySearchTermFromAPI(
      JNIEnv* env,
      jobject obj,
      jobjectArray projection,
      jstring selections,
      jobjectArray selection_args,
      jstring sort_order);

  jint RemoveSearchTermFromAPI(JNIEnv* env,
                               jobject obj,
                               jstring selections,
                               jobjectArray selection_args);

  jint UpdateSearchTermFromAPI(JNIEnv* env,
                               jobject obj,
                               jstring search_term,
                               jobject date,
                               jstring selections,
                               jobjectArray selection_args);

  AndroidProviderService* android_provider_service() {
    return service_;
  }

  base::android::ScopedJavaLocalRef<jbyteArray> GetFaviconOrTouchIcon(
      JNIEnv* env,
      jobject obj,
      jstring url);

  base::android::ScopedJavaLocalRef<jbyteArray> GetThumbnail(
      JNIEnv* env,
      jobject obj,
      jstring url);

 private:
  virtual ~ChromeBrowserProvider();

  history::FaviconData GetBookmarkFaviconOrTouchIconInternal(const GURL& url);
  scoped_refptr<RefCountedMemory> GetBookmarkThumbnailInternal(const GURL& url);

  // Add a bookmark. This method is used to support Android public API.
  int64 AddBookmarkFromAPIInternal(const history::BookmarkRow& row);

  history::AndroidStatement* QueryBookmarkFromAPIInternal(
      const std::vector<history::BookmarkRow::BookmarkColumnID>& projections,
      const std::string& selection,
      const std::vector<string16>& selection_args,
      const std::string& sort_order);

  int UpdateBookmarkFromAPIInternal(
      const history::BookmarkRow& row,
      const std::string& selection,
      const std::vector<string16>& selection_args);

  int RemoveBookmarkFromAPIInternal(
      const std::string& selection,
      const std::vector<string16>& selection_args);

  int RemoveHistoryFromAPIInternal(
      const std::string& selection,
      const std::vector<string16>& selection_args);

  // Search APIs.--------------------------------------------------------------
  int64 AddSearchTermFromAPI(const history::SearchRow& row);

  history::AndroidStatement* QuerySearchTermFromAPI(
      const std::vector<history::SearchRow::SearchColumnID>& projections,
      const std::string& selection,
      const std::vector<string16>& selection_args,
      const std::string& sort_order);

  int UpdateSearchTermFromAPI(const history::SearchRow& row,
                              const std::string& selection,
                              const std::vector<string16>& selection_args);

  int RemoveSearchTermFromAPI(const std::string& selection,
                              const std::vector<string16>& selection_args);

  // Override BaseBookmarkModelObserver.
  virtual void BookmarkModelChanged() OVERRIDE;

  // Override NotificationObserver.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Wait for TemplateURLModel being loaded.
  void WaitTemplateURLModelLoaded();

  class JavaBrowserProviderWeakGlobalRef : public JavaObjectWeakGlobalRef {
   public:
    JavaBrowserProviderWeakGlobalRef(JNIEnv* env, jobject obj);

    void OnBookmarkChanged() const;
    void OnSearchTermChanged() const;

   private:
    jmethodID on_bookmark_changed_;
    jmethodID on_search_term_changed_;
  };
  JavaBrowserProviderWeakGlobalRef java_browser_provider_;

  Profile* profile_;
  history::TopSites* top_sites_;

  scoped_refptr<AndroidProviderService> service_;
  scoped_ptr<FaviconService> favicon_service_;

  CancelableRequestConsumer cancelable_consumer_history_;
  CancelableRequestConsumer cancelable_consumer_favicon_;

  // Used to register/unregister notification observer.
  content::NotificationRegistrar notification_registrar_;

  // Signaled if TemplateURLModel has been loaded.
  base::WaitableEvent template_loaded_event_;

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserProvider);
};

// register the Tab's native methods through jni
bool RegisterChromeBrowserProvider(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_CHROME_BROWSER_PROVIDER_H_
