// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_GPU_PROFILER_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_GPU_PROFILER_H_

#include "base/memory/scoped_ptr.h"
#include "content/browser/android/jni_helper.h"

// This class implements the native methods of GpuProfiler.java
class GpuProfiler {
 public:
  GpuProfiler(JNIEnv* env, jobject obj);
  void Destroy(JNIEnv* env, jobject obj);

  // Called on startup after the command line is established; initiates startup
  // profiling if it was requested on the command line.
  static void Init();

  bool StartProfiling(JNIEnv* env, jobject obj, jstring filename);
  void StopProfiling(JNIEnv* env, jobject obj);
  void OnProfilingStopped();

 private:
  ~GpuProfiler();
  class Subscriber;
  JavaObjectWeakGlobalRef weak_java_object_;
  scoped_ptr<Subscriber> subscriber_;

  DISALLOW_COPY_AND_ASSIGN(GpuProfiler);
};

// register this class's native methods through jni
bool RegisterGpuProfiler(JNIEnv* env);

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_GPU_PROFILER_H_
