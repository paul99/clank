// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_BROWSER_MAIN_ANDROID_H_
#define CHROME_BROWSER_CHROME_BROWSER_MAIN_ANDROID_H_

// Register a function to be used for DidEndMainMessageLoop().
// Android's implementation is tied to the Java/Android glue, and thus
// lives outside of Chrome.
void RegisterDidEndMainMessageLoop(void (*)());

// Perform platform-specific work that needs to be done after the main event
// loop has ended. The embedder must be sure to call this.
// TODO(yfriedman): Replace with BrowserMainParts hierarchy.
// See http://b/5632260
void DidEndMainMessageLoop();

#endif  // CHROME_BROWSER_CHROME_BROWSER_MAIN_ANDROID_H_
