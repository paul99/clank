// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/synced_window_delegate_registry.h"

#include "chrome/browser/sync/glue/synced_window_delegate.h"

namespace browser_sync {

// See http://b/issue?id=5861833
#if defined(__clang__)
#define DEFINE_STATIC_GLOBAL(type, name) \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wglobal-constructors\"") \
    _Pragma("clang diagnostic ignored \"-Wexit-time-destructors\"") \
    type name; \
    _Pragma("clang diagnostic pop")
#else
#define DEFINE_STATIC_GLOBAL(type, name) \
    type name;
#endif  // __clang__

DEFINE_STATIC_GLOBAL(std::set<SyncedWindowDelegate*>, SyncedWindowDelegateRegistry::delegates_);

/* static */
void SyncedWindowDelegateRegistry::Register(SyncedWindowDelegate* delegate) {
  delegates_.insert(delegate);
}

/* static */
void SyncedWindowDelegateRegistry::Unregister(
    SyncedWindowDelegate* delegate) {
  delegates_.erase(delegate);
}

/* static */
const std::set<SyncedWindowDelegate*>&
SyncedWindowDelegateRegistry::GetSyncedWindowDelegates() {
  return delegates_;
}

}  // namespace browser_sync
