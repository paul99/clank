// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_SURFACE_TEXTURE_PEER_H_
#define CONTENT_BROWSER_ANDROID_SURFACE_TEXTURE_PEER_H_

#include <jni.h>

#include "base/process.h"
#include "content/browser/android/surface_texture_bridge.h"

class SurfaceTexturePeer {
 public:
  enum SurfaceTextureTarget {
    // These are used in java so don't change them unless we can
    // share an enum from java and remove this enum.
    SET_GPU_SURFACE_TEXTURE    = 0, // Contains gpu surface_texture_ and id
    SET_VIDEO_SURFACE_TEXTURE  = 1, // contains video surface_texture_ and id
  };

  static SurfaceTexturePeer* GetInstance();

  static void InitInstance(SurfaceTexturePeer* instance);

  // Establish the producer end for the given surface texture in another
  // process.
  virtual void EstablishSurfaceTexturePeer(
      base::ProcessHandle pid,
      SurfaceTextureTarget type,
      scoped_refptr<SurfaceTextureBridge> surface_texture,
      int primary_id,
      int secondary_id) = 0;

 protected:
  SurfaceTexturePeer();
  virtual ~SurfaceTexturePeer();

 private:
  DISALLOW_COPY_AND_ASSIGN(SurfaceTexturePeer);
};

#endif  // CONTENT_BROWSER_ANDROID_SURFACE_TEXTURE_PEER_H_
