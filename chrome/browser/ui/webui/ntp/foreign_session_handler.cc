// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/ntp/foreign_session_handler.h"

#include <algorithm>
#include <string>
#include <vector>
#include "base/base64.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/memory/scoped_vector.h"
#include "base/string_number_conversions.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_restore.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/time_format.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/web_ui.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace browser_sync {

// Maximum number of session we're going to display on the NTP
static const int kMaxSessionsToShow = 10;

// Invalid value, used to note that we don't have a tab or window number.
static const int kInvalidId = -1;

ForeignSessionHandler::ForeignSessionHandler() {
}

void ForeignSessionHandler::RegisterMessages() {
  Init();
  web_ui()->RegisterMessageCallback("getForeignSessions",
      base::Bind(&ForeignSessionHandler::HandleGetForeignSessions,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback("openForeignSession",
      base::Bind(&ForeignSessionHandler::HandleOpenForeignSession,
                 base::Unretained(this)));
#if defined(OS_ANDROID)
  web_ui()->RegisterMessageCallback("deleteForeignSession",
        base::Bind(&ForeignSessionHandler::HandleDeleteForeignSession,
                 base::Unretained(this)));
#endif
}

void ForeignSessionHandler::Init() {
  Profile* profile = Profile::FromWebUI(web_ui());
  registrar_.Add(this, chrome::NOTIFICATION_SYNC_CONFIGURE_DONE,
                 content::Source<Profile>(profile));
  registrar_.Add(this, chrome::NOTIFICATION_FOREIGN_SESSION_UPDATED,
                 content::Source<Profile>(profile));
  registrar_.Add(this, chrome::NOTIFICATION_FOREIGN_SESSION_DISABLED,
                 content::Source<Profile>(profile));
  registrar_.Add(this, chrome::NOTIFICATION_SESSION_ASSOCIATION_CYCLE_COMPLETE,
                 content::Source<Profile>(profile));
}

void ForeignSessionHandler::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  ListValue list_value;
  switch (type) {
    case chrome::NOTIFICATION_SESSION_ASSOCIATION_CYCLE_COMPLETE:
    case chrome::NOTIFICATION_SYNC_CONFIGURE_DONE:
    case chrome::NOTIFICATION_FOREIGN_SESSION_UPDATED:
      HandleGetForeignSessions(&list_value);
      break;
    case chrome::NOTIFICATION_FOREIGN_SESSION_DISABLED:
      // Calling foreignSessions with empty list will automatically hide
      // foreign session section.
      web_ui()->CallJavascriptFunction("foreignSessions", list_value);
      break;
    default:
      NOTREACHED();
  }
}

SessionModelAssociator* ForeignSessionHandler::GetModelAssociator() {
  ProfileSyncService* service(ProfileSyncServiceFactory::
      GetInstance()->GetForProfile(Profile::FromWebUI(web_ui())));
  if (service == NULL)
    return NULL;

  // We only want to set the model associator if there is one, and it is done
  // syncing sessions.
  SessionModelAssociator* model_associator =
      service->GetSessionModelAssociator();
  if (model_associator == NULL ||
      !service->ShouldPushChanges()) {
    return NULL;
  }
  return model_associator;
}

#if defined(OS_ANDROID)
void ForeignSessionHandler::HandleDeleteForeignSession(
    const ListValue* args) {
  SessionModelAssociator* associator = GetModelAssociator();
  size_t num_args = args->GetSize();
  for (size_t i = 0; i < num_args; i++) {
    std::string session_string_value;
    if (args->GetString(i, &session_string_value)) {
      associator->DeleteForeignSession(session_string_value);
    } else {
      LOG(ERROR) << "Failed to extract session tag.";
    }
  }
}
#endif

void ForeignSessionHandler::HandleGetForeignSessions(const ListValue* args) {
  SessionModelAssociator* associator = GetModelAssociator();
  std::vector<const SyncedSession*> sessions;
  ListValue session_list;

  // Note: we don't own the SyncedSessions themselves.
  if (associator == NULL) {
    LOG(ERROR) << "ForeignSessionHandler failed to get session data from"
        "SessionModelAssociator as the associator could not be retrieved.";
  } else if (associator->GetAllForeignSessions(&sessions)) {
    int added_count = 0;
    for (std::vector<const SyncedSession*>::const_iterator i =
        sessions.begin(); i != sessions.end() &&
        added_count < kMaxSessionsToShow; ++i) {
      const SyncedSession* foreign_session = *i;
      scoped_ptr<ListValue> window_list(new ListValue());
      for (SyncedSession::SyncedWindowMap::const_iterator it =
          foreign_session->windows.begin(); it != foreign_session->windows.end();
          ++it) {
        SessionWindow* window = it->second;
        scoped_ptr<DictionaryValue> window_data(new DictionaryValue());
        if (SessionWindowToValue(associator, *window, window_data.get())) {
          window_data->SetString("sessionTag", foreign_session->session_tag);
          if (!foreign_session->session_name.empty()) {
            window_data->SetString("sessionName", foreign_session->session_name);
          }
          window_data->SetString("deviceType", foreign_session->DeviceTypeAsString());

          // Give ownership to |list_value|.
          window_list->Append(window_data.release());
        }
      }
      added_count++;

      // Give ownership to |session_list|.
      session_list.Append(window_list.release());
    }
  } else {
    LOG(ERROR) << "ForeignSessionHandler failed to get session data from"
        "SessionModelAssociator. No data available.";
  }
  web_ui()->CallJavascriptFunction("foreignSessions", session_list);
}

void ForeignSessionHandler::HandleOpenForeignSession(
    const ListValue* args) {
  size_t num_args = args->GetSize();
  if (num_args > 3U || num_args == 0) {
    LOG(ERROR) << "openForeignWindow called with only " << args->GetSize()
               << " arguments.";
    return;
  }

  // Extract the machine tag (always provided).
  std::string session_string_value;
  if (!args->GetString(0, &session_string_value)) {
    LOG(ERROR) << "Failed to extract session tag.";
    return;
  }

  // Extract window number.
  std::string window_num_str;
  int window_num = kInvalidId;
  if (num_args >= 2 && (!args->GetString(1, &window_num_str) ||
      !base::StringToInt(window_num_str, &window_num))) {
    LOG(ERROR) << "Failed to extract window number.";
    return;
  }

  // Extract tab id.
  std::string tab_id_str;
  SessionID::id_type tab_id = kInvalidId;
  if (num_args == 3 && (!args->GetString(2, &tab_id_str) ||
      !base::StringToInt(tab_id_str, &tab_id))) {
    LOG(ERROR) << "Failed to extract tab SessionID.";
    return;
  }

  SessionModelAssociator* associator = GetModelAssociator();

  Profile* profile = Profile::FromWebUI(web_ui());
  if (tab_id != kInvalidId) {
    // We don't actually care about |window_num|, this is just a sanity check.
    DCHECK_LT(kInvalidId, window_num);
    const SessionTab* tab;
    if (!associator->GetForeignTab(session_string_value, tab_id, &tab)) {
      LOG(ERROR) << "Failed to load foreign tab.";
      return;
    }
    SessionRestore::RestoreForeignSessionTab(profile, *tab);
  } else {
    std::vector<const SessionWindow*> windows;
    // Note: we don't own the ForeignSessions themselves.
    if (!associator->GetForeignSession(session_string_value, &windows)) {
      LOG(ERROR) << "ForeignSessionHandler failed to get session data from"
          "SessionModelAssociator.";
      return;
    }
    std::vector<const SessionWindow*>::const_iterator iter_begin =
        windows.begin() + ((window_num == kInvalidId) ? 0 : window_num);
    std::vector<const SessionWindow*>::const_iterator iter_end =
        ((window_num == kInvalidId) ?
        std::vector<const SessionWindow*>::const_iterator(windows.end()) :
        iter_begin + 1);
    SessionRestore::RestoreForeignSessionWindows(profile, iter_begin, iter_end);
  }
}

bool ForeignSessionHandler::SessionTabToValue(
    SessionModelAssociator* associator,
    const SessionTab& tab,
    DictionaryValue* dictionary) {
  if (tab.navigations.empty())
    return false;
  int selected_index = tab.current_navigation_index;
  selected_index = std::max(
      0,
      std::min(selected_index,
               static_cast<int>(tab.navigations.size() - 1)));
  const TabNavigation& current_navigation =
      tab.navigations.at(selected_index);
  GURL tab_url = current_navigation.virtual_url();
  if (tab_url == GURL(chrome::kChromeUINewTabURL))
    return false;
  NewTabUI::SetURLTitleAndDirection(dictionary, current_navigation.title(),
                                    tab_url);
  dictionary->SetString("type", "tab");
  dictionary->SetDouble("timestamp",
                        static_cast<double>(tab.timestamp.ToInternalValue()));
  dictionary->SetInteger("sessionId", tab.tab_id.id());

  // Include favicon data as a base64-encode PNG, if it has been synced.
  std::string favicon_data;
  if (associator->GetSyncedFaviconForPageURL(tab_url.spec(), &favicon_data)) {
    std::string encoded_favicon_data;
    if (base::Base64Encode(favicon_data, &encoded_favicon_data))
      dictionary->SetString("favicon_base64_data", encoded_favicon_data);
    else
     NOTREACHED() << "Failed to base64 encode favicon.";
  }
  return true;
}

bool ForeignSessionHandler::SessionWindowToValue(
    SessionModelAssociator* associator,
    const SessionWindow& window,
    DictionaryValue* dictionary) {
  if (window.tabs.empty()) {
    NOTREACHED();
    return false;
  }
  scoped_ptr<ListValue> tab_values(new ListValue());
  // Calculate the last mtime for all entries within a window.
  base::Time mtime = window.timestamp;
  for (size_t i = 0; i < window.tabs.size(); ++i) {
    scoped_ptr<DictionaryValue> tab_value(new DictionaryValue());
    if (SessionTabToValue(associator, *window.tabs[i], tab_value.get())) {
      mtime = std::max(mtime, window.tabs[i]->timestamp);
      tab_values->Append(tab_value.release());
    }
  }
  if (tab_values->GetSize() == 0)
    return false;
  dictionary->SetString("type", "window");
  dictionary->SetDouble("timestamp", mtime.ToInternalValue());
  const base::TimeDelta last_synced = base::Time::Now() - mtime;
  // If clock skew leads to a future time, or we last synced less than a minute
  // ago, output "Just now"
  if (last_synced.ToInternalValue() < 0 ||
      last_synced < base::TimeDelta::FromMinutes(1)) {
    dictionary->SetString("userVisibleTimestamp",
                          l10n_util::GetStringUTF16(IDS_SYNC_TIME_JUST_NOW));
  } else {
    dictionary->SetString("userVisibleTimestamp",
                          TimeFormat::TimeElapsed(last_synced));
  }
  dictionary->SetInteger("sessionId", window.window_id.id());
  dictionary->Set("tabs", tab_values.release());
  return true;
}

}  // namespace browser_sync
