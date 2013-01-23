// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/media/webmediaplayer_in_process_android.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "content/browser/android/media_player_bridge.h"
#include "media/base/media_player_bridge_manager.h"
#include "net/base/mime_util.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebCookieJar.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebGraphicsContext3D.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebKit.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebMediaPlayerClient.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebRect.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "webkit/media/media_player_constants_android.h"
#include "webkit/media/streamtextureproxy_factory_android.h"
#include "webkit/media/webmediaplayer_in_process_proxy_android.h"
#include "webkit/media/webmediaplayer_manager_android.h"
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

InProcessCookiesRetriever::InProcessCookiesRetriever(
    WebKit::WebCookieJar* cookie_jar)
    : cookie_jar_(cookie_jar) {
}

InProcessCookiesRetriever::~InProcessCookiesRetriever() {
}

void InProcessCookiesRetriever::GetCookies(
    const std::string& url,
    const std::string& first_party_for_cookies,
    const GetCookieCB& callback) {
  std::string cookies;
  if (cookie_jar_ != NULL) {
    cookies = UTF16ToUTF8(cookie_jar_->cookies(
        GURL(url), GURL(first_party_for_cookies)));
  }
  callback.Run(cookies);
}

// ---------------------------------------------------------------------------
// WebMediaPlayerInProcessAndroid

WebMediaPlayerInProcessAndroid::WebMediaPlayerInProcessAndroid(
    WebFrame* frame,
    WebMediaPlayerClient* client,
    WebKit::WebCookieJar* cookie_jar,
    WebMediaPlayerManagerAndroid* manager,
    media::MediaPlayerBridgeManager* resource_manager,
    WebKit::WebGraphicsContext3D* context,
    bool disable_media_history_logging,
    int routing_id)
    : owner_(frame),
      client_(client),
      cookie_jar_(cookie_jar),
      manager_(manager),
      resource_manager_(resource_manager),
      disable_history_logging_(disable_media_history_logging),
      media_player_(NULL),
      natural_size_(WebSize(0,0)),
      main_loop_(MessageLoop::current()),
      proxy_(new WebMediaPlayerInProcessProxyAndroid(
          main_loop_->message_loop_proxy(),
          AsWeakPtr())),
      prepared_(false),
      duration_(0),
      time_to_seek_(0),
      seeking_(false),
      current_time_(0),
      buffered_bytes_(0),
      pending_play_event_(false),
      network_state_(WebMediaPlayer::Empty),
      ready_state_(WebMediaPlayer::HaveNothing),
      texture_id_(0),
      stream_id_(0),
      needs_reestablish_peer_(false),
      context_(context) {
  main_loop_->AddDestructionObserver(this);
  if (manager_)
    player_id_ = manager_->RegisterMediaPlayer(this);
  video_frame_.reset(new WebVideoFrameImplAndroid());
  stream_texture_proxy_factory_.reset(
      webkit_media::StreamTextureProxyFactory::Create(
          routing_id, player_id_));
}

WebMediaPlayerInProcessAndroid::~WebMediaPlayerInProcessAndroid() {
  if (stream_id_)
    DestroyStreamTexture();

  if (manager_)
    manager_->UnregisterMediaPlayer(player_id_);

  if (main_loop_)
    main_loop_->RemoveDestructionObserver(this);
}

void WebMediaPlayerInProcessAndroid::enterFullscreen() {
  NOTIMPLEMENTED();
}

void WebMediaPlayerInProcessAndroid::exitFullscreen() {
  NOTIMPLEMENTED();
}

void WebMediaPlayerInProcessAndroid::load(const WebURL& url) {
  CHECK(!prepared_);
  url_ = url;

  LoadUrl();

  UpdateNetworkState(WebMediaPlayer::Loading);
  UpdateReadyState(WebMediaPlayer::HaveNothing);
}

void WebMediaPlayerInProcessAndroid::cancelLoad() {
  NOTIMPLEMENTED();
}

void WebMediaPlayerInProcessAndroid::play() {
  if (media_player_.get()) {
    if (!prepared_)
      pending_play_event_ = true;
    else
      PlayInternal();
  } else {
    pending_play_event_ = true;
    LoadUrl();
  }
}

void WebMediaPlayerInProcessAndroid::pause() {
  if (media_player_.get()) {
    if (!prepared_)
      pending_play_event_ = false;
    else
      PauseInternal();
  } else {
    // We don't need to load media if pause is called().
    pending_play_event_ = false;
  }
}

void WebMediaPlayerInProcessAndroid::seek(float seconds) {
  time_to_seek_ = seconds;
  if (media_player_.get()) {
    if (prepared_)
      SeekInternal(seconds);
  } else {
    LoadUrl();
  }
}

bool WebMediaPlayerInProcessAndroid::supportsFullscreen() const {
  return true;
}

bool WebMediaPlayerInProcessAndroid::supportsSave() const {
  return false;
}

void WebMediaPlayerInProcessAndroid::setEndTime(float seconds) {
  // Deprecated.
}

void WebMediaPlayerInProcessAndroid::setRate(float rate) {
  if (rate != 1.0f) {
    NOTIMPLEMENTED();
  }
}

void WebMediaPlayerInProcessAndroid::setVolume(float volume) {
  if (prepared_)
    media_player_->SetVolume(volume, volume);
}

void WebMediaPlayerInProcessAndroid::setVisible(bool visible) {
  // Deprecated.
}

bool WebMediaPlayerInProcessAndroid::setAutoBuffer(bool autoBuffer) {
  NOTIMPLEMENTED();
  return false;
}

bool WebMediaPlayerInProcessAndroid::totalBytesKnown() {
  NOTIMPLEMENTED();
  return false;
}

bool WebMediaPlayerInProcessAndroid::hasVideo() const {
  // We don't know if the file has video until the player
  // is prepared. While it is not prepared we fall back on the
  // mime-type. There may be no mime-type on a redirect URL
  // so we conservatively assume it contains video (so that
  // supportsFullscreen will be true and entering fullscreen
  // will not fail).
  if (!prepared_) {
    if(!url_.has_path())
      return false;
    std::string mime;
    if(!net::GetMimeTypeFromFile(FilePath(url_.path()), &mime))
      return true;
    return mime.find("audio") == std::string::npos;
  }

  return natural_size_.width != 0 && natural_size_.height != 0;
}

bool WebMediaPlayerInProcessAndroid::hasAudio() const {
  // TODO(hclam): Query status of audio and return the actual value.
  return true;
}

bool WebMediaPlayerInProcessAndroid::paused() const {
  if (!prepared_)
    return !pending_play_event_;
  return !media_player_->IsPlaying();
}

bool WebMediaPlayerInProcessAndroid::seeking() const {
  return seeking_;
}

float WebMediaPlayerInProcessAndroid::duration() const {
  return duration_;
}

float WebMediaPlayerInProcessAndroid::currentTime() const {
  if (!prepared_ || seeking())
    return time_to_seek_;

  int current_time_ms = media_player_->GetCurrentPosition();

  // Return the greater of the two values so that the current time is most
  // updated.
  return std::max(current_time_, current_time_ms / 1000.0f);
}

int WebMediaPlayerInProcessAndroid::dataRate() const {
  // Deprecated.
  return 0;
}

WebSize WebMediaPlayerInProcessAndroid::naturalSize() const {
  return natural_size_;
}

WebMediaPlayer::NetworkState WebMediaPlayerInProcessAndroid::networkState() const {
  return network_state_;
}

WebMediaPlayer::ReadyState WebMediaPlayerInProcessAndroid::readyState() const {
  return ready_state_;
}

const WebTimeRanges& WebMediaPlayerInProcessAndroid::buffered() {
  return time_ranges_;
}

float WebMediaPlayerInProcessAndroid::maxTimeSeekable() const {
  // TODO(hclam): If this stream is not seekable this should return 0.
  return duration();
}

unsigned long long WebMediaPlayerInProcessAndroid::bytesLoaded() const {
  return buffered_bytes_;
}

unsigned long long WebMediaPlayerInProcessAndroid::totalBytes() const {
  // Deprecated.
  return 0;
}

void WebMediaPlayerInProcessAndroid::setSize(const WebSize& size) {
  texture_size_ = size;
}

void WebMediaPlayerInProcessAndroid::paint(WebCanvas* canvas, const WebRect& rect) {
  // TODO(hclam): Painting to a canvas is used in WebGL and drawing a video
  // to a canvas. We need to read the video frame from texture and paint to
  // canvas.
}

bool WebMediaPlayerInProcessAndroid::hasSingleSecurityOrigin() const {
  return false;
}

WebMediaPlayer::MovieLoadType
    WebMediaPlayerInProcessAndroid::movieLoadType() const {
  // Deprecated.
  return WebMediaPlayer::Unknown;
}

float WebMediaPlayerInProcessAndroid::mediaTimeForTimeValue(float timeValue) const {
  return ConvertSecondsToTimestamp(timeValue).InSecondsF();
}

unsigned WebMediaPlayerInProcessAndroid::decodedFrameCount() const {
  NOTIMPLEMENTED();
  return 0;
}

unsigned WebMediaPlayerInProcessAndroid::droppedFrameCount() const {
  NOTIMPLEMENTED();
  return 0;
}

unsigned WebMediaPlayerInProcessAndroid::audioDecodedByteCount() const {
  NOTIMPLEMENTED();
  return 0;
}

unsigned WebMediaPlayerInProcessAndroid::videoDecodedByteCount() const {
  NOTIMPLEMENTED();
  return 0;
}

void WebMediaPlayerInProcessAndroid::OnMediaPrepared(float duration) {
  WebKit::WebTimeRanges new_buffered(static_cast<size_t>(1));
  new_buffered[0].start = 0.0f;
  new_buffered[0].end = 0.0f;
  time_ranges_.swap(new_buffered);
  prepared_ = true;

  // Update the media duration first so that webkit will get the correct
  // duration when UpdateReadyState is called.
  duration_ = media_player_->GetDuration() / 1000.0f;

  if (url_.SchemeIs("file"))
    UpdateNetworkState(WebMediaPlayer::Loaded);

  if (ready_state_ != WebMediaPlayer::HaveEnoughData) {
    UpdateReadyState(WebMediaPlayer::HaveMetadata);
    UpdateReadyState(WebMediaPlayer::HaveEnoughData);
  } else {
    // If the status is already set to HaveEnoughData, set it again to make
    // sure that Videolayerchromium will get created.
    UpdateReadyState(WebMediaPlayer::HaveEnoughData);
  }

  // If media player was recovered from a saved state, play all the pending
  // events.
  seek(time_to_seek_);

  if (pending_play_event_) {
    PlayInternal();
  }

  pending_play_event_ = false;
}

void WebMediaPlayerInProcessAndroid::OnPlaybackComplete() {
  current_time_ = duration();
  client_->timeChanged();
}

void WebMediaPlayerInProcessAndroid::OnBufferingUpdate(int percentage) {
  if (time_ranges_.size() > 0) {
    time_ranges_[0].end = duration() * percentage / 100;
    // Implement a trick here to fake progress event, as WebKit checks
    // consecutive bytesLoaded() to see if any progress made.
    // See HTMLMediaElement::progressEventTimerFired.
    buffered_bytes_++;
  }
}

void WebMediaPlayerInProcessAndroid::OnSeekComplete(int msec) {
  seeking_ = false;

  UpdateReadyState(WebMediaPlayer::HaveEnoughData);

  client_->timeChanged();
}

void WebMediaPlayerInProcessAndroid::OnMediaError(int error) {
  switch (error) {
    case MEDIA_ERROR_UNKNOWN:
      // TODO(zhenghao): Android Media Player only has 3 errors, and when playing
      // an bogus URL or bad file, it only fire an MEDIA_ERROR_UNKNOWN. As WebKit
      // uses FormatError to indicate an error for bogus URL or bad file, we
      // default an MEDIA_ERROR_UNKNOWN to FormatError.
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

void WebMediaPlayerInProcessAndroid::UpdateNetworkState(
    WebMediaPlayer::NetworkState state) {
  network_state_ = state;
  client_->networkStateChanged();
}

void WebMediaPlayerInProcessAndroid::UpdateReadyState(
    WebMediaPlayer::ReadyState state) {
  ready_state_ = state;
  client_->readyStateChanged();
}

void WebMediaPlayerInProcessAndroid::OnVideoSizeChanged(int width, int height) {
  if (natural_size_.width == width && natural_size_.height == height)
    return;

  natural_size_.width = width;
  natural_size_.height = height;

  WebVideoFrameImplAndroid* frame =
      static_cast<WebVideoFrameImplAndroid*>(video_frame_.get());
  frame->setWidth(natural_size_.width);
  frame->setHeight(natural_size_.height);
}

void WebMediaPlayerInProcessAndroid::ReleaseMediaResources() {
  if (!paused())
    pause();

  // Now release the media player and surface texture, but not the texture_id
  // for screenshot purpose.
  if (media_player_.get()) {
    // Save the current media player status.
    time_to_seek_ = currentTime();
    duration_ = duration();

    media_player_->Release();
    needs_reestablish_peer_ = true;
  }
  prepared_ = false;
}

void WebMediaPlayerInProcessAndroid::SetVideoSurface(
    jobject j_surface) {
  if (media_player_.get())
    media_player_->SetVideoSurface(j_surface, false);
}

void WebMediaPlayerInProcessAndroid::InitializeMediaPlayer() {
  if (!media_player_.get()) {
    prepared_ = false;
    GURL first_party_url = owner_->document().firstPartyForCookies();
    media_player_.reset(new MediaPlayerBridge(
      player_id_, url_.spec(), first_party_url.spec(),
      new InProcessCookiesRetriever(cookie_jar_),
      disable_history_logging_,
      resource_manager_,
      base::Bind(&WebMediaPlayerInProcessProxyAndroid::MediaErrorCallback,
                 proxy_),
      base::Bind(
          &WebMediaPlayerInProcessProxyAndroid::VideoSizeChangedCallback,
          proxy_),
      base::Bind(&WebMediaPlayerInProcessProxyAndroid::BufferingUpdateCallback,
                 proxy_),
      base::Bind(&WebMediaPlayerInProcessProxyAndroid::MediaPreparedCallback,
                 proxy_),
      base::Bind(
          &WebMediaPlayerInProcessProxyAndroid::PlaybackCompleteCallback,
          proxy_),
      base::Bind(&WebMediaPlayerInProcessProxyAndroid::SeekCompleteCallback,
                 proxy_),
      base::Bind(&WebMediaPlayerInProcessProxyAndroid::TimeUpdateCallback,
                 proxy_)));
  }
}

void WebMediaPlayerInProcessAndroid::LoadUrl() {
  if (!media_player_.get())
    InitializeMediaPlayer();

  media_player_->Reset();

  media_player_->Prepare();
}

void WebMediaPlayerInProcessAndroid::PlayInternal() {
  CHECK(prepared_);

  if (hasVideo()) {
    if (!stream_id_ && context_)
      CreateStreamTexture();

    if (needs_reestablish_peer_) {
      stream_texture_proxy_factory_->ReestablishPeer(stream_id_);
      needs_reestablish_peer_ = false;
    }
  }

  if (paused()) {
    media_player_->Start();
  }
}

void WebMediaPlayerInProcessAndroid::PauseInternal() {
  CHECK(prepared_);
  media_player_->Pause();
}

void WebMediaPlayerInProcessAndroid::SeekInternal(float seconds) {
  CHECK(prepared_);
  if (seconds == 0.0f && currentTime() == 0.0f) {
    client_->timeChanged();
    return;
  }
  seeking_ = true;
  media_player_->SeekTo(
      ConvertSecondsToTimestamp(seconds).InMilliseconds());
}

void WebMediaPlayerInProcessAndroid::CreateStreamTexture() {
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

void WebMediaPlayerInProcessAndroid::DestroyStreamTexture() {
  DCHECK(texture_id_ && stream_id_ && context_);
  if (context_->makeContextCurrent()) {
    context_->destroyStreamTextureCHROMIUM(texture_id_);
    context_->deleteTexture(texture_id_);
    texture_id_ = 0;
    stream_id_ = 0;
    context_->flush();
  }
}

void WebMediaPlayerInProcessAndroid::WillDestroyCurrentMessageLoop() {
  main_loop_ = NULL;
}

// -------------------------------------------------------------------------
// Methods that are called on the compositor thread if useThreadedCompositor
// flag  is set.
WebVideoFrame* WebMediaPlayerInProcessAndroid::getCurrentFrame() {
  return video_frame_.get();
}

void WebMediaPlayerInProcessAndroid::putCurrentFrame(
    WebVideoFrame* web_video_frame) {
}

WebKit::WebStreamTextureProxy*
WebMediaPlayerInProcessAndroid::createStreamTextureProxy() {
  return stream_texture_proxy_factory_->CreateProxy(
      stream_id_, video_frame_->width(), video_frame_->height());
}

// -------------------------------------------------------------------------

}  // namespace webkit_glue
