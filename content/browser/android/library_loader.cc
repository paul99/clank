// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/logging.h"
#include "content/browser/android/chrome_library_loader_hooks.h"
#include "content/browser/android/jni_helper.h"

using base::android::GetClass;;
using base::android::ScopedJavaLocalRef;

namespace {

union UnionJNIEnvToVoid {
  JNIEnv* env;
  void* venv;
};

bool RegisterLibraryLoaderEntryHook(JNIEnv* env,
                                    void* LibraryLoaderEntryHookPtr) {
  // TODO(bulach): use the jni generator once we move jni_helper methods here.
  const JNINativeMethod kMethods[] = {
      { "nativeLibraryLoadedOnMainThread", "([Ljava/lang/String;)Z",
        LibraryLoaderEntryHookPtr },
  };
  const int kMethodsSize = arraysize(kMethods);
  const char* const kLibraryLoaderPath = "org/chromium/content/browser/LibraryLoader";
  ScopedJavaLocalRef<jclass> clazz = GetClass(env, kLibraryLoaderPath);

  if (env->RegisterNatives(clazz.obj(),
                           kMethods,
                           kMethodsSize) < 0) {
    return false;
  }
  return true;
}

}  // namespace

// This is called by the VM when the shared library is first loaded.
// We're loading in a separate java thread to avoid blocking the main UI thread.
// We'll do most of the the initialization later on during
// OnLibraryLoadedOnUIThread
CLANK_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  UnionJNIEnvToVoid uenv;
  uenv.venv = NULL;
  if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_4) != JNI_OK) {
    return -1;
  }
  JNIEnv* env = uenv.env;
  if (!RegisterLibraryLoaderEntryHook(
      env, reinterpret_cast<void*>(LibraryLoaderEntryHook))) {
    return -1;
  }
  return JNI_VERSION_1_4;
}
