// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_SURFACE_TEXTURE_LISTENER_H_
#define CONTENT_BROWSER_ANDROID_SURFACE_TEXTURE_LISTENER_H_

#include <jni.h>
#include "base/callback.h"
#include "base/memory/ref_counted.h"

namespace base {
class MessageLoopProxy;
}

class SurfaceTextureListener {
public:
  void Destroy(JNIEnv* env, jobject obj);
  void FrameAvailable(JNIEnv* env, jobject obj);

private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SurfaceTextureListener);

  SurfaceTextureListener(const base::Closure& callback);
  ~SurfaceTextureListener();

  friend class SurfaceTextureBridge;
  // Static factory method for the creation of a SurfaceTextureListener.
  // The native code should not hold any reference to the returned object,
  // but only use it to pass it up to Java for being referenced by a
  // SurfaceTexture instance.
  static jobject CreateSurfaceTextureListener(JNIEnv* env,
                                              const base::Closure& callback);

  base::Closure callback_;

  scoped_refptr<base::MessageLoopProxy> loop_;
};

bool RegisterSurfaceTextureListener(JNIEnv* env);

#endif  // CONTENT_BROWSER_ANDROID_SURFACE_TEXTURE_LISTENER_H_
