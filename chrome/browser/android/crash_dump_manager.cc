// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/crash_dump_manager.h"

#include <inttypes.h>

#include "base/bind.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process.h"
#include "base/rand_util.h"
#include "base/stringprintf.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"

using content::BrowserThread;

CrashDumpManager::CrashDumpManager() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  notification_registrar_.Add(this,
                              content::NOTIFICATION_RENDERER_PROCESS_CREATED,
                              content::NotificationService::AllSources());
  notification_registrar_.Add(this,
                              content::NOTIFICATION_RENDERER_PROCESS_TERMINATED,
                              content::NotificationService::AllSources());
  notification_registrar_.Add(this,
                              content::NOTIFICATION_RENDERER_PROCESS_CLOSED,
                              content::NotificationService::AllSources());
}

CrashDumpManager::~CrashDumpManager() {
}

int CrashDumpManager::CreateMinidumpFile() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::PROCESS_LAUNCHER));
  FilePath minidump_path;
  if (!file_util::CreateTemporaryFile(&minidump_path))
    return base::kInvalidPlatformFileValue;

  base::PlatformFileError error;
  // We need read permission as the minidump is generated in several phases
  // and needs to be read at some point.
  int flags = base::PLATFORM_FILE_OPEN | base::PLATFORM_FILE_READ |
      base::PLATFORM_FILE_WRITE;
  base::PlatformFile f =
      base::CreatePlatformFile(minidump_path, flags, NULL, &error);
  if (f == base::kInvalidPlatformFileValue) {
    LOG(ERROR) << "Failed to create temporary file, crash won't be reported.";
    return base::kInvalidPlatformFileValue;
  }

  // Associate that file descriptor with its path, we'll need the path when the
  // process terminates.
  {
    // WARNING: the UI process may be blocked on this lock. We should release as
    // soon as possible and cannot do any IO from here.
    base::AutoLock auto_lock(file_to_path_lock_);
    file_to_path_[f] = minidump_path;
  }

  return f;
}

void CrashDumpManager::ProcessMinidump(const MinidumpInfo& minidump) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  // Close the file descriptor, it is still open.
  bool r = base::ClosePlatformFile(minidump.file);
  DCHECK(r) << "Failed to close minidump file descriptor.";

  int64 file_size = 0;
  r = file_util::GetFileSize(minidump.path, &file_size);
  DCHECK(r) << "Failed to retrieve size for minidump " <<
      minidump.path.value();

  if (file_size == 0) {
    // Empty minidump, this process did not crash. Just remove the file.
    r = file_util::Delete(minidump.path, false);
    DCHECK(r) << "Failed to delete temporary minidump file " <<
        minidump.path.value();
    return;
  }

  // We are dealing with a valid minidump. Copy it to the crash report
  // directory from where Java code will upload it later on.
  FilePath crash_dump_dir;
  r = PathService::Get(chrome::DIR_CRASH_DUMPS, &crash_dump_dir);
  if (!r) {
    NOTREACHED() << "Failed to retrieve the crash dump directory.";
    return;
  }

  const uint64 rand = base::RandUint64();
  const std::string filename =
      base::StringPrintf("chromium-renderer-minidump-%016" PRIx64 ".dmp%d",
                         rand, minidump.pid);
  FilePath dest_path = crash_dump_dir.Append(filename);
  r = file_util::Move(minidump.path, dest_path);
  if (!r) {
    LOG(ERROR) << "Failed to move crash dump from " << minidump.path.value() <<
      " to " << dest_path.value();
    file_util::Delete(minidump.path, false);
    return;
  }
  LOG(INFO) << "Crash minidump successfully generated: " <<
      crash_dump_dir.Append(filename).value();
}

void CrashDumpManager::Observe(int type,
                               const content::NotificationSource& source,
                               const content::NotificationDetails& details) {
  switch (type) {
    case content::NOTIFICATION_RENDERER_PROCESS_CREATED: {
      content::RenderProcessHost* rph =
          content::Source<content::RenderProcessHost>(source).ptr();
      int* minidump_fd = content::Details<int>(details).ptr();
      FilePath minidump_path;
      {
        base::AutoLock auto_lock(file_to_path_lock_);
        FileToPath::iterator iter = file_to_path_.find(*minidump_fd);
        if (iter == file_to_path_.end()) {
          LOG(ERROR) << "Failed to find the path of minidump FD.";
          return;
        }
        minidump_path = iter->second;
        file_to_path_.erase(iter);
      }
      MinidumpInfo minidump_info;
      minidump_info.file = *minidump_fd;
      minidump_info.path = minidump_path;
      base::Process child_process(rph->GetHandle());
      minidump_info.pid = child_process.pid();
      rph_to_minidump_info_[rph] = minidump_info;
      break;
    }
    case content::NOTIFICATION_RENDERER_PROCESS_TERMINATED:
      // NOTIFICATION_RENDERER_PROCESS_TERMINATED is sent when the renderer
      // process is cleanly shutdown. However, we need to fallthrough so that
      // we close the minidump_fd we kept open from
      // NOTIFICATION_RENDERER_PROCESS_CREATED.
    case content::NOTIFICATION_RENDERER_PROCESS_CLOSED: {
      content::RenderProcessHost* rph =
          content::Source<content::RenderProcessHost>(source).ptr();
      RPHToMinidumpInfo::iterator iter = rph_to_minidump_info_.find(rph);
      if (iter == rph_to_minidump_info_.end()) {
        // We might get a NOTIFICATION_RENDERER_PROCESS_TERMINATED and a
        // NOTIFICATION_RENDERER_PROCESS_CLOSED when the renderer crashed.
        return;
      }
      BrowserThread::PostTask(
          BrowserThread::IO, FROM_HERE,
          base::Bind(&ProcessMinidump, iter->second));
      rph_to_minidump_info_.erase(iter);
      break;
    }
    default:
      NOTREACHED();
      break;
  }
}
