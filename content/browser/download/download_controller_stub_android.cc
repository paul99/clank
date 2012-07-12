// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_controller_stub_android.h"

// static
DownloadControllerAndroid* DownloadControllerAndroid::GetInstance() {
  return DownloadControllerStubAndroid::GetInstance();
}

// static
DownloadControllerStubAndroid* DownloadControllerStubAndroid::GetInstance() {
  return Singleton<DownloadControllerStubAndroid>::get();
}

DownloadControllerStubAndroid::DownloadControllerStubAndroid() {
}

DownloadControllerStubAndroid::~DownloadControllerStubAndroid() {
}

void DownloadControllerStubAndroid::NewGetDownload(DownloadInfoAndroid info) {
}

void DownloadControllerStubAndroid::NewPostDownload(
    DownloadInfoAndroid info, const PostDownloadOptionsCallback& callback) {
}

void DownloadControllerStubAndroid::UpdatePostDownloadPath(
    int download_id, FilePath new_path) {
}

void DownloadControllerStubAndroid::OnPostDownloadUpdated(
    const ProgressUpdates& progress_updates) {
}

void DownloadControllerStubAndroid::OnPostDownloadCompleted(
    int download_id, int64 total_bytes, bool successful) {
}
