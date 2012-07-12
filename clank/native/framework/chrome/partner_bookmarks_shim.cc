// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/partner_bookmarks_shim.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "content/public/browser/browser_thread.h"
#include "jni/partner_bookmarks_shim_jni.h"
#include "ui/gfx/codec/png_codec.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ConvertJavaStringToUTF16;
using base::LazyInstance;
using content::BrowserThread;

namespace {

const int64 PARTNER_BOOKMARK_ID_BITS = 0x7FFF0000;
static LazyInstance<PartnerBookmarksShim> g_partner_bookmarks_shim_instance =
    LAZY_INSTANCE_INITIALIZER;

void SetFaviconTask(FaviconService* favicon_service_,
                    const GURL& page_url, const GURL& icon_url,
                    const std::vector<unsigned char>& image_data,
                    history::IconType icon_type) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  favicon_service_->SetFavicon(page_url, icon_url, image_data, icon_type);
}

void SetFaviconCallback(FaviconService* favicon_service_,
                        const GURL& page_url, const GURL& icon_url,
                        const std::vector<unsigned char>& image_data,
                        history::IconType icon_type,
                        base::WaitableEvent* bookmark_added_event) {
  SetFaviconTask(favicon_service_, page_url, icon_url, image_data, icon_type);
  if (bookmark_added_event)
    bookmark_added_event->Signal();
}

void PrepareAndSetFavicon(JNIEnv* env, jbyte* icon_bytes, int icon_len,
                          BookmarkNode* node, FaviconService* favicon_service,
                          history::IconType icon_type) {
  SkBitmap icon_bitmap;
  if (!gfx::PNGCodec::Decode(
      reinterpret_cast<const unsigned char*>(icon_bytes),
      icon_len, &icon_bitmap))
    return;
  std::vector<unsigned char> image_data;
  if (!gfx::PNGCodec::EncodeBGRASkBitmap(icon_bitmap, false, &image_data))
    return;
  // TODO(aruslan): TODO(tedchoc): Follow up on how to avoid this through js.
  // Since the favicon URL is used as a key in the history's thumbnail DB,
  // this gives us a value which does not collide with others.
  GURL fake_icon_url = node->url();
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    SetFaviconTask(favicon_service,
                   node->url(), fake_icon_url,
                   image_data, icon_type);
  } else {
    base::WaitableEvent event(false, false);
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(&SetFaviconCallback,
                   favicon_service, node->url(), fake_icon_url,
                   image_data, icon_type, &event));
    // TODO(aruslan): http://b/6397072 If possible - avoid using favicon service
    event.Wait();
  }
}

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
      wip_next_available_id_(PARTNER_BOOKMARK_ID_BITS),
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
  java_partner_bookmarks_shim_ = new JavaObjectWeakGlobalRef(env, obj);
}

PartnerBookmarksShim::~PartnerBookmarksShim() {
  FOR_EACH_OBSERVER(PartnerBookmarksShim::Observer, observers_,
                    ShimBeingDeleted(this));
  delete java_partner_bookmarks_shim_;
  java_partner_bookmarks_shim_ = NULL;
}

void PartnerBookmarksShim::Destroy(JNIEnv* env, jobject obj) {
  delete java_partner_bookmarks_shim_;
  java_partner_bookmarks_shim_ = NULL;
}

bool PartnerBookmarksShim::IsPartnerBookmarkId(int64 id) {
  return (id & PARTNER_BOOKMARK_ID_BITS) == PARTNER_BOOKMARK_ID_BITS;
}

bool PartnerBookmarksShim::IsBookmarkEditable(const BookmarkNode* node) {
  return !IsPartnerBookmarkId(node->id()) &&
      (node->type() == BookmarkNode::FOLDER ||
          node->type() == BookmarkNode::URL);
}

void PartnerBookmarksShim::SetFaviconService(FaviconService* favicon_service) {
  DCHECK(!favicon_service_ || !favicon_service ||
      favicon_service_ == favicon_service);
  favicon_service_ = favicon_service;
}

void PartnerBookmarksShim::AttachTo(
    BookmarkModel* bookmark_model,
    const BookmarkNode* attach_point) {
  if (bookmark_model_ == bookmark_model && attach_point_ == attach_point)
    return;
  DCHECK(!bookmark_model_ || !bookmark_model);
  DCHECK(!attach_point_ || !attach_point);
  bookmark_model_ = bookmark_model;
  attach_point_ = attach_point;
}

const BookmarkNode* PartnerBookmarksShim::GetParentOf(
    const BookmarkNode* node) const {
  DCHECK(IsLoaded());
  DCHECK(node != NULL);
  const BookmarkNode* parent = node->parent();
  if (HasPartnerBookmarks() && GetPartnerBookmarksRoot()->id() == node->id())
    parent = GetAttachPoint();
  return parent;
}

bool PartnerBookmarksShim::IsRootNode(const BookmarkNode* node) const {
  DCHECK(node != NULL);
  return node->is_root() && !IsPartnerBookmarkId(node->id());
}

const BookmarkNode* PartnerBookmarksShim::GetNodeByID(int64 id) const {
  DCHECK(IsLoaded());
  if (HasPartnerBookmarks() && IsPartnerBookmarkId(id))
    return GetNodeByID(GetPartnerBookmarksRoot(), id);
  if (bookmark_model_)
    return bookmark_model_->GetNodeByID(id);
  return NULL;
}

const BookmarkNode* PartnerBookmarksShim::GetNodeByID(
    const BookmarkNode* parent, int64 id) const {
  if (parent->id() == id)
    return parent;
  if (IsLoaded() && HasPartnerBookmarks() &&
      IsPartnerBookmarkId(id) && !IsPartnerBookmarkId(parent->id())) {
    DCHECK(parent == attach_point_);
    parent = GetPartnerBookmarksRoot();
  }
  for(int i= 0, child_count = parent->child_count(); i < child_count; ++i) {
    const BookmarkNode* result = GetNodeByID(parent->GetChild(i), id);
    if (result)
      return result;
  }
  return NULL;
}

bool PartnerBookmarksShim::IsLoaded() const {
  return loaded_;
}

bool PartnerBookmarksShim::HasPartnerBookmarks() const {
  if (partner_bookmarks_root_.get() != NULL)
    return true;
  DCHECK(IsLoaded());
  return partner_bookmarks_root_.get() != NULL;
}

const BookmarkNode* PartnerBookmarksShim::GetPartnerBookmarksRoot() const {
  DCHECK(HasPartnerBookmarks());
  return partner_bookmarks_root_.get();
}

void PartnerBookmarksShim::PartnerBookmarksCreationComplete(JNIEnv*, jobject) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  partner_bookmarks_root_.swap(wip_partner_bookmarks_root_);
  wip_partner_bookmarks_root_.reset();
  wip_next_available_id_ = PARTNER_BOOKMARK_ID_BITS;
  loaded_ = true;
  FOR_EACH_OBSERVER(PartnerBookmarksShim::Observer, observers_,
                    PartnerShimLoaded(this));
}

jlong PartnerBookmarksShim::AddPartnerBookmark(JNIEnv* env,
                                               jobject obj,
                                               jstring jurl,
                                               jstring jtitle,
                                               jboolean is_folder,
                                               jlong parent_id,
                                               jbyteArray favicon,
                                               jbyteArray touchicon) {
  string16 url;
  string16 title;
  if (jurl)
    url = ConvertJavaStringToUTF16(env, jurl);
  if (jtitle)
    title = ConvertJavaStringToUTF16(env, jtitle);

  BookmarkNode* node = NULL;
  if (wip_partner_bookmarks_root_.get()) {
    DCHECK(parent_id > 0);
    node = new BookmarkNode(wip_next_available_id_++, GURL(url));
    node->set_type(is_folder ? BookmarkNode::FOLDER : BookmarkNode::URL);
    node->SetTitle(title);

    // Handle favicon and touchicon
    if (favicon_service_ != NULL && (favicon != NULL || touchicon != NULL)) {
      jbyteArray icon = (touchicon != NULL) ? touchicon : favicon;
      const history::IconType icon_type =
          touchicon ? history::TOUCH_ICON : history::FAVICON;
      const int icon_len = env->GetArrayLength(icon);
      jbyte* icon_bytes = env->GetByteArrayElements(icon, NULL);
      if (icon_bytes)
        PrepareAndSetFavicon(env, icon_bytes, icon_len,
                             node, favicon_service_, icon_type);
      env->ReleaseByteArrayElements(icon, icon_bytes, JNI_ABORT);
    }

    const BookmarkNode* parent =
        GetNodeByID(wip_partner_bookmarks_root_.get(), parent_id);
    if (!parent) {
      LOG(WARNING) << "partner_bookmarks_shim: invalid/unknown parent_id=" <<
          parent_id << ": adding to the root";
      parent = wip_partner_bookmarks_root_.get();
    }
    const_cast<BookmarkNode*>(parent)->Add(node, parent->child_count());
  } else {
    DCHECK(parent_id <= 0);
    node = new BookmarkPermanentNode(wip_next_available_id_++);
    node->SetTitle(title);
    wip_partner_bookmarks_root_.reset(node);
  }
  return node->id();
}


// ----------------------------------------------------------------

static int Init(JNIEnv* env, jobject obj) {
  PartnerBookmarksShim* partner_bookmarks_shim =
      PartnerBookmarksShim::GetInstance();
  partner_bookmarks_shim->Init(env, obj);
  return reinterpret_cast<jint>(partner_bookmarks_shim);
}

bool RegisterPartnerBookmarksShim(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
