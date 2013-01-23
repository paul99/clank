// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_MEDIA_WEBMEDIAPLAYER_MANAGER_ANDROID_H_
#define WEBKIT_MEDIA_WEBMEDIAPLAYER_MANAGER_ANDROID_H_

#include <map>

#include "webkit/media/webmediaplayer_android.h"

namespace webkit_glue {

// Manager for keeping track of all WebMediaPlayerAndroid objects and
// assigning them IDs..
class WebMediaPlayerManagerAndroid {
 public:
  WebMediaPlayerManagerAndroid();
  virtual ~WebMediaPlayerManagerAndroid();

  // Register and unregister a WebMediaPlayer object.
  virtual int RegisterMediaPlayer(WebMediaPlayerAndroid*);
  virtual void UnregisterMediaPlayer(int player_id);

  WebMediaPlayerAndroid* GetMediaPlayer(int player_id);

  // Release all the media resources on the renderer process.
  void ReleaseMediaResources();

 private:
  // Info for all available WebMediaPlayerAndroid on a page; kept so that
  // we can enumerate them to send updates about tab focus and visibily.
  std::map<int32, WebMediaPlayerAndroid*> media_players_;

  // ID used to create the next media player for passing the Surface.
  int32 next_media_player_id_;

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerManagerAndroid);
};

}  // namespace webkit_glue

#endif  // WEBKIT_MEDIA_WEBMEDIAPLAYER_MANAGER_ANDROID_H_
