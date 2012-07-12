// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_CHROME_STARTUP_FLAGS_H_
#define CONTENT_BROWSER_ANDROID_CHROME_STARTUP_FLAGS_H_
#pragma once

#include <string>

// Force-appends flags to the Chrome command line turning on Clank features (for
// example, sync and Android graphics modes).  This is called as soon as
// possible during initialization to make sure code sees the new flags.
void SetAndroidCommandLineFlags(bool is_tablet,
                                const std::string& plugin_descriptor);

#endif  // CONTENT_BROWSER_ANDROID_CHROME_STARTUP_FLAGS_H_
