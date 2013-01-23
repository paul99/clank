// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_file_manager_android.h"

#include <vector>

#include "base/bind.h"
#include "content/browser/download/base_file.h"
#include "content/browser/download/download_buffer.h"
#include "content/browser/download/download_create_info.h"
#include "content/browser/download/download_info_android.h"
#include "content/browser/download/download_types.h"
#include "content/browser/renderer_host/resource_dispatcher_host.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/global_request_id.h"
#include "content/public/common/url_constants.h"
#include "googleurl/src/gurl.h"
#include "net/base/cookie_options.h"
#include "net/base/cookie_store.h"
#include "net/base/net_errors.h"
#include "net/base/io_buffer.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"

using content::BrowserThread;
using content::DownloadFile;

namespace {
// Notification update period for Http POST downloads.
const int kUpdatePeriodMs = 1000;

}  // namespace

DownloadFileManagerAndroid::DownloadFileManagerAndroid(
    ResourceDispatcherHost *rdh)
  : DownloadFileManager(rdh, NULL) {
}

DownloadFileManagerAndroid::~DownloadFileManagerAndroid() {
}

void DownloadFileManagerAndroid::StartDownload(
    DownloadCreateInfo* info,
    const DownloadRequestHandle& request_handle) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&DownloadFileManagerAndroid::QueryURLRequest, this,
          DownloadInfoAndroid(*info, request_handle)));
  delete info;
}

void DownloadFileManagerAndroid::UpdateDownload(
    DownloadId id, content::DownloadBuffer* buffer) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  scoped_ptr<content::ContentVector> contents(buffer->ReleaseContents());

  BaseFile* download = GetDownloadFile(id);
  for (size_t i = 0; i < contents->size(); ++i) {
    net::IOBuffer* data = (*contents)[i].first;
    const int data_len = (*contents)[i].second;
    if (download)
      download->AppendDataToFile(data->data(), data_len);
    data->Release();
  }
}

void DownloadFileManagerAndroid::CancelDownload(DownloadId id) {
  NOTREACHED() << "Android should never receive cancelled message";
}

void DownloadFileManagerAndroid::OnResponseCompleted(
    DownloadId id,
    InterruptReason reason,
    const std::string& security_info) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  bool successful = (reason == DOWNLOAD_INTERRUPT_REASON_NONE);
  BaseFile* download = GetDownloadFile(id);
  if (download == NULL ) {
    // This happens when DownloadFile::Initialize in CreateDownloadFileOrCancel
    // fails. OnPostDownloadCompleted is called by CreateDownloadFileOrCancel.
    return;
  }

  download->Finish();
  if (successful) {
    download->Detach();
  }
  downloads_.erase(id.local());
  int64 bytes_so_far = download->bytes_so_far();
  delete download;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&DownloadFileManagerAndroid::OnPostDownloadCompleted, this,
          id.local(), bytes_so_far, successful));

  if (downloads_.empty())
    StopUpdateTimer();
}

void DownloadFileManagerAndroid::QueryURLRequest(
    DownloadInfoAndroid info) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // Retrieve URLRequest
  net::URLRequest* request = resource_dispatcher_host_->GetURLRequest(
      content::GlobalRequestID(info.child_id, info.request_id));
  DCHECK(request);

  const GURL& request_url = request->url();
  if ( (request_url.SchemeIs(chrome::kHttpScheme) ||
        request_url.SchemeIs(chrome::kHttpsScheme)) &&
      request->method() == net::HttpRequestHeaders::kGetMethod) {
    // If Http GET request, then retrieve user agent and cookie and start
    // Android download on UI thread.
    request->extra_request_headers().GetHeader(
        net::HttpRequestHeaders::kUserAgent,
        &info.user_agent);

    net::CookieOptions options;
    options.set_include_httponly();

    request->context()->cookie_store()->GetCookiesWithOptionsAsync(
        request_url, options,
        base::Bind(&DownloadFileManagerAndroid::OnCookieResponse, this, info));

    CancelDownloadRequest(info.child_id, info.request_id);
  } else {
    // Ignore download urls with file scheme. A file scheme means that the file
    // is already present on the device, we do not want to redownload the file.
    // This also prevents us from inadvertendly downloading files accessible by
    // chrome to the sdcard.
    if (request_url.SchemeIs(chrome::kFileScheme)) {
      VLOG(2) << "Ignoring local file download: " << request_url;
      CancelDownloadRequest(info.child_id, info.request_id);
    } else {
      // All other downloads are handled directly
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&DownloadFileManagerAndroid::GetPostDownloadOptions, this,
                     info));
    }
  }
}

void DownloadFileManagerAndroid::StartAndroidDownload(
        const DownloadInfoAndroid& info) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DownloadControllerAndroid::GetInstance()->NewGetDownload(info);
}

void DownloadFileManagerAndroid::OnCookieResponse(
        DownloadInfoAndroid info, const std::string& cookie) {
  info.cookie = cookie;
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
          base::Bind(&DownloadFileManagerAndroid::StartAndroidDownload,
              this, info));
}

void DownloadFileManagerAndroid::GetPostDownloadOptions(
    DownloadInfoAndroid info) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  DownloadControllerAndroid* controller =
    DownloadControllerAndroid::GetInstance();
  controller->NewPostDownload(
      info,
      base::Bind(
          &DownloadFileManagerAndroid::PostDownloadOptionsReady, this));
}

void DownloadFileManagerAndroid::PostDownloadOptionsReady(
    DownloadInfoAndroid info) {
  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&DownloadFileManagerAndroid::CreateDownloadFileOrCancel, this,
          info));
}

void DownloadFileManagerAndroid::CreateDownloadFileOrCancel(
    DownloadInfoAndroid info) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));

  if (!info.should_download) {
    CancelDownloadRequest(info.child_id, info.request_id);
    return;
  }

  int uniquifier = DownloadFile::GetUniquePathNumber(info.save_info.file_path);
  if (uniquifier > 0) {
    DownloadFile::AppendNumberToPath(&info.save_info.file_path, uniquifier);
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&DownloadFileManagerAndroid::DownloadPathUniquified, this,
            info.download_id, info.save_info.file_path));
  }

  scoped_ptr<BaseFile> download_file(
      new BaseFile(
          info.save_info.file_path,
          info.url, info.referrer_url,
          0,  // received_bytes
          false,  // calculate_hash
          "",  // hash_state
          info.save_info.file_stream));

  // Android does not support safe browsing so no need to calculate hash.
  if (net::OK != download_file->Initialize()) {
    CancelDownloadRequest(info.child_id, info.request_id);
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&DownloadFileManagerAndroid::OnPostDownloadCompleted, this,
            info.download_id, 0 /* bytes_so_far */, false /* successful */));
    return;
  }

  // TODO(teramerge): We don't have a |download_manager| here but Android isn't
  // multi-profile
  DownloadId global_id(NULL, info.download_id);
  DCHECK(GetDownloadFile(global_id) == NULL);
  downloads_[global_id.local()] = download_file.release();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&DownloadFileManagerAndroid::ResumePostDownload, this,
          info.child_id, info.request_id));
  StartUpdateTimer();
}

void DownloadFileManagerAndroid::DownloadPathUniquified(
    int download_id, FilePath new_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DownloadControllerAndroid::GetInstance()->UpdatePostDownloadPath(
      download_id, new_path);
}

void DownloadFileManagerAndroid::ResumePostDownload(int child_id,
                                                    int request_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // This balances the pause in DownloadResourceHandler::OnResponseStarted.
  resource_dispatcher_host_->PauseRequest(child_id, request_id, false);
}

void DownloadFileManagerAndroid::OnPostDownloadCompleted(
    int download_id, int64 total_bytes, bool successful) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DownloadControllerAndroid::GetInstance()->OnPostDownloadCompleted(
      download_id, total_bytes, successful);
}

void DownloadFileManagerAndroid::StartUpdateTimer() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  if (!update_timer_.IsRunning()) {
    update_timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(kUpdatePeriodMs),
        this, &DownloadFileManagerAndroid::UpdateInProgressDownloads);
  }
}

void DownloadFileManagerAndroid::StopUpdateTimer() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  update_timer_.Stop();
}

void DownloadFileManagerAndroid::UpdateInProgressDownloads() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  DownloadControllerAndroid::ProgressUpdates progress_updates;
  for (DownloadFileMap::iterator i = downloads_.begin();
       i != downloads_.end(); ++i) {
    int id = i->first;
    BaseFile* file = i->second;
    progress_updates[id] = file->bytes_so_far();
  }
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&DownloadFileManagerAndroid::OnPostDownloadUpdated, this,
          progress_updates));
}

BaseFile* DownloadFileManagerAndroid::GetDownloadFile(DownloadId id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  DownloadFileMap::iterator it = downloads_.find(id.local());
  return it == downloads_.end() ? NULL : it->second;
}

void DownloadFileManagerAndroid::CancelDownloadRequest(int render_process_id,
                                                       int request_id) {
  if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    resource_dispatcher_host_->CancelRequest(render_process_id,
                                             request_id,
                                             false);
  } else {
    // If not called on IO thread, then call this method on IO thread.
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&DownloadFileManagerAndroid::CancelDownloadRequest, this,
            render_process_id, request_id));
  }
}

void DownloadFileManagerAndroid::OnPostDownloadUpdated(
    DownloadControllerAndroid::ProgressUpdates progress_updates) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DownloadControllerAndroid::GetInstance()->OnPostDownloadUpdated(
      progress_updates);
}
