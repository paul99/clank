// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_ANDROID_NATIVE_WINDOW_INTERFACE_H_
#define CONTENT_BROWSER_ANDROID_ANDROID_NATIVE_WINDOW_INTERFACE_H_
#pragma once

#include "ui/gfx/gl/gl_surface.h"
#include "ui/gfx/gl/native_window_interface_android.h"

#include <android/native_window.h>

// Hide Android specifics in this object, so we can pass it through
// WebKit more easily.
class AndroidNativeWindow : public gfx::NativeWindowInterface {
 public:
  explicit AndroidNativeWindow(ANativeWindow* window);
  virtual ~AndroidNativeWindow();

  virtual ANativeWindow* GetNativeHandle() const OVERRIDE;

 private:
  ANativeWindow* window_;

  DISALLOW_COPY_AND_ASSIGN(AndroidNativeWindow);
};

#endif  // CONTENT_BROWSER_ANDROID_ANDROID_NATIVE_WINDOW_INTERFACE_H_
