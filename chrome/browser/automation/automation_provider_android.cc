// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "chrome/browser/automation/automation_provider.h"
#include "chrome/browser/automation/ui_controls.h"
#include "chrome/browser/automation/automation_browser_tracker.h"
#include "chrome/browser/automation/automation_window_tracker.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"

void AutomationProvider::OverrideEncoding(int tab_handle,
                                          const std::string& encoding_name,
                                          bool* success) {
  NOTIMPLEMENTED();
  *success = false;
}

void AutomationProvider::PrintAsync(int tab_handle) {
  NOTIMPLEMENTED();
}

void AutomationProvider::WindowSimulateDrag(int handle,
                                            const std::vector<gfx::Point>& drag_path,
                                            int flags,
                                            bool press_escape_en_route,
                                            IPC::Message* reply_message) {
  NOTIMPLEMENTED();
}
