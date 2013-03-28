// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/nacl_types.h"

namespace nacl {

NaClStartParams::NaClStartParams()
    : validation_cache_enabled(false),
      enable_exception_handling(false) {
}

NaClStartParams::~NaClStartParams() {
}

}  // namespace nacl
