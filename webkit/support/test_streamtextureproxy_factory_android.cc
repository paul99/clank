// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/support/test_streamtextureproxy_factory_android.h"

// static
namespace webkit_media {

StreamTextureProxyFactory* StreamTextureProxyFactory::Create(
    int view_id, int player_id) {
  return new TestStreamTextureProxyFactory();
}

} // namespace webkit_media

TestStreamTextureProxyFactory::TestStreamTextureProxyFactory() {
}
