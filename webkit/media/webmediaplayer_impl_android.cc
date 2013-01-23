// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/media/webmediaplayer_impl_android.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "net/base/mime_util.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebCookieJar.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebGraphicsContext3D.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebKit.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebMediaPlayerClient.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebRect.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "webkit/media/media_metadata_android.h"
#include "webkit/media/media_player_constants_android.h"
#include "webkit/media/streamtextureproxy_factory_android.h"
#include "webkit/media/webmediaplayer_manager_android.h"
#include "webkit/media/webmediaplayer_proxy_android.h"
#include "webkit/media/webmediaplayer_util.h"
#include "webkit/media/webvideoframe_impl_android.h"

using WebKit::WebCanvas;
using WebKit::WebFrame;
using WebKit::WebMediaPlayerClient;
using WebKit::WebMediaPlayer;
using WebKit::WebRect;
using WebKit::WebSize;
using WebKit::WebTimeRanges;
using WebKit::WebURL;
using WebKit::WebVideoFrame;
using WebKit::WebStreamTextureProxy;

namespace webkit_glue {

WebMediaPlayerImplAndroid::WebMediaPlayerImplAndroid(
    WebFrame* frame,
    WebMediaPlayerClient* client,
    WebMediaPlayerManagerAndroid* manager,
    webkit_glue::WebMediaPlayerProxyAndroid* proxy,
    WebKit::WebGraphicsContext3D* context,
    int routing_id)
    : owner_(frame),
      client_(client),
      main_loop_(MessageLoop::current()),
      manager_(manager),
      proxy_(proxy),
      duration_(0),
      time_to_seek_(0),
      seeking_(false),
      fullscreen_(false),
      current_time_(0),
      buffered_bytes_(0),
      is_playing_(false),
      network_state_(WebMediaPlayer::Empty),
      ready_state_(WebMediaPlayer::HaveNothing),
      texture_id_(0),
      stream_id_(0),
      needs_reestablish_peer_(true),
      context_(context) {
  main_loop_->AddDestructionObserver(this);
  player_id_ = manager_->RegisterMediaPlayer(this);
  video_frame_.reset(new WebVideoFrameImplAndroid());
  stream_texture_proxy_factory_.reset(
      webkit_media::StreamTextureProxyFactory::Create(
          routing_id, player_id_));
  if (context_)
    CreateStreamTexture();
}

WebMediaPlayerImplAndroid::~WebMediaPlayerImplAndroid() {
  if (stream_id_)
    DestroyStreamTexture();

  if (manager_)
    manager_->UnregisterMediaPlayer(player_id_);

  if (proxy_)
    proxy_->DestroyPlayer(player_id_);

  if (main_loop_)
    main_loop_->RemoveDestructionObserver(this);
}

void WebMediaPlayerImplAndroid::enterFullscreen() {
  if (proxy_ && !fullscreen_) {
    proxy_->EnterFullscreen(player_id_);
    fullscreen_ = true;
  }
}

void WebMediaPlayerImplAndroid::exitFullscreen() {
  fullscreen_ = false;

  if (proxy_) {
    proxy_->LeaveFullscreen(player_id_);
  }

  if (!paused()) {
    // We had the fullscreen surface connected to Android MediaPlayer,
    // so reconnect our surface texture for embedded playback.
    stream_texture_proxy_factory_->ReestablishPeer(stream_id_);
  } else {
    // Set |needs_reestablish_peer_| to true so that surface texture will be
    // passed to the media player when playInternal() gets called.
    needs_reestablish_peer_ = true;
  }

  client_->repaint();
}

void WebMediaPlayerImplAndroid::load(const WebURL& url) {
  url_ = url;

  if (proxy_) {
    GURL first_party_url = owner_->document().firstPartyForCookies();
    proxy_->Load(player_id_, url_.spec(), first_party_url.spec());
    if (proxy_->IsFullscreenMode() && proxy_->CanEnterFullscreen(player_id_)) {
      proxy_->EnterFullscreen(player_id_);
      fullscreen_ = true;
    }
  }
  // Set the initial duration value to 100 so that user can still
  // touch the seek bar to perform seek.
  duration_ = 100;

  // Pretend everything has been loaded so that webkit can
  // still call play() and seek().
  UpdateReadyState(WebMediaPlayer::HaveMetadata);
  UpdateReadyState(WebMediaPlayer::HaveEnoughData);
}

void WebMediaPlayerImplAndroid::cancelLoad() {
  NOTIMPLEMENTED();
}

void WebMediaPlayerImplAndroid::play() {
  if (hasVideo() && !fullscreen_) {
    if (needs_reestablish_peer_) {
      stream_texture_proxy_factory_->ReestablishPeer(stream_id_);
      needs_reestablish_peer_ = false;
    }
  }

  if (paused() && proxy_) {
    is_playing_ = true;
    proxy_->Start(player_id_);
  }
}

void WebMediaPlayerImplAndroid::pause() {
  if (proxy_) {
    is_playing_ = false;
    proxy_->Pause(player_id_);
  }
}

void WebMediaPlayerImplAndroid::seek(float seconds) {
  time_to_seek_ = seconds;
  if (proxy_) {
    seeking_ = true;
    proxy_->Seek(player_id_,
        ConvertSecondsToTimestamp(seconds).InMilliseconds());
  }
}

bool WebMediaPlayerImplAndroid::supportsFullscreen() const {
  return true;
}

bool WebMediaPlayerImplAndroid::supportsSave() const {
  return false;
}

void WebMediaPlayerImplAndroid::setEndTime(float seconds) {
  // Deprecated.
}

void WebMediaPlayerImplAndroid::setRate(float rate) {
  if (rate != 1.0f) {
    NOTIMPLEMENTED();
  }
}

void WebMediaPlayerImplAndroid::setVolume(float volume) {
  NOTIMPLEMENTED();
}

void WebMediaPlayerImplAndroid::setVisible(bool visible) {
  // Deprecated.
}

bool WebMediaPlayerImplAndroid::setAutoBuffer(bool autoBuffer) {
  NOTIMPLEMENTED();
  return false;
}

bool WebMediaPlayerImplAndroid::totalBytesKnown() {
  NOTIMPLEMENTED();
  return false;
}

bool WebMediaPlayerImplAndroid::hasVideo() const {
  // We don't know if the file has video until the player
  // is prepared. While it is not prepared we fall back on the
  // mime-type. There may be no mime-type on a redirect URL
  // so we conservatively assume it contains video (so that
  // supportsFullscreen will be true and entering fullscreen
  // will not fail).
  if (natural_size_.width != 0 && natural_size_.height != 0)
    return true;

  if(!url_.has_path())
    return false;
  std::string mime;
  if(!net::GetMimeTypeFromFile(FilePath(url_.path()), &mime))
    return true;
  return mime.find("audio") == std::string::npos;
}

bool WebMediaPlayerImplAndroid::hasAudio() const {
  // TODO(hclam): Query status of audio and return the actual value.
  return true;
}

bool WebMediaPlayerImplAndroid::paused() const {
  return !is_playing_;
}

bool WebMediaPlayerImplAndroid::seeking() const {
  return seeking_;
}

float WebMediaPlayerImplAndroid::duration() const {
  return duration_;
}

float WebMediaPlayerImplAndroid::currentTime() const {
  if (seeking())
    return time_to_seek_;

  return current_time_;
}

int WebMediaPlayerImplAndroid::dataRate() const {
  // Deprecated.
  return 0;
}

WebSize WebMediaPlayerImplAndroid::naturalSize() const {
  return natural_size_;
}

WebMediaPlayer::NetworkState WebMediaPlayerImplAndroid::networkState() const {
  return network_state_;
}

WebMediaPlayer::ReadyState WebMediaPlayerImplAndroid::readyState() const {
  return ready_state_;
}

const WebTimeRanges& WebMediaPlayerImplAndroid::buffered() {
  return time_ranges_;
}

float WebMediaPlayerImplAndroid::maxTimeSeekable() const {
  // TODO(hclam): If this stream is not seekable this should return 0.
  return duration();
}

unsigned long long WebMediaPlayerImplAndroid::bytesLoaded() const {
  return buffered_bytes_;
}

unsigned long long WebMediaPlayerImplAndroid::totalBytes() const {
  // Deprecated.
  return 0;
}

void WebMediaPlayerImplAndroid::setSize(const WebSize& size) {
  texture_size_ = size;
}

void WebMediaPlayerImplAndroid::paint(WebCanvas* canvas, const WebRect& rect) {
  // TODO(hclam): Painting to a canvas is used in WebGL and drawing a video
  // to a canvas. We need to read the video frame from texture and paint to
  // canvas.
}

bool WebMediaPlayerImplAndroid::hasSingleSecurityOrigin() const {
  return false;
}

WebMediaPlayer::MovieLoadType
    WebMediaPlayerImplAndroid::movieLoadType() const {
  // Deprecated.
  return WebMediaPlayer::Unknown;
}

float WebMediaPlayerImplAndroid::mediaTimeForTimeValue(float timeValue) const {
  return ConvertSecondsToTimestamp(timeValue).InSecondsF();
}

unsigned WebMediaPlayerImplAndroid::decodedFrameCount() const {
  NOTIMPLEMENTED();
  return 0;
}

unsigned WebMediaPlayerImplAndroid::droppedFrameCount() const {
  NOTIMPLEMENTED();
  return 0;
}

unsigned WebMediaPlayerImplAndroid::audioDecodedByteCount() const {
  NOTIMPLEMENTED();
  return 0;
}

unsigned WebMediaPlayerImplAndroid::videoDecodedByteCount() const {
  NOTIMPLEMENTED();
  return 0;
}

void WebMediaPlayerImplAndroid::OnMediaPrepared(float duration) {
  WebKit::WebTimeRanges new_buffered(static_cast<size_t>(1));
  new_buffered[0].start = 0.0f;
  new_buffered[0].end = 0.0f;
  time_ranges_.swap(new_buffered);

  // If the status is already set to HaveEnoughData, set it again to make sure
  // that Videolayerchromium will get created.
  UpdateReadyState(WebMediaPlayer::HaveEnoughData);

  if (duration_ != duration) {
    duration_ = duration;
    client_->durationChanged();
  }
}

void WebMediaPlayerImplAndroid::OnPlaybackComplete() {
  current_time_ = duration();
  client_->timeChanged();
}

void WebMediaPlayerImplAndroid::OnBufferingUpdate(int percentage) {
  if (time_ranges_.size() > 0) {
    time_ranges_[0].end = duration() * percentage / 100;
    // Implement a trick here to fake progress event, as WebKit checks
    // consecutive bytesLoaded() to see if any progress made.
    // See HTMLMediaElement::progressEventTimerFired.
    buffered_bytes_++;
  }
}

void WebMediaPlayerImplAndroid::OnSeekComplete(int msec) {
  seeking_ = false;
  current_time_ = static_cast<float>(msec) / 1000.0;

  UpdateReadyState(WebMediaPlayer::HaveEnoughData);

  client_->timeChanged();
}

void WebMediaPlayerImplAndroid::OnTimeUpdate(int msec) {
  current_time_ = static_cast<float>(msec) / 1000.0;
}

void WebMediaPlayerImplAndroid::OnMediaError(int error) {
  switch (error) {
    case MEDIA_ERROR_UNKNOWN:
      // TODO(zhenghao): Android Media Player only has 3 errors, and when
      // playing an bogus URL or bad file, it only fire an MEDIA_ERROR_UNKNOWN.
      // As WebKit uses FormatError to indicate an error for bogus URL or
      // bad file, we default an MEDIA_ERROR_UNKNOWN to FormatError.
      UpdateNetworkState(WebMediaPlayer::FormatError);
      break;
    case MEDIA_ERROR_SERVER_DIED:
      // TODO(zhenghao): Media server died. In this case, the application must
      // release the MediaPlayer object and instantiate a new one.
      UpdateNetworkState(WebMediaPlayer::DecodeError);
      break;
    case MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK:
      UpdateNetworkState(WebMediaPlayer::FormatError);
      break;
    default:
      // When surfaceTexture got deleted before we call setSuface(0),
      // mediaplayer will report error -38 on subsequent calls. So it is
      // possible that we reach here.
      break;
  }
  client_->repaint();
}

void WebMediaPlayerImplAndroid::OnExitFullscreen() {
  fullscreen_ = false;
  client_->endFullscreen();       // This will trigger a call to exitFullscreen.
}

void WebMediaPlayerImplAndroid::OnMediaPlayerPlay() {
  is_playing_ = true;
  client_->playbackStateChanged();
}

void WebMediaPlayerImplAndroid::OnMediaPlayerPause() {
  is_playing_ = false;
  client_->playbackStateChanged();
}

void WebMediaPlayerImplAndroid::UpdateNetworkState(
    WebMediaPlayer::NetworkState state) {
  network_state_ = state;
  client_->networkStateChanged();
}

void WebMediaPlayerImplAndroid::UpdateReadyState(
    WebMediaPlayer::ReadyState state) {
  ready_state_ = state;
  client_->readyStateChanged();
}

void WebMediaPlayerImplAndroid::OnVideoSizeChanged(int width, int height) {
  if (natural_size_.width == width && natural_size_.height == height)
    return;

  natural_size_.width = width;
  natural_size_.height = height;

  WebVideoFrameImplAndroid* frame =
      static_cast<WebVideoFrameImplAndroid*>(video_frame_.get());
  frame->setWidth(natural_size_.width);
  frame->setHeight(natural_size_.height);
}

void WebMediaPlayerImplAndroid::OnSurfaceReleased() {
  needs_reestablish_peer_ = true;
}

void WebMediaPlayerImplAndroid::ReleaseMediaResources() {
  // Pause the media player first.
  pause();

  // Now release the media player and surface texture, but not the texture_id
  // for screenshot purpose.
  if (proxy_) {
    // Save the current media player status.
    time_to_seek_ = currentTime();
    duration_ = duration();

    proxy_->ReleaseResources(player_id_);
    needs_reestablish_peer_ = true;
  }
}

void WebMediaPlayerImplAndroid::CreateStreamTexture() {
  DCHECK(!stream_id_ && context_);
  if (context_->makeContextCurrent()) {
    if (!texture_id_) {
      texture_id_ = context_->createTexture();
      WebVideoFrameImplAndroid* frame =
          static_cast<WebVideoFrameImplAndroid*>(video_frame_.get());
      frame->setTextureId(texture_id_);
    }
    stream_id_ = context_->createStreamTextureCHROMIUM(texture_id_);
    context_->flush();
  }
}

void WebMediaPlayerImplAndroid::DestroyStreamTexture() {
  DCHECK(texture_id_ && stream_id_ && context_);
  if (context_->makeContextCurrent()) {
    context_->destroyStreamTextureCHROMIUM(texture_id_);
    context_->deleteTexture(texture_id_);
    texture_id_ = 0;
    stream_id_ = 0;
    context_->flush();
  }
}

void WebMediaPlayerImplAndroid::WillDestroyCurrentMessageLoop() {
  main_loop_ = NULL;
  proxy_ = NULL;
  manager_ = NULL;
}

// -------------------------------------------------------------------------
// Methods that are called on the compositor thread if useThreadedCompositor
// flag  is set.
WebVideoFrame* WebMediaPlayerImplAndroid::getCurrentFrame() {
  if (fullscreen_)
    return NULL;

  return video_frame_.get();
}

void WebMediaPlayerImplAndroid::putCurrentFrame(
    WebVideoFrame* web_video_frame) {
}

WebKit::WebStreamTextureProxy*
WebMediaPlayerImplAndroid::createStreamTextureProxy() {
  return stream_texture_proxy_factory_->CreateProxy(
      stream_id_, video_frame_->width(), video_frame_->height());
}

}  // namespace webkit_glue
