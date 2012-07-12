// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#ifndef CHROME_BROWSER_SYNC_NOTIFIER_ANDROID_NOTIFICATION_LISTENER_H_
#define CHROME_BROWSER_SYNC_NOTIFIER_ANDROID_NOTIFICATION_LISTENER_H_

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list_threadsafe.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace sync_notifier {

class SyncNotifierImplAndroid;

class AndroidNotificationListener :
    public content::NotificationObserver,
    public base::SupportsWeakPtr<AndroidNotificationListener> {
 public:
  AndroidNotificationListener();
  virtual ~AndroidNotificationListener();

  // These can be called on any thread.
  void AddObserver(SyncNotifierImplAndroid* observer);
  void RemoveObserver(SyncNotifierImplAndroid* observer);

  // NotificationObserver implementation.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

 private:
  content::NotificationRegistrar registrar_;
  scoped_refptr<ObserverListThreadSafe<SyncNotifierImplAndroid> > observers_;
};

}  // namespace sync_notifier

#endif  // CHROME_BROWSER_SYNC_NOTIFIER_ANDROID_NOTIFICATION_LISTENER_H_
