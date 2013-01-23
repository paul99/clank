// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_resource/json_asynchronous_unpacker.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/web_resource/web_resource_service.h"
#include "chrome/common/web_resource/web_resource_unpacker.h"

#if defined(OS_ANDROID)
#include "content/public/browser/browser_thread.h"
#else
#include "base/threading/worker_pool.h"
#endif

// This class coordinates a web resource unpack and parse task which is run by a
// WorkerPool (iOS) / CACHE (Android) thread.
// Results are sent back to this class and routed to the WebResourceService.
class JSONAsynchronousUnpackerImpl
    : public base::RefCountedThreadSafe<JSONAsynchronousUnpackerImpl>,
      public JSONAsynchronousUnpacker {
 public:
  explicit JSONAsynchronousUnpackerImpl(
      JSONAsynchronousUnpackerDelegate* delegate)
      : JSONAsynchronousUnpacker(delegate) {
  }

  void Start(const std::string& json_data) {
    WebResourceUnpacker* unpacker = new WebResourceUnpacker(json_data);
    // TODO(aruslan): this should use WorkerPool, but that doesn't work.
    // Note that this code is a m18-specific workaround.
#if defined(OS_ANDROID)
    content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::CACHE, FROM_HERE,
#else
    base::WorkerPool::PostTaskAndReply(FROM_HERE,
#endif
        base::Bind(&JSONAsynchronousUnpackerImpl::Unpack,
                   this,
                   base::Unretained(unpacker)),
        base::Bind(&JSONAsynchronousUnpackerImpl::OnUnpackFinished,
                   this,
                   base::Owned(unpacker)));
  }

  void Unpack(WebResourceUnpacker* unpacker) {
    unpacker->Run();
  }

  void OnUnpackFinished(WebResourceUnpacker* unpacker) {
    if (!delegate_)
      return;

    if (unpacker->error_message().empty())
      delegate_->OnUnpackFinished(*unpacker->parsed_json());
    else
      delegate_->OnUnpackError(unpacker->error_message());
  }
};

JSONAsynchronousUnpacker* JSONAsynchronousUnpacker::Create(
    JSONAsynchronousUnpackerDelegate* delegate) {
  return new JSONAsynchronousUnpackerImpl(delegate);
}
