// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_NOTIFIER_SYNC_NOTIFIER_FACTORY_H_
#define CHROME_BROWSER_SYNC_NOTIFIER_SYNC_NOTIFIER_FACTORY_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#if defined(OS_ANDROID)
#include "chrome/browser/sync/notifier/android_notification_listener.h"
#endif  // defined(OS_ANDROID)
#include "chrome/browser/sync/notifier/chrome_sync_notification_bridge.h"
#include "chrome/browser/sync/notifier/invalidation_version_tracker.h"
#include "chrome/browser/sync/util/weak_handle.h"

class CommandLine;
class Profile;

namespace net {
class URLRequestContextGetter;
}

namespace sync_notifier {

class SyncNotifier;

// Class to instantiate various implementations of the SyncNotifier
// interface.  Must be created/destroyed on the UI thread.
class SyncNotifierFactory {
 public:
  // |client_info| is a string identifying the client, e.g. a user
  // agent string.  |invalidation_version_tracker| may be NULL (for
  // tests).
  SyncNotifierFactory(
      const Profile* profile,
      const std::string& client_info,
      const scoped_refptr<net::URLRequestContextGetter>&
          request_context_getter,
      const base::WeakPtr<InvalidationVersionTracker>&
          invalidation_version_tracker,
      const CommandLine& command_line);
  ~SyncNotifierFactory();

  // Creates a sync notifier. Caller takes ownership of the returned
  // object.  However, the returned object must not outlive the
  // factory from which it was created.  Can be called on any thread.
  SyncNotifier* CreateSyncNotifier();

 private:
  // A thread-safe listener for handling notifications triggered by chrome
  // events.
  ChromeSyncNotificationBridge chrome_notification_bridge_;

  const std::string client_info_;
  const scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  const InvalidationVersionMap initial_max_invalidation_versions_;
  const browser_sync::WeakHandle<InvalidationVersionTracker>
      invalidation_version_tracker_;
  const CommandLine& command_line_;

#if defined(OS_ANDROID)
  AndroidNotificationListener listener_;
#endif  // defined(OS_ANDROID)
};

}  // namespace sync_notifier

#endif  // CHROME_BROWSER_SYNC_NOTIFIER_SYNC_NOTIFIER_FACTORY_H_
