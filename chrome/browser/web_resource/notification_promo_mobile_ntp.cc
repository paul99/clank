// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_resource/notification_promo_mobile_ntp.h"

#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/web_resource/notification_promo.h"

NotificationPromoMobileNtp::NotificationPromoMobileNtp(
    const NotificationPromo& promo)
    : valid_(false),
      action_args_(NULL),
      requires_mobile_only_sync_(false),
      requires_sync_(false),
      show_on_most_visited_(false),
      show_on_open_tabs_(false),
      show_as_virtual_computer_(false),
      payload_(NULL) {
  type_ = promo.promo_type();
  payload_ = promo.promo_payload();
  if (!payload_)
    return;
  if (!payload_->GetString("promo_message_short", &text_))
    return;
  if (!payload_->GetString("promo_message_long", &text_long_))
    return;
  if (!payload_->GetString("promo_action_type", &action_type_))
    return;
  ListValue* args;
  if (!payload_->GetList("promo_action_args", &args))
    return;
  action_args_ = const_cast<const base::ListValue*>(args);
  DCHECK(action_args_ != NULL);
  payload_->GetBoolean("promo_requires_mobile_only_sync",
      &requires_mobile_only_sync_);
  payload_->GetBoolean("promo_requires_sync", &requires_sync_);
  payload_->GetBoolean("promo_show_on_most_visited", &show_on_most_visited_);
  payload_->GetBoolean("promo_show_on_open_tabs", &show_on_open_tabs_);
  payload_->GetBoolean("promo_show_as_virtual_computer",
      &show_as_virtual_computer_);
  payload_->GetString("promo_virtual_computer_title", &virtual_computer_title_);
  payload_->GetString("promo_virtual_computer_lastsync",
      &virtual_computer_lastsync_);
  valid_ = true;
}

NotificationPromoMobileNtp::~NotificationPromoMobileNtp() {}

