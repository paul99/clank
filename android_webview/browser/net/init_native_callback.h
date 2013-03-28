// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_NET_INIT_NATIVE_CALLBACK_H_
#define ANDROID_WEBVIEW_BROWSER_NET_INIT_NATIVE_CALLBACK_H_

namespace net {
class URLRequestContext;
}  // namespace net

namespace android_webview {
class AwURLRequestJobFactory;

// This is called on the IO thread when the network URLRequestContext has been
// initialized but not used. Note that the UI thread is blocked during this
// call.
void OnNetworkStackInitialized(net::URLRequestContext* context,
                               AwURLRequestJobFactory* job_factory);

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_NET_INIT_NATIVE_CALLBACK_H_
