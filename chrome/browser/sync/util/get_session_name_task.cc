// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/util/get_session_name_task.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/message_loop_proxy.h"
#include "base/sys_info.h"

#if defined(OS_LINUX)
#include "base/posix_util.h"
#elif defined(OS_WIN)
#include <windows.h>
#elif defined(OS_ANDROID)
#include "chrome/browser/android/device_utils.h"
#endif

namespace browser_sync {

GetSessionNameTask::GetSessionNameTask(
    const GetSessionNameTask::OnSessionNameInitialized& callback)
    : callback_(callback),
      thread_(base::MessageLoopProxy::current()) {
}

GetSessionNameTask::~GetSessionNameTask() {}

void GetSessionNameTask::GetSessionNameAsync() {
#if defined(OS_CHROMEOS)
  std::string session_name = "Chromebook";
#elif defined(OS_LINUX)
  std::string session_name = base::GetLinuxDistro();
#elif defined(OS_MACOSX)
  std::string session_name = GetSessionNameTask::GetHardwareModelName();
#elif defined(OS_WIN)
  std::string session_name = GetSessionNameTask::GetComputerName();
#elif defined(OS_ANDROID)
  // If this ever changes, make sure you also change in Android Java-side:
  // com.google.android.apps.chrome.snapshot.SnapshotSettings#generateCpsPrinterName.
  std::string session_name = base::android::GetModel();
#else
  std::string session_name;
#endif

  if (session_name == "Unknown" || session_name.empty())
    session_name = base::SysInfo::OperatingSystemName();

  thread_->PostTask(FROM_HERE,
                    base::Bind(&GetSessionNameTask::InvokeCallback,
                               this,
                               session_name));
}

void GetSessionNameTask::InvokeCallback(const std::string& session_name) {
  callback_.Run(session_name);
}

#if defined(OS_WIN)
// static
std::string GetSessionNameTask::GetComputerName() {
  char computer_name[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD size = sizeof(computer_name);
  if (GetComputerNameA(computer_name, &size))
    return computer_name;
  return std::string();
}
#endif

}  // namespace browser_sync
