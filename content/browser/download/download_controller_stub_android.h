// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_CONTROLLER_STUB_ANDROID_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_CONTROLLER_STUB_ANDROID_H_
#pragma once

#include "base/memory/singleton.h"
#include "content/browser/download/download_controller_android.h"

// Stub implementation for testing purpose.
class DownloadControllerStubAndroid : public DownloadControllerAndroid {
 public:
  static DownloadControllerStubAndroid* GetInstance();

  virtual void NewGetDownload(DownloadInfoAndroid info) OVERRIDE;

  virtual void NewPostDownload(
      DownloadInfoAndroid info,
      const PostDownloadOptionsCallback& callback) OVERRIDE;

  virtual void UpdatePostDownloadPath(int download_id,
                                      FilePath new_path) OVERRIDE;

  virtual void OnPostDownloadUpdated(
      const ProgressUpdates& progress_updates) OVERRIDE;

  // Informs the Java application that the download completed.
  virtual void OnPostDownloadCompleted(int download_id,
                                       int64 total_bytes,
                                       bool successful) OVERRIDE;

 protected:
  virtual ~DownloadControllerStubAndroid();

 private:
  friend struct DefaultSingletonTraits<DownloadControllerStubAndroid>;
  DownloadControllerStubAndroid();

  DISALLOW_COPY_AND_ASSIGN(DownloadControllerStubAndroid);
};

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_CONTROLLER_STUB_ANDROID_H_
