// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEB_RESOURCE_NOTIFICATION_PROMO_MOBILE_NTP_H_
#define CHROME_BROWSER_WEB_RESOURCE_NOTIFICATION_PROMO_MOBILE_NTP_H_

#include <string>

#include "base/basictypes.h"
#include "chrome/browser/web_resource/notification_promo.h"

namespace base {
class DictionaryValue;
class ListValue;
}

// Helper class for NotificationPromo that deals with mobile_ntp promos.
// NOTE: Its lifetime should be less than the lifetime of NotificationPromo.
class NotificationPromoMobileNtp {
 public:
  // Loads the NotificationPromoMobileNtp from NotificationPromo.
  explicit NotificationPromoMobileNtp(const NotificationPromo& promo);
  ~NotificationPromoMobileNtp();
  bool valid() const { return valid_; }

  const std::string& type() const { return type_; }
  const std::string& text() const { return text_; }
  const std::string& text_long() const { return text_long_; }
  const std::string& action_type() const { return action_type_; }
  const base::ListValue* action_args() const { return action_args_; }
  bool requires_mobile_only_sync() const { return requires_mobile_only_sync_; }
  bool requires_sync() const { return requires_sync_; }
  bool show_on_most_visited() const { return show_on_most_visited_; }
  bool show_on_open_tabs() const { return show_on_open_tabs_; }
  bool show_as_virtual_computer() const { return show_as_virtual_computer_; }
  const std::string& virtual_computer_title() const {
    return virtual_computer_title_;
  }
  const std::string& virtual_computer_lastsync() const {
    return virtual_computer_lastsync_;
  }
  const base::DictionaryValue* payload() const { return payload_; }

 private:
  bool valid_;
  std::string type_;
  std::string text_;
  std::string text_long_;
  std::string action_type_;
  const base::ListValue* action_args_;
  bool requires_mobile_only_sync_;
  bool requires_sync_;
  bool show_on_most_visited_;
  bool show_on_open_tabs_;
  bool show_as_virtual_computer_;
  std::string virtual_computer_title_;
  std::string virtual_computer_lastsync_;
  const base::DictionaryValue* payload_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPromoMobileNtp);
};

#endif  // CHROME_BROWSER_WEB_RESOURCE_NOTIFICATION_PROMO_MOBILE_NTP_H_
