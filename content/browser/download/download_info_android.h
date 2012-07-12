// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_INFO_ANDROID_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_INFO_ANDROID_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/file_path.h"
#include "content/browser/download/download_types.h"
#include "googleurl/src/gurl.h"

struct DownloadCreateInfo;
class DownloadRequestHandle;

// Used to store all the information about an Android download, which may
// or may not be run in the Chrome network stack.
struct DownloadInfoAndroid {
  explicit DownloadInfoAndroid(
      const DownloadCreateInfo& info,
      const DownloadRequestHandle& request_handle);
  DownloadInfoAndroid();
  ~DownloadInfoAndroid();

  int child_id;
  int render_view_id;
  int request_id;
  int32 download_id;
  int64 total_bytes;
  // The URL from which we are downloading. This is the final URL after any
  // redirection by the server for |original_url_|.
  GURL url;
  // The original URL before any redirection by the server for this URL.
  GURL original_url;
  GURL referrer_url;
  std::string content_disposition;
  // The original mimetype sent from the server.
  std::string mime_type;
  DownloadSaveInfo save_info;

  std::string user_agent;
  std::string cookie;

  // Set by Java application to determine if a Http POST download should
  // continue.
  bool should_download;

  // Default copy constructor is used for passing this struct by value.
};

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_INFO_ANDROID_H_
