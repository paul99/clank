// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/notifications/notification_types.h"

namespace ui {

namespace notifications {

const char kMessageIntentKey[] = "message_intent";
const char kPriorityKey[] = "priority";
const char kTimestampKey[] = "timestamp";
const char kSecondIconUrlKey[] = "second_icon_url";
const char kUnreadCountKey[] = "unread_count";
const char kButtonOneTitleKey[] = "button_one_title";
const char kButtonOneIntentKey[] = "button_one_intent";
const char kButtonTwoTitleKey[] = "button_two_title";
const char kButtonTwoIntentKey[] = "button_two_intent";
const char kExpandedMessageKey[] = "expanded_message";
const char kImageUrlKey[] = "image_url";
const char kItemsKey[] = "items";
const char kItemTitleKey[] = "title";
const char kItemMessageKey[] = "message";

const char kSimpleType[] = "simple";
const char kBaseFormatType[] = "base";
const char kMultipleType[] = "multiple";

NotificationType StringToNotificationType(std::string& string_type) {
  if (string_type == kSimpleType)
    return NOTIFICATION_TYPE_SIMPLE;
  if (string_type == kBaseFormatType)
    return NOTIFICATION_TYPE_BASE_FORMAT;
  if (string_type == kMultipleType)
    return NOTIFICATION_TYPE_MULTIPLE;

  // In case of unrecognized string, fall back to most common type.
  return NOTIFICATION_TYPE_SIMPLE;
}

}  // namespace notifications

}  // namespace ui
