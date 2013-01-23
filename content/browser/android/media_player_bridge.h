// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_MEDIA_PLAYER_BRIDGE_H_
#define CONTENT_BROWSER_ANDROID_MEDIA_PLAYER_BRIDGE_H_

#include <jni.h>
#include <map>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer.h"

class MediaPlayerListener;
class MediaResourceManagerAndroid;

namespace net {
class CookieStore;
class CookiesRetriever;
}

namespace media {
class MediaPlayerBridgeManager;
}

class MediaPlayerBridge : public base::SupportsWeakPtr<MediaPlayerBridge> {
 public:
  // Callback when error happens. Args: error type.
  typedef base::Callback<void(int, int)> MediaErrorCB;

  // Callback when video size has changed. Args: width, height.
  typedef base::Callback<void(int, int, int)> VideoSizeChangedCB;

  // Callback when buffering has changed. Args: percentage of the media.
  typedef base::Callback<void(int, int)> BufferingUpdateCB;

  // Callback when player got prepared. Args: duration of the media.
  typedef base::Callback<void(int, int)> MediaPreparedCB;

  // Callbacks when seek completed. Args: current time.
  typedef base::Callback<void(int, int)> SeekCompleteCB;

  // Callbacks when playback completed.
  typedef base::Callback<void(int)> PlaybackCompleteCB;

  // Callback when time update messages need to be sent. Args: current time.
  typedef base::Callback<void(int, int)> TimeUpdateCB;

  MediaPlayerBridge(int player_id,
                    const std::string& url,
                    const std::string& first_party_for_cookies,
                    net::CookiesRetriever* cookies_retriever,
                    bool hide_url_log,
                    media::MediaPlayerBridgeManager* manager,
                    const MediaErrorCB& media_error_cb,
                    const VideoSizeChangedCB& video_size_changed_cb,
                    const BufferingUpdateCB& buffering_update_cb,
                    const MediaPreparedCB& media_prepared_cb,
                    const PlaybackCompleteCB& playback_complete_cb,
                    const SeekCompleteCB& seek_complete_cb,
                    const TimeUpdateCB& time_update_cb);
  ~MediaPlayerBridge();

  typedef std::map<std::string, std::string> HeadersMap;

  void SetVideoSurface(jobject surface, bool fullscreen);
  void SetVideoSurfaceTexture(jobject surface_texture);

  bool IsPlaying();
  int GetVideoWidth();
  int GetVideoHeight();
  int GetCurrentPosition();
  int GetDuration();
  void Prepare();
  void Start();
  void Pause();
  void SeekTo(int msec);
  void Reset();
  void Release();
  void SetVolume(float leftVolume, float rightVolume);
  void GetMetadata(bool* can_pause,
                   bool* can_seek_forward,
                   bool* can_seek_backward);
  void SetWakeMode(int mode);

  // An internal method that checks for current time routinely and generates
  // time update events.
  void DoTimeUpdate();

  // Called by MediaPlayerListener and mirrored to corresponding callbacks.
  void OnMediaError(int error_type);
  void OnVideoSizeChanged(int width, int height);
  void OnBufferingUpdate(int percent);
  void OnPlaybackComplete();
  void OnSeekComplete();
  void OnMediaPrepared();

  int player_id() { return player_id_; }
  bool can_pause() { return can_pause_; }
  bool can_seek_forward() { return can_seek_forward_; }
  bool can_seek_backward() { return can_seek_backward_; }
  bool prepared() { return prepared_; }
  bool fullscreen() { return fullscreen_; }

  void PrepareWithoutCookies();
  void PrepareWithCookies(const std::string& cookies);

 private:
  void CallVoidMethod(std::string method_name);
  int CallIntMethod(std::string method_name);

  void InitializePlayer();
  void StartInternal();
  void PauseInternal();
  void SeekInternal(int msec);

  // Callbacks when events are received.
  MediaErrorCB media_error_cb_;
  VideoSizeChangedCB video_size_changed_cb_;
  BufferingUpdateCB buffering_update_cb_;
  MediaPreparedCB media_prepared_cb_;
  PlaybackCompleteCB playback_complete_cb_;
  SeekCompleteCB seek_complete_cb_;

  // Callbacks when events are received.
  TimeUpdateCB time_update_cb_;

  bool is_data_source_set_;
  int player_id_;
  bool prepared_;
  bool pending_play_;
  int pending_seek_;
  std::string url_;
  std::string first_party_for_cookies_;
  bool has_cookies_;
  bool hide_url_log_;
  int duration_;
  int width_;
  int height_;
  bool fullscreen_;

  // Meta data about actions can be taken.
  bool can_pause_;
  bool can_seek_forward_;
  bool can_seek_backward_;
  std::string cookies_;

  media::MediaPlayerBridgeManager* manager_;

  scoped_refptr<net::CookiesRetriever> cookies_retriever_;

  // Java MediaPlayer class and instance.
  base::android::ScopedJavaGlobalRef<jclass> j_class_;
  base::android::ScopedJavaGlobalRef<jobject> j_media_player_;

  base::RepeatingTimer<MediaPlayerBridge> time_update_timer_;

  // Listener object that listens to all the media player events.
  scoped_refptr<MediaPlayerListener> listener_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerBridge);
};

#endif  // CONTENT_BROWSER_ANDROID_MEDIA_PLAYER_BRIDGE_H_
