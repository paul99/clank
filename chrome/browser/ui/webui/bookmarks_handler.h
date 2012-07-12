// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_BOOKMARKS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_BOOKMARKS_HANDLER_H_
#pragma once

#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "chrome/browser/bookmarks/base_bookmark_model_observer.h"
#include "chrome/browser/cancelable_request.h"
#include "chrome/browser/favicon/favicon_service.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "clank/native/framework/chrome/partner_bookmarks_shim.h"

// The handler for Javascript messages related to the bookmarks.
//
// In Javascript if getBookmarks() is called without any parameter, the 'Other
// Bookmark' folder and bookmark bar's bookmarks and folders are returned.
// If getBookmarks() is called with a valid bookmark folder id, the given
// folder's bookmarks and sub folders of are returned.
//
// All bookmarks and subfolder is returned by bookmarks() javascript callback
// function.
// The returned field 'folder' indicates whether the data is a folder. The
// returned field 'root' indicates whether or not the bookmark list that was
// returned is the root list or not.  Besides these fields, a folder has id
// and title fields; A bookmark has url and title fields.
//
// A sample result looks like:
// {
//   title: 'Bookmark Bar',
//   id: '1',
//   root: true,
//   bookmarks: [
//     {
//       title: 'Cake',
//       url: 'http://www.google.com',
//       folder: false
//     },
//     {
//       title: 'Puppies',
//       folder: true,
//       id: '2'
//     }
//   ]
// }
class BookmarksHandler : public content::WebUIMessageHandler,
                         public BaseBookmarkModelObserver,
                         public PartnerBookmarksShim::Observer {
 public:
  BookmarksHandler();
  virtual ~BookmarksHandler();

  // WebUIMessageHandler override and implementation.
  virtual void RegisterMessages() OVERRIDE;

  // Callback for the "getBookmarks" message.
  void HandleGetBookmarks(const ListValue* args);
  // Callback for the "deleteBookmark" message.
  void HandleDeleteBookmark(const ListValue* args);
  // Callback for the "shortcutToBookmark" message.
  void HandleShortcutToBookmark(const base::ListValue* args);

  // Notify the UI that a change occurred to the bookmark model.
  virtual void NotifyModelChanged(const DictionaryValue& status);

  // Override the methods of BookmarkModelObserver
  virtual void Loaded(BookmarkModel* model, bool ids_reassigned) OVERRIDE;
  virtual void BookmarkModelChanged() OVERRIDE;
  virtual void ExtensiveBookmarkChangesBeginning(BookmarkModel* model) OVERRIDE;
  virtual void ExtensiveBookmarkChangesEnded(BookmarkModel* model) OVERRIDE;
  virtual void BookmarkNodeRemoved(BookmarkModel* model,
                                   const BookmarkNode* parent,
                                   int old_index,
                                   const BookmarkNode* node) OVERRIDE;
  virtual void BookmarkNodeAdded(BookmarkModel* model,
                                 const BookmarkNode* parent,
                                 int index) OVERRIDE;
  virtual void BookmarkNodeChanged(BookmarkModel* model,
                                   const BookmarkNode* node) OVERRIDE;

  // Override the methods of PartnerBookmarksShim::Observer
  virtual void PartnerShimLoaded(PartnerBookmarksShim* shim) OVERRIDE;
  virtual void ShimBeingDeleted(PartnerBookmarksShim* shim) OVERRIDE;

 private:
  // The bookmark model being observed (if it has been attached).
  BookmarkModel* bookmark_model_;

  // Information about the Partner bookmarks (must check for IsLoaded())
  PartnerBookmarksShim* partner_bookmarks_shim_;

  // Indicates that extensive changes to the BookmarkModel is on-going.
  bool extensive_changes_;

  // Indicates that a shortcut is being created.
  bool creating_shortcut_;

  // Used for loading bookmark node.
  CancelableRequestConsumerTSimple<BookmarkNode*> cancelable_consumer_;

  // Sends all bookmarks and sub folders in the given folder back to the NTP.
  void QueryBookmarkFolder(const int64& id);

  // Sends bookmark bar's bookmarks and sub folders and other folders back to
  // NTP.
  void QueryInitialBookmarks();

  // Sends the result back to Javascript
  void SendResult(const scoped_ptr<DictionaryValue>& result);

  // Given a bookmark folder node, |folder|, populate the |result| with the
  // structured javascript-formatted data regarding the folder.
  void PopulateBookmarksInFolder(const BookmarkNode* folder,
                                 const scoped_ptr<DictionaryValue>& result);

  // Called once the favicon is loaded and is available for use.
  void OnFaviconDataAvailable(FaviconService::Handle handle,
                              history::FaviconData favicon);

  DISALLOW_COPY_AND_ASSIGN(BookmarksHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_BOOKMARKS_HANDLER_H_
