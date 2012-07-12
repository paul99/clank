// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_info_android.h"

#include "content/browser/download/download_create_info.h"
#include "content/browser/download/download_request_handle.h"

DownloadInfoAndroid::DownloadInfoAndroid(
    const DownloadCreateInfo& info,
    const DownloadRequestHandle& request_handle)
    : child_id(request_handle.child_id_),
      render_view_id(request_handle.render_view_id_),
      request_id(request_handle.request_id_),
      download_id(info.download_id.local()),
      total_bytes(info.total_bytes),
      referrer_url(info.referrer_url),
      content_disposition(info.content_disposition),
      mime_type(info.original_mime_type),
      save_info(info.save_info),
      should_download(false) {
  DCHECK(info.received_bytes == 0);
  if (!info.url_chain.empty()) {
    original_url = info.url_chain.front();
    url = info.url_chain.back();
  }
}

DownloadInfoAndroid::DownloadInfoAndroid()
    : child_id(-1),
      render_view_id(-1),
      request_id(-1),
      download_id(-1),
      total_bytes(0),
      should_download(false) {
}

DownloadInfoAndroid::~DownloadInfoAndroid() {
}
