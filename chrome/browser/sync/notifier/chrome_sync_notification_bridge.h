// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_NOTIFIER_CHROME_SYNC_NOTIFICATION_BRIDGE_H_
#define CHROME_BROWSER_SYNC_NOTIFIER_CHROME_SYNC_NOTIFICATION_BRIDGE_H_

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list_threadsafe.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class Profile;

namespace sync_notifier {

class SyncNotifierObserver;

// A thread-safe bridge for chrome events that can trigger sync notifications.
// Currently only listens to NOTIFICATION_SYNC_REFRESH, triggering each
// observer's OnIncomingNotification method. Only the SESSIONS datatype is
// currently expected to be able to trigger this notification.
// Note: Notifications are expected to arrive on the UI thread, but observers
// may live on any thread.
class ChromeSyncNotificationBridge : public content::NotificationObserver {
 public:
  explicit ChromeSyncNotificationBridge(const Profile* profile);
  virtual ~ChromeSyncNotificationBridge();

  // These can be called on any thread.
  virtual void AddObserver(SyncNotifierObserver* observer);
  virtual void RemoveObserver(SyncNotifierObserver* observer);

  // NotificationObserver implementation. Called on UI thread.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

 private:
  content::NotificationRegistrar registrar_;

  // Because [Add/Remove]Observer can be called from any thread, we need a
  // thread-safe observerlist.
  scoped_refptr<ObserverListThreadSafe<SyncNotifierObserver> > observers_;
};

}  // namespace sync_notifier

#endif  // CHROME_BROWSER_SYNC_NOTIFIER_CHROME_SYNC_NOTIFICATION_BRIDGE_H_
