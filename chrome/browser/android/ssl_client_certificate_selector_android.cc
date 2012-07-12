// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "chrome/browser/ssl_client_certificate_selector.h"
#include "ui/gfx/native_widget_types.h"

namespace browser {

void ShowSSLClientCertificateSelector(
    gfx::NativeWindow parent,
    net::SSLCertRequestInfo* cert_request_info,
    SSLClientAuthHandler* delegate) {
  NOTIMPLEMENTED();
}

}  // namespace browser
