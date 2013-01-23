// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_COOKIES_RETRIEVER_IMPL_H_
#define CONTENT_BROWSER_ANDROID_COOKIES_RETRIEVER_IMPL_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "googleurl/src/gurl.h"
#include "net/android/cookies_retriever.h"

namespace net {
class URLRequestContextGetter;
}

namespace content {

class BrowserContext;
class ResourceContext;

// This class will be called on the UI thread to retrieve cookies for a
// particular url. The callback also happens on the UI thread.
class CookiesRetrieverImpl : public net::CookiesRetriever {
 public:
  CookiesRetrieverImpl(
    BrowserContext* browser_context, int renderer_id, int routing_id);
  virtual ~CookiesRetrieverImpl();

  // Need to be called on the UI thread.
  virtual void GetCookies(const std::string& url,
                          const std::string& first_party_for_cookies,
                          const GetCookieCB& callback) OVERRIDE;

 private:
  void GetCookiesCallback(
      const GetCookieCB& callback, const std::string& cookies);

  // BrowserContext to retrieve URLRequestContext and ResourceContext.
  BrowserContext* browser_context_;

  // Used to post tasks.
  base::WeakPtrFactory<CookiesRetrieverImpl> weak_this_;

  // Render process id, used to check whether the process can access cookies.
  int renderer_id_;

  // Routing id for the render view, used to check tab specific cookie policy.
  int routing_id_;

  DISALLOW_COPY_AND_ASSIGN(CookiesRetrieverImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_COOKIES_RETRIEVER_IMPL_H_
