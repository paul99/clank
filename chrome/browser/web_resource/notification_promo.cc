// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_resource/notification_promo.h"

#include <cmath>
#include <vector>

#include "base/bind.h"
#include "base/rand_util.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/time.h"
#include "base/values.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile_impl.h"
#include "chrome/browser/net/browser_url_util.h"
#include "chrome/browser/web_resource/promo_resource_service.h"
#include "chrome/common/chrome_version_info.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/user_metrics.h"
#include "googleurl/src/gurl.h"

#if defined(OS_ANDROID)
#include "base/command_line.h"
#include "chrome/common/chrome_switches.h"
#endif  // defined(OS_ANDROID)

using content::UserMetricsAction;

namespace {

const int kDefaultGroupSize = 100;

const char promo_server_url[] = "https://clients3.google.com/crsignal/client";

const char kPrefPromoObject[] = "promo";
const char kPrefPromoText[] = "text";
#if defined(OS_ANDROID)
const char kPrefPromoPayload[] = "payload";
#endif
const char kPrefPromoStart[] = "start";
const char kPrefPromoEnd[] = "end";
const char kPrefPromoNumGroups[] = "num_groups";
const char kPrefPromoSegment[] = "segment";
const char kPrefPromoIncrement[] = "increment";
const char kPrefPromoIncrementFrequency[] = "increment_frequency";
const char kPrefPromoIncrementMax[] = "increment_max";
const char kPrefPromoMaxViews[] = "max_views";
const char kPrefPromoGroup[] = "group";
const char kPrefPromoViews[] = "views";
const char kPrefPromoClosed[] = "closed";
const char kPrefPromoGPlusRequired[] = "gplus_required";

#if defined(OS_ANDROID)
const int kCurrentMobilePayloadFormatVersion = 3;
#endif  // defined(OS_ANDROID)

// Returns a string suitable for the Promo Server URL 'osname' value.
std::string PlatformString() {
#if defined(OS_WIN)
  return "win";
#elif defined(OS_IOS)
  // TODO(noyau): add iOS-specific implementation
  const bool isTablet = false;
  return std::string("ios-") + (isTablet ? "tablet" : "phone");
#elif defined(OS_MACOSX)
  return "mac";
#elif defined(OS_CHROMEOS)
  return "chromeos";
#elif defined(OS_LINUX)
  return "linux";
#elif defined(OS_ANDROID)
  const bool isTablet =
      CommandLine::ForCurrentProcess()->HasSwitch(switches::kTabletUi);
  return std::string("android-") + (isTablet ? "tablet" : "phone");
#else
  return "none";
#endif
}

// Returns a string suitable for the Promo Server URL 'dist' value.
const char* ChannelString() {
#if defined (OS_WIN)
  // GetChannel hits the registry on Windows. See http://crbug.com/70898.
  // TODO(achuith): Move NotificationPromo::PromoServerURL to the blocking pool.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
#endif
  const chrome::VersionInfo::Channel channel =
      chrome::VersionInfo::GetChannel();
  switch (channel) {
    case chrome::VersionInfo::CHANNEL_CANARY:
      return "canary";
    case chrome::VersionInfo::CHANNEL_DEV:
      return "dev";
    case chrome::VersionInfo::CHANNEL_BETA:
      return "beta";
    case chrome::VersionInfo::CHANNEL_STABLE:
      return "stable";
    default:
#if defined(OS_ANDROID) && defined(OFFICIAL_BUILD)
      return "stable";
#endif
      return "none";
  }
}

// TODO(achuith): remove this in m23.
void ClearDeprecatedPrefs(PrefService* prefs) {
  prefs->RegisterStringPref(prefs::kNtpPromoLine,
                            std::string(),
                            PrefService::UNSYNCABLE_PREF);
  prefs->ClearPref(prefs::kNtpPromoLine);
  prefs->RegisterDoublePref(prefs::kNtpPromoStart,
                            0,
                            PrefService::UNSYNCABLE_PREF);
  prefs->RegisterDoublePref(prefs::kNtpPromoEnd,
                            0,
                            PrefService::UNSYNCABLE_PREF);

  prefs->RegisterIntegerPref(prefs::kNtpPromoNumGroups,
                             0,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kNtpPromoInitialSegment,
                             0,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kNtpPromoIncrement,
                             1,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kNtpPromoGroupTimeSlice,
                             0,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kNtpPromoGroupMax,
                             0,
                             PrefService::UNSYNCABLE_PREF);

  prefs->RegisterIntegerPref(prefs::kNtpPromoViewsMax,
                             0,
                             PrefService::UNSYNCABLE_PREF);

  prefs->RegisterIntegerPref(prefs::kNtpPromoGroup,
                             0,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterIntegerPref(prefs::kNtpPromoViews,
                             0,
                             PrefService::UNSYNCABLE_PREF);
  prefs->RegisterBooleanPref(prefs::kNtpPromoClosed,
                             false,
                             PrefService::UNSYNCABLE_PREF);

  prefs->RegisterBooleanPref(prefs::kNtpPromoGplusRequired,
                             false,
                             PrefService::UNSYNCABLE_PREF);

  prefs->ClearPref(prefs::kNtpPromoStart);
  prefs->ClearPref(prefs::kNtpPromoEnd);
  prefs->ClearPref(prefs::kNtpPromoNumGroups);
  prefs->ClearPref(prefs::kNtpPromoInitialSegment);
  prefs->ClearPref(prefs::kNtpPromoIncrement);
  prefs->ClearPref(prefs::kNtpPromoGroupTimeSlice);
  prefs->ClearPref(prefs::kNtpPromoGroupMax);
  prefs->ClearPref(prefs::kNtpPromoViewsMax);
  prefs->ClearPref(prefs::kNtpPromoGroup);
  prefs->ClearPref(prefs::kNtpPromoViews);
  prefs->ClearPref(prefs::kNtpPromoClosed);
  prefs->ClearPref(prefs::kNtpPromoGplusRequired);
}

// Deep-copies a node, replacing any "value" that is a key
// into "strings" dictionary with its value from "strings".
// E.g. for
//   {promo_action_args:['MSG_SHORT']} + strings:{MSG_SHORT:'yes'}
// it will return
//   {promo_action_args:['yes']}
base::Value* deepCopyAndResolveStrings(
    base::Value* node,
    base::DictionaryValue* strings) {
  DCHECK(node);
  switch (node->GetType()) {
    case base::Value::TYPE_LIST: {
      base::ListValue* list = static_cast<base::ListValue*>(node);
      base::ListValue* copy = new base::ListValue;
      for (base::ListValue::const_iterator it = list->begin();
           it != list->end();
           ++it) {
        base::Value* child_copy = deepCopyAndResolveStrings(*it, strings);
        copy->Append(child_copy);
      }
      return copy;
    }

    case Value::TYPE_DICTIONARY: {
      base::DictionaryValue* dict = static_cast<base::DictionaryValue*>(node);
      base::DictionaryValue* copy = new base::DictionaryValue;
      for (base::DictionaryValue::key_iterator it = dict->begin_keys();
           it != dict->end_keys(); ++it) {
        base::Value* child = NULL;
        bool rv = dict->GetWithoutPathExpansion(*it, &child);
        DCHECK(rv);
        base::Value* child_copy = deepCopyAndResolveStrings(child, strings);
        copy->SetWithoutPathExpansion(*it, child_copy);
      }
      return copy;
    }

    case Value::TYPE_STRING: {
      base::StringValue* str = static_cast<base::StringValue*>(node);
      std::string value;
      bool rv = str->GetAsString(&value);
      DCHECK(rv);
      std::string actual_key;
      std::string actual_value;
      if (!strings->GetString(value, &actual_value)) {
        actual_value = value;
      }
      base::StringValue* copy = new base::StringValue(actual_value);
      return copy;
    }

    default:
      // For everything else, just make a copy.
      return node->DeepCopy();
  }
}

}  // namespace

const char NotificationPromo::kNtpNotificationPromoType[] =
#if !defined(OS_ANDROID)
    "ntp_notification_promo";
#else
    "mobile_ntp_sync_promo";
#endif  // !defined(OS_ANDROID)
const char NotificationPromo::kBubblePromoType[] = "bubble_promo";

NotificationPromo::NotificationPromo(Profile* profile)
    : profile_(profile),
      prefs_(profile_->GetPrefs()),
      promo_type_(kNtpNotificationPromoType),
#if defined(OS_ANDROID)
      promo_payload_(new base::DictionaryValue),
#endif  // defined(OS_ANDROID)
      start_(0.0),
      end_(0.0),
      num_groups_(kDefaultGroupSize),
      initial_segment_(0),
      increment_(1),
      time_slice_(0),
      max_group_(0),
      max_views_(0),
      group_(0),
      views_(0),
      closed_(false),
      gplus_required_(false),
      new_notification_(false) {
  DCHECK(profile);
  DCHECK(prefs_);
}

NotificationPromo::~NotificationPromo() {}

void NotificationPromo::InitFromJson(const DictionaryValue& json) {
  ListValue* promo_list = NULL;
  if (!json.GetList(promo_type_, &promo_list))
    return;

  // No support for multiple promos yet. Only consider the first one.
  DictionaryValue* promo = NULL;
  if (!promo_list->GetDictionary(0, &promo))
    return;

  // Strings. Assume the first one is the promo text.
  DictionaryValue* strings = NULL;
  if (promo->GetDictionary("strings", &strings)) {
#if !defined(OS_ANDROID)
    DictionaryValue::Iterator iter(*strings);
    iter.value().GetAsString(&promo_text_);
    DVLOG(1) << "promo_text_=" << promo_text_;
#endif  // defined(OS_ANDROID)
  }

  // Date.
  ListValue* date_list = NULL;
  if (promo->GetList("date", &date_list)) {
    DictionaryValue* date;
    if (date_list->GetDictionary(0, &date)) {
      std::string time_str;
      base::Time time;
      if (date->GetString("start", &time_str) &&
          base::Time::FromString(time_str.c_str(), &time)) {
        start_ = time.ToDoubleT();
        DVLOG(1) << "start str=" << time_str
                 << ", start_="<< base::DoubleToString(start_);
      }
      if (date->GetString("end", &time_str) &&
          base::Time::FromString(time_str.c_str(), &time)) {
        end_ = time.ToDoubleT();
        DVLOG(1) << "end str =" << time_str
                 << ", end_=" << base::DoubleToString(end_);
      }
    }
  }

  // Grouping.
  DictionaryValue* grouping = NULL;
  if (promo->GetDictionary("grouping", &grouping)) {
    grouping->GetInteger("buckets", &num_groups_);
    grouping->GetInteger("segment", &initial_segment_);
    grouping->GetInteger("increment", &increment_);
    grouping->GetInteger("increment_frequency", &time_slice_);
    grouping->GetInteger("increment_max", &max_group_);

    DVLOG(1) << "num_groups_ = " << num_groups_
             << ", initial_segment_ = " << initial_segment_
             << ", increment_ = " << increment_
             << ", time_slice_ = " << time_slice_
             << ", max_group_ = " << max_group_;
  }

  // Payload.
  DictionaryValue* payload = NULL;
  if (promo->GetDictionary("payload", &payload)) {
    payload->GetBoolean("gplus_required", &gplus_required_);

    DVLOG(1) << "gplus_required_ = " << gplus_required_;
  }

  promo->GetInteger("max_views", &max_views_);
  DVLOG(1) << "max_views_ " << max_views_;

#if defined(OS_ANDROID)
  int payload_version = 0;
  if (!payload) {
    LOG(ERROR) << "Malformed JSON: no payload";
    return;
  }
  if (!strings) {
    LOG(ERROR) << "Malformed JSON: no strings";
    return;
  }
  if (!payload->GetInteger("payload_format_version", &payload_version) ||
      payload_version != kCurrentMobilePayloadFormatVersion) {
    LOG(ERROR) << "Unsupported promo payload_format_version " << payload_version
               << "; expected " << kCurrentMobilePayloadFormatVersion;
    return;
  }

  base::Value* ppcopy = deepCopyAndResolveStrings(payload, strings);
  DCHECK(ppcopy != NULL);
  DCHECK(ppcopy->IsType(base::Value::TYPE_DICTIONARY));
  promo_payload_.reset(static_cast<base::DictionaryValue*>(ppcopy));
  promo_payload_->GetString("promo_message_short", &promo_text_);
#endif  // defined(OS_ANDROID)

  CheckForNewNotification();
}

void NotificationPromo::CheckForNewNotification() {
  NotificationPromo old_promo(profile_);
  old_promo.InitFromPrefs();
  const double old_start = old_promo.start_;
  const double old_end = old_promo.end_;
  const std::string old_promo_text = old_promo.promo_text_;

  new_notification_ =
      old_start != start_ || old_end != end_ || old_promo_text != promo_text_;
  if (new_notification_)
    OnNewNotification();
}

void NotificationPromo::OnNewNotification() {
  // Create a new promo group.
  group_ = base::RandInt(0, num_groups_ - 1);
  WritePrefs();
}

// static
void NotificationPromo::RegisterUserPrefs(PrefService* prefs) {
  ClearDeprecatedPrefs(prefs);
  prefs->RegisterDictionaryPref("promo",
                                new base::DictionaryValue,
                                PrefService::UNSYNCABLE_PREF);
}

void NotificationPromo::WritePrefs() {
  DVLOG(1) << "WritePrefs";
  base::DictionaryValue* ntp_promo = new base::DictionaryValue;
  ntp_promo->SetString(kPrefPromoText, promo_text_);
#if defined(OS_ANDROID)
  ntp_promo->Set(kPrefPromoPayload, promo_payload_->DeepCopy());
#endif  // defined(OS_ANDROID)
  ntp_promo->SetDouble(kPrefPromoStart, start_);
  ntp_promo->SetDouble(kPrefPromoEnd, end_);

  ntp_promo->SetInteger(kPrefPromoNumGroups, num_groups_);
  ntp_promo->SetInteger(kPrefPromoSegment, initial_segment_);
  ntp_promo->SetInteger(kPrefPromoIncrement, increment_);
  ntp_promo->SetInteger(kPrefPromoIncrementFrequency, time_slice_);
  ntp_promo->SetInteger(kPrefPromoIncrementMax, max_group_);

  ntp_promo->SetInteger(kPrefPromoMaxViews, max_views_);

  ntp_promo->SetInteger(kPrefPromoGroup, group_);
  ntp_promo->SetInteger(kPrefPromoViews, views_);
  ntp_promo->SetBoolean(kPrefPromoClosed, closed_);

  ntp_promo->SetBoolean(kPrefPromoGPlusRequired, gplus_required_);

  base::ListValue* promo_list = new base::ListValue;
  promo_list->Set(0, ntp_promo);  // Only support 1 promo for now.

  base::DictionaryValue promo_dict;
  promo_dict.Set(promo_type_, promo_list);
  prefs_->Set(kPrefPromoObject, promo_dict);
}

void NotificationPromo::InitFromPrefs() {
  const base::DictionaryValue* promo_dict =
      prefs_->GetDictionary(kPrefPromoObject);
  if (!promo_dict)
    return;

  base::ListValue* promo_list(NULL);
  promo_dict->GetList(promo_type_, &promo_list);
  if (!promo_list)
    return;

  base::DictionaryValue* ntp_promo(NULL);
  promo_list->GetDictionary(0, &ntp_promo);
  if (!ntp_promo)
    return;

  ntp_promo->GetString(kPrefPromoText, &promo_text_);
#if defined(OS_ANDROID)
  base::DictionaryValue* dv(NULL);
  ntp_promo->GetDictionary(kPrefPromoPayload, &dv);
  DCHECK(dv != NULL);
  promo_payload_.reset(dv->DeepCopy());
#endif  // defined(OS_ANDROID)

  ntp_promo->GetDouble(kPrefPromoStart, &start_);
  ntp_promo->GetDouble(kPrefPromoEnd, &end_);

  ntp_promo->GetInteger(kPrefPromoNumGroups, &num_groups_);
  ntp_promo->GetInteger(kPrefPromoSegment, &initial_segment_);
  ntp_promo->GetInteger(kPrefPromoIncrement, &increment_);
  ntp_promo->GetInteger(kPrefPromoIncrementFrequency, &time_slice_);
  ntp_promo->GetInteger(kPrefPromoIncrementMax, &max_group_);

  ntp_promo->GetInteger(kPrefPromoMaxViews, &max_views_);

  ntp_promo->GetInteger(kPrefPromoGroup, &group_);
  ntp_promo->GetInteger(kPrefPromoViews, &views_);
  ntp_promo->GetBoolean(kPrefPromoClosed, &closed_);

  ntp_promo->GetBoolean(kPrefPromoGPlusRequired, &gplus_required_);
}

bool NotificationPromo::CanShow() const {
  return !closed_ &&
         !promo_text_.empty() &&
         !ExceedsMaxGroup() &&
         !ExceedsMaxViews() &&
         base::Time::FromDoubleT(StartTimeForGroup()) < base::Time::Now() &&
         base::Time::FromDoubleT(EndTime()) > base::Time::Now() &&
         IsGPlusRequired();
}

void NotificationPromo::HandleClosed() {
  content::RecordAction(UserMetricsAction("NTPPromoClosed"));
  InitFromPrefs();
  if (!closed_) {
    closed_ = true;
    WritePrefs();
  }
}

bool NotificationPromo::HandleViewed() {
  content::RecordAction(UserMetricsAction("NTPPromoShown"));
  InitFromPrefs();
  ++views_;
  WritePrefs();
  return ExceedsMaxViews();
}

bool NotificationPromo::ExceedsMaxGroup() const {
  return (max_group_ == 0) ? false : group_ >= max_group_;
}

bool NotificationPromo::ExceedsMaxViews() const {
  return (max_views_ == 0) ? false : views_ >= max_views_;
}

bool NotificationPromo::IsGPlusRequired() const {
  // TODO(aruslan): http:/b/6800531
  // On M18 we will NOT support GPlusRequired promotions.
  return !gplus_required_;
}

// static
GURL NotificationPromo::PromoServerURL() {
  GURL url(promo_server_url);
  url = chrome_browser_net::AppendQueryParameter(
      url, "dist", ChannelString());
  url = chrome_browser_net::AppendQueryParameter(
      url, "osname", PlatformString());
  url = chrome_browser_net::AppendQueryParameter(
      url, "branding", chrome::VersionInfo().Version());
  DVLOG(1) << "PromoServerURL=" << url.spec();
  // Note that locale param is added by WebResourceService.
  return url;
}

double NotificationPromo::StartTimeForGroup() const {
  if (group_ < initial_segment_)
    return start_;
  return start_ +
      std::ceil(static_cast<float>(group_ - initial_segment_ + 1) / increment_)
      * time_slice_;
}

double NotificationPromo::EndTime() const {
  return end_;
}
