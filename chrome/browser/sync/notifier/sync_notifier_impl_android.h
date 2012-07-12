// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_NOTIFIER_SYNC_NOTIFIER_IMPL_ANDROID_H_
#define CHROME_BROWSER_SYNC_NOTIFIER_SYNC_NOTIFIER_IMPL_ANDROID_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list_threadsafe.h"
#include "chrome/browser/sync/notifier/sync_notifier.h"
#include "chrome/browser/sync/syncable/model_type.h"
#include "google/cacheinvalidation/v2/types.pb.h"
#include "google/cacheinvalidation/include/types.h"

namespace sync_notifier {

class AndroidNotificationListener;

class SyncNotifierImplAndroid : public SyncNotifier {
 public:
  explicit SyncNotifierImplAndroid(
      const base::WeakPtr<AndroidNotificationListener>& listener);

  virtual ~SyncNotifierImplAndroid();

  // SyncNotifier implementation
  virtual void UpdateCredentials(
      const std::string& email, const std::string& token) OVERRIDE;

  virtual void SetUniqueId(const std::string& unique_id) OVERRIDE;
  virtual void SetState(const std::string& state) OVERRIDE;

  virtual void AddObserver(SyncNotifierObserver* observer) OVERRIDE;
  virtual void RemoveObserver(SyncNotifierObserver* observer) OVERRIDE;

  virtual void UpdateEnabledTypes(const syncable::ModelTypeSet types) OVERRIDE;
  virtual void SendNotification(
      const syncable::ModelTypeSet changed_types) OVERRIDE;

  // Android-specific implementation. Called by AndroidNotificationListener.
  void OnNotificationReceived(const invalidation::Invalidation& invalidation);

 private:

  syncable::ModelTypeSet enabled_types_;
  std::string state_;
  std::string unique_id_;

  const scoped_refptr<ObserverListThreadSafe<SyncNotifierObserver> >
      observer_list_;
  const base::WeakPtr<AndroidNotificationListener> listener_;
  std::map<syncable::ModelType, int64> max_invalidation_versions_;

  DISALLOW_COPY_AND_ASSIGN(SyncNotifierImplAndroid);
};

}  // namespace sync_notifier
#endif  // CHROME_BROWSER_SYNC_NOTIFIER_SYNC_NOTIFIER_IMPL_ANDROID_H_
