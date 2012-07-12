// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TAB_CONTENTS_AUTO_LOGIN_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_TAB_CONTENTS_AUTO_LOGIN_INFOBAR_DELEGATE_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/string16.h"
#include "chrome/browser/infobars/infobar_delegate.h"
#include "chrome/browser/tab_contents/confirm_infobar_delegate.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace content {
class WebContents;
}

// An interface derived from InfoBarDelegate implemented by objects wishing to
// control an AutologinInfoBar.
// TODO(newt) or TODO(jrg): merge with AutoLoginInfoBarDelegate in
// chrome/browser/ui/autologin_infobar_delegate.h
class XAutoLoginInfoBarDelegate : public InfoBarDelegate,
                                  public content::NotificationObserver {
 public:
  class Params {
   public:
    // |value| is unparsed content of the x-auto-login header.
    explicit Params(const std::string& header_value);

    ~Params();

    // Return the value from the x-auto-login header.
    std::string header_value() const;

    // Return components from the (parsed) x-auto-login header.
    std::string realm() const;
    std::string account() const;  // "" is a valid value
    std::string args() const;

    // Is the x-auto-login request valid?  E.g. do we have any hope of finding
    // matching accounts?
    bool Valid() const {
      return !realm_.empty() && !args_.empty();
    }

   private:
    // Parse the x-auto-login header into component pieces.
    void ParseLoginHeader();

    // Value from the x-auto-login header.
    std::string header_value_;

    // Parsed components.
    std::string realm_;
    std::string account_;
    std::string args_;

    DISALLOW_COPY_AND_ASSIGN(Params);
  };

  // Create a new XAutoLoginInfoBarDelegate and attach it to the
  // specified tab.  |render_process_id| and |tab_contents_id| are IDs
  // needed by tab_util::GetTabContentsByID() to find a TabContents*.
  static void Create(int render_process_id, int tab_contents_id,
                     const std::string& header_value);

  // Called when the login button is pressed. If the function returns true, the
  // InfoBarDelegate should be removed from the associated TabContents.
  bool Login(const std::string& url);

  // Return the contents of the x-auto-login header, used as params for the
  // infobar.
  const Params& params() const;

  // Returns the text to display on an autologin infobar, given the account
  // name (e.g. foo@gmail.com) with which we'll sign in.
  string16 GetMessageText(string16 account) const;

  // Returns the text for the "ok" and "cancel" buttons on the infobar.
  string16 GetButtonLabel(ConfirmInfoBarDelegate::InfoBarButton button) const;

  // Overrides

  // Called to send the infobar away.  (AutoLogin infobars do not have
  // Cancel buttons.)  If the function returns true, the
  // InfoBarDelegate should be removed from the associated
  // TabContents.
  virtual bool Cancel();

 private:
  // Implemented by the platform to inform us whether or not we should show
  // an InfoBar for the given params.
  static bool ShouldShowInfoBar(content::WebContents* web_contents,
                                const Params& autologin_params);

  // Takes ownership of |auto_login_params|.
  XAutoLoginInfoBarDelegate(InfoBarTabHelper* infobar_helper, Params* params);
  virtual ~XAutoLoginInfoBarDelegate();

  // InfoBarDelegate:
  virtual InfoBar* CreateInfoBar(InfoBarTabHelper* owner) OVERRIDE;
  virtual bool EqualsDelegate(InfoBarDelegate* delegate) const OVERRIDE;
  virtual bool ShouldExpire(
      const content::LoadCommittedDetails& details) const OVERRIDE;
  virtual XAutoLoginInfoBarDelegate* AsXAutoLoginInfoBarDelegate() OVERRIDE;

  // NotificationObserver:
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  scoped_ptr<Params> auto_login_params_;
  content::WebContents* web_contents_;
  content::NotificationRegistrar registrar_;

  // Has this delegate been added to the InfoBarTabContents?
  bool has_been_added_;

  DISALLOW_COPY_AND_ASSIGN(XAutoLoginInfoBarDelegate);
};

#endif  // CHROME_BROWSER_TAB_CONTENTS_AUTO_LOGIN_INFOBAR_DELEGATE_H_
