// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ANDROID_MEMORY_THRESHILD_H_
#define CONTENT_RENDERER_ANDROID_MEMORY_THRESHILD_H_

#include <jni.h>
#include <sys/types.h>

namespace content {
namespace renderer {
namespace android {

size_t GetMemoryThresholdMB();

bool RegisterMemoryThreshold(JNIEnv* env);

}  // namespace content
}  // namespace renderer
}  // namespace android

#endif  // CONTENT_RENDERER_ANDROID_MEMORY_THRESHILD_H_
