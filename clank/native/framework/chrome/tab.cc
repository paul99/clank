// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/tab.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/compiler_specific.h"
#include "base/pickle.h"
#include "chrome/browser/automation/testing_automation_provider_android.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/browser_about_handler.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/google/google_url_tracker.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/search_engines/search_terms_data.h"
#include "chrome/browser/sessions/restore_tab_helper.h"
#include "chrome/browser/sessions/session_types.h"
#include "chrome/browser/sessions/tab_restore_service.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/sync/glue/synced_tab_delegate.h"
#include "chrome/browser/tab_contents/thumbnail_generator.h"
#include "chrome/browser/ui/blocked_content/blocked_content_tab_helper.h"
#include "chrome/browser/ui/sync/tab_contents_wrapper_synced_tab_delegate.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/browser/net/url_fixer_upper.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "clank/native/framework/chrome/location_bar.h"
#include "content/browser/android/chrome_view.h"
#include "content/browser/tab_contents/interstitial_page.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "ipc/ipc_message.h"
#include "jni/tab_jni.h"
#include "net/base/net_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "webkit/glue/user_agent.h"

using base::android::ClearException;
using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertUTF16ToJavaString;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetClass;
using base::android::GetFieldID;
using base::android::HasClass;
using base::android::HasField;
using base::android::ScopedJavaLocalRef;
using content::NavigationController;
using content::WebContents;

namespace {

const char kChromeNewTabURL[] = "chrome://newtab/";
const char kCachedNTPTag[] = "cached_ntp";
bool sFirstSearchNotified = false;

// A simple interstitial used when an interstitial is triggered by Java (so far
// only for test purposes).
class ClankInterstitialPage : public InterstitialPage {
 public:
  ClankInterstitialPage(WebContents* tab,
                        const GURL& url,
                        const std::string& html_content)
      : InterstitialPage(tab, true, url),
        html_content_(html_content) {
  }

  virtual ~ClankInterstitialPage() {
  }

  virtual std::string GetHTMLContents() {
    return html_content_;
  }

 private:
  std::string html_content_;

  DISALLOW_COPY_AND_ASSIGN(ClankInterstitialPage);
};

void WriteNavigationEntryToPickle(content::NavigationEntry* entry,
                                  Pickle* pickle) {
  pickle->WriteString(entry->GetVirtualURL().spec());
  pickle->WriteString(entry->GetReferrer().url.spec());
  pickle->WriteString16(entry->GetTitle());
  pickle->WriteString(entry->GetContentState());
  pickle->WriteInt(entry->GetTransitionType());
}

void WriteStateHeaderToPickle(bool off_the_record, int entry_count,
    int current_entry_index, Pickle* pickle) {
  pickle->WriteBool(off_the_record);
  pickle->WriteInt(entry_count);
  pickle->WriteInt(current_entry_index);
}

// Returns true if the navigation to the specified |url| seems to have been
// triggered by a search omnibox suggestion.
bool LooksLikeGoogleSearch(const GURL& url, int page_transition) {
  return page_transition == content::PAGE_TRANSITION_GENERATED &&
      url.host().find("google") != std::string::npos;
}

}  // namespace

static jfieldID g_native_tab;

Tab::Tab(JNIEnv* env, jobject obj)
    : weak_java_tab_(env, obj),
      synced_tab_delegate_(NULL),
      tab_id_(-1) {}

Tab::~Tab() {}

void Tab::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

WebContents* Tab::RestoreStateFromByteArray(void* data, int size) {
  bool is_off_the_record;
  int entry_count;
  int current_entry_index;
  std::string user_agent_override;

  Pickle pickle(static_cast<char*>(data), size);
  void* iter = NULL;
  pickle.ReadBool(&iter, &is_off_the_record);
  pickle.ReadInt(&iter, &entry_count);
  pickle.ReadInt(&iter, &current_entry_index);

  std::vector<content::NavigationEntry*> navigations;
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();

  for (int i = 0; i < entry_count; ++i) {
    std::string virtual_url;
    std::string str_referrer;
    string16 title;
    std::string content_state;
    int transition;

    pickle.ReadString(&iter, &virtual_url);
    pickle.ReadString(&iter, &str_referrer);
    pickle.ReadString16(&iter, &title);
    pickle.ReadString(&iter, &content_state);
    pickle.ReadInt(&iter, &transition);

    TabNavigation tn(i,
                     GURL(virtual_url),
                     content::Referrer(GURL(str_referrer),
                                       WebKit::WebReferrerPolicyDefault),
                     title,
                     content_state,
                     content::PageTransitionFromInt(transition));
    navigations.push_back(tn.ToNavigationEntry(i, profile));
  }

  // Read the user agent info for each navigation entry back.
  for (int i = 0; i < entry_count; ++i) {
    std::string initial_url;
    bool user_agent_overridden;

    if (!pickle.ReadString(&iter, &initial_url) ||
        !pickle.ReadBool(&iter, &user_agent_overridden)) {
      // We failed to read back the info required to perform a user agent
      // override; the tab is likely to be corrupted.
      break;
    }

    navigations[i]->SetOriginalRequestURL(GURL(initial_url));
    navigations[i]->SetUserAgentOverride(user_agent_overridden ?
        webkit_glue::GetDesktopUserAgent() : "");
  }

  if (is_off_the_record)
    profile = profile->GetOffTheRecordProfile();
  scoped_ptr<WebContents> new_tab(WebContents::Create(
      profile, NULL, MSG_ROUTING_NONE, 0, 0));
  new_tab->GetController().Restore(current_entry_index,
                                false, /* from_last_session */
                                &navigations);
  return new_tab.release();
}

void Tab::SaveTabIdForSessionSync(JNIEnv* env, jobject obj, jobject view) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  DCHECK(chrome_view);
  synced_tab_delegate_ = chrome_view->tab_contents_wrapper()->
      synced_tab_delegate();
  tab_id_ = synced_tab_delegate_->GetSessionId();
}

ScopedJavaLocalRef<jbyteArray> Tab::GetStateAsByteArray(
    JNIEnv* env, jobject obj, jobject view) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  Profile* profile = chrome_view->GetProfile();
  if (!profile)
    return ScopedJavaLocalRef<jbyteArray>();

  NavigationController& controller = chrome_view->web_contents()->GetController();
  const int pending_index = controller.GetPendingEntryIndex();
  int entry_count = controller.GetEntryCount();
  if (entry_count == 0 && pending_index == 0)
    entry_count++;

  if (entry_count == 0)
    return ScopedJavaLocalRef<jbyteArray>();

  int current_entry = controller.GetCurrentEntryIndex();
  if (current_entry == -1 && entry_count > 0)
    current_entry = 0;

  Pickle pickle;
  WriteStateHeaderToPickle(profile->IsOffTheRecord(), entry_count,
                           current_entry, &pickle);

  // Write out all of the NavigationEntrys.
  for (int i = 0; i < entry_count; ++i) {
    content::NavigationEntry* entry = (i == pending_index) ?
        controller.GetPendingEntry() : controller.GetEntryAtIndex(i);
    WriteNavigationEntryToPickle(entry, &pickle);
  }

  // Write out info we need for flipping user agents.
  for (int i = 0; i < entry_count; ++i) {
    content::NavigationEntry* entry = (i == pending_index) ?
        controller.GetPendingEntry() : controller.GetEntryAtIndex(i);
    pickle.WriteString(entry->GetInitialURL().spec());
    pickle.WriteBool(!entry->GetUserAgentOverride().empty());
  }

  ScopedJavaLocalRef<jbyteArray> jb(env, env->NewByteArray(pickle.size()));
  if (ClearException(env) ) {
    // We can run out of memory when allocating that array. In that case we'll
    // only save the current entry.
    // TODO(jcivelli): http://b/issue?id=5869635 we should save more entries.
    LOG(WARNING) << "Failed to allocate Java byte-array to save tab state (" <<
        entry_count << " entries, "<< pickle.size() << " bytes).";
    Pickle one_entry_pickle;
    content::NavigationEntry* entry = controller.GetActiveEntry();
    WriteStateHeaderToPickle(profile->IsOffTheRecord(), 1, 0,
                             &one_entry_pickle);
    WriteNavigationEntryToPickle(entry, &one_entry_pickle);
    one_entry_pickle.WriteString(entry->GetInitialURL().spec());
    one_entry_pickle.WriteBool(!entry->GetUserAgentOverride().empty());
    jb.Reset(env, env->NewByteArray(one_entry_pickle.size()));
    if (ClearException(env)) {
      LOG(ERROR) << "Failed to allocate Java byte-array to save tab state for "
          "a single entry (" << one_entry_pickle.size() << " bytes).";
      return ScopedJavaLocalRef<jbyteArray>();
    }
    env->SetByteArrayRegion(jb.obj(), 0, one_entry_pickle.size(),
                            static_cast<const jbyte*>(one_entry_pickle.data()));

  } else {
    env->SetByteArrayRegion(jb.obj(), 0, pickle.size(),
                            static_cast<const jbyte*>(pickle.data()));
  }
  return jb;
}

// ----------------------------------------------------------------------------
// Native JNI methods
// ----------------------------------------------------------------------------

static jint Init(JNIEnv* env, jobject obj) {
  Tab* tab = new Tab(env, obj);
  return  reinterpret_cast<jint>(tab);
}

static jint RestoreStateFromByteArray(JNIEnv* env, jclass clazz,
                                     jbyteArray state) {
  int size = env->GetArrayLength(state);
  jbyte* data = env->GetByteArrayElements(state, NULL);
  WebContents* web_contents = Tab::RestoreStateFromByteArray(data, size);
  env->ReleaseByteArrayElements(state, data, 0);
  return reinterpret_cast<int>(web_contents);
}

static void CreateHistoricalTabFromContents(WebContents* web_contents,
                                            jint tab_index) {
  DCHECK(web_contents);

  TabRestoreService* service =
      TabRestoreServiceFactory::GetForProfile(
          Profile::FromBrowserContext(web_contents->GetBrowserContext()));
  if (!service)
    return;

  // Exclude internal pages from being marked as recent when they are closed.
  const GURL& tab_url = web_contents->GetURL();
  if (tab_url.SchemeIs(chrome::kChromeUIScheme) ||
      tab_url.SchemeIs(chrome::kAboutScheme)) {
    return;
  }

  service->CreateHistoricalTab(&web_contents->GetController(), tab_index);
}

static void CreateHistoricalTab(JNIEnv* env, jobject obj, jobject view,
                                jint tab_index) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  DCHECK(chrome_view);
  CreateHistoricalTabFromContents(chrome_view->web_contents(), tab_index);
}

// Creates a historical tab entry from the serialized tab contents contained
// within |state|.
static void CreateHistoricalTabFromState(JNIEnv* env, jclass clazz,
                                         jbyteArray state,
                                         jint tab_index) {
  scoped_ptr<WebContents> web_contents(
      reinterpret_cast<WebContents*>(
          RestoreStateFromByteArray(env, clazz, state)));
  CreateHistoricalTabFromContents(web_contents.get(), tab_index);
}

static jboolean IsInitialNavigation(JNIEnv* env, jobject obj, jobject view) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  DCHECK(chrome_view);
  return chrome_view->web_contents()->GetController().IsInitialNavigation();
}

static void LaunchBlockedPopups(JNIEnv* env, jobject obj, jobject view) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  DCHECK(chrome_view);
  TabContentsWrapper* wrapper = chrome_view->tab_contents_wrapper();

  std::vector<TabContentsWrapper*> blocked;
  wrapper->blocked_content_tab_helper()->GetBlockedContents(&blocked);
  for (std::vector<TabContentsWrapper*>::iterator it = blocked.begin();
      it != blocked.end(); ++it)
    wrapper->blocked_content_tab_helper()->LaunchForContents(*it);
}

static jint GetPopupBlockedCount(JNIEnv* env, jobject obj, jobject view) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  DCHECK(chrome_view);
  TabContentsWrapper* wrapper = chrome_view->tab_contents_wrapper();

  return wrapper->blocked_content_tab_helper()->GetBlockedContentsCount();
}

static void SetWindowId(JNIEnv* env, jobject obj, jobject view, jint id) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  DCHECK(chrome_view);
  TabContentsWrapper* wrapper = chrome_view->tab_contents_wrapper();
  SessionID session_id;
  session_id.set_id(id);
  wrapper->restore_tab_helper()->SetWindowID(session_id);
}

static jstring GetTabContentDisplayHost(JNIEnv* env,
                                        jobject obj,
                                        jobject view) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  DCHECK(chrome_view);

  WebContents* web_contents = chrome_view->web_contents();
  GURL url = web_contents->GetURL();
  string16 display_host_utf16;
  Profile* profile = Profile::FromBrowserContext(web_contents->GetBrowserContext());
  net::AppendFormattedHost(url,
                           profile->GetPrefs()->GetString(
                               prefs::kAcceptLanguages),
                           &display_host_utf16);

  // OK to release, JNI binding.
  return ConvertUTF16ToJavaString(env, display_host_utf16).Release();
}

static jint GetRenderProcessPid(JNIEnv* env,
                                jobject obj,
                                jobject view) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  DCHECK(chrome_view);
  jint render_process_handle = chrome_view->GetCurrentRenderProcess(env, obj);
  if (render_process_handle != 0) {
    return base::GetProcId(render_process_handle);
  } else {
    return 0;
  }
}

static jint GetRenderProcessPrivateSizeKBytes(JNIEnv* env,
                                              jobject obj,
                                              jobject view) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  DCHECK(chrome_view);
  jint render_process_handle = chrome_view->GetCurrentRenderProcess(env, obj);
  if (render_process_handle == 0)
    return 0;
  scoped_ptr<base::ProcessMetrics> process_metrics(
      base::ProcessMetrics::CreateProcessMetrics(render_process_handle));
  base::WorkingSetKBytes ws_usage;
  if (process_metrics->GetWorkingSetKBytes(&ws_usage)) {
    // Please make sure to use the same field as in TabMemoryHandler for
    // measuring how much memory the whole application consumes.
    return ws_usage.priv;
  } else {
    return 0;
  }
}

static void PurgeRenderProcessNativeMemory(JNIEnv* env,
                                           jobject obj,
                                           jobject view) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  DCHECK(chrome_view);
  chrome_view->PurgeNativeMemory(env, view);
}

static void ShowInterstitialPage(JNIEnv* env,
                                 jobject obj,
                                 jobject view,
                                 jstring jurl,
                                 jstring html_content) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  DCHECK(chrome_view);
  GURL url(base::android::ConvertJavaStringToUTF8(env, jurl));
  std::string content =
      base::android::ConvertJavaStringToUTF8(env, html_content);
  ClankInterstitialPage* interstitial =
      new ClankInterstitialPage(chrome_view->web_contents(), url, content);
  interstitial->Show();
}

static jboolean IsShowingInterstitialPage(JNIEnv* env,
                                          jobject obj,
                                          jobject view) {
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  return chrome_view->web_contents()->ShowingInterstitialPage();
}

// static
Tab* Tab::GetNativeTab(JNIEnv* env, jobject obj) {
  return reinterpret_cast<Tab*>(env->GetIntField(obj, g_native_tab));
}

// Register native methods


bool RegisterTab(JNIEnv* env) {
  if (!HasClass(env, kTabClassPath)) {
    DLOG(ERROR) << "Unable to find class ChromeView!";
    return false;
  }
  ScopedJavaLocalRef<jclass> clazz = GetClass(env, kTabClassPath);
  if (!HasField(env, clazz, "mNativeTab", "I")) {
    DLOG(ERROR) << "Unable to find Tab.mNativeTab!";
    return false;
  }
  g_native_tab = GetFieldID(env, clazz, "mNativeTab", "I");

  return RegisterNativesImpl(env);
}


static jboolean ShouldUpdateThumbnail(JNIEnv* env,
                                      jobject obj,
                                      jobject view,
                                      jstring jurl) {
  GURL url(base::android::ConvertJavaStringToUTF8(env, jurl));
  if (view == NULL)
    return false;
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  Profile* profile =
      Profile::FromBrowserContext(chrome_view->web_contents()->GetBrowserContext());
  history::TopSites* top_sites = profile->GetTopSites();
  return ThumbnailGenerator::ShouldUpdateThumbnail(profile, top_sites, url);
}

void Tab::UpdateThumbnail(JNIEnv* env, jobject obj, jobject view, jobject bitmap) {
  AutoLockJavaBitmap bitmap_lock(bitmap);
  SkBitmap sk_bitmap;
  gfx::Size size = bitmap_lock.size();
  sk_bitmap.setConfig(SkBitmap::kARGB_8888_Config, size.width(), size.height(), 0);
  sk_bitmap.setPixels(bitmap_lock.pixels());
  ThumbnailGenerator::ClipResult clip_result = ThumbnailGenerator::kNotClipped;
  ThumbnailGenerator* generator = g_browser_process->GetThumbnailGenerator();
  if (view == NULL)
    return;
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  if (generator) {
    generator->UpdateThumbnail(chrome_view->web_contents(), sk_bitmap, clip_result);
  }
}

static jlong GetBookmarkId(JNIEnv* env, jobject obj, jobject view) {
  if (view == NULL)
    return -1;
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  if (chrome_view) {
    const BookmarkNode* bookmarkNode = chrome_view->GetProfile()->GetBookmarkModel()->
        GetMostRecentlyAddedNodeForURL(chrome_view->web_contents()->GetURL());
    if (bookmarkNode) {
      return bookmarkNode->id();
    }
  }
  return -1;
}

void Tab::LoadUrl(JNIEnv* env, jobject obj, jobject view, jstring jurl,
                  int page_transition) {
  if (view == NULL)
    return;
  ChromeView* chrome_view = ChromeView::GetNativeChromeView(env, view);
  GURL url(base::android::ConvertJavaStringToUTF8(env, jurl));
  if (!chrome_view || url.is_empty())
    return;
  // Check if it's a cached NTP request, remove the tab_contents from list of
  // tab tracker since the cached NTP will close itself soon.
  if (StartsWithASCII(url.spec(), kChromeNewTabURL, true) &&
      url.ref() == kCachedNTPTag && chrome_view->tab_contents_wrapper()) {
    TestingAutomationProvider::OnTabRemoved(chrome_view->tab_contents_wrapper());
  }

  GURL fixed_url(URLFixerUpper::FixupURL(url.possibly_invalid_spec(),
                                         std::string()));
  if (!HandleNonNavigationAboutURL(fixed_url)) {
    // Notify the GoogleURLTracker of searches, it might want to change the
    // actual Google site used (for instance when in the UK, google.co.uk, when
    // in the US google.com).
    // Note that this needs to happen before we initiate the navigation as the
    // GoogleURLTracker uses the navigation pending notification to trigger the
    // infobar.
    if (LooksLikeGoogleSearch(fixed_url, page_transition)) {
      GoogleURLTracker::GoogleURLSearchCommitted();
      if (!sFirstSearchNotified) {
        sFirstSearchNotified = true;
        Java_Tab_onFirstSearch(env, obj);
      }
    }

    chrome_view->LoadUrlInternal(fixed_url, page_transition);
  }
}

static void SetSearchClient(JNIEnv* env, jclass clazz, jstring jclient) {
  SearchTermsData::SetSearchClient(base::android::ConvertJavaStringToUTF8(env, jclient));
}


static void SetStaticRlz(JNIEnv* env, jclass clazz, jstring jrlz) {
  SearchTermsData::SetStaticRlz(base::android::ConvertJavaStringToUTF16(env, jrlz));
}
