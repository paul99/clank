// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_FIND_MATCH_RECT_ANDROID_H_
#define CONTENT_PUBLIC_COMMON_FIND_MATCH_RECT_ANDROID_H_
#pragma once

struct FindMatchRect {
 public:
  FindMatchRect();
  FindMatchRect(float left, float top, float right, float bottom);
  float left;
  float top;
  float right;
  float bottom;
};

#endif  // CONTENT_PUBLIC_COMMON_FIND_MATCH_RECT_ANDROID_H_
