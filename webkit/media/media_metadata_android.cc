// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/media/media_metadata_android.h"

namespace webkit_glue {

MediaMetadataAndroid::MediaMetadataAndroid()
    : width_(0),
      height_(0),
      duration_(0.0f),
      current_time_(0.0f),
      paused_(false) {
}

MediaMetadataAndroid::MediaMetadataAndroid(int width,
                                           int height,
                                           float duration,
                                           float current_time,
                                           bool paused)
  : width_(width),
    height_(height),
    duration_(duration),
    current_time_(current_time),
    paused_(paused) {
}

MediaMetadataAndroid::~MediaMetadataAndroid() {
}

}  // namespace webkit_glue
