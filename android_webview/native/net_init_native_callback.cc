// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/net/init_native_callback.h"

#include "android_webview/browser/net/aw_url_request_job_factory.h"
#include "android_webview/native/android_protocol_handler.h"
#include "android_webview/native/cookie_manager.h"
#include "net/url_request/url_request_job_factory.h"

namespace android_webview {
class AwURLRequestJobFactory;

scoped_ptr<net::URLRequestJobFactory> CreateAndroidJobFactoryAndCookieMonster(
    net::URLRequestContext* context,
    scoped_ptr<AwURLRequestJobFactory> job_factory) {
  SetCookieMonsterOnNetworkStackInit(context);
  return CreateAndroidRequestJobFactory(job_factory.Pass());
}

}  // namespace android_webview
