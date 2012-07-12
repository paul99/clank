// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_WEBMEDIAPLAYER_DELEGATE_H_
#define WEBKIT_GLUE_WEBMEDIAPLAYER_DELEGATE_H_

namespace webkit_glue {

class WebMediaPlayer;

// An interface to allow a WebMediaPlayerImpl to communicate changes of state
// to objects that need to know.
class WebMediaPlayerDelegate {
 public:
  WebMediaPlayerDelegate() {}
  virtual ~WebMediaPlayerDelegate() {}

  // The specified player started playing media.
  virtual void DidPlay(WebMediaPlayer* player) {}

  // The specified player stopped playing media.
  virtual void DidPause(WebMediaPlayer* player) {}

  // The specified player was destroyed. Do not call any methods on it.
  virtual void PlayerGone(WebMediaPlayer* player) {}
};

}  // namespace webkit_glue

#endif  // WEBKIT_GLUE_WEBMEDIAPLAYER_DELEGATE_H_
