// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains stubs for some Chrome for Android specific code that is
// needed to compile some tests.

#include "clank/native/framework/chrome/partner_bookmarks_shim.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "chrome/browser/bookmarks/bookmark_model.h"

using base::LazyInstance;

namespace {

static LazyInstance<PartnerBookmarksShim> g_partner_bookmarks_shim_instance =
    LAZY_INSTANCE_INITIALIZER;

} //namespace

PartnerBookmarksShim* PartnerBookmarksShim::GetInstance() {
  return g_partner_bookmarks_shim_instance.Pointer();
}

PartnerBookmarksShim::PartnerBookmarksShim()
    : bookmark_model_(NULL),
      attach_point_(NULL),
      favicon_service_(NULL),
      loaded_(false),
      observers_(
          ObserverList<PartnerBookmarksShim::Observer>::NOTIFY_EXISTING_ONLY),
      wip_next_available_id_(0),
      java_partner_bookmarks_shim_(NULL) {
}

void PartnerBookmarksShim::AddObserver(
    PartnerBookmarksShim::Observer* observer) {
  observers_.AddObserver(observer);
}

void PartnerBookmarksShim::RemoveObserver(
    PartnerBookmarksShim::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void PartnerBookmarksShim::Init(JNIEnv* env, jobject obj) {
}

PartnerBookmarksShim::~PartnerBookmarksShim() {
  FOR_EACH_OBSERVER(PartnerBookmarksShim::Observer, observers_,
                    ShimBeingDeleted(this));
}

void PartnerBookmarksShim::Destroy(JNIEnv* env, jobject obj) {
}

bool PartnerBookmarksShim::IsPartnerBookmarkId(int64 id) {
  return false;
}

bool PartnerBookmarksShim::IsBookmarkEditable(const BookmarkNode* node) {
  return !IsPartnerBookmarkId(node->id()) &&
      (node->type() == BookmarkNode::FOLDER ||
          node->type() == BookmarkNode::URL);
}

void PartnerBookmarksShim::SetFaviconService(FaviconService* favicon_service) {
}

void PartnerBookmarksShim::AttachTo(
    BookmarkModel* bookmark_model,
    const BookmarkNode* attach_point) {
  if (bookmark_model_ == bookmark_model && attach_point_ == attach_point)
    return;
  DCHECK(!bookmark_model_ || !bookmark_model);
  bookmark_model_ = bookmark_model;
}

const BookmarkNode* PartnerBookmarksShim::GetParentOf(
    const BookmarkNode* node) const {
  DCHECK(node != NULL);
  const BookmarkNode* parent = node->parent();
  return parent;
}

bool PartnerBookmarksShim::IsRootNode(const BookmarkNode* node) const {
  DCHECK(node != NULL);
  return node->is_root();
}

const BookmarkNode* PartnerBookmarksShim::GetNodeByID(int64 id) const {
  if (bookmark_model_)
    return bookmark_model_->GetNodeByID(id);
  return NULL;
}

const BookmarkNode* PartnerBookmarksShim::GetNodeByID(
    const BookmarkNode* parent, int64 id) const {
  if (parent->id() == id)
    return parent;
  for(int i= 0, child_count = parent->child_count(); i < child_count; ++i) {
    const BookmarkNode* result = GetNodeByID(parent->GetChild(i), id);
    if (result)
      return result;
  }
  return NULL;
}

bool PartnerBookmarksShim::IsLoaded() const {
  return true;
}

bool PartnerBookmarksShim::HasPartnerBookmarks() const {
  return false;
}

const BookmarkNode* PartnerBookmarksShim::GetPartnerBookmarksRoot() const {
  return NULL;
}

void PartnerBookmarksShim::PartnerBookmarksCreationComplete(JNIEnv*, jobject) {
}

jlong PartnerBookmarksShim::AddPartnerBookmark(JNIEnv* env,
                                               jobject obj,
                                               jstring jurl,
                                               jstring jtitle,
                                               jboolean is_folder,
                                               jlong parent_id,
                                               jbyteArray favicon,
                                               jbyteArray touchicon) {
  return -1;
}


// ----------------------------------------------------------------

bool RegisterPartnerBookmarksShim(JNIEnv* env) {
  return false;
}

