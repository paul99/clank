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
                       bool paused_,
                       bool can_pause,
                       bool can_seek_forward,
                       bool can_seek_backward);
  virtual ~MediaMetadataAndroid();

  int width() const { return width_; }
  int height() const { return height_; }
  float duration() const { return duration_; }
  float current_time() const { return current_time_; }
  bool paused() const {return paused_; }
  bool can_pause() const { return can_pause_; }
  bool can_seek_forward() const { return can_seek_forward_; }
  bool can_seek_backward() const { return can_seek_backward_; }

  void set_width(int width) { width_ = width; }
  void set_height(int height) { height_ = height; }
  void set_duration(float duration) { duration_ = duration; }
  void set_current_time(float current_time) { current_time_ = current_time; }
  void set_paused(bool paused) {
    paused_ = paused;
  }
  void set_can_pause(bool can_pause) { can_pause_ = can_pause; }
  void set_can_seek_forward(bool can_seek_forward) {
    can_seek_forward_ = can_seek_forward;
  }
  void set_can_seek_backward(bool can_seek_backward) {
    can_seek_backward_ = can_seek_backward;
  }

private:
  int width_;
  int height_;
  float duration_;
  float current_time_;
  bool can_pause_;
  bool can_seek_forward_;
  bool can_seek_backward_;
  bool paused_;
};

}  // namespace webkit_glue

#endif  // WEBKIT_GLUE_MEDIA_METADATA_ANDROID_H_
