// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/instant_tab.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "chrome/browser/infobars/infobar_tab_helper.h"
#include "chrome/browser/instant/instant_loader.h"
#include "chrome/browser/instant/instant_loader_delegate.h"
#include "chrome/browser/ui/blocked_content/blocked_content_tab_helper.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "content/browser/android/chrome_view.h"
#include "jni/instant_tab_jni.h"

using base::android::AttachCurrentThread;

class InstantTab::InstantLoaderDelegateImpl : public InstantLoaderDelegate {
 public:
  explicit InstantLoaderDelegateImpl(InstantTab* tab) : instant_tab_(tab) {
  }

  virtual void ShowInstantLoader(InstantLoader* loader) {
    // We show our content when the URL gets loaded.
  }

  virtual void InstantStatusChanged(InstantLoader* loader) {
  }

  virtual void SetSuggestedTextFor(InstantLoader* loader,
                                   const string16& text,
                                   InstantCompleteBehavior behavior) {
  }

  virtual gfx::Rect GetInstantBounds() {
    // This is only used when displaying search results.
    // Since we are only prefetching pages, this is not used.
    return gfx::Rect();
  }

  virtual bool ShouldCommitInstantOnMouseUp() {
    return false;
  }

  virtual void CommitInstantLoader(InstantLoader* loader) {
  }

  virtual void InstantLoaderDoesntSupportInstant(InstantLoader* loader) {
  }

  virtual void AddToBlacklist(InstantLoader* loader, const GURL& url) {
    // Since we have only one InstantLoader, we should only get that
    // notification from that loader.
    DCHECK(instant_tab_->instant_loader_ == loader);
    host_blacklist_.insert(url.host());
    DiscardPrefetch();
  }

  virtual void InterstitialCreated() {
    DiscardPrefetch();
  }

  virtual void SwappedTabContents(InstantLoader* loader) {
  }

  virtual void TabContentsCreated(TabContentsWrapper* tab_contents_wrapper) {
    instant_tab_->TabContentsCreated(tab_contents_wrapper);
  }

  bool IsURLBlacklisted(const GURL& url) {
    return host_blacklist_.count(url.host()) > 0;
  }

 private:
  void DiscardPrefetch() {
    instant_tab_->canceled_ = true;
    // We cannot invoked DiscardInstant directly as the it would result in the
    // instant TabContents being destroyed and the TabContents is in the call
    // stack when delegate methods are called.  We post a task to make that
    // happen once the stack has unwound.
    MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&InstantTab::DiscardPrefetch,
                          instant_tab_->weak_instant_method_factory_.GetWeakPtr()));
  }

  InstantTab* instant_tab_;
  std::set<std::string> host_blacklist_;

  DISALLOW_COPY_AND_ASSIGN(InstantLoaderDelegateImpl);
};

// ----------------------------------------------------------------

InstantTab::InstantTab(JNIEnv* env,
                       jobject obj,
                       jobject to_replace,
                       jobject my_chrome_view)
    : committed_(false),
      canceled_(false),
      java_instant_tab_(env, obj),
      to_replace_jchrome_view_(env, to_replace),
      my_jchrome_view_(env, my_chrome_view),
      weak_instant_method_factory_(this) {
  instant_loader_delegate_.reset(new InstantLoaderDelegateImpl(this));
  InitInstantLoader();
}

InstantTab::~InstantTab() {
  DiscardPrefetch();
  weak_instant_method_factory_.InvalidateWeakPtrs();
}

void InstantTab::InitInstantLoader() {
  if (!instant_loader_.get())
    instant_loader_.reset(new InstantLoader(instant_loader_delegate_.get(), 0, ""));
}

void InstantTab::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

bool InstantTab::PrefetchUrl(JNIEnv* env, jobject obj,
                             jstring jurl, int page_transition) {
  canceled_ = false;
  GURL url = GURL(base::android::ConvertJavaStringToUTF8(env, jurl));
  if (url.is_empty()) {
    DiscardPrefetch();
    return false;
  }
  // Some sites might refuse to be prefetched.
  if (instant_loader_delegate_->IsURLBlacklisted(url)) {
    DiscardPrefetch();
    return false;
  }

  // We might not have an instant loader anymore if we were previously
  // discarded.
  InitInstantLoader();

  // If infobars showed up due to an instant load, remove them.  Some
  // infobars don't remove themselves when navigating away from a
  // page.  For example, the x-auto-login infobar persists to protect
  // against login redirects.  It can be awkward if gmail.com was
  // instant loaded and the x-auto-login infobar showed up, but you
  // didn't commit instant until "gmail help" was typed.  In that case
  // you might get an infobar when you never "asked" for gmail login.
  TabContentsWrapper* preview_contents = instant_loader_->preview_contents();
  if (preview_contents) {
    while (preview_contents->infobar_tab_helper()->infobar_count()) {
      preview_contents->infobar_tab_helper()->RemoveInfoBar(
          preview_contents->infobar_tab_helper()->GetInfoBarDelegateAt(0));
    }
  }

  ChromeView* to_replace = ChromeView::GetNativeChromeView(
      env, to_replace_jchrome_view_.get(env).obj());
  if (!to_replace) {
    NOTREACHED();
    DiscardPrefetch();
    return false;
  }
  string16 suggested_text;
  instant_loader_->Update(to_replace->tab_contents_wrapper(), NULL,
                          url, content::PageTransitionFromInt(page_transition),
                          string16(), false, &suggested_text);
  return true;
}

void InstantTab::DiscardPrefetch() {
  JNIEnv* env = AttachCurrentThread();
  if (!committed_) {
    ChromeView* my_chrome_view = ChromeView::GetNativeChromeView(
        env, my_jchrome_view_.get(env).obj());
    // Make sure nobody has a pointer to the tab_contents owned by our
    // instant loader.
    if (my_chrome_view)
      my_chrome_view->SetWeakTabContentsWrapper(NULL);
    Java_InstantTab_discardPrefetch(env, java_instant_tab_.get(env).obj());
  }
  instant_loader_.reset();
}

jboolean InstantTab::CommitPrefetch(JNIEnv* env, jobject obj) {
  if (!instant_loader_.get() || !instant_loader_->main_frame_committed()) {
    LOG(WARNING) << "Instant discarded for " << instant_loader_->url().spec() <<
        " main frame not yet committed.";
    return JNI_FALSE;
  }

  TabContentsWrapper* new_tab_contents_wrapper =
      instant_loader_->ReleasePreviewContents(
        INSTANT_COMMIT_PRESSED_ENTER, NULL);
  ChromeView* my_chrome_view = ChromeView::GetNativeChromeView(
      env, my_jchrome_view_.get(env).obj());

  // Copy over nav state
  ChromeView* to_replace_chrome_view = ChromeView::GetNativeChromeView(
      env, to_replace_jchrome_view_.get(env).obj());
  if (!my_chrome_view || !to_replace_chrome_view) {
    NOTREACHED();
    delete new_tab_contents_wrapper;
    return JNI_FALSE;
  }
  TabContentsWrapper* to_replace_tab_contents_wrapper =
      to_replace_chrome_view->tab_contents_wrapper();
  content::WebContents* web_contents = new_tab_contents_wrapper->web_contents();
  web_contents->GetController().CopyStateFromAndPrune(
      &to_replace_tab_contents_wrapper->web_contents()->GetController());
  my_chrome_view->SetTabContentsWrapper(new_tab_contents_wrapper);
  new_tab_contents_wrapper->blocked_content_tab_helper()->SetAllContentsBlocked(
      false);
  committed_ = true;
  return JNI_TRUE;
}

jboolean InstantTab::PrefetchCanceled(JNIEnv* env, jobject obj) {
  return canceled_;
}

void InstantTab::TabContentsCreated(TabContentsWrapper* tab_contents_wrapper) {
  JNIEnv* env = AttachCurrentThread();
  ChromeView* my_chrome_view = ChromeView::GetNativeChromeView(
      env, my_jchrome_view_.get(env).obj());

  // We do NOT pass ownership of the instant tab_contents to the ChromeView.
  my_chrome_view->SetWeakTabContentsWrapper(tab_contents_wrapper);
}

// ----------------------------------------------------------------

static int Init(JNIEnv* env, jobject obj,
                jobject toReplace, jobject myChromeView) {
  InstantTab* instant_tab = new InstantTab(env, obj, toReplace, myChromeView);
  return reinterpret_cast<jint>(instant_tab);
}

bool RegisterInstantTab(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
