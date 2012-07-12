// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DEVICE_UTILS_H_
#define CHROME_BROWSER_ANDROID_DEVICE_UTILS_H_

#include <string>

namespace base {
namespace android {

// Return the unique identifier of this device.
std::string GetAndroidId();

// Return the end-user-visible name for this device.
std::string GetModel();

// Return if the device is a tablet. Should only be called when host is chrome.
bool IsTabletUi();

}  // namespace android
}  // namespace base

#endif  // CHROME_BROWSER_ANDROID_DEVICE_UTILS_H_
