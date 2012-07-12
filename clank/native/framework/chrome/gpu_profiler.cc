// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/gpu_profiler.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "content/browser/trace_controller.h"
#include "content/browser/trace_subscriber_stdio.h"
#include "jni/gpu_profiler_jni.h"

static jint Init(JNIEnv* env, jobject obj) {
  GpuProfiler* profiler = new GpuProfiler(env, obj);
  return reinterpret_cast<jint>(profiler);
}

class GpuProfiler::Subscriber : public TraceSubscriberStdio {
 public:
  Subscriber(GpuProfiler* profiler, const FilePath& path)
      : TraceSubscriberStdio(path),
      profiler_(profiler) {
  }

  void set_profiler(GpuProfiler* profiler) {
    CHECK(!profiler_);
    profiler_ = profiler;
  }

  virtual void OnEndTracingComplete() {
    TraceSubscriberStdio::OnEndTracingComplete();
    profiler_->OnProfilingStopped();
  }

 private:
  GpuProfiler* profiler_;
};

GpuProfiler::GpuProfiler(JNIEnv* env, jobject obj)
    : weak_java_object_(env, obj) {
}

GpuProfiler::~GpuProfiler() {}

void GpuProfiler::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

bool GpuProfiler::StartProfiling(JNIEnv* env, jobject obj, jstring jfilename) {
  if (subscriber_.get()) {
    return false;
  }
  std::string filename = base::android::ConvertJavaStringToUTF8(env, jfilename);
  subscriber_.reset(new Subscriber(this, FilePath(filename)));
  if (!subscriber_->IsValid()) {
    subscriber_.reset();
    return false;
  }
  return TraceController::GetInstance()->BeginTracing(subscriber_.get());
}

void GpuProfiler::StopProfiling(JNIEnv* env, jobject obj) {
  if (!subscriber_.get()) {
    return;
  }
  TraceController* controller = TraceController::GetInstance();
  if (!controller->EndTracingAsync(subscriber_.get())) {
      LOG(ERROR) << "EndTracingAsync failed, forcing an immediate stop";
      controller->CancelSubscriber(subscriber_.get());
      OnProfilingStopped();
  }
}

void GpuProfiler::OnProfilingStopped() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = weak_java_object_.get(env);
  if (obj.obj()) {
    Java_GpuProfiler_onProfilingStopped(env, obj.obj());
  }
  subscriber_.reset();
}

bool RegisterGpuProfiler(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
