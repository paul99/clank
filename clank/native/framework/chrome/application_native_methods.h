// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_APPLICATION_NATIVE_METHODS_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_APPLICATION_NATIVE_METHODS_H_
#pragma once

#include "content/browser/android/jni_helper.h"

// Registers the native methods needed only by the Chrome application, as
// opposed to those needed by ChromeView.
bool RegisterApplicationNativeMethods(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_APPLICATION_NATIVE_METHODS_H_
