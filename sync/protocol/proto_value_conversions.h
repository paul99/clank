// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Keep this file in sync with the .proto files in this directory.

#ifndef SYNC_PROTOCOL_PROTO_VALUE_CONVERSIONS_H_
#define SYNC_PROTOCOL_PROTO_VALUE_CONVERSIONS_H_

#include "sync/base/sync_export.h"

namespace base {
class DictionaryValue;
}

namespace sync_pb {
class AppNotification;
class AppNotificationSettings;
class AppSettingSpecifics;
class AppSpecifics;
class AutofillProfileSpecifics;
class AutofillSpecifics;
class BookmarkSpecifics;
class ClientToServerMessage;
class ClientToServerResponse;
class DatatypeAssociationStats;
class DebugEventInfo;
class DebugInfo;
class DeviceInfoSpecifics;
class DeviceInformation;
class EncryptedData;
class EntitySpecifics;
class EverythingDirective;
class ExperimentsSpecifics;
class ExtensionSettingSpecifics;
class ExtensionSettingSpecifics;
class ExtensionSpecifics;
class ExtensionSpecifics;
class GlobalIdDirective;
class HistoryDeleteDirectiveSpecifics;
class KeystoreEncryptionFlagsSpecifics;
class NigoriSpecifics;
class PasswordSpecifics;
class PasswordSpecificsData;
class PreferenceSpecifics;
class SearchEngineSpecifics;
class SessionHeader;
class SessionSpecifics;
class SessionTab;
class SessionWindow;
class SyncCycleCompletedEventInfo;
class TabNavigation;
class ThemeSpecifics;
class TimeRangeDirective;
class TypedUrlSpecifics;
}  // namespace sync_pb

// Utility functions to convert sync protocol buffers to dictionaries.
// Each protocol field is mapped to a key of the same name.  Repeated
// fields are mapped to array values and sub-messages are mapped to
// sub-dictionary values.
//
// TODO(akalin): Add has_* information.
//
// TODO(akalin): Improve enum support.

namespace syncer {

// Ownership of all returned DictionaryValues are transferred to the
// caller.

// TODO(akalin): Perhaps extend this to decrypt?
base::DictionaryValue* EncryptedDataToValue(
    const sync_pb::EncryptedData& encrypted_data);

// Sub-protocol of AppSpecifics.
base::DictionaryValue* AppSettingsToValue(
    const sync_pb::AppNotificationSettings& app_notification_settings);

// Sub-protocols of SessionSpecifics.

base::DictionaryValue* SessionHeaderToValue(
    const sync_pb::SessionHeader& session_header);

base::DictionaryValue* SessionTabToValue(
    const sync_pb::SessionTab& session_tab);

base::DictionaryValue* SessionWindowToValue(
    const sync_pb::SessionWindow& session_window);

base::DictionaryValue* TabNavigationToValue(
    const sync_pb::TabNavigation& tab_navigation);

// Sub-protocol of PasswordSpecifics.

base::DictionaryValue* PasswordSpecificsDataToValue(
    const sync_pb::PasswordSpecificsData& password_specifics_data);

// Sub-protocol of NigoriSpecifics.

base::DictionaryValue* DeviceInformationToValue(
    const sync_pb::DeviceInformation& device_information);

// Sub-protocol of HistoryDeleteDirectiveSpecifics.

base::DictionaryValue* GlobalIdDirectiveToValue(
    const sync_pb::GlobalIdDirective& global_id_directive);

base::DictionaryValue* TimeRangeDirectiveToValue(
    const sync_pb::TimeRangeDirective& time_range_directive);

// Sub-protocol of Experiments.

base::DictionaryValue* KeystoreEncryptionToValue(
    const sync_pb::KeystoreEncryptionFlagsSpecifics& proto);

// Main *SpecificsToValue functions.

base::DictionaryValue* AppNotificationToValue(
    const sync_pb::AppNotification& app_notification_specifics);

base::DictionaryValue* AppSettingSpecificsToValue(
    const sync_pb::AppSettingSpecifics& app_setting_specifics);

base::DictionaryValue* AppSpecificsToValue(
    const sync_pb::AppSpecifics& app_specifics);

base::DictionaryValue* AutofillSpecificsToValue(
    const sync_pb::AutofillSpecifics& autofill_specifics);

base::DictionaryValue* AutofillProfileSpecificsToValue(
    const sync_pb::AutofillProfileSpecifics& autofill_profile_specifics);

base::DictionaryValue* BookmarkSpecificsToValue(
    const sync_pb::BookmarkSpecifics& bookmark_specifics);

base::DictionaryValue* DeviceInfoSpecificsToValue(
    const sync_pb::DeviceInfoSpecifics& device_info_specifics);

base::DictionaryValue* ExperimentsSpecificsToValue(
    const sync_pb::ExperimentsSpecifics& proto);

base::DictionaryValue* ExtensionSettingSpecificsToValue(
    const sync_pb::ExtensionSettingSpecifics& extension_setting_specifics);

base::DictionaryValue* ExtensionSpecificsToValue(
    const sync_pb::ExtensionSpecifics& extension_specifics);

SYNC_EXPORT base::DictionaryValue* HistoryDeleteDirectiveSpecificsToValue(
    const sync_pb::HistoryDeleteDirectiveSpecifics&
        history_delete_directive_specifics);

base::DictionaryValue* NigoriSpecificsToValue(
    const sync_pb::NigoriSpecifics& nigori_specifics);

base::DictionaryValue* PasswordSpecificsToValue(
    const sync_pb::PasswordSpecifics& password_specifics);

base::DictionaryValue* PreferenceSpecificsToValue(
    const sync_pb::PreferenceSpecifics& password_specifics);

base::DictionaryValue* SearchEngineSpecificsToValue(
    const sync_pb::SearchEngineSpecifics& search_engine_specifics);

base::DictionaryValue* SessionSpecificsToValue(
    const sync_pb::SessionSpecifics& session_specifics);

base::DictionaryValue* ThemeSpecificsToValue(
    const sync_pb::ThemeSpecifics& theme_specifics);

base::DictionaryValue* TypedUrlSpecificsToValue(
    const sync_pb::TypedUrlSpecifics& typed_url_specifics);

// Any present extensions are mapped to sub-dictionary values with the
// key equal to the extension name.
base::DictionaryValue* EntitySpecificsToValue(
    const sync_pb::EntitySpecifics& specifics);

base::DictionaryValue* ClientToServerMessageToValue(
    const sync_pb::ClientToServerMessage& proto,
    bool include_specifics);

base::DictionaryValue* ClientToServerResponseToValue(
    const sync_pb::ClientToServerResponse& proto,
    bool include_specifics);

base::DictionaryValue* DatatypeAssociationStatsToValue(
    const sync_pb::DatatypeAssociationStats& proto);

base::DictionaryValue* DebugEventInfoToValue(
    const sync_pb::DebugEventInfo& proto);

base::DictionaryValue* DebugInfoToValue(
    const sync_pb::DebugInfo& proto);

base::DictionaryValue* SyncCycleCompletedEventInfoToValue(
    const sync_pb::SyncCycleCompletedEventInfo& proto);

}  // namespace syncer

#endif  // SYNC_PROTOCOL_PROTO_VALUE_CONVERSIONS_H_
