// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// DownloadFileManagerAndroid intercepts download request on Android.
//
// For GET requests, the request is cancelled and handed to Android
// DownloadManager.
//
// For POST requests, the request is handled in the Chrome network stack.
// The file name is retrieved from Java application side. There are no re-names
// involved.
//
// TODO(boliu): Create abstract parent class for DownloadFileManager
// once the download refactoring on Chrome side is done. The remove unused
// chrome download files from clank.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_FILE_MANAGER_ANDROID_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_FILE_MANAGER_ANDROID_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/hash_tables.h"
#include "content/browser/download/download_controller_android.h"
#include "content/browser/download/download_file_manager.h"
#include "content/browser/download/interrupt_reasons.h"
#include "net/base/net_errors.h"

using content::DownloadId;

class BaseFile;
class DownloadManager;
class FilePath;
class ResourceDispatcherHost;
struct DownloadCreateInfo;
struct DownloadInfoAndroid;

namespace content {
class DownloadBuffer;
}

namespace net {
  class URLRequest;
}

class DownloadFileManagerAndroid : public DownloadFileManager {
 public:
  explicit DownloadFileManagerAndroid(ResourceDispatcherHost* rdh);

  // Shutdown() and GetNextId() should do the same thing as DownloadFileManager

  // Override DownloadFileManager.
  virtual void StartDownload(
      DownloadCreateInfo* info,
      const DownloadRequestHandle& reqest_handle) OVERRIDE;
  virtual void UpdateDownload(DownloadId id,
                              content::DownloadBuffer* buffer) OVERRIDE;
  virtual void CancelDownload(DownloadId id)  OVERRIDE;
  virtual void OnResponseCompleted(DownloadId id,
                                   InterruptReason reason,
                                   const std::string& security_info) OVERRIDE;

  // Callback function from DownloadControllerAndroid for setting POST download
  // options. This method can be called from any thread.
  void PostDownloadOptionsReady(DownloadInfoAndroid info);

 protected:
  virtual ~DownloadFileManagerAndroid();

 private:
  // Methods for StartDownload() for GET requests
  // StartDownload() => QueryURLRequest() => StartAndroidDownload()

  // Called on IO thread to retrieve the user agent of request. If GET then
  // cancel request and pass information to controller. If POST, then wait
  // for options from controller.
  void QueryURLRequest(DownloadInfoAndroid info);

  // Called on UI thread to start Android download.
  void StartAndroidDownload(const DownloadInfoAndroid& info);

  void OnCookieResponse(DownloadInfoAndroid info, const std::string& cookie);

  // Methods for StartDownload() for POST requests
  // StartDownload() => QueryURLRequest() => GetPostDownloadOptions() =>
  //    *callback* => PostDownloadOptionsReadyFromAnyThread() =>
  //    CreateDownloadFileOrCancel() => ResumePostDownload()

  // Called on UI thread to get the file path to download to.
  void GetPostDownloadOptions(DownloadInfoAndroid info);

  // Called on FILE thread to create the download file or cancel the download
  // depending on the options returned from controller.
  void CreateDownloadFileOrCancel(DownloadInfoAndroid info);

  // Called on UI thread to inform DownloadControllerAndroid that a number has
  // been appended to the provided download path since a file with the path
  // already exists.
  void DownloadPathUniquified(int download_id, FilePath new_path);

  // Called on IO thread to resume the URLRequest.
  void ResumePostDownload(int child_id, int request_id);

  // Methods for OnResponseComplete() for POST requests
  // OnResponseCompleted() => OnPostDownloadCompleted()

  // Called on UI thread to inform DownloadControllerAndroid about
  // completed download. total_bytes is needed since some downloads do not
  // have content length in header.
  void OnPostDownloadCompleted(
      int download_id, int64 total_bytes, bool successful);

  // Timer helpers for updating the UI about the current progress of a download.
  // TODO(boliu): These methods are almost copied verbatim from
  // DownloadFileManager. Merge these when common super class is created.
  void StartUpdateTimer();
  void StopUpdateTimer();
  void UpdateInProgressDownloads();

  // Helper methods

  // Retrieve the download file matching id.
  BaseFile* GetDownloadFile(DownloadId global_id);

  // Cancel the download request. Similar to
  // download_util::CancelDownloadRequest. Can be called from any thread.
  void CancelDownloadRequest(int render_process_id, int request_id);

  // Call DownloadControllerAndroid::OnPostDownloadUpdated on UI thread.
  void OnPostDownloadUpdated(
      DownloadControllerAndroid::ProgressUpdates progress_updates);

  // Only accessed on FILE thread.
  typedef base::hash_map<int, BaseFile*> DownloadFileMap;
  DownloadFileMap downloads_;

  // Schedule periodic updates of the download progress. This timer
  // is controlled from the FILE thread, and posts updates to the UI thread.
  base::RepeatingTimer<DownloadFileManagerAndroid> update_timer_;

  DISALLOW_COPY_AND_ASSIGN(DownloadFileManagerAndroid);
};

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_FILE_MANAGER_ANDROID_H_
