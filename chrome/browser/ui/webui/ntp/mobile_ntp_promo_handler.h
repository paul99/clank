// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_NTP_MOBILE_NTP_PROMO_HANDLER_H
#define CHROME_BROWSER_UI_WEBUI_NTP_MOBILE_NTP_PROMO_HANDLER_H
#pragma once

#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_ui_message_handler.h"

class PrefService;

// The handler for Javascript messages related to the mobile NTP promo.
class MobileNtpPromoHandler : public content::WebUIMessageHandler,
                              public content::NotificationObserver {
 public:
  MobileNtpPromoHandler();
  virtual ~MobileNtpPromoHandler();

  // WebUIMessageHandler override and implementation.
  virtual void RegisterMessages() OVERRIDE;

  // Register preferences.
  static void RegisterUserPrefs(PrefService* prefs);

 private:
  // NotificationObserver override and implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // Callback for the "promoSendEmail" message
  // Args: [ subject, body, app-chooser-message ].
  void HandlePromoSendEmail(const base::ListValue* args);

  // Callback for the "promoActionTriggered" message
  // Args: [] or NULL.
  void HandlePromoActionTriggered(const base::ListValue* args);

  // Callback for the "promoDisabled" message
  // Args: [] or NULL.
  void HandlePromoDisabled(const base::ListValue* args);

  // Callback for the "getPromotions" message
  // Args: [] or NULL.
  void InjectPromoDecorations(const base::ListValue* args);

  // Callback for the "recordImpression" message
  // Args: [ ("most_visited" | "open_tabs" | "sync_promo") ].
  void RecordImpression(const base::ListValue* args);

  // Returns true if the Chrome Promo is allowed
  bool IsChromePromoAllowedUncached(
      bool promo_requires_sync,
      bool promo_requires_no_active_desktop_sync_sessions);

  // Records an impression; could trigger a refresh if max_views are exceeded.
  void RecordPromotionImpression(const std::string& id);

  // Updates the profile preference if any desktop session was discovered.
  void CheckDesktopSessions();

  // Registrar to receive notification on promo changes.
  content::NotificationRegistrar registrar_;

  // Flag to see if we ever fetched sync service sessions.
  bool did_fetch_sessions_;

  DISALLOW_COPY_AND_ASSIGN(MobileNtpPromoHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_NTP_MOBILE_NTP_PROMO_HANDLER_H
