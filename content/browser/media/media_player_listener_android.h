// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_MEDIA_PLAYER_LISTENER_ANDROID_H_
#define CONTENT_BROWSER_MEDIA_MEDIA_PLAYER_LISTENER_ANDROID_H_

class MediaPlayerListenerAndroid {
 public:
  virtual void Notify(int msg, int ext1, int ext2) = 0;
  virtual ~MediaPlayerListenerAndroid() {}
};

#endif  // CONTENT_BROWSER_MEDIA_MEDIA_PLAYER_LISTENER_ANDROID_H_
