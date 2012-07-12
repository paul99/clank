// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/bookmarks_handler.h"

#include "base/logging.h"
#include "base/string_number_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/browser/android/chrome_view.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/sync/glue/synced_window_delegate.h"
#include "chrome/browser/sync/glue/synced_window_delegate_registry.h"
#include "clank/native/framework/chrome/partner_bookmarks_shim.h"
#include "clank/native/framework/chrome/tab_model.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/color_analysis.h"

using content::BrowserThread;

static const char* kParentIdParam = "parent_id";
static const char* kStateParam = "state";
static const char* kNodeIdParam = "node_id";

static const char* kNodeAddedState = "NODE_ADDED";
static const char* kNodeRemovedState = "NODE_REMOVED";
static const char* kNodeChangedState = "NODE_CHANGED";

namespace {
// Convert the given bookmark node into a dictionary format to be returned to
// Javascript.
void PopulateBookmark(const BookmarkNode* node,
                      ListValue* result) {
  if (result != NULL) {
    DictionaryValue* filler_value = new DictionaryValue();
    filler_value->SetString("title", node->GetTitle());
    // Mark reserved system nodes and partner bookmarks as uneditable
    // (i.e. the bookmark bar along with the "Other Bookmarks" folder).
    filler_value->SetBoolean("editable",
                             PartnerBookmarksShim::IsBookmarkEditable(node));
    if (node->is_url()) {
      filler_value->SetBoolean("folder", false);
      filler_value->SetString("url", node->url().spec());
    } else
      filler_value->SetBoolean("folder", true);
    filler_value->SetString("id", base::Int64ToString(node->id()));
    filler_value->SetString("type", node->type_as_string());
    result->Append(filler_value);
  }
}

// Sets the necessary parent information in the response object to be sent
// to the UI renderer.
void SetParentInBookmarksResult(const BookmarkNode& parent,
                                DictionaryValue* result) {
  result->SetString(kParentIdParam, base::Int64ToString(parent.id()));
}

SkColor GetDominantColorForFavicon(scoped_refptr<RefCountedMemory> png) {
  color_utils::GridSampler sampler;
  // 100 here is the darkness_limit which represents the minimum sum of the RGB
  // components that is acceptable as a color choice. This can be from 0 to 765.
  // 665 here is the brightness_limit represents the maximum sum of the RGB
  // components that is acceptable as a color choice. This can be from 0 to 765.
  return color_utils::CalculateKMeanColorOfPNG(png, 100, 665, sampler);
}

}  // namespace

BookmarksHandler::BookmarksHandler() :
    bookmark_model_(NULL),
    partner_bookmarks_shim_(NULL),
    extensive_changes_(false),
    creating_shortcut_(false) {
}

BookmarksHandler::~BookmarksHandler() {
  if (bookmark_model_)
    bookmark_model_->RemoveObserver(this);
  if (partner_bookmarks_shim_)
    partner_bookmarks_shim_->RemoveObserver(this);
}

void BookmarksHandler::RegisterMessages() {
  // Listen for the bookmark change. We need the both bookmark and folder
  // change, the NotificationService is not sufficient.
  Profile* profile = Profile::FromBrowserContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  bookmark_model_ = profile->GetBookmarkModel();
  if (bookmark_model_) {
    bookmark_model_->AddObserver(this);
    // Since a sync or import could have started before this class is
    // initialized, we need to make sure that our initial state is
    // up to date.
    extensive_changes_ = bookmark_model_->IsDoingExtensiveChanges();
  }
  // Create the partner Bookmarks shim as early as possible (but don't attach)
  if (!partner_bookmarks_shim_) {
    partner_bookmarks_shim_ = PartnerBookmarksShim::GetInstance();
    partner_bookmarks_shim_->AddObserver(this);
    partner_bookmarks_shim_->SetFaviconService(
        profile->GetFaviconService(Profile::EXPLICIT_ACCESS));
  }

  // Register ourselves as the handler for the "getBookmarks" message
  // from Javascript.
  web_ui()->RegisterMessageCallback("getBookmarks",
      base::Bind(&BookmarksHandler::HandleGetBookmarks,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("deleteBookmark",
      base::Bind(&BookmarksHandler::HandleDeleteBookmark,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("shortcutToBookmark",
      base::Bind(&BookmarksHandler::HandleShortcutToBookmark,
                 base::Unretained(this)));
}

void BookmarksHandler::HandleGetBookmarks(const ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  Profile* profile = Profile::FromBrowserContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!profile->GetBookmarkModel()->IsLoaded())
    return;  // is handled with a Loaded() callback in MobileNewTabUI.

  // Attach the Partner Bookmarks shim under the Mobile Bookmarks.
  // Cannot do this earlier because mobile_node is not yet set.
  DCHECK(partner_bookmarks_shim_ != NULL);
  if (bookmark_model_)
    partner_bookmarks_shim_->AttachTo(
        bookmark_model_, bookmark_model_->mobile_node());
  if (!partner_bookmarks_shim_->IsLoaded())
    return; // is handled with a PartnerShimLoaded() callback

  // TODO(tedchoc): Bookmark IDs are int64s, should this be one as well?
  int id;
  if (args && !args->empty() && ExtractIntegerValue(args, &id))
    QueryBookmarkFolder(id);
  else
    QueryInitialBookmarks();
}

void BookmarksHandler::HandleDeleteBookmark(const ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // TODO(tedchoc): Bookmark IDs are int64s, should this be one as well?
  int id;
  if (args && !args->empty() && ExtractIntegerValue(args, &id)) {
    DCHECK(!partner_bookmarks_shim_->IsPartnerBookmarkId(id));
    const BookmarkNode* node = bookmark_model_->GetNodeByID(id);
    if (node && node->parent()) {
      const BookmarkNode* parent_node = node->parent();
      bookmark_model_->Remove(parent_node, parent_node->GetIndexOf(node));
    }
  }
}

// List all bookmarks and sub folders in the given folder, and convert them into
// a dictionary format to be returned to Javascript.
void BookmarksHandler::PopulateBookmarksInFolder(
    const BookmarkNode* folder,
    const scoped_ptr<DictionaryValue>& result) {
  scoped_ptr<ListValue> bookmarks(new ListValue);

  for (int i = 0; i < folder->child_count(); i++) {
    const BookmarkNode* bookmark=
        (const BookmarkNode*) folder->GetChild(i);
    PopulateBookmark(bookmark, bookmarks.get());
  }

  // Make sure we iterate over the partner's attach point
  DCHECK(partner_bookmarks_shim_ != NULL);
  if (partner_bookmarks_shim_->HasPartnerBookmarks() &&
      folder == partner_bookmarks_shim_->GetAttachPoint())
    PopulateBookmark(
        partner_bookmarks_shim_->GetPartnerBookmarksRoot(), bookmarks.get());

  scoped_ptr<ListValue> folder_hierarchy(new ListValue);
  const BookmarkNode* parent = partner_bookmarks_shim_->GetParentOf(folder);

  while (parent != NULL) {
    scoped_ptr<DictionaryValue> hierarchy_entry(new DictionaryValue);
    if (partner_bookmarks_shim_->IsRootNode(parent))
      hierarchy_entry->SetBoolean("root", true);
    hierarchy_entry->SetString("title", parent->GetTitle());
    hierarchy_entry->SetString("id", base::Int64ToString(parent->id()));
    folder_hierarchy->Append(hierarchy_entry.release());
    parent = partner_bookmarks_shim_->GetParentOf(parent);
  }

  result->SetString("title", folder->GetTitle());
  result->SetString("id", base::Int64ToString(folder->id()));
  result->SetBoolean("root", partner_bookmarks_shim_->IsRootNode(folder));
  result->Set("bookmarks", bookmarks.release());
  result->Set("hierarchy", folder_hierarchy.release());
}

void BookmarksHandler::QueryBookmarkFolder(const int64& folder_id) {
  DCHECK(partner_bookmarks_shim_ != NULL);
  const BookmarkNode* bookmarks = partner_bookmarks_shim_->GetNodeByID(folder_id);
  if (bookmarks) {
    scoped_ptr<DictionaryValue> result(new DictionaryValue);
    BookmarksHandler::PopulateBookmarksInFolder(bookmarks, result);
    SendResult(result);
  } else {
    // If we receive an ID that no longer maps to a bookmark folder, just
    // return the initial bookmark folder.
    BookmarksHandler::QueryInitialBookmarks();
  }
}

void BookmarksHandler::QueryInitialBookmarks() {
  scoped_ptr<DictionaryValue> result(new DictionaryValue);
  DCHECK(partner_bookmarks_shim_ != NULL);
  BookmarksHandler::PopulateBookmarksInFolder(
      // We have to go to the partner Root if it exists
      partner_bookmarks_shim_->HasPartnerBookmarks() ?
          partner_bookmarks_shim_->GetPartnerBookmarksRoot() :
          bookmark_model_->mobile_node(),
      result);
  SendResult(result);
}

void BookmarksHandler::SendResult(const scoped_ptr<DictionaryValue>& result) {
  if (result.get()) {
    web_ui()->CallJavascriptFunction("bookmarks",
                                    *(result.get()));
  }
}

void BookmarksHandler::Loaded(BookmarkModel* model,
                              bool ids_reassigned) {
  BookmarkModelChanged();
}

void BookmarksHandler::PartnerShimLoaded(PartnerBookmarksShim* shim) {
  BookmarkModelChanged();
}

void BookmarksHandler::ShimBeingDeleted(PartnerBookmarksShim* shim) {
  partner_bookmarks_shim_ = NULL;
}

void BookmarksHandler::ExtensiveBookmarkChangesBeginning(BookmarkModel* model) {
  extensive_changes_ = true;
}

void BookmarksHandler::ExtensiveBookmarkChangesEnded(BookmarkModel* model) {
  extensive_changes_ = false;
  BookmarkModelChanged();
}

void BookmarksHandler::BookmarkNodeRemoved(BookmarkModel* model,
                                           const BookmarkNode* parent,
                                           int old_index,
                                           const BookmarkNode* node) {
  scoped_ptr<DictionaryValue> result(new DictionaryValue);
  result->SetString(kStateParam, kNodeRemovedState);
  SetParentInBookmarksResult(*parent, result.get());
  result->SetString(kNodeIdParam, base::Int64ToString(node->id()));
  NotifyModelChanged(*(result.get()));
}

void BookmarksHandler::BookmarkNodeAdded(BookmarkModel* model,
                                         const BookmarkNode* parent,
                                         int index) {
  scoped_ptr<DictionaryValue> result(new DictionaryValue);
  result->SetString(kStateParam, kNodeAddedState);
  SetParentInBookmarksResult(*parent, result.get());
  NotifyModelChanged(*(result.get()));
}

void BookmarksHandler::BookmarkNodeChanged(BookmarkModel* model,
                                           const BookmarkNode* node) {
  scoped_ptr<DictionaryValue> result(new DictionaryValue);
  result->SetString(kStateParam, kNodeChangedState);
  DCHECK(partner_bookmarks_shim_ != NULL);
  DCHECK(!partner_bookmarks_shim_->IsPartnerBookmarkId(node->id()));
  SetParentInBookmarksResult(*(node->parent()), result.get());
  result->SetString(kNodeIdParam, base::Int64ToString(node->id()));
  NotifyModelChanged(*(result.get()));
}

void BookmarksHandler::BookmarkModelChanged() {
  if (!extensive_changes_)
    web_ui()->CallJavascriptFunction("bookmarkChanged");
}

void BookmarksHandler::NotifyModelChanged(const DictionaryValue& status) {
  if (!extensive_changes_)
    web_ui()->CallJavascriptFunction("bookmarkChanged", status);
}

void BookmarksHandler::HandleShortcutToBookmark(const ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  Profile* profile = Profile::FromBrowserContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!profile)
    return;

  // TODO(tedchoc): Bookmark IDs are int64s, should this be one as well?
  int id;
  if (args && !args->empty() && ExtractIntegerValue(args, &id)) {
    DCHECK(partner_bookmarks_shim_ != NULL);
    BookmarkNode* node = const_cast<BookmarkNode*>(
        partner_bookmarks_shim_->GetNodeByID(id));
    if (!node)
      return;
    creating_shortcut_ = true;
    FaviconService* favicon_service = profile->GetFaviconService(
        Profile::EXPLICIT_ACCESS);
    FaviconService::Handle handle = favicon_service->GetFaviconForURL(
        node->url(),
        history::FAVICON | history::TOUCH_ICON,
        &cancelable_consumer_,
        base::Bind(&BookmarksHandler::OnFaviconDataAvailable,
                   base::Unretained(this)));
    cancelable_consumer_.SetClientData(favicon_service, handle, node);
  }
}

void BookmarksHandler::OnFaviconDataAvailable(
    FaviconService::Handle handle,
    history::FaviconData favicon) {
  if (creating_shortcut_) {
    SkColor color = SK_ColorWHITE;
    SkBitmap favicon_bitmap;
    if (favicon.is_valid()) {
      color = GetDominantColorForFavicon(favicon.image_data);
      gfx::PNGCodec::Decode(favicon.image_data->front(),
                            favicon.image_data->size(),
                            &favicon_bitmap);
    }
    Profile* profile = Profile::FromBrowserContext(
        web_ui()->GetWebContents()->GetBrowserContext());
    BookmarkNode* node = cancelable_consumer_.GetClientData(
        profile->GetFaviconService(Profile::EXPLICIT_ACCESS), handle);
    ChromeView* view = web_ui()->GetWebContents()->GetNativeView();
    view->AddShortcutToBookmark(node->url(), node->GetTitle(),
                                favicon_bitmap, SkColorGetR(color),
                                SkColorGetG(color), SkColorGetB(color));
    creating_shortcut_ = false;
  }
}
