// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_CHROME_VIEW_H_
#define CONTENT_BROWSER_ANDROID_CHROME_VIEW_H_
#pragma once

#include <deque>
#include <vector>

#include "base/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/process.h"
#include "chrome/browser/browsing_data_remover.h"
#include "chrome/browser/prerender/prerender_manager.h"
#include "chrome/browser/ui/find_bar/find_bar_controller.h"
#include "chrome/browser/ui/find_bar/find_notification_details.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/browser/ui/webui/ntp/menu_item_info.h"
#include "content/browser/android/chrome_http_auth_handler.h"
#include "content/browser/android/jni_helper.h"
#include "content/public/browser/render_view_host_delegate.h"
#include "googleurl/src/gurl.h"
#include "third_party/skia/include/core/SkMatrix.h"

class AutofillExternalDelegateAndroid;
class BrowserCamera;
class ChromeViewClient;
namespace content {
class NotificationObserver;
class NotificationRegistrar;
}
class OnContextMenuItemSelectedCallBack;
class Profile;
class RenderWidgetHostViewAndroid;

class ChromeView {
 public:
  ChromeView(JNIEnv* env,
             jobject obj,
             bool incognito,
             bool hardware_accelerated,
             content::WebContents* web_contents);
  void Destroy(JNIEnv* env, jobject obj);

  static ChromeView* GetNativeChromeView(JNIEnv* env, jobject obj);

  // --------------------------------------------------------------------------
  // Public methods that call to Java via JNI
  // --------------------------------------------------------------------------

  // Called from UI thread
  void Invalidate();

  // Called from non-UI thread
  void PostInvalidate();

  void OnTabCrashed(const base::ProcessHandle handle);

  void UpdateContentSize(int width, int height);

  void UpdateScrollOffsetAndPageScaleFactor(int x, int y, float scale);

  void UpdatePageScaleLimits(float minimum_scale, float maximum_scale);

  void ImeUpdateAdapter(int native_ime_adapter, int text_input_type,
                        const gfx::Rect& caret_rect, const std::string& text,
                        int selection_start, int selection_end,
                        int composition_start, int composition_end,
                        bool show_ime_if_needed);

  void SetTitle(const string16& title);

  void ShowSelectPopupMenu(const std::vector<WebMenuItem>& items,
                           int selected_item,
                           bool multiple);

  void OpenAutofillPopup(AutofillExternalDelegateAndroid* delegate,
                         int query_id,
                         const std::vector<string16>& autofill_values,
                         const std::vector<string16>& autofill_labels,
                         const std::vector<int>& autofill_unique_ids);
  void CloseAutofillPopup();

  bool HasFocus();

  void ConfirmTouchEvent(bool handled);

  void DidSetNeedTouchEvents(bool need_touch_events);

  void OnSelectionChanged(const std::string& text);

  void OnSelectionBoundsChanged(int startx,
                                int starty,
                                base::i18n::TextDirection start_dir,
                                int endx,
                                int endy,
                                base::i18n::TextDirection end_dir);

  void ShowContextMenu(const ContextMenuParams& params);

  // Shows a context menu with custom items in it, controlled by the page (used
  // by WebUI).
  void ShowCustomContextMenu(const std::vector<MenuItemInfo>& menu_items,
                             OnContextMenuItemSelectedCallBack* callback);

  void SetLastPressAck(float x, float y, float width, float height);
  void resetLastPressAck();

  void DidStartLoading();
  void OnPageStarted(const GURL& validated_url);
  void OnPageFinished(const GURL& validated_url);
  void OnPageFailed(int error_code, const string16& description, const GURL& failingUrl);
  void OnReceivedHttpAuthRequest(ChromeHttpAuthHandler* auth_handler,
                                 const string16& host,
                                 const string16& realm);

  void OnAcceleratedCompositingStateChange(RenderWidgetHostViewAndroid* rwhva,
                                           bool activated,
                                           bool force);
  void OnNativeWindowChanged(RenderWidgetHostViewAndroid* rwhva, bool attached);
  void OnCompositorResized(uint64 timestamp, const gfx::Size& size);
  void OnDidCommitMainFrame(const GURL& url, const GURL& base_url);
  void OnInterstitialShown();
  void OnInterstitialHidden();

  void StartContentIntent(const GURL& content_url);

  // Called when context menu option to create the bookmark shortcut on homescreen is called.
  void AddShortcutToBookmark(const GURL& url, const string16& title, const SkBitmap& skbitmap,
                             int r_value, int g_value, int b_value);

  void AddJavascriptInterface(JNIEnv* env, jobject obj, jobject object,
                              jstring name);
  void RemoveJavascriptInterface(JNIEnv* env, jobject obj, jstring name);

  void LoadUrlInternal(GURL url, int page_transition);

  // --------------------------------------------------------------------------
  // Methods called from Java via JNI
  // --------------------------------------------------------------------------

  void SetNetworkAvailable(JNIEnv* env, jobject obj, jboolean network_up);

  void LoadUrlWithoutUrlSanitization(JNIEnv* env,
                                     jobject,
                                     jstring jurl,
                                     int page_transition);

  void PrefetchUrl(JNIEnv* env, jobject, jstring jurl, int page_transition);

  base::android::ScopedJavaLocalRef<jstring> GetURL(JNIEnv* env, jobject) const;

  base::android::ScopedJavaLocalRef<jstring> GetTitle(
      JNIEnv* env, jobject obj) const;

  jdouble GetLoadProgress(JNIEnv* env, jobject obj) const;

  jboolean Crashed(JNIEnv* env, jobject obj) const { return tab_crashed_; }

  jint Draw(JNIEnv* env, jobject obj, jobject canvas);

  void SetSize(JNIEnv* env, jobject obj, jint width, jint height);

  jint TouchEvent(JNIEnv* env, jobject obj, jint type, jlong time,
                  jobjectArray pts);

  jint MouseMoveEvent(JNIEnv* env, jobject obj, jlong time, jint x, jint y);

  void OrientationChange(JNIEnv* env, jobject obj, jint orientation);

  void ShowPressState(JNIEnv* env, jobject obj, jint x, jint y);

  void SingleTap(JNIEnv* env, jobject obj, jint x, jint y,
                 jboolean check_multiple_targets);

  void DoubleTap(JNIEnv* env, jobject obj, jint x, jint y);

  void LongPress(JNIEnv* env, jobject obj, jint x, jint y,
                 jboolean check_multiple_targets);

  void SelectBetweenCoordinates(JNIEnv* env, jobject obj,
                                jint x1, jint y1, jint x2, jint y2);

  void ScrollBy(JNIEnv* env, jobject obj, jint dx, jint dy);

  void FlingStart(JNIEnv* env, jobject obj, jint x, jint y, jint vx, jint vy);

  void FlingCancel(JNIEnv* env, jobject obj);

  void PinchBy(JNIEnv* env, jobject obj, jfloat delta, jint x, jint y);

  jboolean CanGoBack(JNIEnv* env, jobject obj);

  jboolean CanGoForward(JNIEnv* env, jobject obj);

  jboolean CanGoBackOrForward(JNIEnv* env, jobject obj, jint steps);

  void GoBack(JNIEnv* env, jobject obj);

  void GoForward(JNIEnv* env, jobject obj);

  void StopLoading(JNIEnv* env, jobject obj);

  void GoBackOrForward(JNIEnv* env, jobject obj, jint steps);

  void Reload(JNIEnv* env, jobject obj);

  void ClearHistory(JNIEnv* env, jobject obj);

  void StartFinding(JNIEnv* env, jobject obj, jstring search_string,
                    jboolean forward_direction,
                    jboolean case_sensitive);

  jint SendMouseWheelEvent(JNIEnv* env, jobject obj, jint x, jint y,
                                       jlong time, jint direction);

  // Returns number of search results found
  jint FindAll(JNIEnv* env, jobject obj, jstring jsearch_string);

  void FindNext(JNIEnv* env, jobject obj, jboolean forward_direction);

  void ActivateNearestFindResult(JNIEnv* env, jobject obj,
                                 jfloat x, jfloat y);

  void StopFinding(JNIEnv* env, jobject obj, jint selection_action);

  base::android::ScopedJavaLocalRef<jstring> GetPreviousFindText(
      JNIEnv* env, jobject obj) const;

  void RequestFindMatchRects(JNIEnv* env, jobject obj, jint have_version);

  void ZoomToRendererRect(JNIEnv* env, jobject obj,
                          jint x, jint y, jint width, jint height);

  void SetClient(JNIEnv* env, jobject obj, jobject jclient);

  void SetFocus(JNIEnv* env, jobject obj, jboolean focused);

  void ScrollFocusedEditableNodeIntoView(JNIEnv* env, jobject obj);
  void UndoScrollFocusedEditableNodeIntoView(JNIEnv* env, jobject obj);

  // Mark the beginning/end of certain gestures.
  void ScrollBegin(JNIEnv* env, jobject obj, jint x, jint y);
  void ScrollEnd(JNIEnv* env, jobject obj);
  void PinchBegin(JNIEnv* env, jobject obj);
  void PinchEnd(JNIEnv* env, jobject obj);

  // Called when an autofill item was selected.
  void AutofillPopupSelected(JNIEnv* env, jobject obj,
                             jint query_id, jint list_index,
                             jint unique_id);

  // Called when an autofill popup was closed.
  void AutofillPopupClosed(JNIEnv* env, jobject obj);

  // This sets a new fully-zoomed bitmap for on-demand zoom.
  // It assumes an ARGB8888 bitmap config for the data,
  // while rowbytes=size.width*4.
  void SetOnDemandZoomBitmap(const gfx::Size& size,
                             void* memory,
                             size_t mem_size);

  // Notifies the ChromeView that items were selected in the currently
  // showing select popup.
  void SelectPopupMenuItems(JNIEnv* env, jobject obj, jintArray indices);

  jboolean NeedsReload(JNIEnv* env, jobject obj);

  // Called to obtain the page's favicon as a Java bitmap.
  base::android::ScopedJavaLocalRef<jobject> GetFaviconBitmap(
      JNIEnv* env, jobject obj);
  jboolean FaviconIsValid(JNIEnv* env, jobject obj);

  void SetAllowContentAccess(JNIEnv* env, jobject obj, jboolean allow);
  void SetAllowFileAccess(JNIEnv* env, jobject obj, jboolean allow);

  jint EvaluateJavaScript(JNIEnv* env, jobject obj, jstring script);

  jint GetCurrentRenderProcess(JNIEnv* env, jobject obj);

  void PurgeNativeMemory(JNIEnv* env, jobject obj);

  // Get background color to paint in areas we don't have content for.
  jint GetBackgroundColor(JNIEnv* env, jobject obj);

  // Called when a custom context menu has been selected.
  void OnCustomMenuAction(JNIEnv* env, jobject obj, jint action);

  void OnShow(JNIEnv* env, jobject obj, jboolean requestByActivity);
  void OnHide(JNIEnv* env, jobject obj, jboolean requestByActivity);

  // Returns the base URL as it was when the frame was committed, that is before
  // the base tags have been processed.
  base::android::ScopedJavaLocalRef<jstring> GetBaseUrl(
      JNIEnv* env, jobject obj);

  // Returns the localized string based on the id.
  base::android::ScopedJavaLocalRef<jstring> GetLocalizedString(
      JNIEnv* env, jobject obj, jint string_id);

  int GetNativeImeAdapter(JNIEnv* env, jobject obj);

  // Sets whether or not the user agent is being overridden with a desktop one.
  void SetUseDesktopUserAgent(JNIEnv* env,
                              jobject /* obj */,
                              jboolean state,
                              jboolean reload_on_state_change);
  bool GetUseDesktopUserAgent(JNIEnv* env, jobject /* obj */);

  bool GetJavaScriptEnabled(JNIEnv* env, jobject /* obj */);

  void AcknowledgeSwapBuffers(JNIEnv* env, jobject /* obj */);

  void SendVSyncEvent(JNIEnv* env, jobject /* obj */,
                      jlong frameBeginTimeUSec,
                      jlong currentFrameIntervalUSec);

  // --------------------------------------------------------------------------
  // Methods called from native code
  // --------------------------------------------------------------------------

  void Show(bool requestByActivity);
  void Hide(bool requestByActivity);

  Profile* GetProfile() const;

  gfx::Rect GetBounds() const;

  void DidUpdateBackingStore();

  content::WebContents* web_contents() const {
    // It's possible for InstantTab to set our tab_contents_wrapper_ to NULL.
    return tab_contents_wrapper_ ? tab_contents_wrapper_->web_contents() : NULL;
  }

  TabContentsWrapper* tab_contents_wrapper() const {
    return tab_contents_wrapper_;
  }

  // Stores the last touch position for the last tap.  Used for showing the
  // on-demand zoom.
  void SetLastTapPosition(int x, int y);

  // Pops up an "On Demand Zoom" when the user taps between two links, to
  // clarify which link was requested.  This currently is only being
  // implemented in the software renderer, but will be ported once UI is
  // finalized.
  void ShowOnDemandZoom(const gfx::Rect& rect);

  // The caller is responsible for deleting the reference when done.
  // Used by DownloadController to retrieve the Java client.
  jobject GetJavaObject();

  void SetGPUProcess(base::ProcessHandle handle) { gpu_process_ = handle; }
  base::ProcessHandle GetGPUProcess() const { return gpu_process_; }

  // --------------------------------------------------------------------------
 private:
  friend class InstantTab;
  class DrawGLFunctor;

  // Set the tab contents but don't pass ownership of it to the
  // ChromeView.  Needed for instant before we commit.  We must tell
  // the ChromeView about the TabContents to insure infobars work.
  void SetWeakTabContentsWrapper(TabContentsWrapper* tab_contents_wrapper);

  // Instant needs the ability to replace our tab contents on commit.
  // This DOES pass ownership to the ChromeView.
  void SetTabContentsWrapper(TabContentsWrapper* tab_contents_wrapper);

  // --------------------------------------------------------------------------
  // Private methods that call to Java via JNI
  // --------------------------------------------------------------------------

  virtual ~ChromeView();

  void DestroyTabContentsWrapper();

  void OnFindResultAvailable(const FindNotificationDetails* details);

  // --------------------------------------------------------------------------
  // Other private methods and data
  // --------------------------------------------------------------------------

  void InitJNI(JNIEnv* env, jobject obj);

  RenderWidgetHostViewAndroid* GetRenderWidgetHostViewAndroid();

  struct JavaObject;
  JavaObject* java_object_;

  class ChromeViewNotificationObserver;
  scoped_ptr<content::NotificationObserver> notification_observer_;
  scoped_ptr<content::NotificationRegistrar> notification_registrar_;

  class ChromeViewWebContentsObserver;
  scoped_ptr<ChromeViewWebContentsObserver> web_contents_observer_;

  TabContentsWrapper* tab_contents_wrapper_;
  bool owns_tab_contents_wrapper_;

  // We only set this to be the delegate of a tab_contents if
  // we own the tab_contents.
  scoped_ptr<ChromeViewClient> tab_contents_client_;

  int width_;
  int height_;
  bool tab_crashed_;

  // Whether or not the view is currently showing an on-demand zoom.  This will
  // cause the view to be magnified, while a gradient mask is drawn around the
  // zoomed portion.
  bool on_demand_zoom_mode_;

  // The last tap position, used for on demand zoom popup as well as page launch
  // animation.
  int last_tap_x_, last_tap_y_;

  // The external delegate for autofill.  Weak; is owned by a TabContents.
  AutofillExternalDelegateAndroid* autofill_external_delegate_;

  // Set to true when FindAll or StartFinding is called. Set to false when
  // StopFinding is called
  bool find_operation_initiated_;

  // Allow access to content:// URLs.
  bool allow_content_url_access_;

  // Restrict file system access to file:///android_{res,asset}/.
  bool only_allow_file_access_to_android_resources_;

  // This is an optimization to avoid churning in creating TextureView
  bool accelerate_compositing_activated_;

  // Tracks whether a touch cancel event has been sent as a result of switching
  // into scrolling or pinching mode.
  bool touch_cancel_event_sent_;

  // The GPU process handle associated with this view
  base::ProcessHandle gpu_process_;

  // The callback used by custom context menu when selected.
  scoped_refptr<OnContextMenuItemSelectedCallBack>
      context_menu_selected_callback_;

  base::WeakPtrFactory<ChromeView> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChromeView);
};

bool RegisterChromeView(JNIEnv* env);

#endif  // CONTENT_BROWSER_ANDROID_CHROME_VIEW_H_
