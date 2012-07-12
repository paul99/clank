// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/tab_contents/auto_login_infobar_delegate.h"

#include "base/memory/scoped_ptr.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/infobars/infobar_tab_helper.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/notification_source.h"
#include "content/public/common/page_transition_types.h"
#include "googleurl/src/url_parse.h"
#include "grit/generated_resources.h"
#include "net/base/escape.h"
#include "ui/base/l10n/l10n_util.h"

using content::BrowserThread;
using content::NavigationController;
using content::NotificationRegistrar;
using content::NotificationSource;
using content::NotificationDetails;
using content::Source;
using content::WebContents;

XAutoLoginInfoBarDelegate::Params::Params(
    const std::string& header_value)
    : header_value_(header_value) {
  ParseLoginHeader();
}

XAutoLoginInfoBarDelegate::Params::~Params() {
}

std::string XAutoLoginInfoBarDelegate::Params::header_value() const {
  return header_value_;
}

std::string XAutoLoginInfoBarDelegate::Params::realm() const {
  return realm_;
}

std::string XAutoLoginInfoBarDelegate::Params::account() const {
  return account_;
}

std::string XAutoLoginInfoBarDelegate::Params::args() const {
  return args_;
}

// Input: header_value_ = realm=com.google&account=fred.example%40gmail.com&
//                 args=kfdshfwoeriudslkfsdjfhdskjfhsdkr
// Output: realm_ = com.google
//         account_ = fred.example@gmail.com
//         args_ = kfdshfwoeriudslkfsdjfhdskjfhsdkr
// As a quick check, we confirmed that gmail won't let you create
// accounts with chars other than a-z,0-9,.. so we don't worry about
// multibyte strings.
void XAutoLoginInfoBarDelegate::Params::ParseLoginHeader() {
  url_parse::Component query(0, header_value_.size());
  url_parse::Component key, value;
  std::string string_key;
  while (url_parse::ExtractQueryKeyValue(header_value_.c_str(),
                                         &query, &key, &value)) {
    string_key = header_value_.substr(key.begin, key.len);
    std::string decoded_result = net::UnescapeURLComponent(
        header_value_.substr(value.begin, value.len),
        net::UnescapeRule::URL_SPECIAL_CHARS);

    if (string_key == "realm") {
      realm_ = decoded_result;
    } else if (string_key == "account") {
      account_ = decoded_result;
    } else if (string_key == "args") {
      args_ = decoded_result;
    } else {
      // TODO(jrg): google.com also has a continue=BLAH arg.  We drop
      // it on the floor.  The Android browser drops it on the floor
      // as well.  What SHOULD we be doing with it?
      VLOG(0) << "Unknown key \"" << string_key << "\""
              << " in x-auto-login header " << header_value_;
    }
  }
  // Required by x-auto-login spec; account is optional.
#if !defined(OS_ANDROID)  // TODO(tonyg): Enable this http://b/5648218.
  DCHECK(!realm_.empty() && !args_.empty());
#endif
}

// Brief notes on object ownernship:
// Create() will create an XAutoLoginInfoBarDelegate object.
// The delegate is either added to the TabContents when the page stops
// loading or if that never happens, it's deleted when the TabContents
// is destroyed (see Observe()).  If the delegate is added to the TabContents,
// then it deletes itself in an InfoBarClosed() callback by InfoBar::Close().
void XAutoLoginInfoBarDelegate::Create(int render_process_id,
                                       int tab_contents_id,
                                       const std::string& header_value) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  scoped_ptr<Params> params(new Params(header_value));
  if (!params->Valid())
    return;
  WebContents* web_contents = tab_util::GetWebContentsByID(render_process_id,
                                                           tab_contents_id);
  if (!web_contents) {
    LOG(ERROR) << "Failed to find tab for renderer_process_id: "
               << render_process_id << " and " << " tab_contents_id: "
               << tab_contents_id;
    return;
  }

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (!profile->GetPrefs()->GetBoolean(prefs::kAutologinEnabled))
    return;

  // An infobar delegate is not valid if the x-auto-login header does
  // not contain required fields.  If that happens it does not make
  // sense to show anything.
  if (!ShouldShowInfoBar(web_contents, *params))
    return;

  TabContentsWrapper* wrapper =
      TabContentsWrapper::GetCurrentWrapperForContents(web_contents);
  if (!wrapper) {
    // |wrapper| is NULL for TabContents hosted in HTMLDialog.
    return;
  }

  // The XAutoLoginInfoBarDelegate handles deleting itself.
  new XAutoLoginInfoBarDelegate(wrapper->infobar_tab_helper(),
                                params.release());
}

bool XAutoLoginInfoBarDelegate::Login(const std::string& url) {
  VLOG(1) << url;
  if (owner()) {
    owner()->web_contents()->Stop();
    owner()->web_contents()->OpenURL(content::OpenURLParams(
        GURL(url), content::Referrer(), CURRENT_TAB,
        content::PAGE_TRANSITION_TYPED, false));
  }
  return true;
}

const XAutoLoginInfoBarDelegate::Params&
    XAutoLoginInfoBarDelegate::params() const {
  return *auto_login_params_;
}

string16 XAutoLoginInfoBarDelegate::GetMessageText(string16 account) const {
  return l10n_util::GetStringFUTF16(IDS_AUTOLOGIN_INFOBAR_MESSAGE, account);
}

string16 XAutoLoginInfoBarDelegate::GetButtonLabel(
    ConfirmInfoBarDelegate::InfoBarButton button) const {
  return l10n_util::GetStringUTF16(
      (button == ConfirmInfoBarDelegate::BUTTON_OK) ?
      IDS_AUTOLOGIN_INFOBAR_OK_BUTTON : IDS_AUTOLOGIN_INFOBAR_CANCEL_BUTTON);
}

bool XAutoLoginInfoBarDelegate::Cancel() {
  return true;
}

XAutoLoginInfoBarDelegate::XAutoLoginInfoBarDelegate(
    InfoBarTabHelper* infobar_helper,
    Params* auto_login_params)
    : InfoBarDelegate(infobar_helper),
      auto_login_params_(auto_login_params),
      web_contents_(infobar_helper->web_contents()),
      has_been_added_(false) {
  registrar_.Add(this, content::NOTIFICATION_LOAD_STOP,
                 Source<NavigationController>(&web_contents_->GetController()));
  registrar_.Add(this, content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
                 Source<WebContents>(web_contents_));
}

XAutoLoginInfoBarDelegate::~XAutoLoginInfoBarDelegate() {
}

// Return true if types match.  We should only have one autologin
// infobar, ever.
bool XAutoLoginInfoBarDelegate::EqualsDelegate(InfoBarDelegate* delegate) const {
  XAutoLoginInfoBarDelegate* login_delegate =
      delegate->AsXAutoLoginInfoBarDelegate();
  return login_delegate;
}

// The trigger for an x-auto-login infobar happens at the network
// level, not at the page level.
bool XAutoLoginInfoBarDelegate::ShouldExpire(
    const content::LoadCommittedDetails& details) const {
  // Without preventing expiration based on reload, reloading a login page
  // causes the infobar to pop in then right back out.
  if (content::PageTransitionStripQualifier(details.entry->GetTransitionType()) ==
      content::PAGE_TRANSITION_RELOAD) {
    return false;
  }
  // The default implementation handles not expiring info bars based on
  // redirects, so this will handle clearing this info bar based on user
  // initiated navigation (forward and back buttons, clicking on a link,
  // etc...).
  return InfoBarDelegate::ShouldExpire(details);
}

XAutoLoginInfoBarDelegate* XAutoLoginInfoBarDelegate::AsXAutoLoginInfoBarDelegate() {
  return this;
}

void XAutoLoginInfoBarDelegate::Observe(int type,
                                        const NotificationSource& source,
                                        const NotificationDetails& details) {
  if (!has_been_added_) {
    if (type == content::NOTIFICATION_LOAD_STOP) {
      TabContentsWrapper* wrapper =
          TabContentsWrapper::GetCurrentWrapperForContents(web_contents_);
      InfoBarTabHelper* infobar_helper = wrapper->infobar_tab_helper();
      // Throw away any old infobars of our type, if needed.
      for (size_t i = 0; i < infobar_helper->infobar_count(); i++) {
        InfoBarDelegate* prev_infobar = infobar_helper->GetInfoBarDelegateAt(i);
        if (prev_infobar->AsXAutoLoginInfoBarDelegate()) {
          infobar_helper->ReplaceInfoBar(prev_infobar, this);
          has_been_added_ = true;
          return;
        }
      }
      // Nothing to replace, so just add.
      infobar_helper->AddInfoBar(this);
      has_been_added_ = true;
    } else {
      DCHECK(type == content::NOTIFICATION_WEB_CONTENTS_DESTROYED);
      // Either we couldn't add the infobar, we added the infobar, or the tab
      // contents was destroyed before the navigation completed.  In any case
      // there's no reason to live further.
      delete this;
    }
  }
}

// x-auto-login is only relevant for Android.
#if !defined(OS_ANDROID)
InfoBar* XAutoLoginInfoBarDelegate::CreateInfoBar(InfoBarTabHelper* owner) {
  NOTIMPLEMENTED();
  return NULL;
}

bool XAutoLoginInfoBarDelegate::ShouldShowInfoBar(
    WebContents* web_contents, const Params& autologin_params) {
  NOTIMPLEMENTED();
  return false;
}
#endif
