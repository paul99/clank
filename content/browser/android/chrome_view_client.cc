// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/chrome_view_client.h"

#include <android/keycodes.h>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/command_line.h"
#include "chrome/browser/file_select_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_modal_dialogs/message_box_handler.h"
#include "chrome/browser/ui/blocked_content/blocked_content_tab_helper.h"
#include "chrome/browser/ui/find_bar/find_tab_helper.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "clank/native/framework/chrome/location_bar.h"
#include "content/browser/android/chrome_view.h"
#include "content/browser/android/ime_helper.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/renderer_host/render_widget_host_view.h"
#include "content/common/view_messages.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/find_match_rect_android.h"
#include "content/public/common/page_transition_types.h"
#include "content/public/common/referrer.h"
#include "jni/chrome_view_client_jni.h"
#include "ui/gfx/rect.h"
#include "webkit/glue/window_open_disposition.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertUTF16ToJavaString;
using base::android::GetClass;
using base::android::GetMethodID;
using base::android::HasClass;
using base::android::ScopedJavaLocalRef;
using content::WebContents;

using namespace net;

ChromeViewClient::ChromeViewClient(JNIEnv* env, jobject obj)
    : weak_java_client_(env, obj) {
}

ChromeViewClient::~ChromeViewClient() {
}

// static
ChromeViewClient* ChromeViewClient::CreateNativeChromeViewClient(
    JNIEnv* env, jobject obj) {
  if (!obj)
    return NULL;

  return new ChromeViewClient(env, obj);
}

void ChromeViewClient::OnInternalPageLoadRequest(
    content::WebContents* source, const GURL& url) {
  last_requested_navigation_url_ = url;
}

void ChromeViewClient::OnPageStarted(const GURL& url,
                                     const SkBitmap& favicon) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jstring_url =
      ConvertUTF8ToJavaString(env, url.spec());
  ScopedJavaLocalRef<jobject> jobject_favicon;
  if (favicon.getSize() > 0)
    jobject_favicon = ConvertToJavaBitmap(&favicon);
  Java_ChromeViewClient_onPageStarted(env, weak_java_client_.get(env).obj(),
                                      jstring_url.obj(),
                                      jobject_favicon.obj());
}

void ChromeViewClient::OnPageFinished(const GURL& url) {
  if (url == last_requested_navigation_url_)
    last_requested_navigation_url_ = GURL::EmptyGURL();

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jstring_url =
      ConvertUTF8ToJavaString(env, url.spec());

  Java_ChromeViewClient_onPageFinished(env, weak_java_client_.get(env).obj(),
                                       jstring_url.obj());
  CheckException(env);
}

void ChromeViewClient::OnReceivedError(int error_code,
                                      const string16& description,
                                      const GURL& url) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jstring_error_description =
      ConvertUTF8ToJavaString(env, url.spec());
  ScopedJavaLocalRef<jstring> jstring_url =
      ConvertUTF8ToJavaString(env, url.spec());

  Java_ChromeViewClient_onReceivedError(
      env, weak_java_client_.get(env).obj(),
      ToChromeViewClientError(error_code),
      jstring_error_description.obj(), jstring_url.obj());
}

void ChromeViewClient::OnReceivedHttpAuthRequest(
    ChromeHttpAuthHandler* auth_handler,
    const string16& host,
    const string16& realm) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jstring_host =
      ConvertUTF16ToJavaString(env, host);
  ScopedJavaLocalRef<jstring> jstring_realm =
      ConvertUTF16ToJavaString(env, realm);
  jobject jobject_chrome_http_auth_handler = auth_handler->GetJavaObject();
  Java_ChromeViewClient_onReceivedHttpAuthRequest(
      env, weak_java_client_.get(env).obj(),
      jobject_chrome_http_auth_handler,
      jstring_host.obj(),
      jstring_realm.obj());
}

void ChromeViewClient::OnDidCommitMainFrame(const GURL& url,
                                            const GURL& base_url) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jstring_url =
      ConvertUTF8ToJavaString(env, url.spec());
  ScopedJavaLocalRef<jstring> jstring_base_url =
      ConvertUTF8ToJavaString(env, base_url.spec());

  Java_ChromeViewClient_onMainFrameCommitted(
      env, weak_java_client_.get(env).obj(),
      jstring_url.obj(), jstring_base_url.obj());
}

void ChromeViewClient::OnInterstitialShown() {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeViewClient_onInterstitialShown(
      env, weak_java_client_.get(env).obj());
}

void ChromeViewClient::OnInterstitialHidden() {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeViewClient_onInterstitialHidden(
      env, weak_java_client_.get(env).obj());
}

// ----------------------------------------------------------------------------
// TabContentsDelegate methods
// ----------------------------------------------------------------------------

// OpenURLFromTab() will be called when we're performing a browser-intiated
// navigation. The most common scenario for this is opening new tabs (see
// RenderViewImpl::decidePolicyForNavigation for more details).
WebContents* ChromeViewClient::OpenURLFromTab(
    WebContents* source,
    const content::OpenURLParams& params) {
  const GURL& url = params.url;
  WindowOpenDisposition disposition = params.disposition;
  content::PageTransition transition(
      content::PageTransitionFromInt(params.transition));

  if (!source || (disposition != CURRENT_TAB &&
                  disposition != NEW_FOREGROUND_TAB &&
                  disposition != NEW_BACKGROUND_TAB &&
                  disposition != OFF_THE_RECORD)) {
    NOTIMPLEMENTED();
    return NULL;
  }

  if (disposition == NEW_FOREGROUND_TAB ||
      disposition == NEW_BACKGROUND_TAB ||
      disposition == OFF_THE_RECORD) {
    JNIEnv* env = AttachCurrentThread();
    ScopedJavaLocalRef<jstring> java_url =
        ConvertUTF8ToJavaString(env, url.spec());
    Java_ChromeViewClient_openNewTab(env,
                                     weak_java_client_.get(env).obj(),
                                     java_url.obj(),
                                     disposition == OFF_THE_RECORD);
    return NULL;
  }

  // TODO(mkosiba): This should be in platform_utils OpenExternal, b/6174564.
  if (transition == content::PAGE_TRANSITION_LINK &&
      ShouldOverrideLoading(url))
    return NULL;

  source->GetController().LoadURL(url, params.referrer, transition,
                                  std::string());
  return source;
}

// ShouldIgnoreNavigation will be called for every non-local top level
// navigation made by the renderer. If true is returned the renderer will
// not perform the navigation. This is done by using synchronous IPC so we
// should avoid blocking calls from this method.
bool ChromeViewClient::ShouldIgnoreNavigation(
    WebContents* source,
    const GURL& url,
    const content::Referrer& referrer,
    WindowOpenDisposition disposition,
    content::PageTransition transition_type) {

  // Don't override new tabs.
  if (disposition == NEW_FOREGROUND_TAB ||
      disposition == NEW_BACKGROUND_TAB ||
      disposition == OFF_THE_RECORD)
    return false;

  // Don't override the navigation that has just been requested via the
  // ChromeView.loadUrl method.
  if (url == last_requested_navigation_url_) {
    last_requested_navigation_url_ = GURL::EmptyGURL();
    return false;
  }

  // If the url has been typed into the omnibox, then we shouldn't even try to
  // override it unless it's something we can't render as a page.
  // This is to address the issue of going to pages like Play, where we get 3
  // redirects:
  // 1) http://play.google.com
  // 2) https://play.google.com
  // 3) https://play.google.com/store
  // The transition type is retained across redirects.
  if (transition_type == content::PAGE_TRANSITION_TYPED &&
      (url.SchemeIs("http") || url.SchemeIs("https"))) {
        return false;
  }

  return ShouldOverrideLoading(url);
}

void ChromeViewClient::NavigationStateChanged(
    const WebContents* source, unsigned changed_flags) {
  if (changed_flags & (
      content::INVALIDATE_TYPE_TAB | content::INVALIDATE_TYPE_TITLE)) {
    JNIEnv* env = AttachCurrentThread();
    Java_ChromeViewClient_onTabHeaderStateChanged(
        env, weak_java_client_.get(env).obj());
  }
}

void ChromeViewClient::AddNewContents(WebContents* source,
                                      WebContents* new_contents,
                                      WindowOpenDisposition disposition,
                                      const gfx::Rect& initial_pos,
                                      bool user_gesture) {
  // Pulled logic from Browser::AddNewContents to handle whether or not to show or block
  // this popup.  The behavior will mimic Chrome desktop.

  // No code for this yet.
  DCHECK(disposition != SAVE_TO_DISK);
  // Can't create a new contents for the current tab - invalid case.
  DCHECK(disposition != CURRENT_TAB);

  TabContentsWrapper* source_wrapper = NULL;
  BlockedContentTabHelper* source_blocked_content = NULL;
  TabContentsWrapper* new_wrapper =
      TabContentsWrapper::GetCurrentWrapperForContents(new_contents);
  if (!new_wrapper)
    new_wrapper = new TabContentsWrapper(new_contents);
  if (source) {
    source_wrapper = TabContentsWrapper::GetCurrentWrapperForContents(source);
    source_blocked_content = source_wrapper->blocked_content_tab_helper();
  }

  if (source_wrapper) {
    // Handle blocking of all contents.
    if (source_blocked_content->all_contents_blocked()) {
      source_blocked_content->AddTabContents(new_wrapper,
                                             disposition,
                                             initial_pos,
                                             user_gesture);
      return;
    }

    // Handle blocking of popups.
    if ((disposition == NEW_POPUP) && !user_gesture &&
        !CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kDisablePopupBlocking)) {
      // Unrequested popups from normal pages are constrained unless they're in
      // the whitelist.  The popup owner will handle checking this.
      source_wrapper->blocked_content_tab_helper()->
          AddPopup(new_wrapper, initial_pos, user_gesture);
      return;
    }

    RenderViewHost* view = new_contents->GetRenderViewHost();
    view->Send(new ViewMsg_DisassociateFromPopupCount(view->routing_id()));
  }

  // The new contents is either an allowed popup or of a different disposition.
  // We can tell the JNI level to show these contents.
  JNIEnv* env = AttachCurrentThread();
  bool handled = Java_ChromeViewClient_addNewContents(
      env,
      weak_java_client_.get(env).obj(),
      reinterpret_cast<int>(new_contents),
      static_cast<int>(disposition));
  if (!handled) {
    delete new_contents;
  }
}

void ChromeViewClient::ActivateContents(WebContents* contents) {
  // TODO(dtrainor) When doing the merge I came across this.  Should we be
  // activating this tab here?
}

void ChromeViewClient::DeactivateContents(WebContents* contents) {
  // Do nothing.
}

void ChromeViewClient::LoadingStateChanged(WebContents* source) {
  JNIEnv* env = AttachCurrentThread();
  bool has_stopped = source == NULL || !source->IsLoading();

  if (has_stopped)
    Java_ChromeViewClient_onLoadStopped(env, weak_java_client_.get(env).obj());
  else
    Java_ChromeViewClient_onLoadStarted(env, weak_java_client_.get(env).obj());
}

void ChromeViewClient::LoadProgressChanged(double progress) {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeViewClient_onLoadProgressChanged(env,
                                              weak_java_client_.get(env).obj(),
                                              progress);
}

void ChromeViewClient::CloseContents(WebContents* source) {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeViewClient_closeContents(env, weak_java_client_.get(env).obj());
}

void ChromeViewClient::MoveContents(WebContents* source,
                                    const gfx::Rect& pos) {
  // Do nothing.
}

// TODO(merge): WARNING! method no longer available on the base class.
// See http://b/issue?id=5862108
void ChromeViewClient::URLStarredChanged(WebContents* source, bool starred) {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeViewClient_onUrlStarredChanged(env,
                                            weak_java_client_.get(env).obj(),
                                            starred);
}

// This is either called from TabContents::DidNavigateMainFramePostCommit() with
// an empty GURL or responding to RenderViewHost::OnMsgUpateTargetURL(). In
// Chrome, the latter is not always called, especially not during history
// navigation. So we only handle the first case and pass the source TabContents'
// url to Java to update the UI.
void ChromeViewClient::UpdateTargetURL(WebContents* source,
                                       int32 page_id,
                                       const GURL& url) {
  if (url.is_empty()) {
    JNIEnv* env = AttachCurrentThread();
    ScopedJavaLocalRef<jstring> java_url =
        ConvertUTF8ToJavaString(env, source->GetURL().spec());
    Java_ChromeViewClient_onUpdateUrl(env,
                                      weak_java_client_.get(env).obj(),
                                      java_url.obj());
  }
}

void ChromeViewClient::FindReply(WebContents* tab,
                                 int request_id,
                                 int number_of_matches,
                                 const gfx::Rect& selection_rect,
                                 int active_match_ordinal,
                                 bool final_update) {
  TabContentsWrapper* tcw = TabContentsWrapper::GetCurrentWrapperForContents(
      tab);
  if (!tcw || !tcw->find_tab_helper())
    return;

  tcw->find_tab_helper()->HandleFindReply(request_id, number_of_matches,
                                          selection_rect, active_match_ordinal,
                                          final_update);
}

void ChromeViewClient::OnReceiveFindMatchRects(
    int version, const std::vector<FindMatchRect>& rects,
    const FindMatchRect& active_rect) {
  JNIEnv* env = AttachCurrentThread();

  // Constructs an float[] of (left, top, right, bottom) tuples and passes it on
  // to the Java onReceiveFindMatchRects handler which will use it to create
  // RectF objects equivalent to the std::vector<FindMatchRect>.
  ScopedJavaLocalRef<jfloatArray> rect_data(env,
      env->NewFloatArray(rects.size() * 4));
  jfloat* rect_data_floats = env->GetFloatArrayElements(rect_data.obj(), NULL);
  for (size_t i = 0; i < rects.size(); ++i) {
    rect_data_floats[4 * i]     = rects[i].left;
    rect_data_floats[4 * i + 1] = rects[i].top;
    rect_data_floats[4 * i + 2] = rects[i].right;
    rect_data_floats[4 * i + 3] = rects[i].bottom;
  }
  env->ReleaseFloatArrayElements(rect_data.obj(), rect_data_floats, 0);

  ScopedJavaLocalRef<jobject> active_rect_object;
  if (active_rect.left < active_rect.right &&
      active_rect.top < active_rect.bottom) {
    ScopedJavaLocalRef<jclass> rect_clazz =
        GetClass(env, "android/graphics/RectF");
    jmethodID rect_constructor =
        GetMethodID(env, rect_clazz, "<init>", "(FFFF)V");
    active_rect_object.Reset(env, env->NewObject(rect_clazz.obj(),
                                                 rect_constructor,
                                                 active_rect.left,
                                                 active_rect.top,
                                                 active_rect.right,
                                                 active_rect.bottom));
    DCHECK(!active_rect_object.is_null());
  }


  Java_ChromeViewClient_onReceiveFindMatchRects(
      env,
      weak_java_client_.get(env).obj(),
      version, rect_data.obj(),
      active_rect_object.obj());
}

bool ChromeViewClient::ShouldOverrideLoading(const GURL& url) {
  if (!url.is_valid())
    return false;

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jstring_url =
      ConvertUTF8ToJavaString(env, url.spec());
  bool ret = Java_ChromeViewClient_shouldOverrideUrlLoading(
      env, weak_java_client_.get(env).obj(), jstring_url.obj());
  return ret;
}

void ChromeViewClient::HandleKeyboardEvent(
    const NativeWebKeyboardEvent& event) {
  jobject key_event = KeyEventFromNative(event);
  if (key_event) {
    JNIEnv* env = AttachCurrentThread();
    Java_ChromeViewClient_handleKeyboardEvent(env,
                                              weak_java_client_.get(env).obj(),
                                              key_event);
  }
}

bool ChromeViewClient::TakeFocus(bool reverse) {
  JNIEnv* env = AttachCurrentThread();
  return Java_ChromeViewClient_takeFocus(env,
                                         weak_java_client_.get(env).obj(),
                                         reverse);
}

void ChromeViewClient::WasCrashedForReload() {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeViewClient_wasCrashedForReload(
      env, weak_java_client_.get(env).obj());
}

ChromeViewClientError ChromeViewClient::ToChromeViewClientError(int netError)
{
    // Note: many net::Error constants don't have an obvious mapping.
    // These will be handled by the default case, ERROR_UNKNOWN.
    switch(netError) {
        case ERR_UNSUPPORTED_AUTH_SCHEME:
            return ERROR_UNSUPPORTED_AUTH_SCHEME;

        case ERR_INVALID_AUTH_CREDENTIALS:
        case ERR_MISSING_AUTH_CREDENTIALS:
        case ERR_MISCONFIGURED_AUTH_ENVIRONMENT:
            return ERROR_AUTHENTICATION;

        case ERR_TOO_MANY_REDIRECTS:
            return ERROR_REDIRECT_LOOP;

        case ERR_UPLOAD_FILE_CHANGED:
            return ERROR_FILE_NOT_FOUND;

        case ERR_INVALID_URL:
            return ERROR_BAD_URL;

        case ERR_DISALLOWED_URL_SCHEME:
        case ERR_UNKNOWN_URL_SCHEME:
            return ERROR_UNSUPPORTED_SCHEME;

        case ERR_IO_PENDING:
        case ERR_NETWORK_IO_SUSPENDED:
            return ERROR_IO;

        case ERR_CONNECTION_TIMED_OUT:
        case ERR_TIMED_OUT:
            return ERROR_TIMEOUT;

        case ERR_FILE_TOO_BIG:
            return ERROR_FILE;

        case ERR_HOST_RESOLVER_QUEUE_TOO_LARGE:
        case ERR_INSUFFICIENT_RESOURCES:
        case ERR_OUT_OF_MEMORY:
            return ERROR_TOO_MANY_REQUESTS;

        case ERR_CONNECTION_CLOSED:
        case ERR_CONNECTION_RESET:
        case ERR_CONNECTION_REFUSED:
        case ERR_CONNECTION_ABORTED:
        case ERR_CONNECTION_FAILED:
        case ERR_SOCKET_NOT_CONNECTED:
            return ERROR_CONNECT;

        case ERR_ADDRESS_INVALID:
        case ERR_ADDRESS_UNREACHABLE:
        case ERR_NAME_NOT_RESOLVED:
        case ERR_NAME_RESOLUTION_FAILED:
            return ERROR_HOST_LOOKUP;

        case ERR_SSL_PROTOCOL_ERROR:
        case ERR_SSL_CLIENT_AUTH_CERT_NEEDED:
        case ERR_TUNNEL_CONNECTION_FAILED:
        case ERR_NO_SSL_VERSIONS_ENABLED:
        case ERR_SSL_VERSION_OR_CIPHER_MISMATCH:
        case ERR_SSL_RENEGOTIATION_REQUESTED:
        case ERR_CERT_ERROR_IN_SSL_RENEGOTIATION:
        case ERR_BAD_SSL_CLIENT_AUTH_CERT:
        case ERR_SSL_NO_RENEGOTIATION:
        case ERR_SSL_DECOMPRESSION_FAILURE_ALERT:
        case ERR_SSL_BAD_RECORD_MAC_ALERT:
        case ERR_SSL_UNSAFE_NEGOTIATION:
        case ERR_SSL_WEAK_SERVER_EPHEMERAL_DH_KEY:
        case ERR_SSL_CLIENT_AUTH_PRIVATE_KEY_ACCESS_DENIED:
        case ERR_SSL_CLIENT_AUTH_CERT_NO_PRIVATE_KEY:
            return ERROR_FAILED_SSL_HANDSHAKE;

        case ERR_PROXY_AUTH_UNSUPPORTED:
        case ERR_PROXY_AUTH_REQUESTED:
        case ERR_PROXY_CONNECTION_FAILED:
        case ERR_UNEXPECTED_PROXY_AUTH:
            return ERROR_PROXY_AUTHENTICATION;

        /* The certificate errors are handled by onReceivedSslError
         * and don't need to be reported here.
         */
        case ERR_CERT_COMMON_NAME_INVALID:
        case ERR_CERT_DATE_INVALID:
        case ERR_CERT_AUTHORITY_INVALID:
        case ERR_CERT_CONTAINS_ERRORS:
        case ERR_CERT_NO_REVOCATION_MECHANISM:
        case ERR_CERT_UNABLE_TO_CHECK_REVOCATION:
        case ERR_CERT_REVOKED:
        case ERR_CERT_INVALID:
        case ERR_CERT_WEAK_SIGNATURE_ALGORITHM:
        case ERR_CERT_NOT_IN_DNS:
        case ERR_CERT_NON_UNIQUE_NAME:
            return ERROR_OK;

        default:
            VLOG(1) << "ChromeViewClient::ToChromeViewClientError: Unknown chromium error: "
                    << netError;
            return ERROR_UNKNOWN;
    }
}

content::JavaScriptDialogCreator*
    ChromeViewClient::GetJavaScriptDialogCreator() {
  return GetJavaScriptDialogCreatorInstance();
}

void ChromeViewClient::RunFileChooser(WebContents* tab,
                                      const content::FileChooserParams& params) {
  Profile* profile =
      Profile::FromBrowserContext(tab->GetBrowserContext());
  // FileSelectHelper adds a reference to itself and only releases it after
  // sending the result message. It won't be destroyed when this reference
  // goes out of scope.
  scoped_refptr<FileSelectHelper> file_select_helper(
      new FileSelectHelper(profile));
  file_select_helper->RunFileChooser(tab->GetRenderViewHost(), tab, params);
}

// ----------------------------------------------------------------------------
// Native JNI methods
// ----------------------------------------------------------------------------

// Register native methods

bool RegisterChromeViewClient(JNIEnv* env) {
  if (!HasClass(env, kChromeViewClientClassPath)) {
    DLOG(ERROR) << "Unable to find class ChromeViewClient!";
    return false;
  }
  return RegisterNativesImpl(env);
}
