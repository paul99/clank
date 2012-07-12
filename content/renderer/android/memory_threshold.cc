// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/android/memory_threshold.h"

#include "base/android/jni_android.h"
#include "base/logging.h"
#include "jni/memory_threshold_jni.h"

using base::android::AttachCurrentThread;

namespace content {
namespace renderer {
namespace android {

size_t GetMemoryThresholdMB() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  return static_cast<size_t>(Java_MemoryThreshold_getMemoryThresholdMB(env,
      base::android::GetApplicationContext()));
}

bool RegisterMemoryThreshold(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace content
}  // namespace renderer
}  // namespace android
