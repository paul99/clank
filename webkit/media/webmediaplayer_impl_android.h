// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_MEDIA_WEBMEDIAPLAYER_IMPL_ANDROID_H_
#define WEBKIT_MEDIA_WEBMEDIAPLAYER_IMPL_ANDROID_H_

#include <string>

#include <jni.h>

#include "base/basictypes.h"
#include "base/message_loop.h"
#include "base/message_loop_proxy.h"
#include "base/memory/scoped_ptr.h"
#include "base/utf_string_conversions.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebMediaPlayer.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebSize.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebURL.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebVideoFrame.h"
#include "webkit/media/webmediaplayer_android.h"

class MediaPlayerBridge;

namespace WebKit {
class WebCookieJar;
class WebFrame;
class WebGraphicsContext3D;
class WebStreamTextureProxy;
class WebURL;
}

namespace webkit_media {
class StreamTextureProxyFactory;
}

namespace webkit_glue {

class MediaMetadataAndroid;
class WebMediaPlayerManagerAndroid;
class WebMediaPlayerProxyAndroid;

class WebMediaPlayerImplAndroid : public WebKit::WebMediaPlayer,
                                  public MessageLoop::DestructionObserver,
                                  public WebMediaPlayerAndroid {
 public:
  WebMediaPlayerImplAndroid(WebKit::WebFrame* frame,
                            WebKit::WebMediaPlayerClient* client,
                            WebMediaPlayerManagerAndroid* manager,
                            WebMediaPlayerProxyAndroid* proxy,
                            WebKit::WebGraphicsContext3D* context,
                            int routing_id);
  virtual ~WebMediaPlayerImplAndroid();

  // -------------------------------------------------------------------------
  // WebKit::WebMediaPlayer implementation.
  // Resource loading.
  virtual void load(const WebKit::WebURL& url);
  virtual void cancelLoad();

  // Playback controls.
  virtual void play();
  virtual void pause();
  virtual void seek(float seconds);
  virtual void enterFullscreen();
  virtual void exitFullscreen();
  virtual bool supportsFullscreen() const;
  virtual bool supportsSave() const;
  virtual void setEndTime(float seconds);
  virtual void setRate(float rate);
  virtual void setVolume(float volume);
  virtual void setVisible(bool visible);
  virtual bool setAutoBuffer(bool autoBuffer);
  virtual bool totalBytesKnown();
  virtual const WebKit::WebTimeRanges& buffered();
  virtual float maxTimeSeekable() const;

  // Methods for painting.
  virtual void setSize(const WebKit::WebSize& size);

  virtual void paint(WebKit::WebCanvas* canvas, const WebKit::WebRect& rect);

  // True if the loaded media has a playable video/audio track.
  virtual bool hasVideo() const;
  virtual bool hasAudio() const;

  // Dimensions of the video.
  virtual WebKit::WebSize naturalSize() const;

  // Getters of playback state.
  virtual bool paused() const;
  virtual bool seeking() const;
  virtual float duration() const;
  virtual float currentTime() const;

  // Get rate of loading the resource.
  virtual int32 dataRate() const;

  virtual unsigned long long bytesLoaded() const;
  virtual unsigned long long totalBytes() const;

  // Internal states of loading and network.
  virtual WebKit::WebMediaPlayer::NetworkState networkState() const;
  virtual WebKit::WebMediaPlayer::ReadyState readyState() const;

  virtual bool hasSingleSecurityOrigin() const;
  virtual WebKit::WebMediaPlayer::MovieLoadType movieLoadType() const;

  virtual float mediaTimeForTimeValue(float timeValue) const;

  // Provide statistics.
  virtual unsigned decodedFrameCount() const;
  virtual unsigned droppedFrameCount() const;
  virtual unsigned audioDecodedByteCount() const;
  virtual unsigned videoDecodedByteCount() const;

  // Methods called from VideoLayerChromium. These methods are running on the
  // composite thread if useThreadedCompositor flag is set.
  virtual WebKit::WebVideoFrame* getCurrentFrame();
  virtual void putCurrentFrame(WebKit::WebVideoFrame*);
  virtual WebKit::WebStreamTextureProxy* createStreamTextureProxy();

  // WebMediaPlayerAndroid implementation.
  void OnMediaPrepared(float duration) OVERRIDE;
  void OnPlaybackComplete() OVERRIDE;
  void OnBufferingUpdate(int percentage) OVERRIDE;
  void OnSeekComplete(int msec) OVERRIDE;
  void OnMediaError(int error) OVERRIDE;
  void OnVideoSizeChanged(int width, int height) OVERRIDE;
  void OnSurfaceReleased() OVERRIDE;
  void OnTimeUpdate(int msec) OVERRIDE;
  void OnMediaPlayerPlay() OVERRIDE;
  void OnMediaPlayerPause() OVERRIDE;
  void OnExitFullscreen() OVERRIDE;

  // This function is called by the MediaPlayerManager to pause the video
  // and release the media player and surface texture when we switch tabs.
  // However, the actual GlTexture is not released to keep the video screenshot.
  void ReleaseMediaResources() OVERRIDE;

  MediaMetadataAndroid* GetMetadata();

  // Method inherited from DestructionObserver.
  virtual void WillDestroyCurrentMessageLoop() OVERRIDE;

  int player_id() { return player_id_; }
 private:
  friend class MediaPlayerProxy;
  // In case the media_player_ is deleted, we call this function to get
  // a new media_player_.
  void InitializeMediaPlayer();

  // Ask the media player to load the url and start prepareAsync.
  void LoadUrl(WebKit::WebURL url);

  void PlayInternal();
  void PauseInternal();
  void SeekInternal(float seconds);

  // Helper methods for posting task for setting states and update WebKit.
  void UpdateNetworkState(WebKit::WebMediaPlayer::NetworkState state);
  void UpdateReadyState(WebKit::WebMediaPlayer::ReadyState state);

  void CreateStreamTexture();
  void DestroyStreamTexture();
  // -------------------------------------------------------------------------

  WebKit::WebFrame* const owner_;
  WebKit::WebMediaPlayerClient* const client_;

  // Save the list of buffered time ranges.
  WebKit::WebTimeRanges time_ranges_;

  WebKit::WebSize texture_size_;
  WebKit::WebSize natural_size_;

  // The video frame object used for renderering by WebKit.
  scoped_ptr<WebKit::WebVideoFrame> video_frame_;

  // Message loop for main renderer thread.
  MessageLoop* main_loop_;

  WebMediaPlayerManagerAndroid* manager_;

  // Proxy object that delegates method calls on Render Thread.
  // This object is created on the Render Thread and is only called in the
  // destructor.
  WebMediaPlayerProxyAndroid* proxy_;

  // URL of the media file to be fetched.
  GURL url_;

  // Media duration;
  float duration_;

  // Media duration;
  float time_to_seek_;

  // Internal seek state.
  bool seeking_;

  // Fullscreen mode.
  bool fullscreen_;

  // the current playing time.
  float current_time_;

  // Fake it by self increasing on every OnBufferingUpdate event.
  int64 buffered_bytes_;

  // Player Id assigned by the media player proxy.
  int player_id_;

  // Whether the user has clicked the play button while media player
  // is preparing.
  bool is_playing_;

  bool can_pause_;
  bool can_seek_forward_;
  bool can_seek_backward_;

  // Current states.
  WebKit::WebMediaPlayer::NetworkState network_state_;
  WebKit::WebMediaPlayer::ReadyState ready_state_;

  unsigned int texture_id_;
  unsigned int stream_id_;
  bool needs_reestablish_peer_;

  WebKit::WebGraphicsContext3D* context_;
  scoped_ptr<webkit_media::StreamTextureProxyFactory>
      stream_texture_proxy_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerImplAndroid);
};

}  // namespace webkit_glue

#endif  // WEBKIT_MEDIA_WEBMEDIAPLAYER_IMPL_ANDROID_H_
