// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "chrome/browser/platform_util.h"

namespace platform_util {

// Show the given file in a file manager. If possible, select the file.
void ShowItemInFolder(const FilePath& full_path) {
  NOTIMPLEMENTED();
}

// Open the given file in the desktop's default manner.
void OpenItem(const FilePath& full_path) {
  NOTIMPLEMENTED();
}

// Open the given external protocol URL in the desktop's default manner.
// (For example, mailto: URLs in the default mail user agent.)
void OpenExternal(const GURL& url) {
  NOTIMPLEMENTED();
}

// Get the top level window for the native view. This can return NULL.
gfx::NativeWindow GetTopLevel(gfx::NativeView view) {
  return view;
}

// Returns true if |window| is the foreground top level window.
bool IsWindowActive(gfx::NativeWindow window) {
  NOTIMPLEMENTED();
  return false;
}

// Activate the window, bringing it to the foreground top level.
void ActivateWindow(gfx::NativeWindow window) {
  NOTIMPLEMENTED();
}

// Returns true if the view is visible. The exact definition of this is
// platform-specific, but it is generally not "visible to the user", rather
// whether the view has the visible attribute set.
bool IsVisible(gfx::NativeView view) {
  NOTIMPLEMENTED();
  return true;
}

// Pops up an error box with an OK button. If |parent| is non-null, the box
// will be modal on it. (On Mac, it is always app-modal.) Generally speaking,
// this function should not be used for much. Infobars are preferred.
void SimpleErrorBox(gfx::NativeWindow parent,
                    const string16& title,
                    const string16& message) {
  NOTIMPLEMENTED();
}

// Pops up a dialog box with two buttons (Yes/No), with the default button of
// Yes. If |parent| is non-null, the box will be modal on it. (On Mac, it is
// always app-modal.) Returns true if the Yes button was chosen.
bool SimpleYesNoBox(gfx::NativeWindow parent,
                    const string16& title,
                    const string16& message) {
  NOTIMPLEMENTED();
  return false;
}

// Return a human readable modifier for the version string.  For a
// branded Chrome (not Chromium), this modifier is the channel (dev,
// beta, but "" for stable).
std::string GetVersionStringModifier() {
  NOTIMPLEMENTED();
  return std::string();
}

} // namespace platform_util
