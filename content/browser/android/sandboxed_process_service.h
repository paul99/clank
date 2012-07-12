// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_SANDBOXED_PROCESS_SERVICE_H_
#define CONTENT_BROWSER_ANDROID_SANDBOXED_PROCESS_SERVICE_H_
#pragma once

#include <jni.h>

#include "base/callback.h"
#include "content/browser/android/surface_texture_peer.h"

struct ANativeWindow;

namespace base {
class MessageLoopProxy;
class WaitableEvent;
}

// Asynchronously sets the Surface. This can be called from any thread.
// The Surface will be set to the proper thread based on the type. The
// nature of primary_id and secondary_id depend on the type, see
// sandboxed_process_service.cc.
// This method will call release() on the jsurface object to release
// all the resources. So after calling this method, the caller should
// not use the jsurface object again.
void SetSurfaceAsync(JNIEnv* env,
                     jobject jsurface,
                     SurfaceTexturePeer::SurfaceTextureTarget type,
                     int primary_id,
                     int secondary_id,
                     base::WaitableEvent* completion);

bool RegisterSandboxedProcessService(JNIEnv* env);

void RegisterRenderThread(base::MessageLoopProxy* loop);

typedef base::Callback<void(
    int, int, ANativeWindow*, base::WaitableEvent*)> NativeWindowCallback;
void RegisterNativeWindowCallback(base::MessageLoopProxy* loop,
                                  NativeWindowCallback& callback);

#endif  // CONTENT_BROWSER_ANDROID_SANDBOXED_PROCESS_SERVICE_H_
