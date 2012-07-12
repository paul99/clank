// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/find_match_rect_android.h"

FindMatchRect::FindMatchRect()
    : left(0.0f),
      top(0.0f),
      right(0.0f),
      bottom(0.0f) {
}

FindMatchRect::FindMatchRect(float left, float top, float right, float bottom)
    : left(left),
      top(top),
      right(right),
      bottom(bottom) {
}
