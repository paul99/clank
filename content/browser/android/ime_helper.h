// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_IME_HELPER_H_
#define CONTENT_BROWSER_ANDROID_IME_HELPER_H_
#pragma once

#include "content/browser/android/jni_helper.h"

class AndroidKeyEvent;
struct NativeWebKeyboardEvent;


// Returns a java KeyEvent from a KeyEvent, NULL if it fails.
jobject KeyEventFromNative(const NativeWebKeyboardEvent& event);

// Maps a KeyEvent into a NativeWebKeyboardEvent.
NativeWebKeyboardEvent NativeWebKeyboardEventFromKeyEvent(
    JNIEnv* env, jobject java_key_event, int action, int meta_state,
    long event_time, int key_code, bool is_system_key, int unicode_char);

#endif  // CONTENT_BROWSER_ANDROID_IME_HELPER_H_
