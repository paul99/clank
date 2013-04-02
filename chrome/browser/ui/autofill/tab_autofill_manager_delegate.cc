// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/autofill/tab_autofill_manager_delegate.h"

#include "base/logging.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/api/infobars/infobar_service.h"
#include "chrome/browser/autofill/password_generator.h"
#include "chrome/browser/password_manager/password_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/autofill/autocheckout_bubble.h"
#include "chrome/browser/ui/autofill/autofill_dialog_controller_impl.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/common/url_constants.h"
#include "content/public/common/password_form.h"
#include "ui/gfx/rect.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(TabAutofillManagerDelegate);

TabAutofillManagerDelegate::TabAutofillManagerDelegate(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      web_contents_(web_contents),
      autofill_dialog_controller_(NULL) {
  DCHECK(web_contents);
}

content::BrowserContext* TabAutofillManagerDelegate::GetBrowserContext() const {
  return web_contents_->GetBrowserContext();
}

content::BrowserContext*
TabAutofillManagerDelegate::GetOriginalBrowserContext() const {
  return GetOriginalProfile();
}

Profile* TabAutofillManagerDelegate::GetOriginalProfile() const {
  return Profile::FromBrowserContext(web_contents_->GetBrowserContext())->
      GetOriginalProfile();
}

InfoBarService* TabAutofillManagerDelegate::GetInfoBarService() {
  return InfoBarService::FromWebContents(web_contents_);
}

PrefServiceBase* TabAutofillManagerDelegate::GetPrefs() {
  return Profile::FromBrowserContext(web_contents_->GetBrowserContext())->
      GetPrefs();
}

ProfileSyncServiceBase* TabAutofillManagerDelegate::GetProfileSyncService() {
  return ProfileSyncServiceFactory::GetForProfile(
      Profile::FromBrowserContext(web_contents_->GetBrowserContext()));
}

bool TabAutofillManagerDelegate::IsSavingPasswordsEnabled() const {
  return PasswordManager::FromWebContents(web_contents_)->IsSavingEnabled();
}

void TabAutofillManagerDelegate::ShowAutofillSettings() {
#if defined(OS_ANDROID)
  NOTIMPLEMENTED();
#else
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents_);
  if (browser)
    chrome::ShowSettingsSubPage(browser, chrome::kAutofillSubPage);
#endif  // #if defined(OS_ANDROID)
}

void TabAutofillManagerDelegate::ShowPasswordGenerationBubble(
      const gfx::Rect& bounds,
      const content::PasswordForm& form,
      autofill::PasswordGenerator* generator) {
#if defined(OS_ANDROID)
  NOTIMPLEMENTED();
#else
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents_);
  browser->window()->ShowPasswordGenerationBubble(bounds, form, generator);
#endif  // #if defined(OS_ANDROID)
}

void TabAutofillManagerDelegate::ShowAutocheckoutBubble(
    const gfx::RectF& bounding_box,
    const gfx::NativeView& native_view,
    const base::Closure& callback) {
  autofill::ShowAutocheckoutBubble(bounding_box, native_view, callback);
}

void TabAutofillManagerDelegate::ShowRequestAutocompleteDialog(
    const FormData& form,
    const GURL& source_url,
    const content::SSLStatus& ssl_status,
    const base::Callback<void(const FormStructure*)>& callback) {
  HideRequestAutocompleteDialog();

  autofill_dialog_controller_ =
      new autofill::AutofillDialogControllerImpl(web_contents_,
                                                 form,
                                                 source_url,
                                                 ssl_status,
                                                 callback);
  autofill_dialog_controller_->Show();
}

void TabAutofillManagerDelegate::RequestAutocompleteDialogClosed() {
  autofill_dialog_controller_ = NULL;
}

void TabAutofillManagerDelegate::UpdateProgressBar(double value) {
  autofill_dialog_controller_->UpdateProgressBar(value);
}

void TabAutofillManagerDelegate::HideRequestAutocompleteDialog() {
  if (autofill_dialog_controller_)
    autofill_dialog_controller_->Hide();
  RequestAutocompleteDialogClosed();
}

void TabAutofillManagerDelegate::DidNavigateMainFrame(
    const content::LoadCommittedDetails& details,
    const content::FrameNavigateParams& params) {
  // TODO(dbeam): selectively allow this dialog to remain open when going
  // through the autocheckout flow (when the behavior is more fleshed out).
  HideRequestAutocompleteDialog();
}
