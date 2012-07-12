// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_PARTNER_BOOKMARKS_SHIM_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_PARTNER_BOOKMARKS_SHIM_H_

// scoped_ptr requires complete class BookmarkNode, so we don't forward declare
#include "base/memory/scoped_ptr.h"
#include "content/browser/android/jni_helper.h"
#include "chrome/browser/bookmarks/bookmark_model.h"

// PartnerBookmarksShim: partner bookmarks shim over the BookmarkModel.
// Partner bookmarks folder is pseudo-injected as a subfolder to "attach node".
// Because we cannot modify the BookmarkModel, the following needs to
// be done on a client side:
// 1. bookmark_node->is_root() -> shim->IsRootNode(bookmark_node)
// 2. bookmark_node->parent() -> shim->GetParentOf(bookmark_node)
// 3. bookmark_model->GetNodeByID(id) -> shim->GetNodeByID(id)

class PartnerBookmarksShim {
 public:
  // Returns the active instance of the shim.
  static PartnerBookmarksShim* GetInstance();

  // FaviconService must be set before Java side will read the content.
  void SetFaviconService(FaviconService* favicon_service);
  // Pseudo-injects partner bookmarks (if any) under the "attach_node".
  void AttachTo(BookmarkModel* bookmark_model,
                const BookmarkNode* attach_node);
  // Returns true if everything got loaded and attached
  bool IsLoaded() const;
  // Returns true if there are partner bookmarks
  bool HasPartnerBookmarks() const;

  // For "Loaded" and "ShimBeingDeleted" notifications
  struct Observer {
    // Called when everything is loaded and attached
    virtual void PartnerShimLoaded(PartnerBookmarksShim*) {}
    // Called just before everything got destroyed
    virtual void ShimBeingDeleted(PartnerBookmarksShim*) {}
   protected:
    virtual ~Observer() {}
  };
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Replacements for BookmarkModel/BookmarkNode methods
  static bool IsBookmarkEditable(const BookmarkNode* node);
  bool IsRootNode(const BookmarkNode* node) const;
  const BookmarkNode* GetNodeByID(int64 id) const;
  const BookmarkNode* GetParentOf(const BookmarkNode* node) const;
  const BookmarkNode* GetAttachPoint() const { return attach_point_; }
  static bool IsPartnerBookmarkId(int64 id);
  const BookmarkNode* GetPartnerBookmarksRoot() const;

  // JNI methods
  void Init(JNIEnv* env, jobject obj);
  void Destroy(JNIEnv* env, jobject obj);
  jlong AddPartnerBookmark(JNIEnv* env,
                           jobject obj,
                           jstring jurl,
                           jstring jtitle,
                           jboolean is_folder,
                           jlong parent_id,
                           jbyteArray favicon,
                           jbyteArray touchicon);
  void PartnerBookmarksCreationComplete(JNIEnv* env, jobject obj);

  // Constructor is public for LazyInstance; DON'T CALL; use ::GetInstance().
  PartnerBookmarksShim();
  // Destructor is public for LazyInstance; DON'T CALL; called from Java.
  ~PartnerBookmarksShim();

 private:
  const BookmarkNode* GetNodeByID(const BookmarkNode* parent, int64 id) const;

  scoped_ptr<BookmarkNode> partner_bookmarks_root_;
  BookmarkModel* bookmark_model_;
  const BookmarkNode* attach_point_;
  FaviconService* favicon_service_;
  bool loaded_;  // Set only on UI thread

  // The observers.
  ObserverList<Observer> observers_;

  // JNI
  scoped_ptr<BookmarkNode> wip_partner_bookmarks_root_;
  int64 wip_next_available_id_;
  JavaObjectWeakGlobalRef* java_partner_bookmarks_shim_;

  DISALLOW_COPY_AND_ASSIGN(PartnerBookmarksShim);
};

// JNI registration
bool RegisterPartnerBookmarksShim(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_PARTNER_BOOKMARKS_SHIM_H_
