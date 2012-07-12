// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_SUPPORT_TEST_STREAMTEXTUREPROXY_FACTORY_ANDROID_H_
#define WEBKIT_SUPPORT_TEST_STREAMTEXTUREPROXY_FACTORY_ANDROID_H_

#include "base/basictypes.h"
#include "webkit/media/streamtextureproxy_factory_android.h"

namespace WebKit {
class WebStreamTextureProxy;
}

class TestStreamTextureProxyFactory
    : public webkit_media::StreamTextureProxyFactory {
 public:
  TestStreamTextureProxyFactory();
  virtual ~TestStreamTextureProxyFactory() { }

  // webkit_media::StreamTextureProxyFactory implementation:
  virtual WebKit::WebStreamTextureProxy* CreateProxy(
      int stream_id, int width, int height) { return NULL; }

  virtual void ReestablishPeer(int stream_id) { }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestStreamTextureProxyFactory);
};

#endif  // WEBKIT_SUPPORT_TEST_STREAMTEXTUREPROXY_FACTORY_ANDROID_H_
