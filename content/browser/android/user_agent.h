// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_USER_AGENT_H_
#define CONTENT_BROWSER_ANDROID_USER_AGENT_H_

#include <jni.h>
#include <string>

// Returns the OS information component required by user agent composition.
std::string GetUserAgentOSInfo();

// Returns the mobile component required by user agent composition.
std::string GetUserAgentMobile();

// Returns the Chrome Version information required by user agent composition.
std::string GetUserAgentChromeVersionInfo();

// Register JNI method.
bool RegisterUserAgent(JNIEnv* env);

#endif  // CONTENT_BROWSER_ANDROID_USER_AGENT_H_
