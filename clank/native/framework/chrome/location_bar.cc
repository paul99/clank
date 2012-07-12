// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/location_bar.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/autocomplete/autocomplete.h"
#include "chrome/browser/autocomplete/autocomplete_match.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/search_engines/template_url_service.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/ui/toolbar/toolbar_model.h"
#include "chrome/common/chrome_notification_types.h"
#include "clank/native/framework/chrome/page_info_viewer.h"
#include "content/browser/android/chrome_view.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "jni/location_bar_jni.h"
#include "ui/base/resource/resource_bundle.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ScopedJavaLocalRef;
using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertUTF8ToJavaString;
using content::NavigationController;
using content::WebContents;

class TemplateURL;

namespace {

// The LocationBar::mNativeLocationBar field.
LocationBar* g_location_bar = NULL;
}  // namespace

LocationBar::LocationBar(JNIEnv* env, jobject obj, Profile* profile) :
    weak_java_location_bar_(env, obj) {
  notification_registrar_.Add(
      this, content::NOTIFICATION_SSL_VISIBLE_STATE_CHANGED,
      content::NotificationService::AllSources());
  notification_registrar_.Add(
      this, chrome::NOTIFICATION_URLS_STARRED,
      content::NotificationService::AllSources());
}

LocationBar::~LocationBar() {
  g_location_bar = NULL;
}

void LocationBar::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

// NotificationObserver implementation:
void LocationBar::Observe(int type,
                          const content::NotificationSource& source,
                          const content::NotificationDetails& details) {
  switch (type) {
    case content::NOTIFICATION_SSL_VISIBLE_STATE_CHANGED:
      OnSSLVisibleStateChanged(source, details);
      break;
    case chrome::NOTIFICATION_URLS_STARRED:
      OnUrlsStarred(source, details);
      break;
    default:
      NOTREACHED() << "LocationBar got a notification it did not register for:"
                   << type;
      break;
  }
}

void LocationBar::OnSecurityButtonClicked(JNIEnv*, jobject, jobject context) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> chrome_view =
       Java_LocationBar_getChromeView(env, weak_java_location_bar_.get(env).obj());
  if (chrome_view.is_null())
    return;

  WebContents* web_contents =
      ChromeView::GetNativeChromeView(env, chrome_view.obj())->web_contents();
  if (!web_contents)
    return;

  PageInfoViewer::Show(env, context, chrome_view.obj(), web_contents);
}

void LocationBar::OnSSLVisibleStateChanged(const content::NotificationSource& source,
                                           const content::NotificationDetails& details) {
  // We only have one location bar per tab but each LocationBar instance
  // receives notifications from all other tabs (as it's not attached to any
  // TabContents on creation). We therefore need to filter our
  // notifications that are not for the TabContents associated with this
  // LocationBar instance.
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> chrome_view =
      Java_LocationBar_getChromeView(env, weak_java_location_bar_.get(env).obj());
  if (chrome_view.is_null())
    return;

  WebContents* web_contents =
      ChromeView::GetNativeChromeView(env, chrome_view.obj())->web_contents();
  if (!web_contents)
    return;

  NavigationController* navigation_controller = &web_contents->GetController();
  if (navigation_controller == content::Source<NavigationController>(source).ptr()) {
    Java_LocationBar_updateLoadingState(env,
      weak_java_location_bar_.get(env).obj(), false, false);
  }
}

void LocationBar::OnUrlsStarred(
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  JNIEnv* env = AttachCurrentThread();

  // Filter out changes for the non-active tab.
  ScopedJavaLocalRef<jobject> chrome_view =
        Java_LocationBar_getChromeView(env, weak_java_location_bar_.get(env).obj());
  if (chrome_view.is_null())
    return;

  Java_LocationBar_refreshUrlStar(env, weak_java_location_bar_.get(env).obj());
}

int LocationBar::GetSecurityLevel(JNIEnv*, jobject) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> chrome_view =
          Java_LocationBar_getChromeView(env, weak_java_location_bar_.get(env).obj());
  if (chrome_view.is_null())
    return ToolbarModel::NONE;

  WebContents* web_contents =
      ChromeView::GetNativeChromeView(env, chrome_view.obj())->web_contents();
  if (!web_contents)
    return ToolbarModel::NONE;

  NavigationController* navigation_controller = &web_contents->GetController();
  ToolbarModel toolbar_model(navigation_controller);
  return toolbar_model.GetSecurityLevel();
}

jstring GetUrlForSearchQuery(JNIEnv* env, jclass, jstring jquery) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  const TemplateURL* default_provider =
      TemplateURLServiceFactory::GetForProfile(profile)->GetDefaultSearchProvider();

  string16 query(ConvertJavaStringToUTF16(env, jquery));

  std::string url;
  if (default_provider && default_provider->url() &&
      default_provider->url()->SupportsReplacement() && !query.empty()) {
    url = default_provider->url()->ReplaceSearchTerms(*default_provider,
                                                      query,
                                                      TemplateURLRef::NO_SUGGESTIONS_AVAILABLE,
                                                      string16());
  }

  // OK to release, JNI binding.
  return ConvertUTF8ToJavaString(env, url).Release();
}

// JNI functions
static int Init(JNIEnv* env, jobject obj) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  // We should only have one LocationBar instance.
  DCHECK(!g_location_bar);
  g_location_bar = new LocationBar(env, obj, profile);
  return reinterpret_cast<jint>(g_location_bar);
}

// Register native methods
bool RegisterLocationBar(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
