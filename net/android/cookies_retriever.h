// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_ANDROID_COOKIES_RETRIEVER_H_
#define NET_ANDROID_COOKIES_RETRIEVER_H_

#include <string>

#include "base/callback.h"

namespace net {

class CookiesRetriever :
   public base::RefCountedThreadSafe<CookiesRetriever>{
 public:
  typedef base::Callback<void(const std::string&)> GetCookieCB;

  virtual ~CookiesRetriever() {};

  virtual void GetCookies(const std::string& url,
                          const std::string& first_party_for_cookies,
                          const GetCookieCB& callback) = 0;
};

}  // namespace net

#endif  // NET_ANDROID_COOKIES_RETRIEVER_H_
