// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_ANDROID_PROTOCOL_ADAPTER_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_ANDROID_PROTOCOL_ADAPTER_H_
#pragma once

#include <memory>

#include "base/android/scoped_java_ref.h"
#include "net/http/http_byte_range.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job.h"

class AndroidProtocolAdapter {
 public:
  static net::URLRequest::ProtocolFactory Factory;

  // Register the protocol factories for all supported Android protocol
  // schemes.
  static bool RegisterProtocols(JNIEnv* env);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(AndroidProtocolAdapter);
};

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_ANDROID_PROTOCOL_ADAPTER_H_
