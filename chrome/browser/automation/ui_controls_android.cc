// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "chrome/browser/automation/ui_controls.h"

namespace ui_controls {

bool SendMouseClick(MouseButton type) {
  NOTIMPLEMENTED();
  return false;
}

bool SendMouseMoveNotifyWhenDone(long x, long y, const base::Closure& task) {
  NOTIMPLEMENTED();
  return false;
}

bool SendMouseMove(long x, long y) {
  NOTIMPLEMENTED();
  return false;
}

bool SendKeyPress(gfx::NativeWindow window,
                  ui::KeyboardCode key,
                  bool control, bool shift, bool alt, bool command) {
  NOTIMPLEMENTED();
  return false;
}
}
