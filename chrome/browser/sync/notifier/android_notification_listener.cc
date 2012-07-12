// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/notifier/android_notification_listener.h"

#include "chrome/browser/sync/notifier/sync_notifier_impl_android.h"
#include "chrome/common/chrome_notification_types.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"

using content::BrowserThread;

namespace sync_notifier {

AndroidNotificationListener::AndroidNotificationListener()
    : observers_(new ObserverListThreadSafe<SyncNotifierImplAndroid>()) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  registrar_.Add(this, chrome::NOTIFICATION_SYNC_INCOMING_NOTIFICATION,
      content::NotificationService::AllSources());
}

AndroidNotificationListener::~AndroidNotificationListener() {}

void AndroidNotificationListener::AddObserver(
    SyncNotifierImplAndroid* observer) {
  observers_->AddObserver(observer);
}

void AndroidNotificationListener::RemoveObserver(
    SyncNotifierImplAndroid* observer) {
  observers_->RemoveObserver(observer);
}

void AndroidNotificationListener::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK_EQ(type, chrome::NOTIFICATION_SYNC_INCOMING_NOTIFICATION);
  content::Details<invalidation::Invalidation> invalidation(details);
  observers_->Notify(&SyncNotifierImplAndroid::OnNotificationReceived,
                     *(invalidation.ptr()));
}

}  // namespace sync_notifier
