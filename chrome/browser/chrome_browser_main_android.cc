// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_browser_main.h"

namespace {
void (*g_did_end_main_message_loop_func)();
}

// TODO(yfriedman): Replace with BrowserMainParts hierarchy.
// See http://b/5632260
void RegisterDidEndMainMessageLoop(void (*func)()) {
  g_did_end_main_message_loop_func = func;
}

void DidEndMainMessageLoop() {
  if (g_did_end_main_message_loop_func)
    (*g_did_end_main_message_loop_func)();
}
