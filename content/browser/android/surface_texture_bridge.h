// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_SURFACE_TEXTURE_BRIDGE_H_
#define CONTENT_BROWSER_ANDROID_SURFACE_TEXTURE_BRIDGE_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/callback.h"

class SurfaceTextureBridge {
 public:
  explicit SurfaceTextureBridge(int texture_id);
  ~SurfaceTextureBridge();

  // Set the listener callback, which will be invoked on the same thread that
  // is being called from here for registration.
  // Note: Since callbacks come in from Java objects that might outlive objects
  // being referenced from the callback, the only robust way here is to create
  // the callback from a weak pointer to your object.
  void SetFrameAvailableCallback(const base::Closure& callback);

  void UpdateTexImage();

  void GetTransformMatrix(float mtx[16]);

  void SetDefaultBufferSize(int width, int height);

  int texture_id() const {
    return texture_id_;
  }
  const base::android::JavaRef<jobject>& j_surface_texture() const {
    return j_surface_texture_;
  }

 private:
  const int texture_id_;

  // Java SurfaceTexture class and instance.
  base::android::ScopedJavaGlobalRef<jclass> j_class_;
  base::android::ScopedJavaGlobalRef<jobject> j_surface_texture_;
};

#endif  // CONTENT_BROWSER_ANDROID_SURFACE_TEXTURE_BRIDGE_H_
