// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_CONTROLLER_ANDROID_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_CONTROLLER_ANDROID_H_
#pragma once

#include <string>
#include <map>

#include "base/basictypes.h"
#include "base/callback.h"
#include "content/browser/download/download_info_android.h"


// Responsible for delegating downloads to java side.
class DownloadControllerAndroid {
 public:
  static DownloadControllerAndroid* GetInstance();

  typedef base::Callback<void(DownloadInfoAndroid)> PostDownloadOptionsCallback;

  // map from download_id to bytes_so_far
  typedef std::map<int, int64> ProgressUpdates;

  // Start a new GET download. The request on chrome side is cancelled before
  // this method is called.
  virtual void NewGetDownload(DownloadInfoAndroid info) = 0;

  // Let Java application determine the download options asynchronously. The
  // options are passed back asynchronously through the callback.
  virtual void NewPostDownload(
      DownloadInfoAndroid info,
      const PostDownloadOptionsCallback& callback) = 0;

  // Tells the controller that the provided path name has been uniquified (a
  // number appended) since a file at original path already exists.
  virtual void UpdatePostDownloadPath(int download_id, FilePath new_path) = 0;

  // Notify Java side of download progress
  virtual void OnPostDownloadUpdated(
      const ProgressUpdates& progress_updates) = 0;

  // Informs the Java application that the download completed.
  virtual void OnPostDownloadCompleted(
      int download_id, int64 total_bytes, bool successful) = 0;

 protected:
  virtual ~DownloadControllerAndroid() {}
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_CONTROLLER_ANDROID_H_
