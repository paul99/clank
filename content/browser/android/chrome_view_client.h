// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_CHROME_VIEW_CLIENT_H_
#define CONTENT_BROWSER_ANDROID_CHROME_VIEW_CLIENT_H_
#pragma once

#include "base/compiler_specific.h"
#include "content/browser/android/jni_helper.h"
#include "content/public/browser/web_contents_delegate.h"
#include "net/base/net_errors.h"

struct FindMatchRect;
class ChromeView;
class ChromeHttpAuthHandler;

// This enum must be kept in sync with ChromeViewClient.java
enum ChromeViewClientError {
    // Success
    ERROR_OK = 0,
    // Generic error
    ERROR_UNKNOWN = -1,
    // Server or proxy hostname lookup failed
    ERROR_HOST_LOOKUP = -2,
    // Unsupported authentication scheme (not basic or digest)
    ERROR_UNSUPPORTED_AUTH_SCHEME = -3,
    // User authentication failed on server
    ERROR_AUTHENTICATION = -4,
    // User authentication failed on proxy
    ERROR_PROXY_AUTHENTICATION = -5,
    // Failed to connect to the server
    ERROR_CONNECT = -6,
    // Failed to read or write to the server
    ERROR_IO = -7,
    // Connection timed out
    ERROR_TIMEOUT = -8,
    // Too many redirects
    ERROR_REDIRECT_LOOP = -9,
    // Unsupported URI scheme
    ERROR_UNSUPPORTED_SCHEME = -10,
    // Failed to perform SSL handshake
    ERROR_FAILED_SSL_HANDSHAKE = -11,
    // Malformed URL
    ERROR_BAD_URL = -12,
    // Generic file error
    ERROR_FILE = -13,
    // File not found
    ERROR_FILE_NOT_FOUND = -14,
    // Too many requests during this load
    ERROR_TOO_MANY_REQUESTS = -15,
};

class ChromeViewClient : public content::WebContentsDelegate {
 public:
  ChromeViewClient(JNIEnv* env, jobject obj);

  static ChromeViewClient* CreateNativeChromeViewClient(JNIEnv* env,
                                                        jobject obj);

  // Called by ChromeView:
  void OnInternalPageLoadRequest(content::WebContents* source,
                                 const GURL& url);
  void OnPageStarted(const GURL& url, const SkBitmap& favicon);
  void OnPageFinished(const GURL& url);
  void OnLoadStarted();
  void OnLoadStopped();
  void OnReceivedError(int error_code,
                       const string16& description,
                       const GURL& url);
  void OnReceivedHttpAuthRequest(ChromeHttpAuthHandler* auth_handler,
                                 const string16& host,
                                 const string16& realm);
  void OnDidCommitMainFrame(const GURL& url,
                            const GURL& base_url);
  void OnInterstitialShown();
  void OnInterstitialHidden();

  // Overridden from WebContentsDelegate:
  virtual content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) OVERRIDE;

  virtual bool ShouldIgnoreNavigation(
      content::WebContents* source,
      const GURL& url,
      const content::Referrer& referrer,
      WindowOpenDisposition disposition,
      content::PageTransition transition_type) OVERRIDE;

  virtual void NavigationStateChanged(const content::WebContents* source,
                                      unsigned changed_flags) OVERRIDE;

  virtual void AddNewContents(content::WebContents* source,
                              content::WebContents* new_contents,
                              WindowOpenDisposition disposition,
                              const gfx::Rect& initial_pos,
                              bool user_gesture) OVERRIDE;

  virtual void ActivateContents(content::WebContents* contents) OVERRIDE;

  virtual void DeactivateContents(content::WebContents* contents) OVERRIDE;

  virtual void LoadingStateChanged(content::WebContents* source) OVERRIDE;

  virtual void LoadProgressChanged(double load_progress) OVERRIDE;

  virtual void CloseContents(content::WebContents* source) OVERRIDE;

  virtual void MoveContents(content::WebContents* source, const gfx::Rect& pos) OVERRIDE;

  // TODO(merge): WARNING! method no longer available on the base class.
  // See http://b/issue?id=5862108
  virtual void URLStarredChanged(content::WebContents* source, bool starred);

  virtual void UpdateTargetURL(content::WebContents* source,
                               int32 page_id,
                               const GURL& url) OVERRIDE;

  virtual void FindReply(content::WebContents* tab,
                         int request_id,
                         int number_of_matches,
                         const gfx::Rect& selection_rect,
                         int active_match_ordinal,
                         bool final_update) OVERRIDE;

  virtual void OnReceiveFindMatchRects(int version,
                                       const std::vector<FindMatchRect>& rects,
                                       const FindMatchRect& active_rect) OVERRIDE;

  virtual bool ShouldOverrideLoading(const GURL& url) OVERRIDE;

  virtual void HandleKeyboardEvent(const NativeWebKeyboardEvent& event) OVERRIDE;

  virtual content::JavaScriptDialogCreator* GetJavaScriptDialogCreator() OVERRIDE;

  virtual void RunFileChooser(content::WebContents* tab,
                              const content::FileChooserParams& params) OVERRIDE;

  virtual bool TakeFocus(bool reverse) OVERRIDE;

  virtual void WasCrashedForReload() OVERRIDE;

  virtual ~ChromeViewClient();

 private:
  // Get the closest ChromeViewClient match to the given Chrome error code.
  static ChromeViewClientError ToChromeViewClientError(int netError);

  // We use this to keep track of whether the navigation we get in
  // ShouldIgnoreNavigation has been initiated by the ChromeView or not.  We
  // need the GURL, because the active navigation entry doesn't change on
  // redirects.
  GURL last_requested_navigation_url_;

  // We depend on ChromeView.java to hold a ref to the client object. If we were
  // to hold a hard ref from native we could end up with a cyclic ownership leak
  // (the GC can't collect cycles if part of the cycle is caused by native).
  JavaObjectWeakGlobalRef weak_java_client_;
};

bool RegisterChromeViewClient(JNIEnv* env);

#endif  // CONTENT_BROWSER_ANDROID_CHROME_VIEW_CLIENT_H_
