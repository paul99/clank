// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_CHILD_PROCESS_LAUNCHER_H_
#define CONTENT_BROWSER_ANDROID_CHILD_PROCESS_LAUNCHER_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/command_line.h"
#include "base/process.h"

namespace content {
namespace browser {
namespace android {

class SandboxedProcessClient {
 public:
  // Called back when the process was created with the process pid, or 0
  // if the process could not be created.
  virtual void OnSandboxedProcessStarted(base::ProcessHandle handle) = 0;
};

// Starts a process as a sandboxed process spawned by the Android
// ActivityManager.
// The connection object returned may be used with a subsequent call to
// CancelStartSandboxedProcess()
base::android::ScopedJavaLocalRef<jobject> StartSandboxedProcess(
    const CommandLine::StringVector& argv,
    int ipc_fd,
    int crash_fd,
    SandboxedProcessClient* client);

void CancelStartSandboxedProcess(
    const base::android::JavaRef<jobject>& connection);

// Stops a sandboxed process based on the handle returned form
// StartSandboxedProcess.
void StopSandboxedProcess(base::ProcessHandle handle);

// Registers JNI methods, this must be called before any other methods in this
// file.
bool RegisterSandboxedProcessLauncher(JNIEnv* env);

}  // namespace android
}  // namespace browser
}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_CHILD_PROCESS_LAUNCHER_H_
