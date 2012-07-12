// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_COMMAND_LINE_H_
#define CONTENT_BROWSER_ANDROID_COMMAND_LINE_H_
#pragma once

#include <jni.h>

#include "content/browser/android/jni_helper.h"

// Appends all strings in the given array as flags to the Chrome command line.
void InitNativeCommandLineFromJavaArray(JNIEnv* env,
                                        jobjectArray init_command_line);

// JNI registration boilerplate.
bool RegisterCommandLine(JNIEnv* env);

#endif  // CONTENT_BROWSER_ANDROID_COMMAND_LINE_H_
