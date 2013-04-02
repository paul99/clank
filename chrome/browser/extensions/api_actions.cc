// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include "base/logging.h"
#include "base/stringprintf.h"
#include "chrome/browser/extensions/api_actions.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace extensions {

const char* APIAction::kTableName = "activitylog_apis";
const char* APIAction::kTableStructure = "("
    "extension_id LONGVARCHAR NOT NULL, "
    "time INTEGER NOT NULL, "
    "api_type LONGVARCHAR NOT NULL, "
    "api_action_type LONGVARCHAR NOT NULL, "
    "target_type LONGVARCHAR NOT NULL, "
    "api_call LONGVARCHAR NOT NULL, "
    "extra LONGVARCHAR NOT NULL)";

APIAction::APIAction(const std::string& extension_id,
                     const base::Time& time,
                     const Type type,
                     const Verb verb,
                     const Target target,
                     const std::string& api_call,
                     const std::string& extra)
    : extension_id_(extension_id),
      time_(time),
      type_(type),
      verb_(verb),
      target_(target),
      api_call_(api_call),
      extra_(extra) { }

APIAction::~APIAction() {
}

// static
bool APIAction::InitializeTable(sql::Connection* db) {
  if (!db->DoesTableExist(kTableName)) {
    std::string table_creator = base::StringPrintf(
        "CREATE TABLE %s %s", kTableName, kTableStructure);
    if (!db->Execute(table_creator.c_str()))
      return false;
  } else if (!db->DoesColumnExist(kTableName, "api_type")) {
    // Old versions of the table lack the api_type column.  Add it if
    // needed, with values defaulting to "CALL".
    //
    // TODO(mvrable): Remove this update code once we're fairly certain that
    // everyone will have converted to the new schema.
    std::string table_updater = base::StringPrintf(
        "ALTER TABLE %s ADD COLUMN api_type LONGVARCHAR; "
        "UPDATE %s SET api_type = 'CALL'",
        kTableName, kTableName);
    if (!db->Execute(table_updater.c_str()))
      return false;
  }
  return true;
}

void APIAction::Record(sql::Connection* db) {
  std::string sql_str = "INSERT INTO " + std::string(kTableName)
      + " (extension_id, time, api_type, api_action_type, target_type,"
      " api_call, extra) VALUES (?,?,?,?,?,?,?)";
  sql::Statement statement(db->GetCachedStatement(
      sql::StatementID(SQL_FROM_HERE), sql_str.c_str()));
  statement.BindString(0, extension_id_);
  statement.BindInt64(1, time_.ToInternalValue());
  statement.BindString(2, TypeAsString());
  statement.BindString(3, VerbAsString());
  statement.BindString(4, TargetAsString());
  statement.BindString(5, api_call_);
  statement.BindString(6, extra_);

  if (!statement.Run())
    LOG(ERROR) << "Activity log database I/O failed: " << sql_str;
}

std::string APIAction::PrettyPrintFori18n() {
  // TODO(felt): implement this for real when the UI is redesigned.
  return PrettyPrintForDebug();
}

std::string APIAction::PrettyPrintForDebug() {
  // TODO(felt): implement this for real when the UI is redesigned.
  return "ID: " + extension_id_ + + ", CATEGORY: " + TypeAsString() +
      ", VERB: " + VerbAsString() + ", TARGET: " + TargetAsString() +
      ", API: " + api_call_;
}

std::string APIAction::TypeAsString() const {
  switch (type_) {
    case CALL:
      return "CALL";
    case EVENT_CALLBACK:
      return "EVENT_CALLBACK";
    default:
      return "UNKNOWN_TYPE";
  }
}

std::string APIAction::VerbAsString() const {
  switch (verb_) {
    case READ:
      return "READ";
    case MODIFIED:
      return "MODIFIED";
    case DELETED:
      return "DELETED";
    case ADDED:
      return "ADDED";
    case ENABLED:
      return "ENABLED";
    case DISABLED:
      return "DISABLED";
    case CREATED:
      return "CREATED";
    default:
      return "UNKNOWN_VERB";
  }
}

std::string APIAction::TargetAsString() const {
  switch (target_) {
    case BOOKMARK:
      return "BOOKMARK";
    case TABS:
      return "TABS";
    case HISTORY:
      return "HISTORY";
    case COOKIES:
      return "COOKIES";
    case BROWSER_ACTION:
      return "BROWSER_ACTION";
    case NOTIFICATION:
      return "NOTIFICATION";
    case OMNIBOX:
      return "OMNIBOX";
    default:
      return "UNKNOWN_TARGET";
  }
}

APIAction::Type APIAction::StringAsType(
    const std::string& str) {
  if (str == "CALL") {
    return CALL;
  } else if (str == "EVENT_CALLBACK") {
    return EVENT_CALLBACK;
  } else {
    return UNKNOWN_TYPE;
  }
}

APIAction::Verb APIAction::StringAsVerb(
    const std::string& str) {
  if (str == "READ") {
    return READ;
  } else if (str == "MODIFIED") {
    return MODIFIED;
  } else if (str == "DELETED") {
    return DELETED;
  } else if (str == "ADDED") {
    return ADDED;
  } else if (str == "ENABLED") {
    return ENABLED;
  } else if (str == "DISABLED") {
    return DISABLED;
  } else if (str == "CREATED") {
    return CREATED;
  } else {
    return UNKNOWN_VERB;
  }
}

// The all-caps strings match the enum names.  The lowercase strings match the
// actual object names (e.g., cookies.remove(...);).
APIAction::Target APIAction::StringAsTarget(
    const std::string& str) {
  if (str == "BOOKMARK" || str == "bookmark") {
    return BOOKMARK;
  } else if (str == "TABS" || str == "tabs") {
    return TABS;
  } else if (str == "HISTORY" || str == "history") {
    return HISTORY;
  } else if (str == "COOKIES" || str == "cookies") {
    return COOKIES;
  } else if (str == "BROWSER_ACTION" || str == "browser_action") {
    return BROWSER_ACTION;
  } else if (str == "NOTIFICATION" || str == "notification") {
    return NOTIFICATION;
  } else if (str == "OMNIBOX" || str == "omnibox") {
    return OMNIBOX;
  } else {
    return UNKNOWN_TARGET;
  }
}

}  // namespace extensions
