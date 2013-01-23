// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/sandboxed_process_launcher.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/logging.h"
#include "jni/sandboxed_process_launcher_jni.h"

using base::android::AttachCurrentThread;
using base::android::ToJavaArrayOfStrings;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;
using content::browser::android::StartSandboxedProcessCallback;

static void OnSandboxedProcessStarted(JNIEnv*,
                                      jclass,
                                      jint jcallback,
                                      jint handle) {
  StartSandboxedProcessCallback* callback =
      reinterpret_cast<StartSandboxedProcessCallback*>(jcallback);
  if (handle)
    callback->Run(static_cast<base::ProcessHandle>(handle));
  delete callback;
}

namespace content {
namespace browser {
namespace android {

void StartSandboxedProcess(
    const CommandLine::StringVector& argv,
    int ipc_fd,
    int minidump_fd,
    int chrome_pak_fd,
    int locale_pak_fd,
    const StartSandboxedProcessCallback& callback) {
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  // Create the Command line String[]
  ScopedJavaLocalRef<jobjectArray> j_argv = ToJavaArrayOfStrings(env, argv);

  Java_SandboxedProcessLauncher_start(env,
          base::android::GetApplicationContext(),
          static_cast<jobjectArray>(j_argv.obj()),
          static_cast<jint>(ipc_fd),
          static_cast<jint>(minidump_fd),
          static_cast<jint>(chrome_pak_fd),
          static_cast<jint>(locale_pak_fd),
          reinterpret_cast<jint>(new StartSandboxedProcessCallback(callback)));
}

void StopSandboxedProcess(base::ProcessHandle handle) {
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);
  Java_SandboxedProcessLauncher_stop(env, static_cast<jint>(handle));
}

bool RegisterSandboxedProcessLauncher(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace android
}  // namespace browser
}  // namespace content
