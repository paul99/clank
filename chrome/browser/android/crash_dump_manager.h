// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CRASH_DUMP_MANAGER_H
#define CHROME_BROWSER_ANDROID_CRASH_DUMP_MANAGER_H

#include <map>

#include "base/file_path.h"
#include "base/memory/singleton.h"
#include "base/platform_file.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace content {
class RenderProcessHost;
}

// This class manages the crash minidumps.
// On Android, because of process isolation, each renderer process runs with a
// different UID. As a result, we cannot generate the minidumps in the browser
// (as the browser process does not have access to some system files for the
// crashed process). So the minidump is generated in the renderer process.
// Since the isolated process cannot open files, we provide it on creation with
// a file descriptor where to write the minidump in the event of a crash.
// This class creates these file descriptors and associates them with render
// processes and take the appropriate action when the render process terminates.
class CrashDumpManager : public content::NotificationObserver {
 public:
  // Should be created on the UI thread.
  CrashDumpManager();
  virtual ~CrashDumpManager();

  // Returns a file descriptor that the render process associated with |rph|
  // should use to generate minidumps.
  int CreateMinidumpFile();

 private:
  struct MinidumpInfo {
    MinidumpInfo() : file(base::kInvalidPlatformFileValue) {}
    base::PlatformFile file;
    FilePath path;
    int pid;
  };

  static void ProcessMinidump(const MinidumpInfo& minidump);

  // NotificationObserver implementation:
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  content::NotificationRegistrar notification_registrar_;

  typedef std::map<content::RenderProcessHost*, MinidumpInfo> RPHToMinidumpInfo;
  RPHToMinidumpInfo rph_to_minidump_info_;

  typedef std::map<base::PlatformFile, FilePath> FileToPath;
  // This map should only be accessed with its lock aquired, as it is accessed
  // from the PROCESS_LAUNCHER and UI threads.
  FileToPath file_to_path_;
  base::Lock file_to_path_lock_;

  DISALLOW_COPY_AND_ASSIGN(CrashDumpManager);
};

#endif  // CHROME_BROWSER_ANDROID_CRASH_DUMP_MANAGER_H
