// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_SWITCHES_ANDROID_H_
#define CHROME_COMMON_SWITCHES_ANDROID_H_
#pragma once

// This file defines the extra command line switches that we use to send some
// extra information to our renderer process on Android.
namespace switches {

// The asset path
extern const char kAssetPath[];

// The package path ie chrome.apk
extern const char kPackagePath[];

// The memory threshold for GC
extern const char kMemoryThresholdMB[];

// The mobile string to be used when generating the user agent
extern const char kUserAgentMobile[];

// The OS information to be used when generating the user agent.
extern const char kUserAgentOSInfo[];

} // namespace switches

#endif // CHROME_COMMON_SWITCHES_ANDROID_H_
