// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/chrome_view.h"

#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <jni.h>

#include <android/native_window_jni.h>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/debug/debugger.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "base/string_number_conversions.h"
#include "base/synchronization/lock.h"
#include "chrome/browser/autofill/autofill_external_delegate_android.h"
#include "chrome/browser/automation/testing_automation_provider_android.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/favicon/favicon_tab_helper.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/find_bar/find_tab_helper.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/browser/ui/webui/ntp/on_context_menu_item_selected_callback.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/render_messages.h"
#include "chrome/common/switches_android.h"
#include "content/browser/android/chrome_view_client.h"
#include "content/browser/android/devtools_server.h"
#include "content/browser/android/sandboxed_process_service.h"
#include "content/browser/android/touch_point.h"
#include "content/browser/android/waitable_native_event.h"
#include "content/browser/geolocation/android_location_api_adapter.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/browser/device_orientation/device_orientation_android.h"
#include "content/browser/renderer_host/backing_store_android.h"
#include "content/browser/renderer_host/java/java_bound_object.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/renderer_host/render_widget_host.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/browser/tab_contents/interstitial_page.h"
#include "content/browser/tab_contents/render_view_host_manager.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/browser/tab_contents/tab_contents_view_android.h"
#include "content/common/view_messages.h"
#include "content/gpu/gpu_child_thread.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/page_transition_types.h"
#include "content/public/common/process_type.h"
#include "grit/generated_resources.h"
#include "jni/chrome_view_jni.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebBindings.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/android/WebInputEventFactory.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkDevice.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/gl/gl_surface_android.h"
#include "webkit/glue/webmenuitem.h"
#include "webkit/glue/context_menu.h"
#include "webkit/glue/user_agent.h"

using base::android::AttachCurrentThread;
using base::android::GetClass;
using base::android::GetMethodID;
using base::android::CheckException;
using base::android::ClearException;
using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertUTF16ToJavaString;
using base::android::ConvertUTF8ToJavaString;
using base::android::HasField;
using base::android::ScopedJavaLocalRef;
using base::android::ScopedJavaGlobalRef;
using content::NavigationController;
using content::Source;
using content::WebContents;

// Describes the type and enabled state of a select popup item.
// Keep in sync with the value defined in SelectPopupDialog.java
enum PopupItemType {
  POPUP_ITEM_TYPE_GROUP = 0,
  POPUP_ITEM_TYPE_DISABLED,
  POPUP_ITEM_TYPE_ENABLED
};

// Describes the action we took when handling a touch event.
// Keep in sync with the value defined in ChromeView.java
enum TouchEventActionType {
  EVENT_FORWARDED_TO_NATIVE = 0,
  EVENT_NOT_FORWARDED_TO_NATIVE,
  EVENT_CONVERTED_TO_CANCEL
};

namespace {
jfieldID g_native_chrome_view;

// This mapping is mirrored in ChromeView.java.  Please ensure they stay in sync.
static const int LOCALIZED_STRING_IDS[] = {
  IDS_SAD_TAB_MESSAGE,
  IDS_SAD_TAB_TITLE
};

}  // namespace

struct ChromeView::JavaObject {
  jweak obj;
  ScopedJavaGlobalRef<jclass> view_clazz;

  ScopedJavaGlobalRef<jclass> bitmap_clazz;
  ScopedJavaGlobalRef<jclass> matrix_clazz;
  jmethodID matrix_constructor;
  jmethodID matrix_setvalues;
  ScopedJavaGlobalRef<jclass> rect_clazz;
  jmethodID rect_constructor;
  ScopedJavaGlobalRef<jclass> point_clazz;
  jfieldID point_x_field;
  jfieldID point_y_field;
  jmethodID point_constructor;
  ScopedJavaGlobalRef<jclass> string_clazz;

  ScopedJavaLocalRef<jobject> View(JNIEnv* env) {
    return GetRealObject(env, obj);
  }
};

class ChromeView::ChromeViewNotificationObserver : public content::NotificationObserver {
 public:
  explicit ChromeViewNotificationObserver(ChromeView* parent)
      : parent_(parent) {
  }
  void Observe(int type, const content::NotificationSource& source,
               const content::NotificationDetails& details);
 private:
  ChromeView* parent_;
};

void ChromeView::ChromeViewNotificationObserver::Observe(int type,
    const content::NotificationSource& source, const content::NotificationDetails& details) {
  switch(type) {
    case chrome::NOTIFICATION_FIND_RESULT_AVAILABLE: {
      DCHECK(content::Source<WebContents>(source).ptr() == parent_->web_contents());
      const FindNotificationDetails* result =
          content::Details<const FindNotificationDetails>(details).ptr();
      parent_->OnFindResultAvailable(result);
      break;
    }
    case content::NOTIFICATION_INTERSTITIAL_ATTACHED: {
      DCHECK(parent_->web_contents()->ShowingInterstitialPage());
      parent_->OnInterstitialShown();
      RenderWidgetHostViewAndroid* old_rwhva =
          static_cast<RenderWidgetHostViewAndroid*>(
              parent_->web_contents()->GetRenderWidgetHostView());
      if (old_rwhva)
        parent_->OnAcceleratedCompositingStateChange(old_rwhva, false, true);
      RenderWidgetHost* new_host =
          parent_->web_contents()->GetInterstitialPage()->GetRenderViewHost();
      DCHECK(new_host);
      if (new_host->is_accelerated_compositing_active()) {
        RenderWidgetHostViewAndroid* rwhva =
            static_cast<RenderWidgetHostViewAndroid*>(new_host->view());
        DCHECK(rwhva);
        parent_->OnAcceleratedCompositingStateChange(rwhva, true, true);
      }
      break;
    }
    case content::NOTIFICATION_INTERSTITIAL_DETACHED: {
      DCHECK(parent_->web_contents()->ShowingInterstitialPage());
      parent_->OnInterstitialHidden();
      RenderWidgetHost* host =
          parent_->web_contents()->GetInterstitialPage()->GetRenderViewHost();
      DCHECK(host);
      RenderWidgetHostViewAndroid* rwhva =
          static_cast<RenderWidgetHostViewAndroid*>(host->view());
      DCHECK(rwhva);
      parent_->OnAcceleratedCompositingStateChange(rwhva, false, true);
      rwhva = static_cast<RenderWidgetHostViewAndroid*>(
          parent_->web_contents()->GetRenderWidgetHostView());
      if (rwhva && rwhva->GetRenderWidgetHost() &&
          rwhva->GetRenderWidgetHost()->is_accelerated_compositing_active()) {
        parent_->OnAcceleratedCompositingStateChange(rwhva, true, true);
      }
      break;
    }
    case content::NOTIFICATION_RENDER_VIEW_HOST_CHANGED: {
      std::pair<RenderViewHost*, RenderViewHost*>* switched_details =
          content::Details<std::pair<RenderViewHost*, RenderViewHost*> >(
              details).ptr();
      RenderWidgetHost* host = switched_details->first;
      int old_pid = 0;
      if (host) {
        RenderWidgetHostViewAndroid* rwhva =
            static_cast<RenderWidgetHostViewAndroid*>(host->view());
        if (rwhva)
          parent_->OnAcceleratedCompositingStateChange(rwhva, false, true);
        content::RenderProcessHost* render_process = host->process();
        DCHECK(render_process);
        if (render_process->HasConnection())
          old_pid = render_process->GetHandle();
      }
      host = switched_details->second;
      DCHECK(host);
      if (host->is_accelerated_compositing_active()) {
        RenderWidgetHostViewAndroid* rwhva =
            static_cast<RenderWidgetHostViewAndroid*>(host->view());
        DCHECK(rwhva);
        parent_->OnAcceleratedCompositingStateChange(rwhva, true, true);
      }
      content::RenderProcessHost* render_process = host->process();
      DCHECK(render_process);
      int new_pid = render_process->GetHandle();
      if (new_pid != old_pid) {
        // notify the Java side of the change of the current renderer process.
        JNIEnv* env = AttachCurrentThread();
        Java_ChromeView_onRenderProcessSwap(
            env, parent_->java_object_->View(env).obj(), old_pid, new_pid);
      }
      break;
    }
    case content::NOTIFICATION_CHILD_PROCESS_HOST_CONNECTED: {
      if (content::Details<content::ChildProcessData>(details)->type ==
          content::PROCESS_TYPE_GPU) {
        RenderWidgetHostViewAndroid* rwhva =
            parent_->GetRenderWidgetHostViewAndroid();
        if (!rwhva) {
          LOG(ERROR) << "GPU process connected while no render process. Hmm...";
          return;
        }
        // If GPU process killed and restarted, we want to call invalidate to
        // trigger drawGL where we can set up SurfaceTexture.
        if (rwhva->IsShowing())
          parent_->Invalidate();
      }
      break;
    }
    case content::NOTIFICATION_CHILD_PROCESS_HOST_DISCONNECTED: {
      if (content::Details<content::ChildProcessData>(details)->type ==
          content::PROCESS_TYPE_GPU) {
        RenderWidgetHostViewAndroid* rwhva =
            parent_->GetRenderWidgetHostViewAndroid();
        if (!rwhva) {
          LOG(WARNING) <<
              "GPU process crashed while render process also crashed. Ignore";
          return;
        } else if (content::Details<content::ChildProcessData>(details)->handle ==
            parent_->GetGPUProcess()) {
            parent_->SetGPUProcess(base::kNullProcessHandle);
            parent_->OnAcceleratedCompositingStateChange(rwhva, false, true);
        }
      }
      break;
    }
    case content::NOTIFICATION_EXECUTE_JAVASCRIPT_RESULT: {
      const RenderViewHost* render_view_host = parent_->web_contents() ?
          parent_->web_contents()->GetRenderViewHost() : NULL;
      if (content::Source<RenderViewHost>(source).ptr() != render_view_host) {
        return;
      }

      JNIEnv* env = base::android::AttachCurrentThread();
      std::pair<int, Value*>* result_pair =
          content::Details<std::pair<int, Value*> >(details).ptr();
      std::string json;
      base::JSONWriter::Write(result_pair->second, false, &json);
      ScopedJavaLocalRef<jstring> j_json = ConvertUTF8ToJavaString(env, json);
      Java_ChromeView_onEvaluateJavaScriptResult(env,
          parent_->java_object_->View(env).obj(),
          static_cast<jint>(result_pair->first), j_json.obj());
      break;
    }
    case content::NOTIFICATION_RENDERER_PROCESS_CREATED: {
      content::RenderProcessHost* process = content::Source<content::RenderProcessHost>(source).ptr();
      RenderWidgetHostViewAndroid* rwhva = parent_->GetRenderWidgetHostViewAndroid();
      if (rwhva) {
        RenderWidgetHost* host = rwhva->GetRenderWidgetHost();
        content::RenderProcessHost* render_process = host->process();
        // notify the Java side of the current renderer process.
        if (render_process == process) {
          JNIEnv* env = AttachCurrentThread();
          int pid = process->GetHandle();
          Java_ChromeView_onRenderProcessSwap(
              env, parent_->java_object_->View(env).obj(), 0, pid);
        }
      }
      break;
    }
    case chrome::NOTIFICATION_NEW_TAB_READY: {
      if (content::Source<WebContents>(source).ptr() != parent_->web_contents())
        return;
      JNIEnv* env = base::android::AttachCurrentThread();
      Java_ChromeView_onNewTabPageReady(env,
          parent_->java_object_->View(env).obj());
      break;
    }
    default:
      NOTREACHED();
      break;
  }
}

class ChromeView::ChromeViewWebContentsObserver
    : public content::NotificationObserver,
      public content::WebContentsObserver {
 public:
  ChromeViewWebContentsObserver(WebContents* web_contents, ChromeView* parent);
  virtual ~ChromeViewWebContentsObserver();

  // NotificationObserver
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details);

  // WebContentsObserver
  virtual void DidStartProvisionalLoadForFrame(
      int64 frame_id,
      bool is_main_frame,
      const GURL& validated_url,
      bool is_error_page,
      RenderViewHost* GetRenderViewHost);
  virtual void DidFailProvisionalLoad(int64 frame_id,
                                      bool is_main_frame,
                                      const GURL& validated_url,
                                      int error_code,
                                      const string16& error_description);
  virtual void DidFailLoad(int64 frame_id,
                           const GURL& validated_url,
                           bool is_main_frame,
                           int error_code,
                           const string16& error_description);
  virtual void DidFinishLoad(int64 frame_id,
                             const GURL& validated_url,
                             bool is_main_frame);
  virtual void DidNavigateMainFramePostCommit(
      const content::LoadCommittedDetails& details,
      const ViewHostMsg_FrameNavigate_Params& params);

 private:
  content::NotificationRegistrar registrar_;

  ChromeView* parent_;
};

ChromeView::ChromeViewWebContentsObserver::ChromeViewWebContentsObserver(
    WebContents* web_contents, ChromeView* parent)
  : content::WebContentsObserver(web_contents),
    parent_(parent) {
  registrar_.Add(this, chrome::NOTIFICATION_WEB_CONTENT_SETTINGS_CHANGED,
                 content::Source<WebContents>(web_contents));
}

ChromeView::ChromeViewWebContentsObserver::~ChromeViewWebContentsObserver() {
}

void ChromeView::ChromeViewWebContentsObserver::Observe(int type,
    const content::NotificationSource& source, const content::NotificationDetails& details) {
  DCHECK(type == chrome::NOTIFICATION_WEB_CONTENT_SETTINGS_CHANGED);
  if (!parent_->tab_contents_wrapper()->content_settings()->
      IsBlockageIndicated(CONTENT_SETTINGS_TYPE_POPUPS)) {
    JNIEnv* env = base::android::AttachCurrentThread();
    Java_ChromeView_onPopupBlockStateChanged(env,
        parent_->java_object_->View(env).obj());
    parent_->tab_contents_wrapper()->content_settings()->
        SetBlockageHasBeenIndicated(CONTENT_SETTINGS_TYPE_POPUPS);
  }
}

void ChromeView::ChromeViewWebContentsObserver::DidStartProvisionalLoadForFrame(
    int64 frame_id, bool is_main_frame, const GURL& validated_url,
    bool is_error_page, RenderViewHost* GetRenderViewHost) {
  // Don't report status for page components like iframes to comply with legacy
  // WebView behavior.
  if (is_main_frame)
    parent_->OnPageStarted(validated_url);
}

void ChromeView::ChromeViewWebContentsObserver::DidFailProvisionalLoad(
    int64 frame_id, bool is_main_frame, const GURL& validated_url,
    int error_code, const string16& error_description) {
  // Don't report status for page components like iframes to comply with legacy
  // WebView behavior.
  if (is_main_frame)
    parent_->OnPageFailed(error_code, error_description, validated_url);
}

void ChromeView::ChromeViewWebContentsObserver::DidFailLoad(
    int64 frame_id, const GURL& validated_url, bool is_main_frame,
    int error_code, const string16& error_description) {
  // This is reported only for legacy WebView compatibility. Chrome only needs
  // to be notified of provisional load failures.
  if (is_main_frame)
    parent_->OnPageFailed(error_code, error_description, validated_url);
}

void ChromeView::ChromeViewWebContentsObserver::DidFinishLoad(
    int64 frame_id, const GURL& validated_url, bool is_main_frame) {
  // Don't report status for page components like iframes to comply with legacy
  // WebView behavior.
  if (is_main_frame)
    parent_->OnPageFinished(validated_url);
}

void ChromeView::ChromeViewWebContentsObserver::DidNavigateMainFramePostCommit(
    const content::LoadCommittedDetails& details,
    const ViewHostMsg_FrameNavigate_Params& params) {
  parent_->OnDidCommitMainFrame(params.url, params.base_url);
}

ChromeView::ChromeView(JNIEnv* env, jobject obj, bool incognito,
                       bool hardware_accelerated, WebContents* web_contents)
    : tab_contents_wrapper_(0),
      owns_tab_contents_wrapper_(false),
      width_(0),
      height_(0),
      tab_crashed_(false),
      on_demand_zoom_mode_(false),
      autofill_external_delegate_(NULL),
      find_operation_initiated_(false),
      allow_content_url_access_(false),
      only_allow_file_access_to_android_resources_(true),
      accelerate_compositing_activated_(false),
      touch_cancel_event_sent_(false),
      gpu_process_(base::kNullProcessHandle),
      weak_ptr_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)) {
  // TODO(husky): Ugly! We need a tidier way to send this flag to the renderer.
  GpuProcessHost::set_gpu_enabled(hardware_accelerated);

  // tab_contents is non NULL when the creation of ChromeView is started from
  // the renderer side. For example, javascript window.open().
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  if (!web_contents) {
    if (incognito)
      profile = profile->GetOffTheRecordProfile();
    web_contents = WebContents::Create(profile, 0, MSG_ROUTING_NONE, 0, 0);
  } else {
    // there are two cases we will reach here. One is the tab is restored from
    // the saved state. Another is the tab is created through JavaScript. In the
    // first case, there is no render process, we treat it like the tab is
    // crashed. If the content is loaded when the tab is shown, tab_crashed_
    // will be reset. As the lifetime of RenderWidgetHostView is associated with
    // the lifetime of the render process, we use it to detect.
    tab_crashed_ = !(web_contents->GetRenderWidgetHostView());
  }
  notification_observer_.reset(new ChromeViewNotificationObserver(this));
  notification_registrar_.reset(new content::NotificationRegistrar());

  notification_registrar_->Add(notification_observer_.get(),
      content::NOTIFICATION_CHILD_PROCESS_HOST_CONNECTED,
      content::NotificationService::AllSources());
  notification_registrar_->Add(notification_observer_.get(),
      content::NOTIFICATION_CHILD_PROCESS_HOST_DISCONNECTED,
      content::NotificationService::AllSources());
  notification_registrar_->Add(notification_observer_.get(),
      content::NOTIFICATION_EXECUTE_JAVASCRIPT_RESULT,
      content::NotificationService::AllSources());
  notification_registrar_->Add(notification_observer_.get(),
      content::NOTIFICATION_RENDERER_PROCESS_CREATED,
      content::NotificationService::AllSources());
  notification_registrar_->Add(notification_observer_.get(),
      chrome::NOTIFICATION_NEW_TAB_READY,
      content::NotificationService::AllSources());

  InitJNI(env, obj);

  // If this is a pop-up, a TabContentsWrapper may have been created
  // in ChromeViewClient::AddNewContents().  Check before creating a
  // new one.
  TabContentsWrapper* new_wrapper =
      TabContentsWrapper::GetCurrentWrapperForContents(web_contents);
  if (!new_wrapper)
    new_wrapper = new TabContentsWrapper(web_contents);
  SetTabContentsWrapper(new_wrapper);

  TestingAutomationProvider::OnTabAdded(tab_contents_wrapper_, incognito);
}

ChromeView::~ChromeView() {
  // Make sure nobody calls back into this object while we are tearing things
  // down (e.g. DestroyTabContentsWrapper() might trigger notifications).
  notification_registrar_->RemoveAll();
  weak_ptr_factory_.InvalidateWeakPtrs();

  if (tab_contents_wrapper_)
    TestingAutomationProvider::OnTabRemoved(tab_contents_wrapper_);

  if (java_object_) {
    JNIEnv* env = AttachCurrentThread();
    env->DeleteWeakGlobalRef(java_object_->obj);
    delete java_object_;
    java_object_ = 0;
  }

  if (tab_contents_wrapper_) {
    DevToolsServer::GetInstance()->RemoveInspectableTab(web_contents());

   // Terminate the renderer process if this is the last tab.
   // If there's no unload listener, FastShutdownForPageCount kills the renderer
   // process. Otherwise, we go with the slow path where renderer process shuts
   // down itself when ref count becomes 0.
   content::RenderProcessHost* process = web_contents()->GetRenderProcessHost();
   process->FastShutdownForPageCount(1);
  }

  DestroyTabContentsWrapper();
}

void ChromeView::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

void ChromeView::SetTabContentsWrapper(
    TabContentsWrapper* tab_contents_wrapper) {
  if (tab_contents_wrapper_) {
    RenderWidgetHostViewAndroid* rwhva = GetRenderWidgetHostViewAndroid();
    if (rwhva)
      OnAcceleratedCompositingStateChange(rwhva, false, true);
  }

  SetWeakTabContentsWrapper(tab_contents_wrapper);
  owns_tab_contents_wrapper_ = true;

  RenderWidgetHostViewAndroid* rwhva = GetRenderWidgetHostViewAndroid();
  if (rwhva &&
      rwhva->GetRenderWidgetHost()->is_accelerated_compositing_active()) {
    OnAcceleratedCompositingStateChange(rwhva, true, true);
  }

  // If we do not own the tab contents, we do not set the delegate.
  // Example: instant needs to control the delegate of an uncommitted
  // instant tab contents.
  if (tab_contents_client_.get())
    web_contents()->SetDelegate(tab_contents_client_.get());
    tab_contents_wrapper_->web_contents()->SetDelegate(tab_contents_client_.get());
}

void ChromeView::DestroyTabContentsWrapper() {
  if (tab_contents_wrapper_ && owns_tab_contents_wrapper_) {
    delete tab_contents_wrapper_;
    tab_contents_wrapper_ = NULL;
    owns_tab_contents_wrapper_ = false;
  }
}

void ChromeView::SetWeakTabContentsWrapper(
    TabContentsWrapper* tab_contents_wrapper) {
  if (tab_contents_wrapper == tab_contents_wrapper_)
    return;

  content::WebContents* new_web_contents =
      tab_contents_wrapper ? tab_contents_wrapper->web_contents() : NULL;
  if (tab_contents_wrapper_) {
    DCHECK(web_contents());
    DCHECK(web_contents() != new_web_contents);
    // We are switching tabs.
    notification_registrar_->Remove(notification_observer_.get(),
                                    chrome::NOTIFICATION_FIND_RESULT_AVAILABLE,
                                    content::Source<WebContents>(web_contents()));
    notification_registrar_->Remove(notification_observer_.get(),
                                    content::NOTIFICATION_INTERSTITIAL_ATTACHED,
                                    Source<WebContents>(web_contents()));
    notification_registrar_->Remove(notification_observer_.get(),
                                    content::NOTIFICATION_INTERSTITIAL_DETACHED,
                                    Source<WebContents>(web_contents()));
    notification_registrar_->Remove(notification_observer_.get(),
        content::NOTIFICATION_RENDER_VIEW_HOST_CHANGED,
        content::Source<NavigationController> (&web_contents()->GetController()));
    static_cast<TabContentsViewAndroid*>(
        web_contents()->GetView())->SetChromeView(base::WeakPtr<ChromeView>());
    content::NotificationService::current()->Notify(
        chrome::NOTIFICATION_CHROME_VIEW_TAB_CONTENTS_REPLACED,
        content::Source<WebContents>(web_contents()),
        content::Details<WebContents>(new_web_contents));
    DevToolsServer::GetInstance()->RemoveInspectableTab(web_contents());
    TestingAutomationProvider::OnTabReplaced(tab_contents_wrapper_,
                                             tab_contents_wrapper);
  }

  DestroyTabContentsWrapper();
  web_contents_observer_.reset(NULL);
  tab_contents_wrapper_ = tab_contents_wrapper;

  if (tab_contents_wrapper_) {
    notification_registrar_->Add(notification_observer_.get(),
                                 chrome::NOTIFICATION_FIND_RESULT_AVAILABLE,
                                 content::Source<WebContents>(new_web_contents));
    notification_registrar_->Add(notification_observer_.get(),
                                 content::NOTIFICATION_INTERSTITIAL_ATTACHED,
                                 Source<WebContents>(new_web_contents));
    notification_registrar_->Add(notification_observer_.get(),
                                 content::NOTIFICATION_INTERSTITIAL_DETACHED,
                                 Source<WebContents>(new_web_contents));
    notification_registrar_->Add(notification_observer_.get(),
        content::NOTIFICATION_RENDER_VIEW_HOST_CHANGED,
        content::Source<NavigationController>(&new_web_contents->GetController()));
    static_cast<TabContentsViewAndroid*>(new_web_contents->GetView())->SetChromeView(
        weak_ptr_factory_.GetWeakPtr());

    web_contents()->SetAllowContentUrlAccess(
        allow_content_url_access_);
    web_contents()->SetOnlyAllowFileAccessToAndroidResources(
        only_allow_file_access_to_android_resources_);

    DevToolsServer::GetInstance()->AddInspectableTab(web_contents());
    web_contents_observer_.reset(
        new ChromeViewWebContentsObserver(new_web_contents, this));
  }
}

void ChromeView::InitJNI(JNIEnv* env, jobject obj) {
  // As we have passed RegisterChromeView(), GetClass() should succeed.
  // Avoid calling GetObjectClass(obj) so that the methods below can be private
  // while ChromeView allows the derived class.
  java_object_ = new JavaObject;
  java_object_->view_clazz.Reset(GetClass(env, kChromeViewClassPath));
  java_object_->obj = env->NewWeakGlobalRef(obj);
  java_object_->matrix_clazz.Reset(GetClass(env, "android/graphics/Matrix"));
  java_object_->matrix_constructor =
      GetMethodID(env, java_object_->matrix_clazz, "<init>", "()V");
  java_object_->matrix_setvalues =
      GetMethodID(env, java_object_->matrix_clazz, "setValues", "([F)V");
  java_object_->rect_clazz.Reset(GetClass(env, "android/graphics/Rect"));
  java_object_->rect_constructor =
      GetMethodID(env, java_object_->rect_clazz, "<init>", "(IIII)V");
  java_object_->bitmap_clazz.Reset(GetClass(env, "android/graphics/Bitmap"));
  java_object_->point_clazz.Reset(GetClass(env, "android/graphics/Point"));
  java_object_->point_x_field =
      GetFieldID(env, java_object_->point_clazz, "x", "I");
  java_object_->point_y_field =
      GetFieldID(env, java_object_->point_clazz, "y", "I");
  java_object_->point_constructor =
      GetMethodID(env, java_object_->point_clazz, "<init>", "(II)V");
  java_object_->string_clazz.Reset(GetClass(env, "java/lang/String"));
}

RenderWidgetHostViewAndroid* ChromeView::GetRenderWidgetHostViewAndroid() {
  RenderWidgetHostView* rwhv = NULL;
  if (!web_contents()) {
    return NULL;
  }
  if (web_contents()->ShowingInterstitialPage()) {
    rwhv = web_contents()->GetInterstitialPage()->GetRenderViewHost()->view();
    // An interstitial page must have had Show() called immediately after
    // construction in order for the render widget to exist. Currently Desktop
    // Chrome does not enforce this is the case, however we do here to keep the
    // state consistent with the TabContents.
    CHECK(rwhv);
  } else {
    rwhv = web_contents()->GetRenderWidgetHostView();
  }
  return static_cast<RenderWidgetHostViewAndroid*>(rwhv);
}

jint ChromeView::GetBackgroundColor(JNIEnv* env, jobject obj) {
  RenderWidgetHostViewAndroid* rwhva = GetRenderWidgetHostViewAndroid();
  if (!rwhva)
    return SK_ColorWHITE;
  return rwhva->GetCachedBackgroundColor();
}

void ChromeView::OnCustomMenuAction(JNIEnv* env, jobject obj, jint action) {
  if (context_menu_selected_callback_.get()) {
    context_menu_selected_callback_->ItemSelected(action);
    context_menu_selected_callback_ = NULL;
  }
}

void ChromeView::OnHide(JNIEnv* env, jobject obj, jboolean requestByActivity) {
    Hide(requestByActivity);
}

void ChromeView::OnShow(JNIEnv* env, jobject obj, jboolean requestByActivity) {
    Show(requestByActivity);
}

// static
ChromeView* ChromeView::GetNativeChromeView(JNIEnv* env, jobject obj) {
  return reinterpret_cast<ChromeView*>(env->GetIntField(obj,
                                                        g_native_chrome_view));
}

void ChromeView::Show(bool requestByActivity) {
  // In the compositor mode, we notify the Chrome that it is visible after the
  // NativeWindow is attached in OnNativeWindowChanged(). This will avoid the
  // black screen flash. If the request comes from Activity due to resume, the
  // NativeWindow is not changed. So OnNativeWindowChanged() will not be called.
  CommandLine* cmd = CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(switches::kForceCompositingMode) || requestByActivity)
    web_contents()->DidBecomeSelected();
  else if (tab_crashed_) {
    // If a render process doesn't exist as the tab is restored, tab_crashed_ is
    // marked as true, trigger the "lazy" reload and reset tab_crashed_.
    web_contents()->GetController().LoadIfNecessary();
    tab_crashed_ = !(GetRenderWidgetHostViewAndroid());
  }
}

void ChromeView::Hide(bool requestByActivity) {
  CommandLine* cmd = CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(switches::kForceCompositingMode) || requestByActivity)
    web_contents()->WasHidden();
}

ScopedJavaLocalRef<jstring> ChromeView::GetLocalizedString(
    JNIEnv* env, jobject obj, jint string_id) {
  return ConvertUTF8ToJavaString(env,
      l10n_util::GetStringUTF8(LOCALIZED_STRING_IDS[string_id]));
}

// ----------------------------------------------------------------------------
// Methods that call to Java via JNI
// ----------------------------------------------------------------------------

// Called from UI thread
void ChromeView::Invalidate() {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_invalidate(env, java_object_->View(env).obj());
}

// Called from non-UI thread
void ChromeView::PostInvalidate() {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_postInvalidate(env, java_object_->View(env).obj());
}

void ChromeView::OnTabCrashed(const base::ProcessHandle handle) {
  // if tab_crashed_ is already true, just return. e.g. if two tabs share the
  // render process, this will be called for each tab when the render process
  // crashed. If user reload one tab, a new render process is created. It can be
  // shared by the other tab. But if user closes the tab before reload the other
  // tab, the new render process will be shut down. This will trigger the other
  // tab's OnTabCrashed() called again as two tabs share the same
  // BrowserRenderProcessHost.
  if (tab_crashed_)
    return;

  // This can happen as TabContents is destroyed after java_object_ is set to 0
  if (!java_object_)
    return;

  tab_crashed_ = true;

  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_onTabCrash(env, java_object_->View(env).obj(), handle);

  // reset accelerate_compositing_activated_ to false as mTextureView is removed
  // in onTabCrash() in ChromeView.java.
  accelerate_compositing_activated_ = false;
}

void ChromeView::UpdateContentSize(int width, int height) {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_updateContentSize(env, java_object_->View(env).obj(),
                                    width,
                                    height);
}

void ChromeView::UpdateScrollOffsetAndPageScaleFactor(int x, int y,
                                                      float scale) {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_updateScrollOffsetAndPageScaleFactor(env,
                                     java_object_->View(env).obj(),
                                     x,
                                     y,
                                     scale);
}

void ChromeView::UpdatePageScaleLimits(float minimum_scale,
                                       float maximum_scale) {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_updatePageScaleLimits(env, java_object_->View(env).obj(),
                                        minimum_scale,
                                        maximum_scale);
}

void ChromeView::ImeUpdateAdapter(int native_ime_adapter,
                                  int text_input_type,
                                  const gfx::Rect& caret_rect,
                                  const std::string& text,
                                  int selection_start,
                                  int selection_end,
                                  int composition_start,
                                  int composition_end,
                                  bool show_ime_if_needed,
                                  const base::Time& request_time) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> jstring_text = ConvertUTF8ToJavaString(env, text);
  Java_ChromeView_imeUpdateAdapter(
      env, java_object_->View(env).obj(),
      native_ime_adapter, text_input_type,
      caret_rect.x(), caret_rect.y(),
      caret_rect.bottom(), caret_rect.right(),
      jstring_text.obj(),
      selection_start, selection_end,
      composition_start, composition_end,
      show_ime_if_needed,
      (request_time.is_null()) ? 0
          : (request_time - base::Time::UnixEpoch()).InMilliseconds());
}

void ChromeView::SetTitle(const string16& title) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jtitle =
      ConvertUTF8ToJavaString(env, UTF16ToUTF8(title));
  Java_ChromeView_setTitle(env, java_object_->View(env).obj(), jtitle.obj());
}

void ChromeView::ShowSelectPopupMenu(const std::vector<WebMenuItem>& items,
                                     int selected_item,
                                     bool multiple) {
  JNIEnv* env = AttachCurrentThread();

  // For multi-select list popups we find the list of previous selections by
  // iterating through the items. But for single selection popups we take the
  // given |selected_item| as is.
  ScopedJavaLocalRef<jintArray> selected_array;
  if (multiple) {
    scoped_array<jint> native_selected_array(new jint[items.size()]);
    size_t selected_count = 0;
    for (size_t i = 0; i < items.size(); ++i) {
      if (items[i].checked)
        native_selected_array[selected_count++] = i;
    }

    selected_array.Reset(env, env->NewIntArray(selected_count));
    env->SetIntArrayRegion(selected_array.obj(), 0, selected_count,
                           native_selected_array.get());
  } else {
    selected_array.Reset(env, env->NewIntArray(1));
    jint value = selected_item;
    env->SetIntArrayRegion(selected_array.obj(), 0, 1, &value);
  }

  ScopedJavaLocalRef<jobjectArray> items_array(env,
      env->NewObjectArray(items.size(),
                          java_object_->string_clazz.obj(), NULL));
  ScopedJavaLocalRef<jintArray> enabled_array(env,
                                              env->NewIntArray(items.size()));
  for (size_t i = 0; i < items.size(); ++i) {
    ScopedJavaLocalRef<jstring> item =
        ConvertUTF16ToJavaString(env, items[i].label);
    env->SetObjectArrayElement(items_array.obj(), i, item.obj());
    jint enabled = (items[i].type == WebMenuItem::GROUP ? POPUP_ITEM_TYPE_GROUP :
                    (items[i].enabled ? POPUP_ITEM_TYPE_ENABLED :
                                        POPUP_ITEM_TYPE_DISABLED));
    env->SetIntArrayRegion(enabled_array.obj(), i, 1, &enabled);
  }
  Java_ChromeView_showSelectPopup(env, java_object_->View(env).obj(),
                                  items_array.obj(), enabled_array.obj(),
                                  multiple, selected_array.obj());
}

void ChromeView::OpenAutofillPopup(
    AutofillExternalDelegateAndroid* delegate,
    int query_id,
    const std::vector<string16>& autofill_values,
    const std::vector<string16>& autofill_labels,
    const std::vector<int>& autofill_unique_ids) {
  // If we have no values or labels this means "no match".
  // Remove the popup and return.
  if (!autofill_values.size()) {
    CloseAutofillPopup();
    return;
  }

  // The popup might currently be open, which means
  // autofill_external_delegate_ isn't NULL.  The Java ChromeView is
  // smart enough to reuse a popup when changing data.
  autofill_external_delegate_ = delegate;

  // We need an array of AutofillData.
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jclass> autofill_data_clazz =
      GetClass(env, "org/chromium/content/browser/AutofillData");
  jmethodID data_constructor = GetMethodID(
      env,
      autofill_data_clazz,
      "<init>",
      "(ILjava/lang/String;Ljava/lang/String;I)V");
  ScopedJavaLocalRef<jobjectArray> data_array(env,
      env->NewObjectArray(autofill_values.size(),
                          autofill_data_clazz.obj(), NULL));
  CheckException(env);
  for (size_t i = 0; i < autofill_values.size(); ++i) {
    ScopedJavaLocalRef<jstring> value =
        ConvertUTF16ToJavaString(env, autofill_values[i]);
    ScopedJavaLocalRef<jstring> label =
        ConvertUTF16ToJavaString(env, autofill_labels[i]);
    int unique_id = autofill_unique_ids[i];
    ScopedJavaLocalRef<jobject> data(env,
        env->NewObject(autofill_data_clazz.obj(), data_constructor,
                       query_id,
                       value.obj(),
                       label.obj(),
                       unique_id));
    CheckException(env);
    env->SetObjectArrayElement(data_array.obj(), i, data.obj());
    CheckException(env);
  }
  Java_ChromeView_openAutofillPopup(env, java_object_->View(env).obj(),
                                    data_array.obj());
}

void ChromeView::AutofillPopupClosed(JNIEnv* env, jobject obj) {
  if (autofill_external_delegate_)
    autofill_external_delegate_->OnClose();
  autofill_external_delegate_ = NULL;
}

void ChromeView::CloseAutofillPopup() {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_closeAutofillPopup(env, java_object_->View(env).obj());
}

void ChromeView::AutofillPopupSelected(JNIEnv *env, jobject obj,
                                       jint query_id, jint list_index,
                                       jint unique_id) {
  if (autofill_external_delegate_) {
    autofill_external_delegate_->SelectAutofillSuggestionAtIndex(list_index);
  }
  CloseAutofillPopup();
}

void ChromeView::ConfirmTouchEvent(bool handled) {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_confirmTouchEvent(env, java_object_->View(env).obj(),
                                    handled);
}

void ChromeView::DidSetNeedTouchEvents(bool need_touch_events) {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_didSetNeedTouchEvents(env, java_object_->View(env).obj(),
                                        need_touch_events);
}

void ChromeView::SetLastTapPosition(int x, int y) {
  last_tap_x_ = x;
  last_tap_y_ = y;
}

void ChromeView::ShowOnDemandZoom(const gfx::Rect& rect) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> rect_object(env,
      env->NewObject(java_object_->rect_clazz.obj(),
                     java_object_->rect_constructor,
                     rect.x(),
                     rect.y(),
                     rect.right(),
                     rect.bottom()));
  DCHECK(!rect_object.is_null());
  Java_ChromeView_onMultipleTargetsTouched(env, java_object_->View(env).obj(),
                                           rect_object.obj());
}

jobject ChromeView::GetJavaObject() {
  JNIEnv* env = AttachCurrentThread();
  jobject java_client = java_object_->View(env).Release();
  CheckException(env);
  return java_client;
}

// Constructs Java equivalent to FindNotificationDetails and passes it to the
// Java onFindResultAvailable handler.
void ChromeView::OnFindResultAvailable(const FindNotificationDetails* details) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jobject> rect_object(env,
      env->NewObject(java_object_->rect_clazz.obj(),
                     java_object_->rect_constructor,
                     details->selection_rect().x(),
                     details->selection_rect().y(),
                     details->selection_rect().right(),
                     details->selection_rect().bottom()));
  DCHECK(!rect_object.is_null());

  ScopedJavaLocalRef<jclass> details_clazz = GetClass(env,
      "org/chromium/content/browser/ChromeView$FindResultReceivedListener"
              "$FindNotificationDetails");
  jmethodID details_constructor = GetMethodID(env, details_clazz, "<init>",
      "(ILandroid/graphics/Rect;IZ)V");
  ScopedJavaLocalRef<jobject> details_object(env,
      env->NewObject(details_clazz.obj(), details_constructor,
                     details->number_of_matches(),
                     rect_object.obj(),
                     details->active_match_ordinal(),
                     details->final_update()));
  DCHECK(!details_object.is_null());

  Java_ChromeView_onFindResultAvailable(env, java_object_->View(env).obj(),
                                        details_object.obj());
}

bool ChromeView::HasFocus() {
  JNIEnv* env = AttachCurrentThread();
  jboolean result = Java_ChromeView_hasFocus(env,
                                             java_object_->View(env).obj());
  return result;
}

void ChromeView::OnSelectionChanged(const std::string& text) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jtext = ConvertUTF8ToJavaString(env, text);
  Java_ChromeView_onSelectionChanged(env, java_object_->View(env).obj(),
                                     jtext.obj());
}

void ChromeView::OnSelectionBoundsChanged(int startx,
                                          int starty,
                                          base::i18n::TextDirection start_dir,
                                          int endx,
                                          int endy,
                                          base::i18n::TextDirection end_dir) {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_onSelectionBoundsChanged(env, java_object_->View(env).obj(),
                                           startx, starty, start_dir,
                                           endx, endy, end_dir);
}

void ChromeView::ShowContextMenu(const ContextMenuParams& params) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jclass> info_clazz = GetClass(env,
      "org/chromium/content/browser/ChromeView$ChromeViewContextMenuInfo");
  ScopedJavaLocalRef<jobject> info_object;
  if (params.custom_items.empty()) {
    jmethodID info_constructor = GetMethodID(env, info_clazz, "<init>",
        "(IIIIIIILjava/lang/String;Ljava/lang/String;Ljava/lang/String;"
        "Ljava/lang/String;Ljava/lang/String;Z)V");
    ScopedJavaLocalRef<jstring> jlinkurl =
        ConvertUTF8ToJavaString(env, params.link_url.spec());
    ScopedJavaLocalRef<jstring> junflinkurl =
        ConvertUTF8ToJavaString(env, params.unfiltered_link_url.spec());
    ScopedJavaLocalRef<jstring> jsrcurl =
        ConvertUTF8ToJavaString(env, params.src_url.spec());
    ScopedJavaLocalRef<jstring> jtext =
        ConvertUTF16ToJavaString(env, params.selection_text);
    ScopedJavaLocalRef<jstring> jlinktext =
        ConvertUTF16ToJavaString(env, params.link_text);
    info_object.Reset(env, env->NewObject(info_clazz.obj(), info_constructor,
                                          params.x, params.y, params.media_type,
                                          params.selection_start.x(),
                                          params.selection_start.y(),
                                          params.selection_end.x(),
                                          params.selection_end.y(),
                                          jlinkurl.obj(), jlinktext.obj(),
                                          junflinkurl.obj(), jsrcurl.obj(),
                                          jtext.obj(), params.is_editable));
  } else {
    // Custom menu case.
    jmethodID info_constructor =
        GetMethodID(env, info_clazz, "<init>", "(II)V");
    info_object.Reset(env,
        env->NewObject(info_clazz.obj(), info_constructor, params.x, params.y));
    jmethodID add_item_method =
        GetMethodID(env, info_clazz, "addCustomItem", "(Ljava/lang/String;I)V");
    for (std::vector<WebMenuItem>::const_iterator iter =
             params.custom_items.begin();
         iter != params.custom_items.end(); ++iter) {
      ScopedJavaLocalRef<jstring> label =
          ConvertUTF16ToJavaString(env, iter->label);
      env->CallVoidMethod(info_object.obj(), add_item_method, label.obj(),
                          iter->action);
      CheckException(env);
    }
  }
  Java_ChromeView_showContextMenu(env, java_object_->View(env).obj(),
                                  info_object.obj());
}

void ChromeView::ShowCustomContextMenu(
    const std::vector<MenuItemInfo>& menu_items,
    OnContextMenuItemSelectedCallBack* callback) {
  if (menu_items.empty()) {
    LOG(WARNING) << "Ignoring request to show an empty custom context menu.";
    return;
  }

  context_menu_selected_callback_ = callback;

  // TODO(jcivelli): we piggy-back on the regular context menu path. Ideally we
  //                 would refactor that code to be more generic.
  ContextMenuParams params;
  for (std::vector<MenuItemInfo>::const_iterator iter = menu_items.begin();
       iter != menu_items.end(); ++iter) {
    WebMenuItem web_menu_item;
    web_menu_item.label = iter->label;
    web_menu_item.action = iter->id;
    params.custom_items.push_back(web_menu_item);
  }
  ShowContextMenu(params);
}

void ChromeView::SetLastPressAck(float x, float y, float width, float height) {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_setLastPressAck(env, java_object_->View(env).obj(),
                                  x, y, width, height);
}

void ChromeView::resetLastPressAck() {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_resetLastPressAck(env, java_object_->View(env).obj());
}

void ChromeView::OnPageStarted(const GURL& validated_url) {
  if (tab_contents_client_.get())
    tab_contents_client_->OnPageStarted(
        validated_url,
        tab_contents_wrapper()->favicon_tab_helper()->GetFavicon());
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_onPageStarted(env, java_object_->View(env).obj());
}

void ChromeView::OnPageFinished(const GURL& validated_url) {
  if (tab_contents_client_.get())
    tab_contents_client_->OnPageFinished(validated_url);

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jvalidated_url =
        ConvertUTF8ToJavaString(env, validated_url.spec());
  Java_ChromeView_onPageFinished(env, java_object_->View(env).obj(),
                                 jvalidated_url.obj());
}

void ChromeView::OnDidCommitMainFrame(const GURL& url, const GURL& base_url) {
  if (tab_contents_client_.get())
    tab_contents_client_->OnDidCommitMainFrame(url, base_url);
}

void ChromeView::OnInterstitialShown() {
  if (tab_contents_client_.get())
    tab_contents_client_->OnInterstitialShown();
}

void ChromeView::OnInterstitialHidden() {
  if (tab_contents_client_.get())
    tab_contents_client_->OnInterstitialHidden();
}

void ChromeView::OnAcceleratedCompositingStateChange(
    RenderWidgetHostViewAndroid* rwhva, bool activated, bool force) {
  static CommandLine* cmd = CommandLine::ForCurrentProcess();
  static bool in_process_gpu = cmd->HasSwitch(switches::kInProcessGPU);
  static bool in_process_webgl = cmd->HasSwitch(switches::kInProcessWebGL);
  static bool single_process = cmd->HasSwitch(switches::kSingleProcess);
  static bool force_compositing =
      cmd->HasSwitch(switches::kForceCompositingMode);

  std::string graphics_mode = cmd->GetSwitchValueASCII(switches::kGraphicsMode);
  if (graphics_mode == switches::kGraphicsModeValueBasic)
    return;

  if (!force && GetRenderWidgetHostViewAndroid() != rwhva) {
    return;
  }

  if (!force && force_compositing && accelerate_compositing_activated_) {
    // Even with force compositing flag on, the WebKit will first deactivate the
    // accelerated compositing, then activate it when loading a new site. This
    // is an optimization to avoid deleting and creating TextureView when
    // switching the site in the same RenderWidget. If RenderWidget is changed,
    // "force" will be true, we only call Java to create TextureView once.
    return;
  }

  accelerate_compositing_activated_ = activated;

  RenderWidgetHost* host = rwhva->GetRenderWidgetHost();
  DCHECK(host);
  content::RenderProcessHost* render_process = host->process();
  DCHECK(render_process);
  DCHECK(render_process->GetHandle());

  // pid can be:
  // 0: the new TextureView's SurfaceTexture will be sent back the browser;
  // gpu id: the new TextureView's SurfaceTexture will be sent to the gpu;
  int pid = 0;
  DCHECK(!in_process_webgl);
  if (!in_process_gpu && !single_process) {
    if (gpu_process_ == base::kNullProcessHandle) {
      LOG(WARNING) << "Trying to active compositing with unknown GPU process.";
      return;
    }
    pid = gpu_process_;
  } else {
    DCHECK(GpuChildThread::current());
  }

  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_activateHardwareAcceleration(
      env,
      java_object_->View(env).obj(),
      activated,
      pid,
      SurfaceTexturePeer::SET_GPU_SURFACE_TEXTURE,
      host->routing_id(),
      render_process->GetID());
}

void ChromeView::OnNativeWindowChanged(RenderWidgetHostViewAndroid* rwhva,
                                       bool attached) {
  if (GetRenderWidgetHostViewAndroid() != rwhva) {
    // This may happen when the old RenderWidgetHostViewAndroid is detached from
    // the ChromeView. e.g. if you load a different domain site in the same tab,
    // a new RenderWidgetHostViewAndroid will be attached to the ChromeView as
    // it uses a new render process. When the old RenderWidgetHostViewAndroid
    // finished clean up, it will come to here. Just return in this case.
    // This can also happen with interstitial pages.
    DCHECK(!attached);
    rwhva->WasHidden();
    return;
  }

  JNIEnv* env = AttachCurrentThread();
  if (attached) {
    if (web_contents()->ShowingInterstitialPage()) {
      // In the interstitial page, the WebContents' current RenderWidgetHostView
      // is for the hidden page, not the interstial.
      rwhva->DidBecomeSelected();
    } else {
      // DidBecomeSelected() does nothing if it is called twice. This may happen
      // when you load a different domain site in the same tab. As the
      // NativeWindow has been changed, it is important to notify the Java side.
      web_contents()->DidBecomeSelected();
    }
    Java_ChromeView_onNativeWindowAttached(env, java_object_->View(env).obj());
  } else {
    rwhva->WasHidden();
    Java_ChromeView_onNativeWindowDetached(env, java_object_->View(env).obj());
  }
}

void ChromeView::OnCompositorResized(uint64 timestamp, const gfx::Size& size) {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeView_onCompositorResized(env, java_object_->View(env).obj(),
      timestamp, size.width(), size.height());
}

void ChromeView::OnPageFailed(int error_code, const string16& description,
                              const GURL& failing_url) {
  if (tab_contents_client_.get())
    tab_contents_client_->OnReceivedError(error_code, description, failing_url);
  OnPageFinished(failing_url);
}

void ChromeView::StartContentIntent(const GURL& content_url) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jcontent_url =
      ConvertUTF8ToJavaString(env, content_url.spec());
  Java_ChromeView_startContentIntent(env,
                                     java_object_->View(env).obj(),
                                     jcontent_url.obj());
}

void ChromeView::OnReceivedHttpAuthRequest(
    ChromeHttpAuthHandler* auth_handler,
    const string16& host,
    const string16& realm) {
  if (tab_contents_client_.get()) {
    tab_contents_client_->OnReceivedHttpAuthRequest(auth_handler, host, realm);
  } else {
    JNIEnv* env = AttachCurrentThread();
    auth_handler->CancelAuth(env, NULL);
  }
}

void ChromeView::LoadUrlInternal(GURL url, int page_transition) {
  content::Referrer referer;

  web_contents()->GetController().LoadURL(
      url, referer, content::PageTransitionFromInt(page_transition), std::string());
  tab_crashed_ = false;

  if (tab_contents_client_.get())
    tab_contents_client_->OnInternalPageLoadRequest(web_contents(), url);
}

void ChromeView::AddShortcutToBookmark(const GURL& url, const string16& title,
                                       const SkBitmap& skbitmap, int r_value, int g_value,
                                       int b_value) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> java_bitmap;
  if (skbitmap.getSize()) {
    java_bitmap = ConvertToJavaBitmap(&skbitmap);
  }
  ScopedJavaLocalRef<jstring> bookmark_url = ConvertUTF8ToJavaString(env, url.spec());
  ScopedJavaLocalRef<jstring> bookmark_title = ConvertUTF16ToJavaString(env, title);
  Java_ChromeView_addShortcutToBookmark(env,
                                        java_object_->View(env).obj(),
                                        bookmark_url.obj(),
                                        bookmark_title.obj(),
                                        java_bitmap.obj(),
                                        r_value,
                                        g_value,
                                        b_value);
}

void ChromeView::PromoSendEmail(const string16& d_email,
                                const string16& d_subj,
                                const string16& d_body,
                                const string16& d_inv) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> j_email = ConvertUTF16ToJavaString(env, d_email);
  ScopedJavaLocalRef<jstring> j_subj = ConvertUTF16ToJavaString(env, d_subj);
  ScopedJavaLocalRef<jstring> j_body = ConvertUTF16ToJavaString(env, d_body);
  ScopedJavaLocalRef<jstring> j_inv = ConvertUTF16ToJavaString(env, d_inv);
  Java_ChromeView_promoSendEmail(env,
                                 java_object_->View(env).obj(),
                                 j_email.obj(),
                                 j_subj.obj(),
                                 j_body.obj(),
                                 j_inv.obj());
}

// ----------------------------------------------------------------------------
// Methods called from Java via JNI
// ----------------------------------------------------------------------------

void ChromeView::SetNetworkAvailable(JNIEnv* env, jobject obj, jboolean network_up) {
  RenderViewHost* host = web_contents()->GetRenderViewHost();
  // TODO(MERGE): http://codereview.chromium.org/7343011/
  host->Send(new ViewMsg_NetworkStateChanged(network_up));
}

void ChromeView::LoadUrlWithoutUrlSanitization(JNIEnv* env,
                                               jobject,
                                               jstring jurl,
                                               int page_transition) {
  GURL url(base::android::ConvertJavaStringToUTF8(env, jurl));

  LoadUrlInternal(url, page_transition);
}

ScopedJavaLocalRef<jstring> ChromeView::GetURL(JNIEnv* env, jobject) const {
  return ConvertUTF8ToJavaString(env, web_contents()->GetURL().spec());
}

ScopedJavaLocalRef<jstring> ChromeView::GetTitle(
    JNIEnv* env, jobject obj) const {
  return ConvertUTF16ToJavaString(env, web_contents()->GetTitle());
}

jdouble ChromeView::GetLoadProgress(JNIEnv* env, jobject obj) const {
  return static_cast<jdouble>(web_contents()->GetLoadProgress());
}

// Non-compositing draw path (for when browser-side backing stores are used).
// This packages all the information needed to paint the content bitmaps into
// JNI data structures and passes them to the Java-side drawImpl() for painting
// onto the screen canvas.  Because we are limiting ourselves to the NDK, we
// cannot do draws directly in C++.  The return value is nonzero if the draw
// failed, and contains an SkColor that the caller should fill the canvas with.
jint ChromeView::Draw(JNIEnv* env, jobject obj, jobject canvas) {
  // In compositing mode this code path will eventually call
  // host->GetBackingStore, which will return null.  The call performs a delay
  // before waiting for a message it does not receive.  This cuts the path short
  // and returns white, as it's normal for this to happen during initialization
  // of TextureView path.
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kForceCompositingMode)) {
    return SK_ColorWHITE;
  }

  RenderWidgetHostViewAndroid* view = GetRenderWidgetHostViewAndroid();

  DCHECK(!tab_crashed_);
  if (!view) {
    LOG(WARNING) << "Draw failed: RenderWidgetHostView unavailable.";
    return SK_ColorGREEN;
  }

  // We're creating a lot of small temporary JNI objects.  Put them in a local
  // frame (this will delete all the local refs when it goes out of scope).
  AutoLocalFrame local_frame(20);

  RenderWidgetHost* host = view->GetRenderWidgetHost();

  // Initialize the tile bitmap/position arrays and create an SkMatrix based
  // on the scroll offset and zoom level.
  SkMatrix skmatrix;
  skmatrix.reset();
  ScopedJavaLocalRef<jobjectArray> tile_array;
  ScopedJavaLocalRef<jobjectArray> pos_array;
  BackingStoreAndroid* backing_store = reinterpret_cast<BackingStoreAndroid*>(
      host->GetBackingStore(true));
  jobject bitmap = NULL;
  if (backing_store) {
    bitmap = backing_store->GetJavaBitmap();
    if (!bitmap) {
      LOG(WARNING) << "Draw failed: null Java bitmap.";
      return SK_ColorGREEN;
    }
  } else {
    LOG(WARNING) << "Draw failed: No backing store available.";
    return SK_ColorGREEN;
  }
  tile_array.Reset(env,
      env->NewObjectArray(1, java_object_->bitmap_clazz.obj(), bitmap));
  CheckException(env);
  ScopedJavaLocalRef<jobject> point(env,
      env->NewObject(java_object_->point_clazz.obj(),
                     java_object_->point_constructor, 0, 0));
  CheckException(env);
  pos_array.Reset(env, env->NewObjectArray(1, java_object_->point_clazz.obj(),
                                           point.obj()));
  CheckException(env);

// TODO(aelias): Repurpose this draw code for compositor-driven browser tiling.
#if 0
  if (tiling) {
    const BrowserTileCache::TileList& paintlist =
        host->browser_camera()->GetWindowPaintList();
    float scale = host->browser_camera()->GetInFlightMagnifyDelta();
    gfx::Point scroll_offset = host->browser_camera()->GetScrollOffset();
    skmatrix.setScale(scale, scale);
    skmatrix.preTranslate(-scroll_offset.x(), -scroll_offset.y());
    tile_array.Reset(env,
        env->NewObjectArray(paintlist.size(),
                            java_object_->bitmap_clazz.obj(), NULL));
    CheckException(env);
    pos_array.Reset(env,
        env->NewObjectArray(paintlist.size(),
                            java_object_->point_clazz.obj(), NULL));
    CheckException(env);
    int c = 0;
    for (BrowserTileCache::TileList::const_iterator i = paintlist.begin();
         i != paintlist.end();
         ++i, ++c) {
      gfx::Point pt = i->position();
      jobject tile_bitmap = i->backing_store()->GetJavaBitmap();
      // Make sure that we're in software path.
      if (!tile_bitmap) {
        LOG(WARNING) << "Draw failed: null Java bitmap.";
        return SK_ColorGREEN;
      }
      env->SetObjectArrayElement(tile_array.obj(), c, tile_bitmap);
      CheckException(env);

      // TODO(aelias): All these point objects may be triggering extra GC.
      // Consider replacing it with two arrays of ints instead.
      ScopedJavaLocalRef<jobject> point(env,
          env->NewObject(java_object_->point_clazz.obj(),
                         java_object_->point_constructor,
                         pt.x(), pt.y()));
      CheckException(env);
      env->SetObjectArrayElement(pos_array, c, point.obj());
      CheckException(env);
    }
  }
#endif

  // Create the Java Matrix object based on the SkMatrix determined above.
  ScopedJavaLocalRef<jfloatArray> matrix_values(env, env->NewFloatArray(9));
  CheckException(env);
  for (int i = 0; i < 9; i++) {
    float val = skmatrix[i];
    env->SetFloatArrayRegion(matrix_values.obj(), i, 1, &val);
    CheckException(env);
  }
  ScopedJavaLocalRef<jobject> matrix(env,
      env->NewObject(java_object_->matrix_clazz.obj(),
                     java_object_->matrix_constructor));
  CheckException(env);
  env->CallVoidMethod(matrix.obj(), java_object_->matrix_setvalues,
                      matrix_values.obj());
  CheckException(env);

  DCHECK(canvas);
  DCHECK(!matrix.is_null());
  DCHECK(!tile_array.is_null());
  DCHECK(!pos_array.is_null());
  // Finally tell Java to draw the tiles on the canvas.
  Java_ChromeView_bitmapDrawImpl(
      env, obj, canvas, matrix.obj(), tile_array.obj(), pos_array.obj());

  return 0;
}

void ChromeView::DidUpdateBackingStore() {
  Invalidate();
}

gfx::Rect ChromeView::GetBounds() const {
  return gfx::Rect(0, 0, width_, height_);
}

Profile* ChromeView::GetProfile() const {
  return Profile::FromBrowserContext(web_contents()->GetBrowserContext());
}

void ChromeView::SetSize(JNIEnv* env, jobject obj, jint width, jint height) {
  if (width != width_ || height != height_) {
    width_ = width;
    height_ = height;
    if (GetRenderWidgetHostViewAndroid())
      GetRenderWidgetHostViewAndroid()->SetSize(gfx::Size(width, height));
  }
}

void ChromeView::SetFocus(JNIEnv* env, jobject obj, jboolean focused) {
  if (!GetRenderWidgetHostViewAndroid())
    return;

  if (focused)
    GetRenderWidgetHostViewAndroid()->Focus();
  else
    GetRenderWidgetHostViewAndroid()->Blur();
}

jint ChromeView::TouchEvent(JNIEnv* env, jobject obj, jint type, jlong time,
                            jobjectArray pts) {
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kDisableTouch))
    return EVENT_NOT_FORWARDED_TO_NATIVE;
  RenderWidgetHostViewAndroid* rwhv = GetRenderWidgetHostViewAndroid();
  if (rwhv) {
    // This throws out scroll and pinch events if the initial touch is rejected
    // by the Javascript.  Without this, scroll/pinch is unreliable on any page
    // capturing onTouchEvent as soon as the renderer stops responding (e.g.
    // while refreshing tiles).
    // TODO(aelias): http://b/5255553 -- Find a way to allow the case where
    // JavaScript can choose to handle some of touch events during one touch
    // sequence, without compromising performance.  For example, renderer
    // handling horizontal move while leaving vertical move to the browser to
    // handle.
    if (rwhv->IsScrolling() || rwhv->IsPinching()) {
      if (!touch_cancel_event_sent_) {
        using WebKit::WebTouchEvent;
        WebKit::WebTouchEvent event;
        TouchPoint::BuildWebTouchEvent(
            env, WebKit::WebInputEvent::TouchCancel, time, pts, event);
        rwhv->TouchEvent(event);
        touch_cancel_event_sent_ = true;
        return EVENT_CONVERTED_TO_CANCEL;
      }
      return EVENT_NOT_FORWARDED_TO_NATIVE;
    }

    touch_cancel_event_sent_ = false;
    using WebKit::WebTouchEvent;
    WebKit::WebTouchEvent event;
    TouchPoint::BuildWebTouchEvent(env, type, time, pts, event);
    rwhv->TouchEvent(event);
    return EVENT_FORWARDED_TO_NATIVE;
  }
  return EVENT_NOT_FORWARDED_TO_NATIVE;
}

jint ChromeView::MouseMoveEvent(JNIEnv* env, jobject obj,
                                jlong time, jint x, jint y) {
  RenderWidgetHostViewAndroid* rwhv = GetRenderWidgetHostViewAndroid();
  if (!rwhv) return EVENT_NOT_FORWARDED_TO_NATIVE;

  double timeStampSeconds =
      static_cast<double>(time) / base::Time::kMillisecondsPerSecond;

  using WebKit::WebInputEventFactory;
  WebKit::WebMouseEvent event = WebInputEventFactory::mouseEvent(
      x, y, x, y, WebInputEventFactory::MouseEventTypeMove, timeStampSeconds);

  rwhv->MouseEvent(event);
  return EVENT_FORWARDED_TO_NATIVE;
}

jint ChromeView::SendMouseWheelEvent(JNIEnv* env, jobject obj, jint x, jint y,
                                     jlong time, jint direction) {
  RenderWidgetHostViewAndroid* rwhv = GetRenderWidgetHostViewAndroid();
  if (!rwhv)
    return EVENT_NOT_FORWARDED_TO_NATIVE;

  double timeStampSeconds =
  static_cast<double>(time) / base::Time::kMillisecondsPerSecond;

  using WebKit::WebInputEventFactory;
  WebInputEventFactory::MouseWheelDirectionType type;
  if (direction == WebInputEventFactory::SCROLL_UP) {
    type = WebInputEventFactory::SCROLL_UP;
  } else if (direction == WebInputEventFactory::SCROLL_DOWN) {
    type = WebInputEventFactory::SCROLL_DOWN;
  } else {
    return EVENT_NOT_FORWARDED_TO_NATIVE;
  }
  WebKit::WebMouseWheelEvent event = WebInputEventFactory::mouseWheelEvent(
      x, y, x, y, timeStampSeconds, type);

  rwhv->MouseWheelEvent(event);
  return EVENT_FORWARDED_TO_NATIVE;
}

void ChromeView::OrientationChange(JNIEnv* env, jobject obj, jint orientation) {
  web_contents()->GetRenderViewHost()->OrientationChange(orientation);
}

void ChromeView::ShowPressState(JNIEnv* env, jobject obj, jint x, jint y) {
  if (GetRenderWidgetHostViewAndroid())
    GetRenderWidgetHostViewAndroid()->ShowPressState(x, y);
}

void ChromeView::SingleTap(JNIEnv* env, jobject obj, jint x, jint y,
                           jboolean check_multiple_targets) {
  if (GetRenderWidgetHostViewAndroid())
    GetRenderWidgetHostViewAndroid()->SingleTap(x, y, check_multiple_targets);
}

void ChromeView::DoubleTap(JNIEnv* env, jobject obj, jint x, jint y) {
  if (GetRenderWidgetHostViewAndroid())
    GetRenderWidgetHostViewAndroid()->DoubleTap(x, y);
}

void ChromeView::LongPress(JNIEnv* env, jobject obj, jint x, jint y,
                           jboolean check_multiple_targets) {
  if (GetRenderWidgetHostViewAndroid())
    GetRenderWidgetHostViewAndroid()->LongPress(x, y, check_multiple_targets);
}

void ChromeView::SelectBetweenCoordinates(JNIEnv* env, jobject obj,
                                          jint x1, jint y1, jint x2, jint y2) {
  if (GetRenderWidgetHostViewAndroid())
    GetRenderWidgetHostViewAndroid()->SelectRange(gfx::Point(x1, y1),
                                                  gfx::Point(x2, y2));
}

void ChromeView::ScrollBy(JNIEnv* env, jobject obj, jint dx, jint dy) {
  if (GetRenderWidgetHostViewAndroid())
    GetRenderWidgetHostViewAndroid()->ScrollBy(dx, dy);
}

void ChromeView::FlingStart(JNIEnv* env, jobject obj,
                            jint x, jint y, jint vx, jint vy) {
  if (GetRenderWidgetHostViewAndroid())
    GetRenderWidgetHostViewAndroid()->FlingStart(x, y, vx, vy);
}

void ChromeView::FlingCancel(JNIEnv* env, jobject obj) {
  if (GetRenderWidgetHostViewAndroid())
    GetRenderWidgetHostViewAndroid()->FlingCancel();
}

void ChromeView::PinchBy(JNIEnv* env, jobject obj, jfloat delta,
                         jint anchor_x, jint anchor_y) {
  if (!GetRenderWidgetHostViewAndroid())
    return;

  GetRenderWidgetHostViewAndroid()->PinchBy(delta, anchor_x, anchor_y);
}

jboolean ChromeView::CanGoBack(JNIEnv* env, jobject obj) {
  return web_contents()->GetController().CanGoBack();
}

jboolean ChromeView::CanGoForward(JNIEnv* env, jobject obj) {
  return web_contents()->GetController().CanGoForward();
}

jboolean ChromeView::CanGoBackOrForward(JNIEnv* env, jobject obj, jint steps) {
  return web_contents()->GetController().CanGoToOffset(steps);
}

void ChromeView::GoBack(JNIEnv* env, jobject obj) {
  web_contents()->GetController().GoBack();
  tab_crashed_ = false;
}

void ChromeView::GoForward(JNIEnv* env, jobject obj) {
  web_contents()->GetController().GoForward();
  tab_crashed_ = false;
}

void ChromeView::GoBackOrForward(JNIEnv* env, jobject obj, jint steps) {
  web_contents()->GetController().GoToOffset(steps);
}

void ChromeView::StopLoading(JNIEnv* env, jobject obj) {
  web_contents()->Stop();
}

void ChromeView::Reload(JNIEnv* env, jobject obj) {
  // Set check_for_repost parameter to false as we have no repost confirmation
  // dialog ("confirm form resubmission" screen will still appear, however).
  web_contents()->GetController().Reload(false);
  tab_crashed_ = false;
}

void ChromeView::ClearHistory(JNIEnv* env, jobject obj) {
  web_contents()->GetController().PruneAllButActive();
}

jint ChromeView::FindAll(JNIEnv* env, jobject obj, jstring search_string) {
  find_operation_initiated_ = true;

  return tab_contents_wrapper_->find_tab_helper()->BlockingFind(
      ConvertJavaStringToUTF16(env, search_string),
      FindTabHelper::FIND_ALL,
      FindTabHelper::FORWARD_DIRECTION);
}

void ChromeView::FindNext(JNIEnv* env, jobject obj,
                          jboolean forward_direction) {
  if (!find_operation_initiated_)
    return;

  tab_contents_wrapper_->find_tab_helper()->BlockingFind(
      ASCIIToUTF16(""), FindTabHelper::FIND_NEXT,
      forward_direction ? FindTabHelper::FORWARD_DIRECTION
                        : FindTabHelper::BACKWARD_DIRECTION);
}

void ChromeView::StartFinding(JNIEnv* env, jobject obj, jstring search_string,
                              jboolean forward_direction,
                              jboolean case_sensitive) {
  find_operation_initiated_ = true;

  tab_contents_wrapper_->find_tab_helper()->StartFinding(
      ConvertJavaStringToUTF16(env, search_string), forward_direction,
      case_sensitive);
}

void ChromeView::ActivateNearestFindResult(JNIEnv* env, jobject obj,
                                           jfloat x, jfloat y) {
  tab_contents_wrapper_->find_tab_helper()->ActivateNearestFindResult(x, y);
}

void ChromeView::StopFinding(JNIEnv* env, jobject obj, jint selection_action) {
  find_operation_initiated_ = false;

  FindBarController::SelectionAction action;
  switch (selection_action) {
    case 0:
      action = FindBarController::kKeepSelection;
      break;
    case 1:
      action = FindBarController::kClearSelection;
      break;
    case 2:
      action = FindBarController::kActivateSelection;
      break;
    default:
      NOTREACHED();
      action = FindBarController::kClearSelection;
  }

  tab_contents_wrapper_->find_tab_helper()->StopFinding(action);
}

ScopedJavaLocalRef<jstring> ChromeView::GetPreviousFindText(
    JNIEnv* env, jobject obj) const {
  return ConvertUTF16ToJavaString(env,
      tab_contents_wrapper_->find_tab_helper()->previous_find_text());
}

void ChromeView::RequestFindMatchRects(JNIEnv* env, jobject obj,
                                       jint have_version) {
  tab_contents_wrapper_->find_tab_helper()->RequestFindMatchRects(have_version);
}

void ChromeView::ZoomToRendererRect(JNIEnv* env, jobject obj,
                                    jint x, jint y, jint width, jint height) {
  if (GetRenderWidgetHostViewAndroid())
    GetRenderWidgetHostViewAndroid()->ZoomToRect(
        gfx::Rect(x, y, width, height));
}

void ChromeView::SetClient(JNIEnv* env, jobject obj, jobject jclient) {
  scoped_ptr<ChromeViewClient> client(
    ChromeViewClient::CreateNativeChromeViewClient(env, jclient));

  if (tab_contents_wrapper_ && owns_tab_contents_wrapper_)
    web_contents()->SetDelegate(client.get());

  tab_contents_client_.swap(client);
}

void ChromeView::ScrollBegin(JNIEnv* env, jobject obj, jint x, jint y) {
  if (GetRenderWidgetHostViewAndroid())
    GetRenderWidgetHostViewAndroid()->ScrollBegin(x, y);
}

void ChromeView::ScrollEnd(JNIEnv* env, jobject obj) {
  if (GetRenderWidgetHostViewAndroid())
    GetRenderWidgetHostViewAndroid()->ScrollEnd();
}

void ChromeView::ScrollFocusedEditableNodeIntoView(JNIEnv* env, jobject obj) {
  RenderViewHost* host = web_contents()->GetRenderViewHost();
  gfx::Rect rect;
  host->Send(new ViewMsg_ScrollFocusedEditableNodeIntoRect(host->routing_id(), rect));
}

void ChromeView::UndoScrollFocusedEditableNodeIntoView(
    JNIEnv* env,
    jobject obj) {
  RenderViewHost* host = web_contents()->GetRenderViewHost();
  host->Send(
      new ViewMsg_UndoScrollFocusedEditableNodeIntoView(host->routing_id()));
}

void ChromeView::PinchBegin(JNIEnv* env, jobject obj) {
  RenderWidgetHostViewAndroid* rwhva = GetRenderWidgetHostViewAndroid();
  if (!rwhva)
    return;

  rwhva->PinchBegin();
}

void ChromeView::PinchEnd(JNIEnv* env, jobject obj) {
  RenderWidgetHostViewAndroid* rwhva = GetRenderWidgetHostViewAndroid();
  if (!rwhva)
    return;

  rwhva->PinchEnd();
}

void ChromeView::SetOnDemandZoomBitmap(const gfx::Size& size,
                                       void* memory,
                                       size_t mem_size) {
  JNIEnv* env = AttachCurrentThread();
  int width = size.width();
  int height = size.height();
  if (width <= 0 || height <= 0) {
    DLOG(ERROR) <<
        "Tried to create an invalid bitmap in ChromeView::SetOnDemandZoomBitmap";
    return;
  }
  ScopedJavaLocalRef<jobject> jByteBuffer(
      env, env->NewDirectByteBuffer(memory, mem_size));
  Java_ChromeView_createNewOnDemandBitmap(env, java_object_->View(env).obj(),
                                          width,
                                          height,
                                          jByteBuffer.obj());
  // Check and clear the exception.
  if (ClearException(env)) {
    LOG(ERROR) << "createNewOnDemandBitmap failed for width:"
        << width << " height:" << height;
  }
}

void ChromeView::SelectPopupMenuItems(JNIEnv* env, jobject obj,
                                      jintArray indices) {
  if (indices == NULL) {
    web_contents()->GetRenderViewHost()->DidCancelPopupMenu();
    return;
  }

  int selected_count = env->GetArrayLength(indices);
  std::vector<int> selected_indices;
  jint* indices_ptr = env->GetIntArrayElements(indices, NULL);
  for (int i = 0; i < selected_count; ++i)
    selected_indices.push_back(indices_ptr[i]);
  env->ReleaseIntArrayElements(indices, indices_ptr, JNI_ABORT);
  web_contents()->GetRenderViewHost()->DidSelectPopupMenuItems(selected_indices);
}

jboolean ChromeView::NeedsReload(JNIEnv* env, jobject obj) {
  return web_contents()->GetController().NeedsReload();
}

void ChromeView::SetAllowContentAccess(JNIEnv* /*env*/, jobject /*obj*/,
                                       jboolean allow) {
  allow_content_url_access_ = allow;
  web_contents()->SetAllowContentUrlAccess(allow_content_url_access_);
  RenderViewHost* host = web_contents()->GetRenderViewHost();
  DCHECK(host);
  host->SyncRendererPrefs();
}

void ChromeView::SetAllowFileAccess(JNIEnv* /*env*/, jobject /*obj*/,
                                    jboolean allow) {
  only_allow_file_access_to_android_resources_ = !allow;
  web_contents()->SetOnlyAllowFileAccessToAndroidResources(
      only_allow_file_access_to_android_resources_);
  RenderViewHost* host = web_contents()->GetRenderViewHost();
  DCHECK(host);
  host->SyncRendererPrefs();
}

jint ChromeView::EvaluateJavaScript(JNIEnv* env, jobject obj, jstring script) {
  RenderViewHost* host = web_contents()->GetRenderViewHost();
  DCHECK(host);

  string16 script_utf16 = ConvertJavaStringToUTF16(env, script);
  return host->ExecuteJavascriptInWebFrameNotifyResult(string16(),
                                                       script_utf16);
}

jint ChromeView::GetCurrentRenderProcess(JNIEnv* env, jobject obj) {
  RenderViewHost* host = web_contents()->GetRenderViewHost();
  DCHECK(host);
  content::RenderProcessHost* render_process = host->process();
  DCHECK(render_process);
  if (render_process->HasConnection())
    return render_process->GetHandle();
  else
    return 0;
}

void ChromeView::PurgeNativeMemory(JNIEnv* env, jobject obj) {
  RenderViewHost* host = web_contents()->GetRenderViewHost();
  DCHECK(host);
  host->Send(new ChromeViewMsg_PurgeMemory());
}

void ChromeView::AddJavascriptInterface(JNIEnv* env,
                                        jobject /* obj */,
                                        jobject object,
                                        jstring name,
                                        jboolean allow_inherited_methods) {
  ScopedJavaLocalRef<jobject> scoped_object(env, object);
  // JavaBoundObject creates the NPObject with a ref count of 1, and
  // JavaBridgeDispatcherHostManager takes its own ref.
  NPObject* bound_object = JavaBoundObject::Create(scoped_object,
                                                   allow_inherited_methods);
  // ChromeView's TabContentsWrapper always wraps a TabContents.
  // TODO(steveblock): We should avoid this cast. See b/5867579.
  TabContents* tab_contents = reinterpret_cast<TabContents*>(web_contents());
  tab_contents->java_bridge_dispatcher_host_manager()->AddNamedObject(
      ConvertJavaStringToUTF16(env, name), bound_object);
  WebKit::WebBindings::releaseObject(bound_object);
}

void ChromeView::RemoveJavascriptInterface(JNIEnv* env,
                                           jobject /* obj */,
                                           jstring name) {
  // ChromeView's TabContentsWrapper always wraps a TabContents.
  // TODO(steveblock): We should avoid this cast. See b/5867579.
  TabContents* tab_contents = reinterpret_cast<TabContents*>(web_contents());
  tab_contents->java_bridge_dispatcher_host_manager()->RemoveNamedObject(
      ConvertJavaStringToUTF16(env, name));
}

bool ChromeView::GetUseDesktopUserAgent(JNIEnv* env, jobject /* obj */) {
  // If the user agent is non-empty, we assume that it is a desktop user agent.
  content::NavigationEntry* entry =
      web_contents()->GetController().GetActiveEntry();
  return entry && !entry->GetUserAgentOverride().empty();
}

void ChromeView::SetUseDesktopUserAgent(JNIEnv* env,
                                        jobject obj,
                                        jboolean enabled,
                                        jboolean reload_on_state_change) {
  if (GetUseDesktopUserAgent(env, obj) != enabled) {
    // Make sure the NavigationEntry actually exists.
    content::NavigationEntry* entry =
        web_contents()->GetController().GetActiveEntry();
    if (!entry) return;

    // Set the flag in the NavigationEntry.
    const std::string& override =
        enabled ? webkit_glue::GetDesktopUserAgent() : std::string();
    entry->SetUserAgentOverride(override);

    // Send the override to the renderer.
    if (reload_on_state_change) {
      // Reloading the page will send the override down as part of the
      // navigation IPC message.
      web_contents()->GetController().ReloadWithOriginalURL();
    } else {
      // We need to manually send the override.
      RenderViewHost* host = web_contents()->GetRenderViewHost();
      host->Send(new ViewMsg_SetUserAgentOverride(host->routing_id(),
                                                  override));
    }
  }
}

bool ChromeView::GetJavaScriptEnabled(JNIEnv* env, jobject /* obj */) {
  WebPreferences prefs =
      web_contents()->GetRenderViewHost()->delegate()->GetWebkitPrefs();

  return prefs.javascript_enabled;
}

void ChromeView::AcknowledgeSwapBuffers(JNIEnv* env, jobject /* obj */) {
  RenderWidgetHostViewAndroid* rwhva = GetRenderWidgetHostViewAndroid();
  if (rwhva)
    rwhva->AcknowledgeSwapBuffers();
}

void ChromeView::SendVSyncEvent(JNIEnv* env, jobject /* obj */,
                                jlong frameBeginTimeUSec,
                                jlong currentFrameIntervalUSec) {
  RenderWidgetHostViewAndroid* rwhva = GetRenderWidgetHostViewAndroid();
  if (rwhva)
    rwhva->SendVSyncEvent(
        base::TimeTicks::FromInternalValue(frameBeginTimeUSec),
        base::TimeDelta::FromMicroseconds(currentFrameIntervalUSec));
}

// ----------------------------------------------------------------------------
// Native JNI methods
// ----------------------------------------------------------------------------


static void RestoreSessionCookies(JNIEnv*, jclass) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  // We set this to let SQLitePersistentCookieMonster know that it should retain
  // session cookies. In desktop Chrome this is set by the SessionRestore class
  // when it reopens tabs. That feature isn't implemented on Android yet.
  // TODO(husky): May need to change this when we implement tab restoration.
  profile->set_restored_last_session(true);
}

// This is called for each ChromeView.
static jint Init(JNIEnv* env, jobject obj, jobject context, jboolean incognito,
                 jboolean hardware_accelerated, jint native_web_contents) {
  CHECK(g_browser_process);
  ChromeView* view = new ChromeView(
      env, obj, incognito, hardware_accelerated,
      reinterpret_cast<WebContents*>(native_web_contents));
  return reinterpret_cast<jint>(view);
}

// This is designed to return null if no specific favicon is stored.
ScopedJavaLocalRef<jobject> ChromeView::GetFaviconBitmap(
    JNIEnv* env, jobject obj) {
  ScopedJavaLocalRef<jobject> bitmap;
  if (!tab_contents_wrapper()->favicon_tab_helper()->FaviconIsValid())
    return bitmap;

  const SkBitmap& skbitmap =
      tab_contents_wrapper()->favicon_tab_helper()->GetFavicon();
  if (skbitmap.getSize() > 0)
    bitmap = ConvertToJavaBitmap(&skbitmap);
  return bitmap;
}

jboolean ChromeView::FaviconIsValid(JNIEnv* env, jobject obj) {
  return tab_contents_wrapper()->favicon_tab_helper()->FaviconIsValid();
}

static void DestroyIncognitoProfile(JNIEnv*, jclass) {
   g_browser_process->profile_manager()->GetDefaultProfile()->
       DestroyOffTheRecordProfile();
}

static void CommitPendingWritesForProfile(Profile* profile) {
  profile->GetPrefs()->CommitPendingWrite();
}

static void CommitPendingWritesForLoadedProfiles() {
  std::vector<Profile*> loaded_profiles =
      g_browser_process->profile_manager()->GetLoadedProfiles();
  std::for_each(loaded_profiles.begin(), loaded_profiles.end(),
                CommitPendingWritesForProfile);
}

static void SendFlushNotification() {
  content::NotificationService* notification_service = content::NotificationService::current();
  if (notification_service) {
    // Tell all registered observers to start an immediate asynchronous flush.
    notification_service->Notify(
        chrome::NOTIFICATION_FLUSH_START,
        content::NotificationService::AllSources(),
        content::NotificationService::NoDetails());

    // We previously sent NOTIFICATION_FLUSH_FINISH and blocked for up to 3 seconds
    // until completion, in an attempt to flush as much as possible without hitting
    // the ANR limit of 5 seconds. We're still getting ANRs in this call stack, so
    // this tactic doesn't seem to be working. In the master branch, both Android
    // and Chrome now have strict guards against any blocking calls in the main
    // thread, so we have just removed this code completely. In M18, we'll omit the
    // NOTIFICATION_FLUSH_FINISH to get the same effect with a minimal diff. The
    // only listener is SQLitePersistentCookieStore, and it is happy to receive only
    // a FLUSH_START without a matching FLUSH_FINISH. See http://b/7085187
#if 0
    // Allow a little time for I/O to complete, determined by --flush-deadline-secs.
    std::string flag = CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
        switches::kFlushDeadlineSecs);
    int secs;
    if (!base::StringToInt(flag, &secs)) {
        secs = 3;  // Default to 3 seconds. We'll get an ANR after 5 seconds.
    }
    base::Time now = base::Time::NowFromSystemTime();
    base::Time deadline = now + base::TimeDelta::FromSeconds(secs);

    // Now tell all observers to block until their respective flushes are done.
    notification_service->Notify(
        chrome::NOTIFICATION_FLUSH_FINISH,
        content::NotificationService::AllSources(),
        content::Details<base::Time>(&deadline));

    // We don't expect to hit the deadline in normal usage. This could indicate
    // an outright bug, like an infinite loop, or just really really slow I/O.
    now = base::Time::NowFromSystemTime();
    if (now >= deadline) {
      LOG(ERROR) << "Missed flush deadline (" << secs << "s) by " << (now - deadline).InSecondsF()
          << " seconds! We may lose data if the OS kills us before I/O is complete.";
    }
#endif
  }
}

static void FlushPersistentData(JNIEnv* env, jclass obj) {
  CommitPendingWritesForLoadedProfiles();
  SendFlushNotification();
}

static jint EvaluateJavaScript(JNIEnv* env, jobject obj, jstring script) {
  ChromeView* view = ChromeView::GetNativeChromeView(env, obj);
  DCHECK(view);

  return view->EvaluateJavaScript(env, obj, script);
}

static void SetSurface(JNIEnv* env, jobject obj,
                       jobject surface,
                       jint primary_id,
                       jint secondary_id,
                       jint native_event) {
  static bool in_process_gpu =
      CommandLine::ForCurrentProcess()->HasSwitch(switches::kInProcessGPU);
  static bool in_process_webgl =
      CommandLine::ForCurrentProcess()->HasSwitch(switches::kInProcessWebGL);
  static bool single_process =
      CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess);

  WaitableNativeEvent* completion =
      reinterpret_cast<WaitableNativeEvent*>(native_event);

  DCHECK(!in_process_webgl);
  DCHECK(in_process_gpu || single_process);
  SetSurfaceAsync(env,
                  surface,
                  SurfaceTexturePeer::SET_GPU_SURFACE_TEXTURE,
                  primary_id,
                  secondary_id,
                  completion);
}

static int GetSurfaceID(
    JNIEnv* env, jobject obj, jint renderer_id, jint view_id) {
  RenderWidgetHost* host = RenderViewHost::FromID(renderer_id, view_id);
  DCHECK(host);
  int surface_id = 0;
  if (host) {
    surface_id = host->surface_id();
    DCHECK(surface_id);
  }
  return surface_id;
}

// ----------------------------------------------------------------------------

bool RegisterChromeView(JNIEnv* env) {
  if (!base::android::HasClass(env, kChromeViewClassPath)) {
    DLOG(ERROR) << "Unable to find class ChromeView!";
    return false;
  }
  ScopedJavaLocalRef<jclass> clazz = GetClass(env, kChromeViewClassPath);
  if (!HasField(env, clazz, "mNativeChromeView", "I")) {
    DLOG(ERROR) << "Unable to find ChromeView.mNativeChromeView!";
    return false;
  }
  g_native_chrome_view = GetFieldID(env, clazz, "mNativeChromeView", "I");

  return RegisterNativesImpl(env) >= 0;
}

int ChromeView::GetNativeImeAdapter(JNIEnv* env, jobject obj) {
  RenderWidgetHostViewAndroid* rwhva = GetRenderWidgetHostViewAndroid();
  if (!rwhva)
    return 0;
  return rwhva->GetNativeImeAdapter();
}
