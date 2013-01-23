// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_MEDIA_METADATA_ANDROID_H_
#define WEBKIT_GLUE_MEDIA_METADATA_ANDROID_H_
#pragma once

namespace webkit_glue {

// Provides the initial media metadata to ChromeVideoView.
class MediaMetadataAndroid {
 public:
  MediaMetadataAndroid();
  MediaMetadataAndroid(int width,
                       int height,
                       float duration,
                       float current_time,
                       bool paused_);
  virtual ~MediaMetadataAndroid();

  int width() const { return width_; }
  int height() const { return height_; }
  float duration() const { return duration_; }
  float current_time() const { return current_time_; }
  bool paused() const {return paused_; }

  void set_width(int width) { width_ = width; }
  void set_height(int height) { height_ = height; }
  void set_duration(float duration) { duration_ = duration; }
  void set_current_time(float current_time) { current_time_ = current_time; }
  void set_paused(bool paused) {
    paused_ = paused;
  }

private:
  int width_;
  int height_;
  float duration_;
  float current_time_;
  bool paused_;
};

}  // namespace webkit_glue

#endif  // WEBKIT_GLUE_MEDIA_METADATA_ANDROID_H_
