// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/search/search_tab_helper.h"

#include "chrome/browser/google/google_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/search_engines/template_url_service.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/search/search.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(chrome::search::SearchTabHelper);

namespace {

bool IsNTP(const GURL& url) {
  return url.SchemeIs(chrome::kChromeUIScheme) &&
         url.host() == chrome::kChromeUINewTabHost;
}

Profile* ProfileFromWebContents(const content::WebContents* web_contents) {
  return Profile::FromBrowserContext(web_contents->GetBrowserContext());
}

bool IsSearchEnabled(Profile* profile) {
  return chrome::search::IsInstantExtendedAPIEnabled(profile);
}


bool IsSearchResults(const GURL& url, Profile* profile) {
  if (chrome::search::IsForcedInstantURL(url))
    return true;

  // Profile can be NULL in unit tests.
  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile);
  if (!template_url_service)
    return false;

  TemplateURL* template_url = template_url_service->GetDefaultSearchProvider();
  if (!template_url)
    return false;

  string16 result;
  return template_url->HasSearchTermsReplacementKey(url) &&
      template_url->ExtractSearchTermsFromURL(url, &result) && !result.empty();
}

}  // namespace

namespace chrome {
namespace search {

SearchTabHelper::SearchTabHelper(content::WebContents* web_contents)
    : is_search_enabled_(IsSearchEnabled(ProfileFromWebContents(web_contents))),
      user_input_in_progress_(false),
      model_(web_contents) {
  if (!is_search_enabled_)
    return;

  registrar_.Add(
      this,
      content::NOTIFICATION_NAV_ENTRY_COMMITTED,
      content::Source<content::NavigationController>(
          &web_contents->GetController()));
}

SearchTabHelper::~SearchTabHelper() {
}

void SearchTabHelper::OmniboxEditModelChanged(bool user_input_in_progress,
                                              bool cancelling) {
  if (!is_search_enabled_)
    return;

  user_input_in_progress_ = user_input_in_progress;
  if (!user_input_in_progress && !cancelling)
    return;

  UpdateModelBasedOnURL(web_contents()->GetURL());
}

void SearchTabHelper::NavigationEntryUpdated() {
  if (!is_search_enabled_)
    return;
  UpdateModelBasedOnURL(web_contents()->GetURL());
}

void SearchTabHelper::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(content::NOTIFICATION_NAV_ENTRY_COMMITTED, type);
  content::LoadCommittedDetails* committed_details =
      content::Details<content::LoadCommittedDetails>(details).ptr();
  UpdateModelBasedOnURL(committed_details->entry->GetVirtualURL());
}

void SearchTabHelper::UpdateModelBasedOnURL(const GURL& url) {
  Mode::Type type = Mode::MODE_DEFAULT;
  Mode::Origin origin = Mode::ORIGIN_DEFAULT;
  if (IsNTP(url)) {
    type = Mode::MODE_NTP;
    origin = Mode::ORIGIN_NTP;
  } else if (IsSearchResults(url, ProfileFromWebContents(web_contents()))) {
    type = Mode::MODE_SEARCH_RESULTS;
    origin = Mode::ORIGIN_SEARCH;
  }
  if (user_input_in_progress_)
    type = Mode::MODE_SEARCH_SUGGESTIONS;
  model_.SetMode(Mode(type, origin));
}

const content::WebContents* SearchTabHelper::web_contents() const {
  return model_.web_contents();
}

}  // namespace search
}  // namespace chrome
