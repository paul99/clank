// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/ash/balloon_collection_impl_ash.h"

#include "ash/shell.h"
#include "ash/system/web_notification/web_notification_tray.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/notifications/balloon.h"
#include "chrome/browser/notifications/desktop_notification_service.h"
#include "chrome/browser/notifications/desktop_notification_service_factory.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/ash/app_icon_loader_impl.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/views/ash/balloon_view_ash.h"
#include "chrome/browser/ui/views/notifications/balloon_view_host.h"
#include "chrome/browser/ui/views/notifications/balloon_view_views.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_set.h"
#include "chrome/common/extensions/permissions/api_permission.h"
#include "ui/views/widget/widget.h"

using message_center::NotifierSettingsView;

BalloonCollectionImplAsh::BalloonCollectionImplAsh()
    : settings_view_(NULL) {
  ash::Shell::GetInstance()->GetWebNotificationTray()->message_center()->
      SetDelegate(this);
}

BalloonCollectionImplAsh::~BalloonCollectionImplAsh() {
}

bool BalloonCollectionImplAsh::HasSpace() const {
  return true;  // Overflow is handled by ash::WebNotificationTray.
}

void BalloonCollectionImplAsh::Add(const Notification& notification,
                                   Profile* profile) {
  if (notification.is_html())
    return;  // HTML notifications are not supported in Ash.
  if (notification.title().empty() && notification.body().empty())
    return;  // Empty notification, don't show.
  // TODO(mukai): add a filter if the source extension is disabled or not.
  // The availability can be checked from DesktopNotificationService.
  return BalloonCollectionImpl::Add(notification, profile);
}

void BalloonCollectionImplAsh::DisableExtension(
    const std::string& notification_id) {
  Balloon* balloon = base().FindBalloonById(notification_id);
  const extensions::Extension* extension = GetBalloonExtension(balloon);
  if (!extension)
    return;
  balloon->profile()->GetExtensionService()->DisableExtension(
      extension->id(), extensions::Extension::DISABLE_USER_ACTION);
}

void BalloonCollectionImplAsh::DisableNotificationsFromSource(
    const std::string& notification_id) {
  Balloon* balloon = base().FindBalloonById(notification_id);
  if (!balloon)
    return;
  DesktopNotificationService* service =
      DesktopNotificationServiceFactory::GetForProfile(balloon->profile());
  service->DenyPermission(balloon->notification().origin_url());
}

void BalloonCollectionImplAsh::NotificationRemoved(
    const std::string& notification_id) {
  RemoveById(notification_id);
}

void BalloonCollectionImplAsh::ShowSettings(
    const std::string& notification_id) {
  Balloon* balloon = base().FindBalloonById(notification_id);
  Profile* profile =
      balloon ? balloon->profile() : ProfileManager::GetDefaultProfile();
  Browser* browser =
      chrome::FindOrCreateTabbedBrowser(profile,
                                        chrome::HOST_DESKTOP_TYPE_ASH);
  if (GetBalloonExtension(balloon))
    chrome::ShowExtensions(browser);
  else
    chrome::ShowContentSettings(browser, CONTENT_SETTINGS_TYPE_NOTIFICATIONS);
}

void BalloonCollectionImplAsh::ShowSettingsDialog(gfx::NativeView context) {
  if (settings_view_) {
    settings_view_->GetWidget()->StackAtTop();
  } else {
    settings_view_ =
        message_center::NotifierSettingsView::Create(this, context);
  }
}

void BalloonCollectionImplAsh::OnClicked(const std::string& notification_id) {
  Balloon* balloon = base().FindBalloonById(notification_id);
  if (!balloon)
    return;
  balloon->OnClick();
}

void BalloonCollectionImplAsh::OnButtonClicked(
    const std::string& notification_id, int button_index) {
  Balloon* balloon = base().FindBalloonById(notification_id);
  if (balloon)
    balloon->OnButtonClick(button_index);
}

void BalloonCollectionImplAsh::GetNotifierList(
    std::vector<NotifierSettingsView::Notifier>* notifiers) {
  DCHECK(notifiers);
  Profile* profile = ProfileManager::GetDefaultProfile();
  DesktopNotificationService* notification_service =
      DesktopNotificationServiceFactory::GetForProfile(profile);

  app_icon_loader_.reset(new ash::AppIconLoaderImpl(
      profile, extension_misc::EXTENSION_ICON_BITTY, this));
  ExtensionService* extension_service = profile->GetExtensionService();
  const ExtensionSet* extension_set = extension_service->extensions();
  for (ExtensionSet::const_iterator iter = extension_set->begin();
       iter != extension_set->end(); ++iter) {
    const extensions::Extension* extension = *iter;
    // Currently, our notification API is provided for experimental apps.
    // TODO(mukai, miket): determine the actual rule and fix here.
    if (!extension->is_app() || !extension->HasAPIPermission(
            extensions::APIPermission::kExperimental)) {
      continue;
    }

    notifiers->push_back(NotifierSettingsView::Notifier(
        extension->id(),
        NotifierSettingsView::Notifier::APPLICATION,
        UTF8ToUTF16(extension->name())));
    app_icon_loader_->FetchImage(extension->id());
    notifiers->back().enabled =
        notification_service->IsExtensionEnabled(extension->id());
  }

  ContentSettingsForOneType settings;
  notification_service->GetNotificationsSettings(&settings);
  for (ContentSettingsForOneType::const_iterator iter = settings.begin();
       iter != settings.end(); ++iter) {
    if (iter->primary_pattern == ContentSettingsPattern::Wildcard() &&
        iter->secondary_pattern == ContentSettingsPattern::Wildcard() &&
        iter->source != "preference") {
      continue;
    }

    std::string url_pattern = iter->primary_pattern.ToString();
    notifiers->push_back(NotifierSettingsView::Notifier(
        url_pattern,
        NotifierSettingsView::Notifier::URL_PATTERN,
        UTF8ToUTF16(url_pattern)));
    // TODO(mukai): add favicon loader here.
    GURL url(url_pattern);
    notifiers->back().enabled =
        notification_service->GetContentSetting(url) == CONTENT_SETTING_ALLOW;
  }
}

void BalloonCollectionImplAsh::SetNotifierEnabled(
    const NotifierSettingsView::Notifier& notifier, bool enabled) {
  Profile* profile = ProfileManager::GetDefaultProfile();
  DesktopNotificationService* notification_service =
      DesktopNotificationServiceFactory::GetForProfile(profile);

  switch (notifier.type) {
    case NotifierSettingsView::Notifier::APPLICATION:
      notification_service->SetExtensionEnabled(notifier.id, enabled);
      break;
    case NotifierSettingsView::Notifier::URL_PATTERN: {
      ContentSetting default_setting =
          notification_service->GetDefaultContentSetting(NULL);
      DCHECK(default_setting == CONTENT_SETTING_ALLOW ||
             default_setting == CONTENT_SETTING_BLOCK ||
             default_setting == CONTENT_SETTING_ASK);
      if ((enabled && default_setting != CONTENT_SETTING_ALLOW) ||
          (!enabled && default_setting == CONTENT_SETTING_ALLOW)) {
        GURL url(notifier.id);
        if (url.is_valid()) {
          if (enabled)
            notification_service->GrantPermission(url);
          else
            notification_service->DenyPermission(url);
        } else {
          LOG(ERROR) << "Invalid url pattern: " << notifier.id;
        }
      } else {
        ContentSettingsPattern pattern =
            ContentSettingsPattern::FromString(notifier.id);
        if (pattern.IsValid())
          notification_service->ClearSetting(pattern);
        else
          LOG(ERROR) << "Invalid url pattern: " << notifier.id;
      }
      break;
    }
  }
}

void BalloonCollectionImplAsh::OnNotifierSettingsClosing(
    NotifierSettingsView* view) {
  settings_view_ = NULL;
}

void BalloonCollectionImplAsh::SetAppImage(const std::string& id,
                                           const gfx::ImageSkia& image) {
  if (!settings_view_)
    return;

  settings_view_->UpdateIconImage(id, image);
}

bool BalloonCollectionImplAsh::AddWebUIMessageCallback(
    const Notification& notification,
    const std::string& message,
    const chromeos::BalloonViewHost::MessageCallback& callback) {
#if defined(OS_CHROMEOS)
  Balloon* balloon = base().FindBalloon(notification);
  if (!balloon)
    return false;

  BalloonHost* balloon_host = balloon->balloon_view()->GetHost();
  if (!balloon_host)
    return false;
  chromeos::BalloonViewHost* balloon_view_host =
      static_cast<chromeos::BalloonViewHost*>(balloon_host);
  return balloon_view_host->AddWebUIMessageCallback(message, callback);
#else
  return false;
#endif
}

bool BalloonCollectionImplAsh::UpdateNotification(
    const Notification& notification) {
  Balloon* balloon = base().FindBalloon(notification);
  if (!balloon)
    return false;
  balloon->Update(notification);
  return true;
}

bool BalloonCollectionImplAsh::UpdateAndShowNotification(
    const Notification& notification) {
  return UpdateNotification(notification);
}

Balloon* BalloonCollectionImplAsh::MakeBalloon(
    const Notification& notification, Profile* profile) {
  Balloon* balloon = new Balloon(notification, profile, this);
  BalloonViewAsh* balloon_view = new BalloonViewAsh(this);
  balloon->set_view(balloon_view);
  return balloon;
}

const extensions::Extension* BalloonCollectionImplAsh::GetBalloonExtension(
    Balloon* balloon) {
  if (!balloon)
    return NULL;
  ExtensionService* extension_service =
      balloon->profile()->GetExtensionService();
  const GURL& origin = balloon->notification().origin_url();
  return extension_service->extensions()->GetExtensionOrAppByURL(
      ExtensionURLInfo(origin));
}

#if defined(OS_CHROMEOS)
// static
BalloonCollection* BalloonCollection::Create() {
  return new BalloonCollectionImplAsh();
}
#endif
