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
      can_pause_(false),
      can_seek_forward_(false),
      can_seek_backward_(false),
      paused_(false) {
}

MediaMetadataAndroid::MediaMetadataAndroid(int width,
                                           int height,
                                           float duration,
                                           float current_time,
                                           bool paused,
                                           bool can_pause,
                                           bool can_seek_forward,
                                           bool can_seek_backward)
  : width_(width),
    height_(height),
    duration_(duration),
    current_time_(current_time),
    can_pause_(can_pause),
    can_seek_forward_(can_seek_forward),
    can_seek_backward_(can_seek_backward),
    paused_(paused) {
}

MediaMetadataAndroid::~MediaMetadataAndroid() {
}

}  // namespace webkit_glue
