// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_MESSAGING_NATIVE_PROCESS_LAUNCHER_H_
#define CHROME_BROWSER_EXTENSIONS_API_MESSAGING_NATIVE_PROCESS_LAUNCHER_H_

#include "base/process.h"
#include "chrome/browser/extensions/api/messaging/native_message_process_host.h"

namespace base {
class FilePath;
}

namespace extensions {

class NativeProcessLauncher {
 public:
  // Callback that's called after the process has been launched.
  // |native_process_handle| is set to base::kNullProcessHandle in case of a
  // failure.
  typedef base::Callback<void (base::ProcessHandle native_process_handle,
                               base::PlatformFile read_file,
                               base::PlatformFile write_file)> LaunchedCallback;

  static scoped_ptr<NativeProcessLauncher> CreateDefault();

  NativeProcessLauncher() {}
  virtual ~NativeProcessLauncher() {}

  // Launches native host with the specified name asynchronously. |callback| is
  // called after the process has been started. If the launcher is destroyed
  // before the callback is called then the call is canceled and the process is
  // killed if it has been started already.
  virtual void Launch(const std::string& native_host_name,
                      LaunchedCallback callback) const = 0;

 protected:
  static bool LaunchNativeProcess(
      const base::FilePath& path,
      base::ProcessHandle* native_process_handle,
      base::PlatformFile* read_file,
      base::PlatformFile* write_file);

 private:
  DISALLOW_COPY_AND_ASSIGN(NativeProcessLauncher);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_MESSAGING_NATIVE_PROCESS_LAUNCHER_H_
