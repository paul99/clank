// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/chrome_desktop_impl.h"

#include "base/command_line.h"
#include "base/file_path.h"
#include "base/process.h"
#include "base/process_util.h"
#include "base/string_number_conversions.h"
#include "chrome/test/chromedriver/chrome_finder.h"
#include "chrome/test/chromedriver/net/sync_websocket_impl.h"
#include "chrome/test/chromedriver/net/url_request_context_getter.h"
#include "chrome/test/chromedriver/status.h"

ChromeDesktopImpl::ChromeDesktopImpl(
    URLRequestContextGetter* context_getter,
    int port,
    const SyncWebSocketFactory& socket_factory)
    : ChromeImpl(context_getter, port, socket_factory) {}

ChromeDesktopImpl::~ChromeDesktopImpl() {
  base::CloseProcessHandle(process_);
}

Status ChromeDesktopImpl::Launch(const base::FilePath& chrome_exe,
                                 const std::string& landing_url) {
  base::FilePath program = chrome_exe;
  if (program.empty()) {
    if (!FindChrome(&program))
      return Status(kUnknownError, "cannot find Chrome binary");
  }

  CommandLine command(program);
  command.AppendSwitchASCII("remote-debugging-port",
                            base::IntToString(GetPort()));
  command.AppendSwitch("no-first-run");
  command.AppendSwitch("enable-logging");
  command.AppendSwitchASCII("logging-level", "1");
  if (!user_data_dir_.CreateUniqueTempDir())
    return Status(kUnknownError, "cannot create temp dir for user data dir");
  command.AppendSwitchPath("user-data-dir", user_data_dir_.path());
  command.AppendArg(landing_url);

  base::LaunchOptions options;
  if (!base::LaunchProcess(command, options, &process_))
    return Status(kUnknownError, "chrome failed to start");

  Status status = Init();
  if (status.IsError()) {
    Quit();
    return status;
  }
  return Status(kOk);
}

Status ChromeDesktopImpl::Quit() {
  if (!base::KillProcess(process_, 0, true))
    return Status(kUnknownError, "cannot kill Chrome");
  return Status(kOk);
}

