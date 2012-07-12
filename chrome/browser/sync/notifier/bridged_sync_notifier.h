// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_NOTIFIER_BRIDGED_SYNC_NOTIFIER_H_
#define CHROME_BROWSER_SYNC_NOTIFIER_BRIDGED_SYNC_NOTIFIER_H_

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/sync/notifier/sync_notifier.h"

namespace sync_notifier {

class ChromeSyncNotificationBridge;

// A SyncNotifier implementation that wraps a ChromeSyncNotificationBridge
// and a SyncNotifier delegate (which it takes ownership of). All SyncNotifier
// calls are passed straight through to the delegate, with the exception of
// AddObserver/RemoveObserver, which also result in the observer being
// registered/deregistered with the ChromeSyncNotificationBridge.
class BridgedSyncNotifier : public SyncNotifier {
 public:
  // Does not take ownership of |bridge|. Takes ownership of |delegate|.
  BridgedSyncNotifier(ChromeSyncNotificationBridge* bridge,
                      SyncNotifier* delegate);
  virtual ~BridgedSyncNotifier();

  // SyncNotifier implementation. Passes through all calls to the delegate.
  // AddObserver/RemoveObserver will also register/deregister |observer| with
  // the bridge.
  virtual void AddObserver(SyncNotifierObserver* observer) OVERRIDE;
  virtual void RemoveObserver(SyncNotifierObserver* observer) OVERRIDE;
  virtual void SetUniqueId(const std::string& unique_id) OVERRIDE;
  virtual void SetState(const std::string& state) OVERRIDE;
  virtual void UpdateCredentials(
      const std::string& email, const std::string& token) OVERRIDE;
  virtual void UpdateEnabledTypes(
      syncable::ModelTypeSet enabled_types) OVERRIDE;
  virtual void SendNotification(
      syncable::ModelTypeSet changed_types) OVERRIDE;

 private:
  // The notification bridge that we register the observers with.
  ChromeSyncNotificationBridge* bridge_;

  // The delegate we are wrapping.
  scoped_ptr<SyncNotifier> delegate_;
};

}  // namespace sync_notifier

#endif  // CHROME_BROWSER_SYNC_NOTIFIER_BRIDGED_SYNC_NOTIFIER_H_
