// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/proxy_settings_dialog.h"

#include "base/utf_string_conversions.h"
#include "chrome/browser/chromeos/login/helper.h"
#include "chrome/browser/chromeos/proxy_config_service_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/url_constants.h"
#include "chrome/common/chrome_notification_types.h"
#include "content/public/browser/notification_service.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

namespace {

// Hints for size of proxy settings dialog.
const int kProxySettingsDialogReasonableWidth = 626;
const int kProxySettingsDialogReasonableHeight = 525;
const float kProxySettingsDialogReasonableWidthRatio = 0.4f;
const float kProxySettingsDialogReasonableHeightRatio = 0.4f;

int CalculateSize(int screen_size, int min_comfortable, float desired_ratio) {
  int desired_size = static_cast<int>(desired_ratio * screen_size);
  desired_size = std::max(min_comfortable, desired_size);
  return std::min(screen_size, desired_size);
}

}  // namespace

namespace chromeos {

ProxySettingsDialog::ProxySettingsDialog(LoginHtmlDialog::Delegate* delegate,
                                         gfx::NativeWindow window)
    : LoginHtmlDialog(
          delegate,
          window,
          std::wstring(),
          GURL(chrome::kChromeUIProxySettingsURL),
          LoginHtmlDialog::STYLE_BUBBLE) {
  gfx::Rect screen_bounds(chromeos::CalculateScreenBounds(gfx::Size()));
  SetDialogSize(CalculateSize(screen_bounds.width(),
                               kProxySettingsDialogReasonableWidth,
                               kProxySettingsDialogReasonableWidthRatio),
                CalculateSize(screen_bounds.height(),
                               kProxySettingsDialogReasonableHeight,
                               kProxySettingsDialogReasonableHeightRatio));

  // Get network name for dialog title.
  Profile* profile = ProfileManager::GetDefaultProfile();
  PrefProxyConfigTracker* proxy_tracker = profile->GetProxyConfigTracker();
  proxy_tracker->UIMakeActiveNetworkCurrent();
  std::string network_name;
  proxy_tracker->UIGetCurrentNetworkName(&network_name);
  SetDialogTitle(l10n_util::GetStringFUTF16(IDS_PROXY_PAGE_TITLE_FORMAT,
                                            ASCIIToUTF16(network_name)));
}

void ProxySettingsDialog::OnDialogClosed(const std::string& json_retval) {
  LoginHtmlDialog::OnDialogClosed(json_retval);
  content::NotificationService::current()->Notify(
    chrome::NOTIFICATION_LOGIN_PROXY_CHANGED,
    content::NotificationService::AllSources(),
    content::NotificationService::NoDetails());
}

}  // namespace chromeos
