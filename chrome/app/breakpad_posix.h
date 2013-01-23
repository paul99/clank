// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_APP_BREAKPAD_LINUX_H_
#define CHROME_APP_BREAKPAD_LINUX_H_
#pragma once

#include "base/basictypes.h"

extern void InitCrashReporter();
#if defined(OS_ANDROID)
extern void InitNonBrowserCrashReporter(int minidump_fd);
#else
extern void InitNonBrowserCrashReporter();
#endif
bool IsCrashReporterEnabled();

static const size_t kMaxActiveURLSize = 1024;
static const size_t kGuidSize = 32;  // 128 bits = 32 chars in hex.
static const size_t kDistroSize = 128;

struct BreakpadInfo {
  int fd;
  const char* filename;
  unsigned filename_length;
  const char* process_type;
  unsigned process_type_length;
  const char* crash_url;
  unsigned crash_url_length;
  const char* guid;
  unsigned guid_length;
  const char* distro;
  unsigned distro_length;
  bool upload;
  uint64_t process_start_time;
  const char* pid_str;
  unsigned pid_str_length;
};

extern int HandleCrashDump(const BreakpadInfo& info);

#endif  // CHROME_APP_BREAKPAD_LINUX_H_
