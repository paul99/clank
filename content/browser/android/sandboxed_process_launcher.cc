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

static void OnSandboxedProcessStarted(JNIEnv*,
                                      jclass,
                                      jint client_context,
                                      jint pid) {
    reinterpret_cast<content::browser::android::SandboxedProcessClient*>(
        client_context)->OnSandboxedProcessStarted(pid);
}

namespace content {
namespace browser {
namespace android {

ScopedJavaLocalRef<jobject> StartSandboxedProcess(
    const CommandLine::StringVector& argv,
    int ipc_fd,
    int crash_fd,
    SandboxedProcessClient* client) {
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  // Create the Command line String[]
  ScopedJavaLocalRef<jobjectArray> j_argv = ToJavaArrayOfStrings(env, argv);

  return Java_SandboxedProcessLauncher_start(env,
          base::android::GetApplicationContext(),
          static_cast<jobjectArray>(j_argv.obj()),
          static_cast<jint>(ipc_fd),
          static_cast<jint>(crash_fd),
          reinterpret_cast<jint>(client));
}

void CancelStartSandboxedProcess(
    const base::android::JavaRef<jobject>& connection) {
  Java_SandboxedProcessLauncher_cancelStart(connection.env(), connection.obj());
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
