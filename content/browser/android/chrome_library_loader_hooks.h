// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_CHROME_LIBRARY_LOADER_HOOKS_H_
#define CONTENT_BROWSER_ANDROID_CHROME_LIBRARY_LOADER_HOOKS_H_

#include <jni.h>

// Called when the library is loaded, from the UI thread.
jboolean LibraryLoaderEntryHook(JNIEnv* env, jclass clazz,
                                jobjectArray init_command_line);

// Call on exit to delete the AtExitManager which OnLibraryLoadedOnUIThread
// created.
void LibraryLoaderExitHook();

#endif  // CONTENT_BROWSER_ANDROID_CHROME_LIBRARY_LOADER_HOOKS_H_
