// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/media/webvideoframe_impl_android.h"

#include "third_party/WebKit/Source/WebKit/chromium/public/WebVideoFrame.h"

using namespace WebKit;

WebVideoFrameImplAndroid::WebVideoFrameImplAndroid()
    : width_(0),
      height_(0),
      texture_id_(0) {
}

WebVideoFrameImplAndroid::~WebVideoFrameImplAndroid() {}

WebVideoFrame::Format WebVideoFrameImplAndroid::format() const {
  return WebVideoFrame::FormatExternalTexture;
}

unsigned WebVideoFrameImplAndroid::width() const {
  return width_;
}

unsigned WebVideoFrameImplAndroid::height() const {
  return height_;
}

unsigned WebVideoFrameImplAndroid::planes() const {
  return 0;
}

int WebVideoFrameImplAndroid::stride(unsigned plane) const {
  return width();
}

const void* WebVideoFrameImplAndroid::data(unsigned plane) const {
  return NULL;
}

unsigned WebVideoFrameImplAndroid::textureId() const {
  return texture_id_;
}

void WebVideoFrameImplAndroid::setWidth(unsigned width) {
  width_ = width;
}

void WebVideoFrameImplAndroid::setHeight(unsigned height) {
  height_ = height;
}

void WebVideoFrameImplAndroid::setTextureId(unsigned texture_id) {
  texture_id_ = texture_id;
}
