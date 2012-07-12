// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sessions/session_restore.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_types.h"
#include "chrome/browser/sync/glue/synced_window_delegate.h"
#include "chrome/browser/sync/glue/synced_window_delegate_registry.h"
#include "clank/native/framework/chrome/tab.h"
#include "clank/native/framework/chrome/tab_model.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "ipc/ipc_message.h"

// static
// The android implementation does not do anything "foreign session" specific.
// We use it to restore tabs from "recently closed" too.
void SessionRestore::RestoreForeignSessionTab(Profile* profile,
    const SessionTab& session_tab) {
  const TabModel* delegate = NULL;
  std::set<browser_sync::SyncedWindowDelegate*> window =
      browser_sync::SyncedWindowDelegateRegistry::GetSyncedWindowDelegates();
  for (std::set<browser_sync::SyncedWindowDelegate*>::const_iterator i =
         window.begin(); i != window.end(); ++i) {
    delegate = static_cast<const TabModel*>(*i);
    if (!delegate->is_incognito()) {
      break;
    }
  }
  DCHECK(delegate);
  std::vector<content::NavigationEntry*> entries;
  TabNavigation::CreateNavigationEntriesFromTabNavigations(
        profile, session_tab.navigations, &entries);
  content::BrowserContext* context = profile;
  content::WebContents* new_web_contents = content::WebContents::Create(
        context, NULL, MSG_ROUTING_NONE, 0, 0);
  int selected_index = session_tab.current_navigation_index;
  // Since the current_navigation_index can be larger than the index for number
  // of navigations in the current sessions (chrome://newtab is not stored), we
  // must perform bounds checking. Lowest value allowed is 0, so we need to
  // check for max if navigations.size() == 0 or if selected_index is negative.
  selected_index = std::max(
      0,
      std::min(selected_index,
               static_cast<int>(session_tab.navigations.size() - 1)));
  new_web_contents->GetController().Restore(selected_index,
                                true, /* from_last_session */
                                &entries);
  delegate->CreateTab(new_web_contents);
}

// static
void SessionRestore::RestoreForeignSessionWindows(
    Profile* profile,
    std::vector<const SessionWindow*>::const_iterator begin,
    std::vector<const SessionWindow*>::const_iterator end) {
  NOTIMPLEMENTED();
}
