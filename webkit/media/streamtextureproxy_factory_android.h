// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_MEDIA_STREAMTEXTUREPROXY_FACTORY_ANDROID_H_
#define WEBKIT_MEDIA_STREAMTEXTUREPROXY_FACTORY_ANDROID_H_

#include "third_party/WebKit/Source/WebKit/chromium/public/WebStreamTextureProxy.h"

namespace webkit_media {

class StreamTextureProxyFactory {
 public:
  virtual ~StreamTextureProxyFactory() { }

  static StreamTextureProxyFactory* Create(int view_id, int player_id);

  virtual WebKit::WebStreamTextureProxy* CreateProxy(
      int stream_id, int width, int height) = 0;

  virtual void ReestablishPeer(int stream_id) = 0;

 protected:
  StreamTextureProxyFactory() { }
};

} // namespace webkit_media

#endif  // WEBKIT_MEDIA_STREAMTEXTUREPROXY_FACTORY_ANDROID_H_
