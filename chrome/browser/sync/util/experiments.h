// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_UTIL_EXPERIMENTS_
#define SYNC_UTIL_EXPERIMENTS_
#pragma once

#include "chrome/browser/sync/syncable/model_type.h"

namespace browser_sync {

// A structure to hold the enable status of experimental sync features.
struct Experiments {
  Experiments() : sync_tabs(false), sync_tab_favicons(false) {}

  bool Matches(const Experiments& rhs) {
    return (sync_tabs == rhs.sync_tabs) &&
           (sync_tab_favicons == rhs.sync_tab_favicons);
  }

  // Enable the tab sync (SESSIONS) datatype.
  bool sync_tabs;

  // Enable syncing of favicons within tab sync (only has an effect if tab sync
  // is already enabled). This takes effect on the next restart.
  bool sync_tab_favicons;
};

}

#endif  // SYNC_UTIL_EXPERIMENTS_
