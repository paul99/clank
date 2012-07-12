// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SPEECH_SPEECH_INPUT_MANAGER_ANDROID_H_
#define CHROME_BROWSER_SPEECH_SPEECH_INPUT_MANAGER_ANDROID_H_
#pragma once

#include <jni.h>

namespace speech_input {

// Registers the speech input manager global methods with JNI so they can be
// invoked from the Java code.
bool RegisterSpeechInputManager(JNIEnv* env);

}  // namespace speech_input


#endif  // CHROME_BROWSER_SPEECH_SPEECH_INPUT_MANAGER_ANDROID_H_
