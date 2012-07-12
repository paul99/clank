// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/chrome_browser_provider.h"

#include <list>
#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/logging.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/history/android_history_types.h"
#include "chrome/browser/history/top_sites.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/search_engines/template_url_service.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/common/chrome_notification_types.h"
#include "clank/native/framework/chrome/sqlite_cursor.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "grit/generated_resources.h"
#include "jni/chrome_browser_provider_jni.h"
#include "sql/statement.h"
#include "ui/base/l10n/l10n_util.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ClearException;
using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertUTF16ToJavaString;
using base::android::GetClass;
using base::android::GetMethodID;
using base::android::JavaRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;

namespace {
const char kClassPathName[] = "com/google/android/apps/chrome/ChromeBrowserProvider";
const char kDefaultUrlScheme[] = "http://";
const int64 kInvalidBookmarkNodeId = -1;

// Convert a BookmarkNode, |node|, to the java representation of a bookmark node
// stored in |*jnode|. Parent node information is optional.
void ConvertBookmarkNode(
    const BookmarkNode* node,
    const JavaRef<jobject>& parent_node,
    ScopedJavaGlobalRef<jobject>* jnode) {
  DCHECK(jnode);
  if (!node)
    return;

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> url;
  if (node->is_url())
    url.Reset(ConvertUTF8ToJavaString(env, node->url().spec()));
  ScopedJavaLocalRef<jstring> title(
      ConvertUTF16ToJavaString(env, node->GetTitle()));

  jnode->Reset(
      Java_BookmarkNode_create(
          env, node->id(), (jint) node->type(), title.obj(), url.obj(),
          parent_node.obj()));
}

// Parse the given url and return a GURL, appending the default scheme
// if one is not present.
GURL ParseAndMaybeAppendScheme(const string16& url,
                               const char* default_scheme) {
  GURL gurl(url);
  if (!gurl.is_valid() && !gurl.has_scheme()) {
    string16 refined_url(ASCIIToUTF16(default_scheme));
    refined_url.append(url);
    gurl = GURL(refined_url);
  }
  return gurl;
}

// Utility method to add a bookmark. It could be called directly if the caller
// is already in UI thread.
int64 AddBookmarkTask(const string16& title,
                      const string16& url,
                      const bool is_folder,
                      const int64 parent_id) {
  Profile* profile = g_browser_process->profile_manager()->GetLastUsedProfile();
  BookmarkModel* bookmark_model = profile->GetBookmarkModel();
  GURL gurl = ParseAndMaybeAppendScheme(url, kDefaultUrlScheme);

  // Check if the bookmark already exists
  const BookmarkNode* node =
      bookmark_model->GetMostRecentlyAddedNodeForURL(gurl);
  if (!node) {
    const BookmarkNode* parent_node = NULL;
    if (parent_id >= 0) {
      parent_node = bookmark_model->GetNodeByID(parent_id);
    }
    if (!parent_node) {
      parent_node = bookmark_model->bookmark_bar_node();
    }
    if (is_folder) {
      node = bookmark_model->AddFolder(
          parent_node, parent_node->child_count(), title);
    } else {
      node = bookmark_model->AddURL(parent_node, 0, title, gurl);
    }
  }
  if (node) {
    return node->id();
  } else {
    return 0L;
  }
}

// The callback is used to add a bookmark in UI thread.
void AddBookmarkCallback(const string16& title,
                         const string16& url,
                         const bool is_folder,
                         const int64 parent_id,
                         base::WaitableEvent* bookmark_added_event,
                         int64* result) {
  int64 id = AddBookmarkTask(title, url, is_folder, parent_id);
  if (result) {
    *result = id;
  }
  if (bookmark_added_event) {
    bookmark_added_event->Signal();
  }
}

// Utility method to remove a bookmark. It could be called directly if the
// caller is already in UI thread.
void RemoveBookmarkTask(const int64 id) {
  Profile* profile = g_browser_process->profile_manager()->GetLastUsedProfile();
  BookmarkModel* bookmark_model = profile->GetBookmarkModel();
  const BookmarkNode* node = bookmark_model->GetNodeByID(id);
  if (node && node->parent()) {
    const BookmarkNode* parent_node = node->parent();
    bookmark_model->Remove(parent_node, parent_node->GetIndexOf(node));
  }
}

// The callback is used to remove a bookmark in UI thread.
void RemoveBookmarkCallback(const int64 bookmarkId,
                            base::WaitableEvent* bookmark_removed_event) {
  RemoveBookmarkTask(bookmarkId);
  if (bookmark_removed_event) {
    bookmark_removed_event->Signal();
  }
}

// Utility method to update a bookmark. It could be called directly if the
// caller is already in UI thread.
void UpdateBookmarkTask(const int64 id,
                        const string16& title,
                        const string16& url,
                        const int64 parent_id) {
  Profile* profile = g_browser_process->profile_manager()->GetLastUsedProfile();
  BookmarkModel* bookmark_model = profile->GetBookmarkModel();
  const BookmarkNode* node = bookmark_model->GetNodeByID(id);
  if (node) {
    if (node->GetTitle() != title) {
      bookmark_model->SetTitle(node, title);
    }
    if (node->type() == BookmarkNode::URL) {
      GURL bookmark_url = ParseAndMaybeAppendScheme(url, kDefaultUrlScheme);
      if (bookmark_url != node->url()) {
        bookmark_model->SetURL(node, bookmark_url);
      }
    }
    if (parent_id >= 0 &&
        (!node->parent() || parent_id != node->parent()->id())) {
      const BookmarkNode* new_parent = bookmark_model->GetNodeByID(parent_id);

      if (new_parent) {
        bookmark_model->Move(node, new_parent, 0);
      }
    }
  }
}

// The callback is used to update a bookmark in UI thread.
void UpdateBookmarkCallback(const int64 bookmarkId,
                            const string16& title,
                            const string16& url,
                            const int64 parent_id,
                            base::WaitableEvent* bookmark_updated_event) {
  UpdateBookmarkTask(bookmarkId, title, url, parent_id);
  if (bookmark_updated_event) {
    bookmark_updated_event->Signal();
  }
}

// Utility method to create the bookmark folder. It could be called directly
// if the caller is already in UI thread.
int64 CreateBookmarksFolderOnceTask(const string16& title, int64 parent_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  if (!profile_manager)
    return kInvalidBookmarkNodeId;

  Profile* profile = profile_manager->GetLastUsedProfile();
  BookmarkModel* bookmark_model = profile->GetBookmarkModel();
  if (!bookmark_model->IsLoaded())
    return kInvalidBookmarkNodeId;

  const BookmarkNode* mobile_node = bookmark_model->mobile_node();
  const BookmarkNode* parent = parent_id > 0 ?
      bookmark_model->GetNodeByID(parent_id) : mobile_node;

  // Ensure that parent_id is a descendant of the mobile node.
  bool mobile_node_is_ancestor = false;
  for (const BookmarkNode* ancestor = parent; ancestor;
      ancestor = ancestor->parent()) {
    if (ancestor == mobile_node) {
      mobile_node_is_ancestor = true;
      break;
    }
  }

  if (!mobile_node_is_ancestor)
    return kInvalidBookmarkNodeId;

  DCHECK(parent);
  const BookmarkNode* node = parent->GetChildFolderByTitle(title);
  if (node)
    return node->id();

  // Create the Import Bookmarks folder in the UI thread.
  return AddBookmarkTask(title, string16(), true, parent->id());
}

// Callback used to create bookmark folders in the UI thread.
// If the folder already exists it will return its id.
void CreateBookmarksFolderOnceCallback(
    const string16& title,
    int64 parent_id,
    base::WaitableEvent* folder_created_event,
    int64* result) {
  int64 id = CreateBookmarksFolderOnceTask(title, parent_id);
  if (result)
    *result = id;
  if (folder_created_event)
    folder_created_event->Signal();
}

// Retrieves the Bookmark Model from the UI thread.
// This task is temporary until a proper refactoring of the provider is done.
void GetBookmarkModelTask(base::WaitableEvent* loaded_event,
                          BookmarkModel** bookmark_model) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(loaded_event);
  DCHECK(bookmark_model);
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  if (!profile_manager)
    return;

  *bookmark_model = profile_manager->GetLastUsedProfile()->GetBookmarkModel();
  loaded_event->Signal();
}

// Utility method to retrieve the Mobile Bookmarks folder node.
// It can be called directly if the caller is already in UI thread.
void GetMobileBookmarksNodeTask(ScopedJavaGlobalRef<jobject>* jnode) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  if (!profile_manager)
    return;

  BookmarkModel* bookmark_model = profile_manager->GetLastUsedProfile()->
      GetBookmarkModel();
  if (!bookmark_model->IsLoaded())
    return;
  ConvertBookmarkNode(bookmark_model->mobile_node(),
                      ScopedJavaLocalRef<jobject>(), jnode);
}

// Callback used to retrieve the Mobile Bookmarks node in the UI thread.
void GetMobileBookmarksNodeCallback(
    base::WaitableEvent* node_obtained_event,
    ScopedJavaGlobalRef<jobject>* result) {
  GetMobileBookmarksNodeTask(result);
  if (node_obtained_event)
    node_obtained_event->Signal();
}

// Utility method to retrieve a bookmark node by its id.
// It can be called directly if the caller is already in UI thread.
void GetBookmarkNodeWithChildrenTask(
    int64 id,
    ScopedJavaGlobalRef<jobject>* jnode) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  if (!profile_manager)
    return;

  BookmarkModel* bookmark_model = profile_manager->GetLastUsedProfile()->
      GetBookmarkModel();
  if (!bookmark_model->IsLoaded())
    return;
  const BookmarkNode* node = bookmark_model->GetNodeByID(id);
  if (!node)
    return;

  ScopedJavaGlobalRef<jobject> jparent;
  ConvertBookmarkNode(node->parent(), ScopedJavaLocalRef<jobject>(), &jparent);
  ConvertBookmarkNode(node, jparent, jnode);
  if (jnode->is_null())
    return;

  JNIEnv* env = AttachCurrentThread();
  if (!jparent.is_null())
    Java_BookmarkNode_addChild(env, jparent.obj(), jnode->obj());

  for (int i = 0; i < node->child_count(); ++i) {
    ScopedJavaGlobalRef<jobject> jchild;
    ConvertBookmarkNode(node->GetChild(i), *jnode, &jchild);
    if (!jchild.is_null())
      Java_BookmarkNode_addChild(env, jnode->obj(), jchild.obj());
  }
}

// Callback used to retrieve a node by its id in the UI thread.
void GetBookmarkNodeWithChildrenCallback(
    int64 id,
    base::WaitableEvent* node_obtained_event,
    ScopedJavaGlobalRef<jobject>* result) {
  GetBookmarkNodeWithChildrenTask(id, result);
  if (node_obtained_event)
    node_obtained_event->Signal();
}

// Utility method find if a node is a descendant of Mobile Bookmarks.
jboolean IsMobileBookmarksDescendantTask(int64 id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  if (!profile_manager)
    return false;

  BookmarkModel* bookmark_model = profile_manager->GetLastUsedProfile()->
      GetBookmarkModel();
  if (!bookmark_model->IsLoaded())
    return false;
  const BookmarkNode* mobile_node = bookmark_model->mobile_node();
  if (!mobile_node)
    return false;

  const BookmarkNode* node = bookmark_model->GetNodeByID(id);
  while (node && node != mobile_node) {
    node = node->parent();
  }

  return node == mobile_node;
}

// Callback used to retrieve a node by its id in the UI thread.
void IsMobileBookmarksDescendantCallback(
    int64 id,
    base::WaitableEvent* finished_event,
    jboolean* result) {
  jboolean is_descendant = IsMobileBookmarksDescendantTask(id);
  if (result)
    *result = is_descendant;
  if (finished_event)
    finished_event->Signal();
}

// Auxiliary method to retrieve the folder hierarchy of a subtree.
void GetAllBookmarkFoldersSubtree(JNIEnv* env,
                                  const BookmarkNode* node,
                                  const JavaRef<jobject>& parent_folder,
                                  ScopedJavaGlobalRef<jobject>* jfolder) {
  DCHECK(node);
  DCHECK(node->is_folder());
  DCHECK(jfolder);

  // Global refs should be used here for thread-safety reasons as this
  // might be invoked from a thread other than UI.
  jfolder->Reset(
      Java_BookmarkNode_create(
          env,
          node->id(),
          static_cast<jint>(node->type()),
          ConvertUTF16ToJavaString(env, node->GetTitle()).obj(),
          NULL /* URL */,
          parent_folder.obj()));

  for (int i = 0; i < node->child_count(); ++i) {
    const BookmarkNode* child = node->GetChild(i);
    if (child->is_folder()) {
      ScopedJavaGlobalRef<jobject> jchild;
      GetAllBookmarkFoldersSubtree(env, child, *jfolder, &jchild);

      Java_BookmarkNode_addChild(env, jfolder->obj(), jchild.obj());
      if (ClearException(env)) {
        LOG(WARNING) << "Java exception while adding child node.";
        return;
      }
    }
  }
}

// Generate a java representation of the bookmark folder hierarchy that matches
// the one defined in the underlying bookmark model.
void GetAllBookmarkFoldersTask(ScopedJavaGlobalRef<jobject>* jroot) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  Profile* profile = g_browser_process->profile_manager()->GetLastUsedProfile();
  BookmarkModel* bookmark_model = profile->GetBookmarkModel();
  if (!bookmark_model->IsLoaded())
    return;
  const BookmarkNode* root = bookmark_model->root_node();
  if (!root || !root->is_folder())
    return;

  // The iterative approach is not possible because ScopedGlobalJavaRefs
  // cannot be copy-constructed, and therefore not used in STL containers.
  GetAllBookmarkFoldersSubtree(AttachCurrentThread(), root,
                               ScopedJavaLocalRef<jobject>(), jroot);
}

// TODO(leandrogracia): remove this as part of the provider refactoring.
// Bug: b/6497846
void GetAllBookmarkFoldersCallback(
    base::WaitableEvent* finished_event,
    ScopedJavaGlobalRef<jobject>* result) {
  GetAllBookmarkFoldersTask(result);
  if (finished_event)
    finished_event->Signal();
}

// Get java representation of the bookmarked node with the given |id| or NULL
// if no such node exists.
void GetBookmarkNodeTask(int64 id, ScopedJavaGlobalRef<jobject>* jnode) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  Profile* profile = g_browser_process->profile_manager()->GetLastUsedProfile();
  BookmarkModel* bookmark_model = profile->GetBookmarkModel();
  if (!bookmark_model->IsLoaded())
    return;
  const BookmarkNode* node = bookmark_model->GetNodeByID(id);
  if (!node)
    return;

  ScopedJavaGlobalRef<jobject> jparent;
  ConvertBookmarkNode(node->parent(), ScopedJavaLocalRef<jobject>(), &jparent);
  ConvertBookmarkNode(node, jparent, jnode);
}

// TODO(leandrogracia): remove this as part of the provider refactoring.
// Bug: b/6497846
void GetBookmarkNodeCallback(
    int64 id,
    base::WaitableEvent* finished_event,
    ScopedJavaGlobalRef<jobject>* result) {
  GetBookmarkNodeTask(id, result);
  if (finished_event)
    finished_event->Signal();
}

// This is base class for all APIHelper classes.
class APIHelperBase : public base::RefCountedThreadSafe<APIHelperBase> {
 public:
  APIHelperBase(CancelableRequestConsumer* cancelable_consumer)
      : event_(false, false),
        cancelable_consumer_(cancelable_consumer) {
  }

  void Wait() {
    // Mustn't wait on UI thread as the callback function is called in UI
    // thread.
    DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::UI));
    event_.Wait();
  }
 protected:
  virtual ~APIHelperBase() {
  }

  base::WaitableEvent event_;
  CancelableRequestConsumer* cancelable_consumer_;

 private:
  friend class base::RefCountedThreadSafe<APIHelperBase>;

  DISALLOW_COPY_AND_ASSIGN(APIHelperBase);
};

// This is the base class for all APIHelpers that involve the provider service.
class ServiceAPIHelperBase : public APIHelperBase {
 public:
  ServiceAPIHelperBase(AndroidProviderService* service,
                       CancelableRequestConsumer* cancelable_consumer)
      : APIHelperBase(cancelable_consumer),
        service_(service) {
  }

 protected:
  virtual ~ServiceAPIHelperBase() {
  }

  AndroidProviderService* service_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ServiceAPIHelperBase);
};

class GetBookmarkFaviconHelper : public APIHelperBase {
 public:
  GetBookmarkFaviconHelper(FaviconService* favicon_service,
                           CancelableRequestConsumer* cancelable_consumer)
      : APIHelperBase(cancelable_consumer),
        favicon_service_(favicon_service) {
  }

  void GetBookmarkFaviconOrTouchIcon(const GURL& url) {
    // PostTask expects the closure to return void. GetFaviconForURL returns
    // an integer handler we don't need, but that causes a compile error.
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &GetBookmarkFaviconHelper::GetBookmarkIconOnUIThread,
        this, url));
  }

  history::FaviconData favicon() const {
    return favicon_;
  }

 private:
  void GetBookmarkIconOnUIThread(const GURL& url) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    favicon_service_->GetFaviconForURL(url,
        history::FAVICON | history::TOUCH_ICON, cancelable_consumer_,
        base::Bind(&GetBookmarkFaviconHelper::OnFaviconRetrieved, this));
  }

  void OnFaviconRetrieved(FaviconService::Handle handle,
      history::FaviconData favicon) {
    favicon_ = favicon;
    event_.Signal();
  }

  FaviconService* favicon_service_;
  history::FaviconData favicon_;

  DISALLOW_COPY_AND_ASSIGN(GetBookmarkFaviconHelper);
};

// This class helps to get the thumbnail associated with a bookmark URL.
class GetBookmarkThumbnailHelper {
 public:
  GetBookmarkThumbnailHelper(history::TopSites* top_sites)
      : top_sites_(top_sites) {}

  void GetBookmarkThumbnail(const GURL& url) {
    if (top_sites_)
      top_sites_->GetPageThumbnail(url, &thumbnail_);
  }

  // Returns a scoped reference-counted pointer to the bytes of the array,
  // or to null if the thumbnail couldn't be retrieved.
  scoped_refptr<RefCountedMemory> thumbnail() const {
    return thumbnail_;
  }

 private:
  history::TopSites* top_sites_;
  scoped_refptr<RefCountedMemory> thumbnail_;

  DISALLOW_COPY_AND_ASSIGN(GetBookmarkThumbnailHelper);
};

// This class helps to add a bookmark from the API. It creates a task to insert
// the bookmark in UI thread.
// The caller mustn't run on UI thread, The expected flow for users to call is:
// 1.) AddBookmark() to begin adding the bookmark.
// 2.) Wait() to be notified when it is completed.
// 3.) Get the result from url_id().
class AddBookmarkFromAPIHelper : public ServiceAPIHelperBase {
 public:
  AddBookmarkFromAPIHelper(AndroidProviderService* service,
                           CancelableRequestConsumer* cancelable_consumer)
      : ServiceAPIHelperBase(service, cancelable_consumer),
        url_id_(0) {
  }

  void AddBookmark(const history::BookmarkRow& row) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &AndroidProviderService::InsertBookmark, service_, row,
        cancelable_consumer_,
        base::Bind(&AddBookmarkFromAPIHelper::OnBookmarkInserted, this)));
  }

  // Returns the URLID of added row, 0 if the bookmark wasn't added.
  history::URLID url_id() const {
    return url_id_;
  }

 protected:
  virtual ~AddBookmarkFromAPIHelper() {
  }

 private:
  // Callback to return the result.
  void OnBookmarkInserted(AndroidProviderService::Handle handle,
                          bool succeeded,
                          history::URLID id) {
    url_id_ = id;
    event_.Signal();
  }

  history::URLID url_id_;

  DISALLOW_COPY_AND_ASSIGN(AddBookmarkFromAPIHelper);
};

// This class helps to query the bookmark from the API. It creates a task to
// start querying the bookmark in UI thread. The result statement is returned
// by statement() if the query succeeded.
//
// The caller mustn't run on UI thread, The expected flow for users to call is:
// 1.) QueryBookmark() to begin adding the bookmark.
// 2.) Wait() to be notified when it is completed.
// 3.) Get the result from statement().
class QueryBookmarkFromAPIHelper : public ServiceAPIHelperBase {
 public:
    QueryBookmarkFromAPIHelper(
        AndroidProviderService* service,
        CancelableRequestConsumer* cancelable_consumer)
        : ServiceAPIHelperBase(service, cancelable_consumer),
          statement_(NULL) {
  }

  void QueryBookmark(
      const std::vector<history::BookmarkRow::BookmarkColumnID>& projections,
      const std::string& selection,
      const std::vector<string16>& selection_args,
      const std::string& sort_order) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &AndroidProviderService::QueryBookmark, service_, projections,
        selection, selection_args, sort_order, cancelable_consumer_,
        base::Bind(&QueryBookmarkFromAPIHelper::OnBookmarkQueried, this)));
  }

  // Returns the query result statement.
  history::AndroidStatement* statement() const {
    return statement_;
  }

 protected:
  virtual ~QueryBookmarkFromAPIHelper() {
  }

 private:
  // Callback to return the result.
  void OnBookmarkQueried(AndroidProviderService::Handle handle,
                         bool succeeded,
                         history::AndroidStatement* statement) {
    statement_ = statement;
    event_.Signal();
  }

  history::AndroidStatement* statement_;

  DISALLOW_COPY_AND_ASSIGN(QueryBookmarkFromAPIHelper);
};

// This class helps to update the bookmark from the API. It creates a task to
// start updating the bookmark in UI thread. The result statement is returned
// by updated_row_count().
//
// The caller mustn't run on UI thread, The expected flow for users to call is:
// 1.) UpdateBookmark() to start updating the bookmark.
// 2.) Wait() to be notified when it is completed.
// 3.) Get the result from updated_row_count().
class UpdateBookmarkFromAPIHelper : public ServiceAPIHelperBase {
 public:
    UpdateBookmarkFromAPIHelper(
        AndroidProviderService* service,
        CancelableRequestConsumer* cancelable_consumer)
        : ServiceAPIHelperBase(service, cancelable_consumer),
          updated_row_count_(0) {
  }

  void UpdateBookmark(const history::BookmarkRow& row,
                      const std::string& selection,
                      const std::vector<string16>& selection_args) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &AndroidProviderService::UpdateBookmark, service_, row, selection,
        selection_args, cancelable_consumer_,
        base::Bind(&UpdateBookmarkFromAPIHelper::OnBookmarkUpdated, this)));
  }

  // Returns the updated row.
  int updated_row_count() const {
    return updated_row_count_;
  }

 protected:
  virtual ~UpdateBookmarkFromAPIHelper() {
  }

 private:
  // Callback to return the result.
  void OnBookmarkUpdated(AndroidProviderService::Handle handle,
                         bool succeeded,
                         int updated_row_count) {
    updated_row_count_ = updated_row_count;
    event_.Signal();
  }

  int updated_row_count_;

  DISALLOW_COPY_AND_ASSIGN(UpdateBookmarkFromAPIHelper);
};

// This class helps to remove the history from the API. It creates a task to
// start removing the history in UI thread. The result statement is returned
// by removed_row_count().
//
// The caller mustn't run on UI thread, The expected flow for users to call is:
// 1.) RemoveHistory() to start remove the history
// 2.) Wait() to be notified when it is completed.
// 3.) Get the result from removed_row_count().
class RemoveHistoryFromAPIHelper : public ServiceAPIHelperBase {
 public:
    RemoveHistoryFromAPIHelper(
        AndroidProviderService* service,
        CancelableRequestConsumer* cancelable_consumer)
      : ServiceAPIHelperBase(service, cancelable_consumer),
        removed_row_count_(0) {
  }

  void RemoveHistory(const std::string& selection,
                     const std::vector<string16>& selection_args) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &AndroidProviderService::DeleteHistory, service_, selection,
        selection_args, cancelable_consumer_,
        base::Bind(&RemoveHistoryFromAPIHelper::OnHistoryRemoved, this)));
  }

  // Returns the removed row.
  int removed_row_count() const {
    return removed_row_count_;
  }

 protected:
  virtual ~RemoveHistoryFromAPIHelper() {
  }

 private:
  // Callback to return the result.
  void OnHistoryRemoved(AndroidProviderService::Handle handle,
                        bool succeeded,
                        int removed_row_count) {
    removed_row_count_ = removed_row_count;
    event_.Signal();
  }

  int removed_row_count_;

  DISALLOW_COPY_AND_ASSIGN(RemoveHistoryFromAPIHelper);
};

// This class helps to remove the bookmarks from the API. It creates a task to
// start removing the bookmarks in UI thread. The result statement is returned
// by removed_row_count().
//
// The caller mustn't run on UI thread, The expected flow for users to call is:
// 1.) RemoveBookmark() to start updating the bookmark.
// 2.) Wait() to be notified when it is completed.
// 3.) Get the result from removed_row_count().
class RemoveBookmarkFromAPIHelper : public ServiceAPIHelperBase {
 public:
    RemoveBookmarkFromAPIHelper(
        AndroidProviderService* service,
        CancelableRequestConsumer* cancelable_consumer)
      : ServiceAPIHelperBase(service, cancelable_consumer),
        removed_row_count_(0) {
  }

  void RemoveBookmark(const std::string& selection,
                      const std::vector<string16>& selection_args) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &AndroidProviderService::DeleteBookmark, service_, selection,
        selection_args, cancelable_consumer_,
        base::Bind(&RemoveBookmarkFromAPIHelper::OnBookmarkRemoved, this)));
  }

  // Returns the removed row.
  int removed_row_count() const {
    return removed_row_count_;
  }

 protected:
  virtual ~RemoveBookmarkFromAPIHelper() {
  }

 private:
  // Callback to return the result.
  void OnBookmarkRemoved(AndroidProviderService::Handle handle,
                         bool succeeded,
                         int removed_row_count) {
    removed_row_count_ = removed_row_count;
    event_.Signal();
  }

  int removed_row_count_;

  DISALLOW_COPY_AND_ASSIGN(RemoveBookmarkFromAPIHelper);
};

// This class provides the common method for the SearchTermAPIHelper.
class SearchTermAPIHelper : public ServiceAPIHelperBase {
 protected:
  SearchTermAPIHelper(AndroidProviderService* service,
                      CancelableRequestConsumer* cancelable_consumer,
                      Profile* profile)
      : ServiceAPIHelperBase(service, cancelable_consumer),
        profile_(profile) {
  }

  virtual ~SearchTermAPIHelper() {
  }

  // Fill SearchRow's template_url_id and url fields according the given
  // search_term. Return true if succeeded. This method should be run in
  // UI thread.
  void BuildSearchRow(history::SearchRow* row) {
    TemplateURLService* template_service =
        TemplateURLServiceFactory::GetForProfile(profile_);
    template_service->Load();
    const TemplateURL* search_engine =
        template_service->GetDefaultSearchProvider();
    if (search_engine) {
      const TemplateURLRef* search_url = search_engine->url();
      std::string url = search_url->ReplaceSearchTerms(*search_engine,
          row->search_term(), TemplateURLRef::NO_SUGGESTIONS_AVAILABLE,
          string16());
      if (!url.empty()) {
        row->set_url(GURL(url));
        row->set_template_url_id(search_engine->id());
      }
    }
  }

 private:
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(SearchTermAPIHelper);
};

// This class helps to add a search from the API. It creates a task to insert
// the search term in UI thread.
// The caller mustn't run on UI thread, The expected flow for users to call is:
// 1.) AddSearchTerm() to begin adding the search term.
// 2.) Wait() to be notified when it is completed.
// 3.) Get the result from serach_id().
class AddSearchTermFromAPIHelper : public SearchTermAPIHelper {
 public:
    AddSearchTermFromAPIHelper(AndroidProviderService* service,
                               CancelableRequestConsumer* cancelable_consumer,
                               Profile* profile)
        : SearchTermAPIHelper(service, cancelable_consumer, profile),
          search_id_(0) {
  }

  void AddSearchTerm(const history::SearchRow& row) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &AddSearchTermFromAPIHelper::RunInUIThread, this, row));
  }

  // Returns the URLID of added row, 0 if the bookmark wasn't added.
  history::URLID search_id() const {
    return search_id_;
  }

 protected:
  virtual ~AddSearchTermFromAPIHelper() {
  }

 private:
  void RunInUIThread(const history::SearchRow& row) {
    row_ = row;
    BuildSearchRow(&row_);
    if (!row_.is_valid(history::SearchRow::SearchTime)) {
      row_.set_search_time(base::Time::Now());
    }
    service_->InsertSearchTerm(row_, cancelable_consumer_,
        base::Bind(&AddSearchTermFromAPIHelper::OnSearchTermInserted, this));
  }

  // Callback to return the result.
  void OnSearchTermInserted(AndroidProviderService::Handle handle,
                            bool succeeded,
                            history::URLID id) {
    search_id_ = id;
    event_.Signal();
  }

  history::URLID search_id_;
  history::SearchRow row_;

  DISALLOW_COPY_AND_ASSIGN(AddSearchTermFromAPIHelper);
};

// This class helps to update the search from the API. It creates a task to
// update the search term in UI thread.
// The caller mustn't run on UI thread, The expected flow for users to call is:
// 1.) UpdateSearchTerm() to begin updating the search term.
// 2.) Wait() to be notified when it is completed.
// 3.) Get the result from update_row_count().
class UpdateSearchTermFromAPIHelper : public SearchTermAPIHelper {
 public:
    UpdateSearchTermFromAPIHelper(
        AndroidProviderService* service,
        CancelableRequestConsumer* cancelable_consumer,
        Profile* profile)
        : SearchTermAPIHelper(service, cancelable_consumer, profile),
          updated_row_count_(0) {
  }

  void UpdateSearchTerm(const history::SearchRow& row,
                        const std::string& selection,
                        const std::vector<string16>& selection_args) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &UpdateSearchTermFromAPIHelper::RunInUIThread, this, row,
        selection, selection_args));
  }

  // Returns the URLID of added row, 0 if the bookmark wasn't added.
  history::URLID updated_row_count() const {
    return updated_row_count_;
  }

 protected:
  virtual ~UpdateSearchTermFromAPIHelper() {
  }

 private:
  void RunInUIThread(const history::SearchRow& row,
                     const std::string& selection,
                     const std::vector<string16>& selection_args) {
    row_ = row;
    BuildSearchRow(&row_);
    service_->UpdateSearchTerm(row_, selection, selection_args,
        cancelable_consumer_,
        base::Bind(&UpdateSearchTermFromAPIHelper::OnSearchTermUpdated, this));
  }

  // Callback to return the result.
  void OnSearchTermUpdated(AndroidProviderService::Handle handle,
                            bool succeeded,
                            int count) {
    updated_row_count_ = count;
    event_.Signal();
  }

  int updated_row_count_;
  history::SearchRow row_;

  DISALLOW_COPY_AND_ASSIGN(UpdateSearchTermFromAPIHelper);
};

// This class helps to remove the search terms from the API. It creates a task
// to start removing the search term in UI thread. The result statement is
// returned by removed_row_count().
//
// The caller mustn't run on UI thread, The expected flow for users to call is:
// 1.) RemoveSearchTerm() to start removing the search term.
// 2.) Wait() to be notified when it is completed.
// 3.) Get the result from removed_row_count().
class RemoveSearchTermFromAPIHelper : public ServiceAPIHelperBase {
 public:
    RemoveSearchTermFromAPIHelper(
        AndroidProviderService* service,
        CancelableRequestConsumer* cancelable_consumer)
        : ServiceAPIHelperBase(service, cancelable_consumer),
          removed_row_count_(0) {
  }

  void RemoveSearchTerm(const std::string& selection,
                        const std::vector<string16>& selection_args) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &AndroidProviderService::DeleteSearchTerm, service_, selection,
        selection_args, cancelable_consumer_,
        base::Bind(&RemoveSearchTermFromAPIHelper::OnSearchTermDeleted, this)));
  }

  // Returns the removed row.
  int removed_row_count() const {
    return removed_row_count_;
  }

 protected:
  virtual ~RemoveSearchTermFromAPIHelper() {
  }

 private:
  // Callback to return the result.
  void OnSearchTermDeleted(AndroidProviderService::Handle handle,
                         bool succeeded,
                         int removed_row_count) {
    removed_row_count_ = removed_row_count;
    event_.Signal();
  }

  int removed_row_count_;

  DISALLOW_COPY_AND_ASSIGN(RemoveSearchTermFromAPIHelper);
};

// This class helps to query the search term from the API. It creates a task to
// start querying the search term in UI thread. The result statement is returned
// by statement() if the query succeeded.
//
// The caller mustn't run on UI thread, The expected flow for users to call is:
// 1.) QuerySearchTerm() to begin query the search term.
// 2.) Wait() to be notified when it is completed.
// 3.) Get the result from statement().
class QuerySearchTermFromAPIHelper : public ServiceAPIHelperBase {
 public:
  QuerySearchTermFromAPIHelper(
      AndroidProviderService* service,
      CancelableRequestConsumer* cancelable_consumer)
      : ServiceAPIHelperBase(service, cancelable_consumer),
        statement_(NULL) {
  }

  void QuerySearchTerm(
      const std::vector<history::SearchRow::SearchColumnID>& projections,
      const std::string& selection,
      const std::vector<string16>& selection_args,
      const std::string& sort_order) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, base::Bind(
        &AndroidProviderService::QuerySearchTerm, service_, projections,
        selection, selection_args, sort_order, cancelable_consumer_,
        base::Bind(&QuerySearchTermFromAPIHelper::OnSearchTermQueried, this)));
  }

  // Returns the query result statement.
  history::AndroidStatement* statement() const {
    return statement_;
  }

 protected:
  virtual ~QuerySearchTermFromAPIHelper() {
  }

 private:
  // Callback to return the result.
  void OnSearchTermQueried(AndroidProviderService::Handle handle,
                           bool succeeded,
                           history::AndroidStatement* statement) {
    statement_ = statement;
    event_.Signal();
  }

  history::AndroidStatement* statement_;

  DISALLOW_COPY_AND_ASSIGN(QuerySearchTermFromAPIHelper);
};

}  // namespace

static jint Init(JNIEnv* env, jobject obj) {
  ChromeBrowserProvider* provider = new ChromeBrowserProvider(env, obj);
  return reinterpret_cast<jint>(provider);
}

// Creates a bookmarks folder node and returns its id.
// If it already exists, returns the existing id.
static jlong CreateBookmarksFolderOnce(JNIEnv* env, jclass clazz,
    jstring jtitle, jlong parent_id) {
  string16 title = ConvertJavaStringToUTF16(env, jtitle);
  if (title.empty())
    return kInvalidBookmarkNodeId;

  // Retrieve or create the folder in the UI thread.
  if (BrowserThread::CurrentlyOn(BrowserThread::UI))
    return CreateBookmarksFolderOnceTask(title, parent_id);

  int64 result = 0;
  base::WaitableEvent event(false, false);
  BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&CreateBookmarksFolderOnceCallback,
            title,
            parent_id,
            &event,
            &result));
  event.Wait();
  return result;
}

static void EnsureBookmarksModelIsLoaded(JNIEnv* env, jclass clazz) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::UI));

  // TODO(leandrogracia): this should be a non-static method and retrieve the
  // bookmark model from a pointer stored during initialization time.
  BookmarkModel* bookmark_model = NULL;
  base::WaitableEvent event(false, false);
  BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&GetBookmarkModelTask,
                   &event,
                   &bookmark_model));
  event.Wait();

  if (bookmark_model)
    bookmark_model->BlockTillLoaded();
}

static jobject GetMobileBookmarksFolder(
    JNIEnv* env,
    jclass clazz) {
  ScopedJavaGlobalRef<jobject> result;
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    GetMobileBookmarksNodeTask(&result);
  } else {
    base::WaitableEvent event(false, false);
    BrowserThread::PostTask(
          BrowserThread::UI,
          FROM_HERE,
          base::Bind(&GetMobileBookmarksNodeCallback,
              &event,
              &result));
    event.Wait();
  }
  // There is a problem in ScopedJavaGlobalRef that leads to a crash if the
  // ref was created in another thread. The cause is that ScopedJavaGlobalRef is
  // trying to use the JNIEnv used to create the reference instead of the one
  // associated with the current thread. To fix this we need to manually delete
  // the global reference using the local env.
  jobject local_result = env->NewLocalRef(result.obj());
  env->DeleteGlobalRef(result.Release());
  return local_result;
}

// Get java representation of the bookmarked node with the given |id| or NULL
// if no such node exists.
static jobject GetBookmarkNodeWithChildren(
    JNIEnv* env,
    jclass clazz,
    jlong id) {
  ScopedJavaGlobalRef<jobject> result;
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    GetBookmarkNodeWithChildrenTask(id, &result);
  } else {
    base::WaitableEvent event(false, false);
    BrowserThread::PostTask(
          BrowserThread::UI,
          FROM_HERE,
          base::Bind(
              &GetBookmarkNodeWithChildrenCallback,
              id,
              &event,
              &result));
    event.Wait();
  }
  jobject local_result = env->NewLocalRef(result.obj());
  env->DeleteGlobalRef(result.Release());
  return local_result;
}

// Find if a node id is a descendant of the Mobile Bookmarks folder.
static jboolean IsMobileBookmarksDescendant(JNIEnv* env,
                                            jclass clazz,
                                            jlong id) {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI))
    return IsMobileBookmarksDescendantTask(id);

  jboolean result;
  base::WaitableEvent event(false, false);
  BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(
            &IsMobileBookmarksDescendantCallback,
            id,
            &event,
            &result));
  event.Wait();
  return result;
}

// Generate a java representation of the bookmark folder hierarchy that matches
// the one defined in the underlying bookmark model.
static jobject GetAllBookmarkFolders(
    JNIEnv* env,
    jclass clazz) {
  ScopedJavaGlobalRef<jobject> result;
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    GetAllBookmarkFoldersTask(&result);
  } else {
    base::WaitableEvent event(false, false);
    BrowserThread::PostTask(
          BrowserThread::UI,
          FROM_HERE,
          base::Bind(
              &GetAllBookmarkFoldersCallback,
              &event,
              &result));
    event.Wait();
  }
  jobject local_result = env->NewLocalRef(result.obj());
  env->DeleteGlobalRef(result.Release());
  return local_result;
}

// Get java representation of the bookmarked node with the given |id| or NULL
// if no such node exists.
static jobject GetBookmarkNode(JNIEnv* env, jclass clazz, jlong id) {
  ScopedJavaGlobalRef<jobject> result;
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    GetBookmarkNodeTask(id, &result);
  } else {
    base::WaitableEvent event(false, false);
    BrowserThread::PostTask(
          BrowserThread::UI,
          FROM_HERE,
          base::Bind(
              &GetBookmarkNodeCallback,
              id,
              &event,
              &result));
    event.Wait();
  }
  jobject local_result = env->NewLocalRef(result.obj());
  env->DeleteGlobalRef(result.Release());
  return local_result;
}

jlong ConvertJLongObjectToPrimitive(JNIEnv* env, jobject long_obj) {
  ScopedJavaLocalRef<jclass> jlong_clazz = GetClass(env, "java/lang/Long");
  jmethodID long_value = GetMethodID(env, jlong_clazz, "longValue", "()J");
  return env->CallLongMethod(long_obj, long_value, NULL);
}

jboolean ConvertJBooleanObjectToPrimitive(JNIEnv* env, jobject boolean_object) {
  ScopedJavaLocalRef<jclass> jboolean_clazz =
      GetClass(env, "java/lang/Boolean");
  jmethodID boolean_value =
      GetMethodID(env, jboolean_clazz, "booleanValue", "()Z");
  return env->CallBooleanMethod(boolean_object, boolean_value, NULL);
}

base::Time ConvertJlongToTime(jlong value) {
  return base::Time::UnixEpoch() +
      base::TimeDelta::FromMilliseconds((int64)value);
}

jint ConvertJIntegerToJint(JNIEnv* env, jobject integer_obj) {
  ScopedJavaLocalRef<jclass> jinteger_clazz =
      GetClass(env, "java/lang/Integer");
  jmethodID int_value = GetMethodID(env, jinteger_clazz, "intValue", "()I");
  return env->CallIntMethod(integer_obj, int_value, NULL);
}

std::vector<string16> ConvertJStringArrayToString16Array(JNIEnv* env,
                                                         jobjectArray array) {
  std::vector<string16> results;
  if (array) {
    jsize len = env->GetArrayLength(array);
    for (int i = 0; i < len; i++) {
      results.push_back(ConvertJavaStringToUTF16(env,
          static_cast<jstring>(env->GetObjectArrayElement(array, i))));
    }
  }
  return results;
}

// Fills the bookmark |row| with the given java objects.
void FillBookmarkRow(JNIEnv* env,
                     jobject obj,
                     jstring url,
                     jobject created,
                     jobject isBookmark,
                     jobject date,
                     jbyteArray favicon,
                     jstring title,
                     jobject visits,
                     jlong parent_id,
                     history::BookmarkRow* row) {
  if (url) {
    string16 raw_url = ConvertJavaStringToUTF16(env, url);
    // GURL doesn't accept the URL without protocol, but the Android CTS
    // allows it. We are trying to prefix with 'http://' to see whether
    // GURL thinks it is a valid URL. The original url will be stored in
    // history::BookmarkRow.raw_url_.
    GURL gurl = ParseAndMaybeAppendScheme(raw_url, kDefaultUrlScheme);
    row->set_url(gurl);
    row->set_raw_url(UTF16ToUTF8(raw_url));
  }

  if (created)
    row->set_created(ConvertJlongToTime(
        ConvertJLongObjectToPrimitive(env, created)));

  if (isBookmark)
    row->set_is_bookmark(ConvertJBooleanObjectToPrimitive(env, isBookmark));

  if (date)
    row->set_last_visit_time(ConvertJlongToTime(ConvertJLongObjectToPrimitive(
        env, date)));

  if (favicon) {
    std::vector<uint8> bytes;
    base::android::JavaByteArrayToByteVector(env, favicon, &bytes);
    row->set_favicon(bytes);
  }

  if (title)
    row->set_title(ConvertJavaStringToUTF16(env, title));

  if (visits)
    row->set_visit_count(ConvertJIntegerToJint(env, visits));

  // TODO(michaelbai): Make sure parent_id is always the subdir of mobile_node.
  if (parent_id > 0)
    row->set_parent_id(parent_id);
}

// Add the bookmark with the given column values.
jlong ChromeBrowserProvider::AddBookmarkFromAPI(JNIEnv* env,
                                               jobject obj,
                                               jstring url,
                                               jobject created,
                                               jobject isBookmark,
                                               jobject date,
                                               jbyteArray favicon,
                                               jstring title,
                                               jobject visits,
                                               jlong parent_id) {
  DCHECK(url);

  history::BookmarkRow row;
  FillBookmarkRow(env, obj, url, created, isBookmark, date, favicon, title,
                  visits, parent_id, &row);

  // URL must be valid.
  if(row.url().is_empty()) {
    LOG(ERROR) << "Not a valid URL " << row.raw_url();
    return 0;
  }
  return AddBookmarkFromAPIInternal(row);
}

ScopedJavaLocalRef<jobject> ChromeBrowserProvider::QueryBookmarkFromAPI(
    JNIEnv* env,
    jobject obj,
    jobjectArray projection,
    jstring selections,
    jobjectArray selection_args,
    jstring sort_order) {
  // Converts the projection to array of BookmarkColumnID and column name.
  // Used to store the projection column ID according their sequence.
  std::vector<history::BookmarkRow::BookmarkColumnID> query_columns;
  // Used to store the projection column names according their sequence.
  std::vector<std::string> columns_name;
  if (projection) {
    jsize len = env->GetArrayLength(projection);
    for (int i = 0; i < len; i++) {
      std::string name = ConvertJavaStringToUTF8(env, static_cast<jstring>(
          env->GetObjectArrayElement(projection, i)));
      history::BookmarkRow::BookmarkColumnID id =
          history::BookmarkRow::GetBookmarkColumnID(name);
      if (id == history::BookmarkRow::ColumnEnd) {
        // Ignore the unknown column; As Android platform will send us
        // the non public column.
        continue;
      }
      query_columns.push_back(id);
      columns_name.push_back(name);
    }
  }

  std::vector<string16> where_args =
      ConvertJStringArrayToString16Array(env, selection_args);

  std::string where_clause;
  if (selections) {
    where_clause = ConvertJavaStringToUTF8(env, selections);
  }

  std::string sort_clause;
  if (sort_order) {
    sort_clause = ConvertJavaStringToUTF8(env, sort_order);
  }

  history::AndroidStatement* statement = QueryBookmarkFromAPIInternal(
      query_columns, where_clause, where_args, sort_clause);
  if (!statement)
    return ScopedJavaLocalRef<jobject>();

  // Creates and returns com.google.android.apps.chrome.SQLiteCursor java object.
  return SQLiteCursor::NewJavaSqliteCursor(env, columns_name, statement,
                                           android_provider_service());
}

// Updates the bookmarks with the given column values. The value is not given if
// it is NULL.
jint ChromeBrowserProvider::UpdateBookmarkFromAPI(JNIEnv* env,
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
                                                  jobjectArray selection_args) {
  history::BookmarkRow row;
  FillBookmarkRow(env, obj, url, created, isBookmark, date, favicon, title,
                  visits, parent_id, &row);

  std::vector<string16> where_args = ConvertJStringArrayToString16Array(env,
                                         selection_args);
  std::string where_clause;
  if (selections)
    where_clause = ConvertJavaStringToUTF8(env, selections);

  return UpdateBookmarkFromAPIInternal(row, where_clause, where_args);
}

jint ChromeBrowserProvider::RemoveBookmarkFromAPI(JNIEnv* env,
                                                  jobject obj,
                                                  jstring selections,
                                                  jobjectArray selection_args) {
  std::vector<string16> where_args =
      ConvertJStringArrayToString16Array(env, selection_args);

  std::string where_clause;
  if (selections)
    where_clause = ConvertJavaStringToUTF8(env, selections);

  return RemoveBookmarkFromAPIInternal(where_clause, where_args);
}

jint ChromeBrowserProvider::RemoveHistoryFromAPI(JNIEnv* env,
                                                 jobject obj,
                                                 jstring selections,
                                                 jobjectArray selection_args) {
  std::vector<string16> where_args =
      ConvertJStringArrayToString16Array(env, selection_args);

  std::string where_clause;
  if (selections)
    where_clause = ConvertJavaStringToUTF8(env, selections);

  return RemoveHistoryFromAPIInternal(where_clause, where_args);
}


// Fills the bookmark |row| with the given java objects if it is not null.
void FillSearchRow(JNIEnv* env,
                   jobject obj,
                   jstring search_term,
                   jobject date,
                   history::SearchRow* row) {
  if (search_term)
    row->set_search_term(ConvertJavaStringToUTF16(env, search_term));

  if (date)
    row->set_search_time(ConvertJlongToTime(ConvertJLongObjectToPrimitive(
        env, date)));
}

// Add the search term with the given column values. The value is not given if
// it is NULL.
jlong ChromeBrowserProvider::AddSearchTermFromAPI(JNIEnv* env,
                                                  jobject obj,
                                                  jstring search_term,
                                                  jobject date) {
  DCHECK(search_term);

  history::SearchRow row;
  FillSearchRow(env, obj, search_term, date, &row);

  // URL must be valid.
  if(row.search_term().empty()) {
    LOG(ERROR) << "Search term is empty.";
    return 0;
  }

  return AddSearchTermFromAPI(row);
}

ScopedJavaLocalRef<jobject> ChromeBrowserProvider::QuerySearchTermFromAPI(
    JNIEnv* env,
    jobject obj,
    jobjectArray projection,
    jstring selections,
    jobjectArray selection_args,
    jstring sort_order) {
  // Converts the projection to array of SearchColumnID and column name.
  // Used to store the projection column ID according their sequence.
  std::vector<history::SearchRow::SearchColumnID> query_columns;
  // Used to store the projection column names according their sequence.
  std::vector<std::string> columns_name;
  if (projection) {
    jsize len = env->GetArrayLength(projection);
    for (int i = 0; i < len; i++) {
      std::string name = ConvertJavaStringToUTF8(env, static_cast<jstring>(
          env->GetObjectArrayElement(projection, i)));
      history::SearchRow::SearchColumnID id =
          history::SearchRow::GetSearchColumnID(name);
      if (id == history::SearchRow::ColumnEnd) {
        LOG(ERROR) << "Can not find " << name;
        return ScopedJavaLocalRef<jobject>();
      }
      query_columns.push_back(id);
      columns_name.push_back(name);
    }
  }

  std::vector<string16> where_args =
      ConvertJStringArrayToString16Array(env, selection_args);

  std::string where_clause;
  if (selections) {
    where_clause = ConvertJavaStringToUTF8(env, selections);
  }

  std::string sort_clause;
  if (sort_order) {
    sort_clause = ConvertJavaStringToUTF8(env, sort_order);
  }

  history::AndroidStatement* statement = QuerySearchTermFromAPI(
      query_columns, where_clause, where_args, sort_clause);
  if (!statement)
    return ScopedJavaLocalRef<jobject>();
  // Creates and returns com.google.android.apps.chrome.SQLiteCursor java object.
  return SQLiteCursor::NewJavaSqliteCursor(env, columns_name, statement,
                                           android_provider_service());
}

// Updates the search terms with the given column values. The value is not
// given if it is NULL.
jint ChromeBrowserProvider::UpdateSearchTermFromAPI(
    JNIEnv* env, jobject obj, jstring search_term, jobject date,
    jstring selections, jobjectArray selection_args) {
  history::SearchRow row;
  FillSearchRow(env, obj, search_term, date, &row);

  std::vector<string16> where_args = ConvertJStringArrayToString16Array(
      env, selection_args);

  std::string where_clause;
  if (selections)
    where_clause = ConvertJavaStringToUTF8(env, selections);

  return UpdateSearchTermFromAPI(row, where_clause, where_args);
}

jint ChromeBrowserProvider::RemoveSearchTermFromAPI(
    JNIEnv* env, jobject obj, jstring selections, jobjectArray selection_args) {
  std::vector<string16> where_args =
      ConvertJStringArrayToString16Array(env, selection_args);

  std::string where_clause;
  if (selections)
    where_clause = ConvertJavaStringToUTF8(env, selections);

  return RemoveSearchTermFromAPI(where_clause, where_args);
}

ChromeBrowserProvider::JavaBrowserProviderWeakGlobalRef::
JavaBrowserProviderWeakGlobalRef(JNIEnv* env, jobject obj)
    : JavaObjectWeakGlobalRef(env, obj) {
  ScopedJavaLocalRef<jclass> clazz = GetClass(env, kClassPathName);
  on_bookmark_changed_ = GetMethodID(env, clazz, "onBookmarkChanged", "()V");
  on_search_term_changed_ =
      GetMethodID(env, clazz, "onSearchTermChanged", "()V");
}

void ChromeBrowserProvider::JavaBrowserProviderWeakGlobalRef::
OnBookmarkChanged() const {
  JNIEnv* env = AttachCurrentThread();
  DCHECK(get(env).obj());
  env->CallVoidMethod(get(env).obj(), on_bookmark_changed_);
  CheckException(env);
}

void ChromeBrowserProvider::JavaBrowserProviderWeakGlobalRef::
OnSearchTermChanged() const {
  JNIEnv* env = AttachCurrentThread();
  DCHECK(get(env).obj());
  env->CallVoidMethod(get(env).obj(), on_search_term_changed_);
  CheckException(env);
}

ChromeBrowserProvider::ChromeBrowserProvider(JNIEnv* env, jobject obj)
    : java_browser_provider_(env, obj),
      template_loaded_event_(true, false) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  profile_ = g_browser_process->profile_manager()->GetLastUsedProfile();
  service_ = new AndroidProviderService(profile_);
  favicon_service_.reset(new FaviconService(profile_));
  top_sites_ = profile_->GetTopSites();

  // Registers the notifications we are interested.
  profile_->GetBookmarkModel()->AddObserver(this);
  notification_registrar_.Add(this, chrome::NOTIFICATION_HISTORY_URL_VISITED,
                              content::NotificationService::AllSources());
  notification_registrar_.Add(this, chrome::NOTIFICATION_HISTORY_URLS_DELETED,
                              content::NotificationService::AllSources());
  notification_registrar_.Add(this,
      chrome::NOTIFICATION_HISTORY_KEYWORD_SEARCH_TERM_UPDATED,
      content::NotificationService::AllSources());
  notification_registrar_.Add(this,
      chrome::NOTIFICATION_TEMPLATE_URL_SERVICE_LOADED,
      content::NotificationService::AllSources());
  TemplateURLService* template_service =
        TemplateURLServiceFactory::GetForProfile(profile_);
  if (template_service->loaded())
    template_loaded_event_.Signal();
  else
    template_service->Load();
}

ChromeBrowserProvider::~ChromeBrowserProvider() {
  Profile* profile = g_browser_process->profile_manager()->GetLastUsedProfile();
  profile->GetBookmarkModel()->RemoveObserver(this);
}

void ChromeBrowserProvider::Destroy(JNIEnv*, jobject) {
  delete this;
}

jlong ChromeBrowserProvider::AddBookmark(JNIEnv* env,
                                         jobject,
                                         jstring jurl,
                                         jstring jtitle,
                                         jboolean is_folder,
                                         jlong parent_id) {
  string16 url;
  if (jurl)
    url = ConvertJavaStringToUTF16(env, jurl);
  string16 title = ConvertJavaStringToUTF16(env, jtitle);

  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    // We are already in UI thread.
    return AddBookmarkTask(title, url, is_folder, parent_id);
  } else {
    int64 result = 0;
    base::WaitableEvent event(false, false);
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&AddBookmarkCallback,
            title,
            url,
            is_folder,
            parent_id,
            &event,
            &result));
    event.Wait();
    return result;
  }
}

void ChromeBrowserProvider::RemoveBookmark(JNIEnv*, jobject, jlong id) {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    // We are already in UI thread.
    return RemoveBookmarkTask(id);
  } else {
    base::WaitableEvent event(false, false);
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&RemoveBookmarkCallback, id, &event));
    event.Wait();
  }
}

void ChromeBrowserProvider::UpdateBookmark(JNIEnv* env,
                                           jobject,
                                           jlong id,
                                           jstring jurl,
                                           jstring jtitle,
                                           jlong parent_id) {
  string16 url;
  if (jurl)
    url = ConvertJavaStringToUTF16(env, jurl);
  string16 title = ConvertJavaStringToUTF16(env, jtitle);
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    // We are already in UI thread.
    return UpdateBookmarkTask(id, title, url, parent_id);
  } else {
    base::WaitableEvent event(false, false);
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&UpdateBookmarkCallback,
            id,
            title,
            url,
            parent_id,
            &event));
    event.Wait();
  }
}

history::FaviconData
ChromeBrowserProvider::GetBookmarkFaviconOrTouchIconInternal(
    const GURL& url) {
  scoped_refptr<GetBookmarkFaviconHelper> helper(
      new GetBookmarkFaviconHelper(favicon_service_.get(),
                                   &cancelable_consumer_favicon_));
  helper->GetBookmarkFaviconOrTouchIcon(url);
  helper->Wait();
  return helper->favicon();
}

scoped_refptr<RefCountedMemory>
ChromeBrowserProvider::GetBookmarkThumbnailInternal(
    const GURL& url) {
  scoped_ptr<GetBookmarkThumbnailHelper> helper(
      new GetBookmarkThumbnailHelper(top_sites_));
  // GetBookmarkThumbnail is synchronous, no need to wait.
  helper->GetBookmarkThumbnail(url);
  return helper->thumbnail();
}

int64 ChromeBrowserProvider::AddBookmarkFromAPIInternal(
    const history::BookmarkRow& row) {
  scoped_refptr<AddBookmarkFromAPIHelper> helper(
      new AddBookmarkFromAPIHelper(service_.get(),
                                   &cancelable_consumer_history_));
  helper->AddBookmark(row);
  helper->Wait();
  return helper->url_id();
}

history::AndroidStatement* ChromeBrowserProvider::QueryBookmarkFromAPIInternal(
    const std::vector<history::BookmarkRow::BookmarkColumnID>& projections,
    const std::string& selection,
    const std::vector<string16>& selection_args,
    const std::string& sort_order) {
  scoped_refptr<QueryBookmarkFromAPIHelper> helper(
      new QueryBookmarkFromAPIHelper(service_.get(),
                                     &cancelable_consumer_history_));
  helper->QueryBookmark(projections, selection, selection_args, sort_order);
  helper->Wait();
  return helper->statement();
}

int ChromeBrowserProvider::UpdateBookmarkFromAPIInternal(
    const history::BookmarkRow& row,
    const std::string& selection,
    const std::vector<string16>& selection_args) {
  scoped_refptr<UpdateBookmarkFromAPIHelper> helper(
      new UpdateBookmarkFromAPIHelper(service_.get(),
                                      &cancelable_consumer_history_));
  helper->UpdateBookmark(row, selection, selection_args);
  helper->Wait();
  return helper->updated_row_count();
}

int ChromeBrowserProvider::RemoveBookmarkFromAPIInternal(
    const std::string& selection,
    const std::vector<string16>& selection_args) {
  scoped_refptr<RemoveBookmarkFromAPIHelper> helper(
      new RemoveBookmarkFromAPIHelper(service_.get(),
                                      &cancelable_consumer_history_));
  helper->RemoveBookmark(selection, selection_args);
  helper->Wait();
  return helper->removed_row_count();
}

int ChromeBrowserProvider::RemoveHistoryFromAPIInternal(
    const std::string& selection,
    const std::vector<string16>& selection_args) {
  scoped_refptr<RemoveHistoryFromAPIHelper> helper(
      new RemoveHistoryFromAPIHelper(service_.get(),
                                     &cancelable_consumer_history_));
  helper->RemoveHistory(selection, selection_args);
  helper->Wait();
  return helper->removed_row_count();
}

int64 ChromeBrowserProvider::AddSearchTermFromAPI(
    const history::SearchRow& row) {
  WaitTemplateURLModelLoaded();
  scoped_refptr<AddSearchTermFromAPIHelper> helper(
      new AddSearchTermFromAPIHelper(service_.get(),
                                     &cancelable_consumer_history_,
                                     profile_));
  helper->AddSearchTerm(row);
  helper->Wait();
  return helper->search_id();
}

history::AndroidStatement* ChromeBrowserProvider::QuerySearchTermFromAPI(
    const std::vector<history::SearchRow::SearchColumnID>& projections,
    const std::string& selection,
    const std::vector<string16>& selection_args,
    const std::string& sort_order) {
  scoped_refptr<QuerySearchTermFromAPIHelper> helper(
      new QuerySearchTermFromAPIHelper(service_.get(),
                                       &cancelable_consumer_history_));
  helper->QuerySearchTerm(projections, selection, selection_args, sort_order);
  helper->Wait();
  return helper->statement();
}

int ChromeBrowserProvider::UpdateSearchTermFromAPI(
    const history::SearchRow& row,
    const std::string& selection,
    const std::vector<string16>& selection_args) {
  WaitTemplateURLModelLoaded();
  scoped_refptr<UpdateSearchTermFromAPIHelper> helper(
      new UpdateSearchTermFromAPIHelper(service_.get(),
                                        &cancelable_consumer_history_,
                                        profile_));
  helper->UpdateSearchTerm(row, selection, selection_args);
  helper->Wait();
  return helper->updated_row_count();
}

int ChromeBrowserProvider::RemoveSearchTermFromAPI(
    const std::string& selection,
    const std::vector<string16>& selection_args) {
  scoped_refptr<RemoveSearchTermFromAPIHelper> helper(
      new RemoveSearchTermFromAPIHelper(service_.get(),
                                        &cancelable_consumer_history_));
  helper->RemoveSearchTerm(selection, selection_args);
  helper->Wait();
  return helper->removed_row_count();
}

base::android::ScopedJavaLocalRef<jbyteArray>
    ChromeBrowserProvider::GetFaviconOrTouchIcon(
    JNIEnv* env, jobject obj, jstring jurl) {
  if (!jurl)
    return ScopedJavaLocalRef<jbyteArray>();

  GURL url = GURL(ConvertJavaStringToUTF16(env, jurl));
  history::FaviconData favicon = GetBookmarkFaviconOrTouchIconInternal(url);
  if (!favicon.is_valid() || !favicon.image_data.get())
    return ScopedJavaLocalRef<jbyteArray>();

  return base::android::ToJavaByteArray(env, favicon.image_data->front(),
                                        favicon.image_data->size());
}

base::android::ScopedJavaLocalRef<jbyteArray>
    ChromeBrowserProvider::GetThumbnail(
    JNIEnv* env, jobject obj, jstring jurl) {
  if (!jurl)
    return ScopedJavaLocalRef<jbyteArray>();

  GURL url = GURL(ConvertJavaStringToUTF16(env, jurl));
  scoped_refptr<RefCountedMemory> thumbnail = GetBookmarkThumbnailInternal(url);
  if (!thumbnail.get() || !thumbnail->front()) {
    return ScopedJavaLocalRef<jbyteArray>();
  }

  return base::android::ToJavaByteArray(env, thumbnail->front(),
      thumbnail->size());
}

void ChromeBrowserProvider::BookmarkModelChanged() {
  java_browser_provider_.OnBookmarkChanged();
}

void ChromeBrowserProvider::Observe(int type,
                                    const content::NotificationSource& source,
                                    const content::NotificationDetails& details) {
  if (type == chrome::NOTIFICATION_HISTORY_URL_VISITED ||
      type == chrome::NOTIFICATION_HISTORY_URLS_DELETED) {
    java_browser_provider_.OnBookmarkChanged();
  } else if (type ==
      chrome::NOTIFICATION_HISTORY_KEYWORD_SEARCH_TERM_UPDATED) {
    java_browser_provider_.OnSearchTermChanged();
  } else if (type == chrome::NOTIFICATION_TEMPLATE_URL_SERVICE_LOADED) {
    template_loaded_event_.Signal();
  }
}

void ChromeBrowserProvider::WaitTemplateURLModelLoaded() {
  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile_);
  if (!template_url_service->loaded())
    template_loaded_event_.Wait();
}

// Register native methods
bool RegisterChromeBrowserProvider(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
