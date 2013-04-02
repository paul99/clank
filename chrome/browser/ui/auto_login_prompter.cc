// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/auto_login_prompter.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/api/infobars/infobar_service.h"
#include "chrome/browser/google/google_url_tracker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/signin/token_service.h"
#include "chrome/browser/signin/token_service_factory.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/browser/ui/auto_login_info_bar_delegate.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/auto_login_parser/auto_login_parser.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"
#include "googleurl/src/gurl.h"
#include "net/url_request/url_request.h"

using content::BrowserThread;
using content::NavigationController;
using content::WebContents;

namespace {

bool FetchUsernameThroughSigninManager(Profile* profile, std::string* output) {
  // In an incognito window, there may not be a profile sync service and/or
  // signin manager.
  if (!ProfileSyncServiceFactory::GetInstance()->HasProfileSyncService(
      profile)) {
    return false;
  }

  if (!TokenServiceFactory::GetForProfile(profile)->AreCredentialsValid())
    return false;

  SigninManager* signin_manager =
      SigninManagerFactory::GetInstance()->GetForProfile(profile);
  if (!signin_manager)
    return false;

  *output = signin_manager->GetAuthenticatedUsername();
  return true;
}

}  // namespace

AutoLoginPrompter::AutoLoginPrompter(
    WebContents* web_contents,
    const Params& params)
    : web_contents_(web_contents),
      params_(params) {
  registrar_.Add(this, content::NOTIFICATION_LOAD_STOP,
                 content::Source<NavigationController>(
                    &web_contents_->GetController()));
  registrar_.Add(this, content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
                 content::Source<WebContents>(web_contents_));
}

AutoLoginPrompter::~AutoLoginPrompter() {
}

// static
void AutoLoginPrompter::ShowInfoBarIfPossible(net::URLRequest* request,
                                              int child_id,
                                              int route_id) {
  if (!CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnableAutologin))
      return;

  // See if the response contains the X-Auto-Login header.  If so, this was
  // a request for a login page, and the server is allowing the browser to
  // suggest auto-login, if available.
  Params params;
  // Currently we only accept GAIA credentials in Chrome.
  if (!components::auto_login::ParserHeaderInResponse(
          request,
          components::auto_login::ONLY_GOOGLE_COM,
          &params.header)) {
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&ShowInfoBarUIThread,
                 params, request->url(), child_id, route_id));
}


// static
void AutoLoginPrompter::ShowInfoBarUIThread(Params params,
                                            const GURL& url,
                                            int child_id,
                                            int route_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  WebContents* web_contents = tab_util::GetWebContentsByID(child_id, route_id);
  if (!web_contents)
    return;

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());

  if (!profile->GetPrefs()->GetBoolean(prefs::kAutologinEnabled))
    return;

#if !defined(OS_ANDROID)
  // On Android, the username is fetched on the Java side from the
  // AccountManager provided by the platform.
  if (!FetchUsernameThroughSigninManager(profile, &params.username))
    return;
#endif

  // Make sure that |account|, if specified, matches the logged in user.
  // However, |account| is usually empty.
  if (!params.username.empty() && !params.header.account.empty() &&
      params.username != params.header.account)
    return;
  // We can't add the infobar just yet, since we need to wait for the tab to
  // finish loading.  If we don't, the info bar appears and then disappears
  // immediately.  Create an AutoLoginPrompter instance to listen for the
  // relevant notifications; it will delete itself.
  new AutoLoginPrompter(web_contents, params);
}

void AutoLoginPrompter::Observe(int type,
                                const content::NotificationSource& source,
                                const content::NotificationDetails& details) {
  if (type == content::NOTIFICATION_LOAD_STOP) {
    InfoBarService* infobar_service =
        InfoBarService::FromWebContents(web_contents_);
    // |infobar_service| is NULL for WebContents hosted in WebDialog.
    if (infobar_service)
      AutoLoginInfoBarDelegate::Create(infobar_service, params_);
  }
  // Either we couldn't add the infobar, we added the infobar, or the tab
  // contents was destroyed before the navigation completed.  In any case
  // there's no reason to live further.
  delete this;
}
