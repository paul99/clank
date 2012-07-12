// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/login/login_prompt.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/string16.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/ui/login/login_prompt.h"
#include "content/browser/android/chrome_http_auth_handler.h"
#include "content/browser/android/chrome_view.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/auth.h"

using content::BrowserThread;
using net::URLRequest;
using net::AuthChallengeInfo;

class LoginHandlerAndroid : public LoginHandler {
 public:
  LoginHandlerAndroid(AuthChallengeInfo* auth_info, URLRequest* request)
      : LoginHandler(auth_info, request) {
  }

  virtual ~LoginHandlerAndroid() {}

  // LoginHandler methods:

  virtual void OnAutofillDataAvailable(
      const string16& username,
      const string16& password) OVERRIDE {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    DCHECK(chrome_http_auth_handler_.get() != NULL);
    chrome_http_auth_handler_->OnAutofillDataAvailable(
        username, password);
  }

  virtual void BuildViewForPasswordManager(
      PasswordManager* manager,
      const string16& explanation) OVERRIDE {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

    // Get pointer to ChromeView.
    content::WebContents* web_contents = GetWebContentsForLogin();
    CHECK(web_contents);
    RenderWidgetHostViewAndroid* rwhva =
        static_cast<RenderWidgetHostViewAndroid*>(
        web_contents->GetRenderWidgetHostView());
    ChromeView* chrome_view = rwhva ? rwhva->GetChromeView() : NULL;

    // Notify ChromeView that HTTP authentication is required for current page.
    if (chrome_view) {
      chrome_http_auth_handler_.reset(new ChromeHttpAuthHandler(explanation));
      chrome_http_auth_handler_->Init();
      chrome_http_auth_handler_->SetObserver(this);
      chrome_view->OnReceivedHttpAuthRequest(
          chrome_http_auth_handler_.get(),
          ASCIIToUTF16(auth_info()->challenger.ToString()),
          UTF8ToUTF16(auth_info()->realm));

      // Register to receive a callback to OnAutofillDataAvailable().
      SetModel(manager);
      NotifyAuthNeeded();
    } else {
      CancelAuth();
      LOG(WARNING) << "HTTP Authentication failed because ChromeView is missing";
    }
  }

 private:
  scoped_ptr<ChromeHttpAuthHandler> chrome_http_auth_handler_;
};

// static
LoginHandler* LoginHandler::Create(net::AuthChallengeInfo* auth_info,
                                   net::URLRequest* request) {
  return new LoginHandlerAndroid(auth_info, request);
}
