// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/ntp/mobile_ntp_promo_handler.h"

#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/signin/signin_manager.h"
#include "chrome/browser/sync/glue/session_model_associator.h"
#include "chrome/browser/sync/glue/synced_session.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/web_resource/notification_promo.h"
#include "chrome/browser/web_resource/notification_promo_mobile_ntp.h"
#include "chrome/browser/web_resource/promo_resource_service.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/pref_names.h"
#include "content/browser/android/chrome_view.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"

using content::BrowserThread;

MobileNtpPromoHandler::MobileNtpPromoHandler()
    : did_fetch_sessions_(false) {
}

MobileNtpPromoHandler::~MobileNtpPromoHandler() {
}

void MobileNtpPromoHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("getPromotions",
      base::Bind(&MobileNtpPromoHandler::InjectPromoDecorations,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("recordImpression",
      base::Bind(&MobileNtpPromoHandler::RecordImpression,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("promoActionTriggered",
      base::Bind(&MobileNtpPromoHandler::HandlePromoActionTriggered,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("promoDisabled",
      base::Bind(&MobileNtpPromoHandler::HandlePromoDisabled,
                 base::Unretained(this)));

  Profile* profile = Profile::FromBrowserContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (profile) {
    // Watch for pref changes that cause us to need to re-inject promolines.
    registrar_.Add(this, chrome::NOTIFICATION_PROMO_RESOURCE_STATE_CHANGED,
                   content::NotificationService::AllSources());
    // Watch for sync service updates that could cause re-injections
    registrar_.Add(this, chrome::NOTIFICATION_SYNC_CONFIGURE_DONE,
                   content::NotificationService::AllSources());
    registrar_.Add(this, chrome::NOTIFICATION_FOREIGN_SESSION_UPDATED,
                   content::NotificationService::AllSources());
  }
}

// static
void MobileNtpPromoHandler::RegisterUserPrefs(PrefService* prefs) {
  prefs->RegisterBooleanPref(prefs::kNtpPromoDesktopSessionFound,
                             false,
                             PrefService::UNSYNCABLE_PREF);
}

void MobileNtpPromoHandler::RecordImpression(const base::ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (args && !args->empty())
    RecordPromotionImpression(UTF16ToASCII(ExtractStringValue(args)));
}

void MobileNtpPromoHandler::RecordPromotionImpression(const std::string& id) {
  if (!id.compare("most_visited"))
    content::RecordAction(
        content::UserMetricsAction("MobilePromoShownOnMostVisited"));
  else if (!id.compare("open_tabs"))
    content::RecordAction(
        content::UserMetricsAction("MobilePromoShownOnOpenTabs"));
  else if (!id.compare("sync_promo"))
    content::RecordAction(
        content::UserMetricsAction("MobilePromoShownOnSyncPromo"));
  else if (!id.compare("send_email"))
    content::RecordAction(
        content::UserMetricsAction("MobilePromoEmailLinkPressed"));
  else if (!id.compare("disable_promo"))
    content::RecordAction(
        content::UserMetricsAction("MobilePromoClosePressedOnOpenTabs"));
  else {
    NOTREACHED() << "Unknown promotion impression: " << id;
    return;
  }

  // Update number of views a promotion has received and trigger refresh
  // if it exceeded max_views set for the promotion.
  Profile* profile = Profile::FromWebUI(web_ui());
  if (profile) {
    NotificationPromo notification_promo(profile);
    if (notification_promo.HandleViewed()) {
      content::NotificationService* service =
          content::NotificationService::current();
      service->Notify(chrome::NOTIFICATION_PROMO_RESOURCE_STATE_CHANGED,
                      content::Source<MobileNtpPromoHandler>(this),
                      content::NotificationService::NoDetails());
    }
  }
}

void MobileNtpPromoHandler::HandlePromoSendEmail(const base::ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  Profile* profile = Profile::FromBrowserContext(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (!profile)
    return;

  string16 data_subject, data_body, data_inv;
  if (!args || args->GetSize() < 3) {
    LOG(ERROR) << "promoSendEmail: expected three args, got "
               << (args ? args->GetSize() : 0);
    return;
  }
  args->GetString(0, &data_subject);
  args->GetString(1, &data_body);
  args->GetString(2, &data_inv);
  if (data_inv.empty() || (data_subject.empty() && data_body.empty()))
    return;

  ChromeView* view = web_ui()->GetWebContents()->GetNativeView();
  if (!view)
    return;

  std::string data_email;
  ProfileSyncService* service =
      ProfileSyncServiceFactory::GetInstance()->GetForProfile(profile);
  if (service && service->signin())
    data_email = service->signin()->GetAuthenticatedUsername();

  view->PromoSendEmail(UTF8ToUTF16(data_email),
                       data_subject, data_body, data_inv);
  RecordPromotionImpression("send_email");
}

bool MobileNtpPromoHandler::IsChromePromoAllowedUncached(
    bool promo_requires_sync,
    bool promo_requires_no_active_desktop_sync_sessions) {
  Profile* profile = Profile::FromWebUI(web_ui());
  if (!profile)
    return false;
  if (!promo_requires_sync)
    return true;
  ProfileSyncService* service =
      ProfileSyncServiceFactory::GetInstance()->GetForProfile(profile);
  if (!service || !service->ShouldPushChanges())
    return false;
  if (!promo_requires_no_active_desktop_sync_sessions)
    return true;
  PrefService* prefs = profile->GetPrefs();
  if (!prefs)
    return true;

  // Fetch the sessions at least once to see what's happened
  // while this NTP was closed.
  if (!did_fetch_sessions_)
    CheckDesktopSessions();

  if (prefs->GetBoolean(prefs::kNtpPromoDesktopSessionFound))
    return false;
  return true;
}

void MobileNtpPromoHandler::HandlePromoActionTriggered(
    const base::ListValue* /*args*/) {
  Profile* profile = Profile::FromWebUI(web_ui());
  if (!profile ||
      !PromoResourceService::CanShowNotificationPromo(profile))
    return;

  NotificationPromo notification_promo(profile);
  notification_promo.InitFromPrefs();
  NotificationPromoMobileNtp promo(notification_promo);
  if (!promo.valid() ||
      !IsChromePromoAllowedUncached(
          promo.requires_sync(), promo.requires_mobile_only_sync()))
    return;
  if (promo.action_type() == "ACTION_EMAIL")
    HandlePromoSendEmail(promo.action_args());
  else {
    LOG(ERROR) << "HandlePromoActionTriggered: Unknown action type "
               << promo.action_type();
  }
}

void MobileNtpPromoHandler::HandlePromoDisabled(
    const base::ListValue* /*args*/) {
  Profile* profile = Profile::FromWebUI(web_ui());
  if (!profile ||
      !PromoResourceService::CanShowNotificationPromo(profile))
    return;

  NotificationPromo notification_promo(profile);
  notification_promo.HandleClosed();
  RecordPromotionImpression("disable_promo");

  content::NotificationService* service =
      content::NotificationService::current();
  service->Notify(chrome::NOTIFICATION_PROMO_RESOURCE_STATE_CHANGED,
                  content::Source<MobileNtpPromoHandler>(this),
                  content::NotificationService::NoDetails());
}

namespace {
string16 ReplaceSimpleMarkupWithHTML(string16 text) {
  const string16 LINE_BREAK = ASCIIToUTF16("<br/>");
  const string16 SYNCGRAPHIC_IMAGE =
      ASCIIToUTF16("<div class=\"promo-sync-graphic\"></div>");
  const string16 BEGIN_HIGHLIGHT =
      ASCIIToUTF16(
          "<div style=\"text-align: center\"><button class=\"promo-button\">");
  const string16 END_HIGHLIGHT =
      ASCIIToUTF16("</button></div>");
  const string16 BEGIN_LINK =
      ASCIIToUTF16("<span style=\"color: blue; text-decoration: underline;\">");
  const string16 END_LINK =
      ASCIIToUTF16("</span>");
  const string16 BEGIN_PROMO_AREA =
      ASCIIToUTF16("<div class=\"promo-action-target\">");
  const string16 END_PROMO_AREA =
      ASCIIToUTF16("</div>");

  ReplaceSubstringsAfterOffset(
      &text, 0, ASCIIToUTF16("LINE_BREAK"), LINE_BREAK);
  ReplaceSubstringsAfterOffset(
      &text, 0, ASCIIToUTF16("SYNCGRAPHIC_IMAGE"), SYNCGRAPHIC_IMAGE);
  ReplaceSubstringsAfterOffset(
      &text, 0, ASCIIToUTF16("BEGIN_HIGHLIGHT"), BEGIN_HIGHLIGHT);
  ReplaceSubstringsAfterOffset(
      &text, 0, ASCIIToUTF16("END_HIGHLIGHT"), END_HIGHLIGHT);
  ReplaceSubstringsAfterOffset(
      &text, 0, ASCIIToUTF16("BEGIN_LINK"), BEGIN_LINK);
  ReplaceSubstringsAfterOffset(
      &text, 0, ASCIIToUTF16("END_LINK"), END_LINK);
  return BEGIN_PROMO_AREA + text + END_PROMO_AREA;
}
}  // namespace

void MobileNtpPromoHandler::InjectPromoDecorations(
    const base::ListValue* /*args*/) {
  scoped_ptr<DictionaryValue> result(new DictionaryValue());
  bool isAllowed = false;
  bool isAllowedOnMostVisited = false;
  bool isAllowedOnOpenTabs = true;
  bool isAllowedAsVC = true;
  string16 promo_line;
  string16 promo_line_long;
  std::string promo_vc_title;
  std::string promo_vc_lastsync;

  Profile* profile = Profile::FromWebUI(web_ui());
  if (profile &&
      PromoResourceService::CanShowNotificationPromo(profile)) {
    NotificationPromo notification_promo(profile);
    notification_promo.InitFromPrefs();
    NotificationPromoMobileNtp promo(notification_promo);
    if (promo.valid() &&
        IsChromePromoAllowedUncached(
            promo.requires_sync(), promo.requires_mobile_only_sync())) {
      promo_vc_title = promo.virtual_computer_title();
      promo_vc_lastsync = promo.virtual_computer_lastsync();
      isAllowedOnMostVisited = promo.show_on_most_visited();
      isAllowedOnOpenTabs = promo.show_on_open_tabs();
      isAllowedAsVC = promo.show_as_virtual_computer();
      const std::string utf8 = promo.text();
      const std::string utf8long = promo.text_long();
      if (!utf8.empty() &&
          UTF8ToUTF16(utf8.c_str(), utf8.length(), &promo_line) &&
          UTF8ToUTF16(utf8long.c_str(), utf8long.length(), &promo_line_long)) {
        promo_line = ReplaceSimpleMarkupWithHTML(promo_line);
        promo_line_long = ReplaceSimpleMarkupWithHTML(promo_line_long);
        isAllowed = true;
      }
    }
  }

  result->SetBoolean("promoIsAllowed", isAllowed);
  result->SetBoolean("promoIsAllowedOnMostVisited", isAllowedOnMostVisited);
  result->SetBoolean("promoIsAllowedOnOpenTabs", isAllowedOnOpenTabs);
  result->SetBoolean("promoIsAllowedAsVC", isAllowedAsVC);
  result->SetString("promoMessage", promo_line);
  result->SetString("promoMessageLong", promo_line_long);
  result->SetString("promoVCTitle", promo_vc_title);
  result->SetString("promoVCLastSynced", promo_vc_lastsync);
  web_ui()->CallJavascriptFunction("setPromotions", *(result.get()));
}

void MobileNtpPromoHandler::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  if (chrome::NOTIFICATION_PROMO_RESOURCE_STATE_CHANGED == type ||
      chrome::NOTIFICATION_SYNC_CONFIGURE_DONE == type ||
      chrome::NOTIFICATION_FOREIGN_SESSION_UPDATED == type) {
    // A change occurred to one of the preferences we care about
    CheckDesktopSessions();
    InjectPromoDecorations(NULL);
  }
}

void MobileNtpPromoHandler::CheckDesktopSessions() {
  Profile* profile = Profile::FromWebUI(web_ui());
  if (!profile)
    return;
  PrefService* prefs = profile->GetPrefs();
  if (!prefs)
    return;
  if (prefs->GetBoolean(prefs::kNtpPromoDesktopSessionFound))
    return;  // already got a desktop session.

  ProfileSyncService* service =
      ProfileSyncServiceFactory::GetInstance()->GetForProfile(profile);
  if (!service || !service->ShouldPushChanges())
    return;
  browser_sync::SessionModelAssociator* associator =
      service->GetSessionModelAssociator();
  // No sessions means no desktop sessions as well, so we are good to go.
  if (!associator)
    return;

  // Let's see if there are no desktop sessions.
  std::vector<const browser_sync::SyncedSession*> sessions;
  ListValue session_list;
  if (!associator->GetAllForeignSessions(&sessions))
    return;

  did_fetch_sessions_ = true;

  for (size_t i = 0; i < sessions.size(); ++i) {
    const browser_sync::SyncedSession::DeviceType device_type =
        sessions[i]->device_type;
    if (device_type == browser_sync::SyncedSession::TYPE_WIN ||
        device_type == browser_sync::SyncedSession::TYPE_MACOSX ||
        device_type == browser_sync::SyncedSession::TYPE_LINUX) {
      // Found a desktop session: write out the pref.
      prefs->SetBoolean(prefs::kNtpPromoDesktopSessionFound, true);
      return;
    }
  }
}