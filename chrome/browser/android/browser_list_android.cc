// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_list.h"

#include "chrome/browser/sync/glue/synced_window_delegate.h"
#include "chrome/browser/sync/glue/synced_window_delegate_registry.h"
#include "clank/native/framework/chrome/tab_model.h"

// static
void BrowserList::AllBrowsersClosedAndAppExiting() {
}

// static
bool BrowserList::IsOffTheRecordSessionActive() {
  // used by metrics to determine when not to log metrics.
  // TODO(wangxianzhu): The dependency to browser_sync module is temporary.
  // Need to refactor native side of TabModel to remove the dependency.
  std::set<browser_sync::SyncedWindowDelegate*> window =
      browser_sync::SyncedWindowDelegateRegistry::GetSyncedWindowDelegates();
  for (std::set<browser_sync::SyncedWindowDelegate*>::const_iterator i =
         window.begin(); i != window.end(); ++i) {
    const TabModel* delegate = static_cast<const TabModel*>(*i);
    if (delegate->is_incognito() && delegate->GetTabCount() > 0)
      return true;
  }
  return false;
}

// Other BrowserList methods that are not implemented are in
// browser_stubs_android.cc.
