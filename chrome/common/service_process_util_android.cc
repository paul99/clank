// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/service_process_util_posix.h"

#include <signal.h>
#include <unistd.h>

bool CheckServiceProcessReady() {
  NOTIMPLEMENTED();
  return false;
}

IPC::ChannelHandle GetServiceProcessChannel() {
  NOTIMPLEMENTED();
  return NULL;
}

bool ServiceProcessState::TakeSingletonLock() {
  NOTIMPLEMENTED();
  return false;
}

bool ForceServiceProcessShutdown(const std::string& version,
                                 base::ProcessId process_id) {
  if (kill(process_id, SIGTERM) < 0) {
    PLOG(ERROR) << "kill";
    return false;
  }
  return true;
}

bool ServiceProcessState::AddToAutoRun() {
  NOTIMPLEMENTED();
  return false;
}

bool ServiceProcessState::RemoveFromAutoRun() {
  NOTIMPLEMENTED();
  return false;
}

