// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_USER_ACTIONS_USER_ACTIONS_UI_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_USER_ACTIONS_USER_ACTIONS_UI_HANDLER_H_

#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_ui_message_handler.h"

// UI Handler for chrome://user-actions/
// It listens to user action notifications and passes those notifications
// into the Javascript to update the page.
class UserActionsUIHandler : public content::NotificationObserver,
                             public content::WebUIMessageHandler {
 public:
  UserActionsUIHandler();
  virtual ~UserActionsUIHandler();

  // NotificationObserver implementation:
  // Listens for user action notifications and passes the message to
  // observeUserAction in user_actions.js.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  // WebUIMessageHandler implementation:
  // Does nothing for now.
  virtual void RegisterMessages() OVERRIDE;

 private:
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(UserActionsUIHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_USER_ACTIONS_USER_ACTIONS_UI_HANDLER_H_
