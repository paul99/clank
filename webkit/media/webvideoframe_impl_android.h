// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_WEBVIDEOFRAME_IMPL_ANDROID_H_
#define WEBKIT_GLUE_WEBVIDEOFRAME_IMPL_ANDROID_H_

#include "base/basictypes.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebVideoFrame.h"

class WebVideoFrameImplAndroid : public WebKit::WebVideoFrame {
 public:
  WebVideoFrameImplAndroid();
  virtual ~WebVideoFrameImplAndroid();

  // WebKit::WebVideoFrame
  virtual WebVideoFrame::Format format() const;
  virtual unsigned width() const;
  virtual unsigned height() const;
  virtual unsigned planes() const;
  virtual int stride(unsigned plane) const;
  virtual const void* data(unsigned plane) const;
  virtual unsigned textureId() const;

  void setWidth(unsigned width);
  void setHeight(unsigned height);
  void setTextureId(unsigned texture_id);

 private:
  unsigned width_;
  unsigned height_;
  unsigned texture_id_;

  DISALLOW_COPY_AND_ASSIGN(WebVideoFrameImplAndroid);
};

#endif  // WEBKIT_GLUE_WEBVIDEOFRAME_IMPL_ANDROID_H_
