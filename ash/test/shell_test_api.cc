// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/shell_test_api.h"

#include "ash/root_window_controller.h"
#include "ash/shell.h"

namespace ash {
namespace test {

ShellTestApi::ShellTestApi(Shell* shell) : shell_(shell) {}

internal::RootWindowLayoutManager* ShellTestApi::root_window_layout() {
  return shell_->GetPrimaryRootWindowController()->root_window_layout();
}

views::corewm::InputMethodEventFilter*
ShellTestApi::input_method_event_filter() {
  return shell_->input_method_filter_.get();
}

internal::SystemGestureEventFilter*
ShellTestApi::system_gesture_event_filter() {
  return shell_->system_gesture_filter_.get();
}

internal::WorkspaceController* ShellTestApi::workspace_controller() {
  return shell_->GetPrimaryRootWindowController()->workspace_controller();
}

internal::ScreenPositionController*
ShellTestApi::screen_position_controller() {
  return shell_->screen_position_controller_.get();
}

LauncherModel* ShellTestApi::launcher_model() {
  return shell_->launcher_model_.get();
}

}  // namespace test
}  // namespace ash
