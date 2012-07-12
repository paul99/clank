// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_MEDIA_PLAYER_DELEGATE_ANDROID_H_
#define CONTENT_BROWSER_MEDIA_MEDIA_PLAYER_DELEGATE_ANDROID_H_

#include "base/memory/scoped_ptr.h"
#include "content/browser/android/chrome_video_view.h"
#include "content/public/browser/render_view_host_observer.h"

class MediaPlayerListenerAndroid;

namespace android {
  class Surface;
}

namespace webkit_glue {
class MediaMetadataAndroid;
}

// This class serves as a delegate for mediaplayer so ChromeVideoView can
// directly call this class instead of the real mediaplayer. It also
// forwards all the notifications from the renderer process to the
// ChromeVideoView.
class MediaPlayerDelegateAndroid : public content::RenderViewHostObserver {
 public:
  MediaPlayerDelegateAndroid(RenderViewHost* render_view_host);
  virtual ~MediaPlayerDelegateAndroid();

  // RenderViewHostObserver overrides.
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  virtual void SetJNIListener(MediaPlayerListenerAndroid* listener);

  // Playback controls.
  virtual void Play();
  virtual void Pause();
  virtual void Seek(float seconds);
  virtual void ExitFullscreen(bool release_media_player);

  // Getters of playback state.
  virtual bool Paused() const;
  virtual bool IsPlaying() const;
  virtual float Duration() const;
  virtual float CurrentTime() const;

  // Get resource metadata
  virtual int GetVideoWidth() const;
  virtual int GetVideoHeight() const;
  virtual bool CanPause() const;
  virtual bool CanSeekForward() const;
  virtual bool CanSeekBackward() const;

  int32 route_id() { return routing_id(); }
  int32 player_id() { return player_id_; }
  int32 render_handle() { return render_handle_; }
 private:
  // Create the fullscreen video view.
  void OnCreateFullscreenView(int player_id,
                              webkit_glue::MediaMetadataAndroid metadata);

  // Destroy the fullscreen video view.
  void OnDestroyFullscreenView();

  // Message handlers for the media player notifications.
  void OnNotify(int msg, int ext1, int ext2);
  void OnTimeUpdate(float seconds);
  void OnUpdateMetadata(webkit_glue::MediaMetadataAndroid metadata);

  int32 player_id_;
  int32 render_handle_;

  // Information related to the media.
  bool paused_;
  float duration_;
  int width_;
  int height_;
  float current_time_;

  // Meta data about actions can be taken.
  bool can_pause_;
  bool can_seek_forward_;
  bool can_seek_backward_;

  // The listener that interacts with the ChromeVideoView.
  MediaPlayerListenerAndroid* jni_listener_;

  scoped_ptr<ChromeVideoView> video_view_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerDelegateAndroid);
};

#endif  // CONTENT_BROWSER_MEDIA_MEDIA_PLAYER_DELEGATE_ANDROID_H_
