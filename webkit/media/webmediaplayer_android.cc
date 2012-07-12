// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/media/webmediaplayer_android.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "content/browser/android/media_player_bridge.h"
#include "content/browser/android/network_info.h"
#include "net/base/mime_util.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebCookieJar.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebGraphicsContext3D.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebKit.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebMediaPlayerClient.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebRect.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "webkit/media/fullscreen_video_proxy_android.h"
#include "webkit/media/media_metadata_android.h"
#include "webkit/media/media_player_constants_android.h"
#include "webkit/media/streamtextureproxy_factory_android.h"
#include "webkit/media/webmediaplayer_manager_android.h"
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

static const int kTimeUpdateInterval = 250; // Time update happens every 250ms.
static const char* kCookie = "Cookie";
static const char* kHideUrlLogs = "x-hide-urls-from-log";
// This needs to be kept in sync with android.os.PowerManager
static const int kAndroidFullWakeLock = 26;

namespace webkit_glue {

namespace {

// Platform independent method for converting and rounding floating point
// seconds to an int64 timestamp.
//
// Refer to https://bugs.webkit.org/show_bug.cgi?id=52697 for details.
base::TimeDelta ConvertSecondsToTimestamp(float seconds) {
  float microseconds = seconds * base::Time::kMicrosecondsPerSecond;
  float integer = ceilf(microseconds);
  float difference = integer - microseconds;

  // Round down if difference is large enough.
  if ((microseconds > 0 && difference > 0.5f) ||
      (microseconds <= 0 && difference >= 0.5f)) {
    integer -= 1.0f;
  }

  // Now we can safely cast to int64 microseconds.
  return base::TimeDelta::FromMicroseconds(static_cast<int64>(integer));
}

// ---------------------------------------------------------------------------
// MediaPlayerListener
//
// A class to listen to media player events and delegate it to
// MediaPlayerProxy.
class MediaPlayerListener : public MediaPlayerBridge::MediaPlayerListener {
 public:
  MediaPlayerListener(scoped_refptr<MediaPlayerProxy> proxy)
      : proxy_(proxy) {
  }

  virtual ~MediaPlayerListener() {}

  void OnNotify(int msg, int ext1, int ext2) {
    proxy_->OnNotify(msg, ext1, ext2);
  }

 private:
  scoped_refptr<MediaPlayerProxy> proxy_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerListener);
};

}  // namespace

// ---------------------------------------------------------------------------
// MediaPlayerProxy
MediaPlayerProxy::MediaPlayerProxy()
    : render_message_loop_(base::MessageLoopProxy::current()) {
}

MediaPlayerProxy::~MediaPlayerProxy() {
}

void MediaPlayerProxy::SetListener(WebMediaPlayerAndroid* listener) {
  listener_ = listener;
}

void MediaPlayerProxy::OnNotify(int msg, int ext1, int ext2) {
  if (!render_message_loop_->BelongsToCurrentThread()) {
    render_message_loop_->PostTask(
        FROM_HERE,
        base::Bind(&MediaPlayerProxy::OnNotify, this, msg, ext1, ext2));
    return;
  }

  if (listener_)
    listener_->OnNotify(msg, ext1, ext2);
}

// ---------------------------------------------------------------------------
// WebMediaPlayerAndroid
bool WebMediaPlayerAndroid::incognito_mode_ = false;

WebMediaPlayerAndroid::WebMediaPlayerAndroid(
    WebFrame* frame,
    WebMediaPlayerClient* client,
    WebKit::WebCookieJar* cookie_jar,
    webkit_glue::WebMediaPlayerManagerAndroid* manager,
    WebKit::WebGraphicsContext3D* context)
    : owner_(frame),
      client_(client),
      media_player_(NULL),
      natural_size_(WebSize(0,0)),
      main_loop_(MessageLoop::current()),
      proxy_(new MediaPlayerProxy()),
      prepared_(false),
      duration_(0),
      time_to_seek_(0),
      seeking_(false),
      current_time_(0),
      buffered_bytes_(0),
      cookie_jar_(cookie_jar),
      manager_(manager),
      pending_play_event_(false),
      can_pause_(true),
      can_seek_forward_(true),
      can_seek_backward_(true),
      skip_preload_(false),
      network_state_(WebMediaPlayer::Empty),
      ready_state_(WebMediaPlayer::HaveNothing),
      texture_id_(0),
      stream_id_(0),
      needs_reestablish_peer_(false),
      context_(context) {
  main_loop_->AddDestructionObserver(this);
  player_id_ = manager_->RegisterMediaPlayer(this);
  fullscreen_proxy_ = manager_->fullscreen_proxy();
  video_frame_.reset(new WebVideoFrameImplAndroid());
  stream_texture_proxy_factory_.reset(
      webkit_media::StreamTextureProxyFactory::Create(
          manager_->routing_id(), player_id_));
  fullscreen_ =  manager_->IsFullscreenMode();
}

WebMediaPlayerAndroid::~WebMediaPlayerAndroid() {
  // Set listener to NULL before we destroy the media_player_, or we risk
  // getting OnNotify  messages after we destroy the media_player_.
  proxy_->SetListener(NULL);

  if (media_player_.get()) {
    media_player_->SetListener(NULL);
    media_player_->Stop();
    media_player_->SetVideoSurface(0);
    media_player_->Release();
    media_player_.reset(NULL);
  }

  // TODO (qinmin): This some how does not guarantee exitFullscreen get called.
  // Need to find out why this happens.
  if (fullscreen_ && fullscreen_proxy_) {
    StopFullscreenTimer();
    fullscreen_proxy_->SetWebMediaPlayer(NULL);
  }

  if (stream_id_)
    DestroyStreamTexture();

  if (manager_)
    manager_->UnRegisterMediaPlayer(player_id_);

  if (main_loop_)
    main_loop_->RemoveDestructionObserver(this);
}

void WebMediaPlayerAndroid::initIncognito(bool incognito_mode) {
     incognito_mode_ = incognito_mode;
}

void WebMediaPlayerAndroid::enterFullscreen() {
  if (!media_player_.get()) {
    LoadUrl(url_);
  }

  if (fullscreen_proxy_ && !fullscreen_) {
    fullscreen_proxy_->EnterFullscreen(this);
    fullscreen_ = true;
    if (!paused())
      StartFullscreenTimer();
  }
}

void WebMediaPlayerAndroid::exitFullscreen() {
  fullscreen_ = false;
  if (media_player_.get()) {
    media_player_->SetVideoSurface(0);
  }
  if (fullscreen_proxy_) {
    fullscreen_proxy_->LeaveFullscreen();
  }
  StopFullscreenTimer();

  if (!paused()) {
    if (!stream_id_)
      CreateStreamTexture();

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

void WebMediaPlayerAndroid::load(const WebURL& url) {
  CHECK(!prepared_);
  url_ = url;

  NetworkInfo info;
  GURL gurl(url_);
  if (manager_->AllowedForPreload(player_id_)
      && (info.IsWifiNetwork() || gurl.SchemeIs("file"))) {
    // Wifi mode, let the media player start buffering the url.
    LoadUrl(url_);
  } else {
    skip_preload_ = true;
  }

  UpdateNetworkState(WebMediaPlayer::Loading);
  UpdateReadyState(WebMediaPlayer::HaveNothing);

  if (skip_preload_) {
    // Set the initial duration value to 100 so that when in non-wifi mode,
    // user can still touch the seek bar to perform seek.
    duration_ = 100;

    // Non-wifi mode, pretend everything has been loaded so that webkit can
    // still call play() and seek().
    UpdateReadyState(WebMediaPlayer::HaveMetadata);
    UpdateReadyState(WebMediaPlayer::HaveEnoughData);
  }
}

void WebMediaPlayerAndroid::cancelLoad() {
  NOTIMPLEMENTED();
}

void WebMediaPlayerAndroid::play() {
  if (media_player_.get()) {
    if (!prepared_)
      pending_play_event_ = true;
    else
      PlayInternal();
  } else {
    pending_play_event_ = true;
    LoadUrl(url_);
  }

  // Update the metadata on the browser side for fullscreen case.
  if (fullscreen_ && fullscreen_proxy_) {
    fullscreen_proxy_->UpdateMetadata(GetMetadata());
  }
}

void WebMediaPlayerAndroid::pause() {
  if (media_player_.get()) {
    if (!prepared_)
      pending_play_event_ = false;
    else
      PauseInternal();
  } else {
    // We don't need to load media if pause is called().
    pending_play_event_ = false;
  }
  // Update the metadata on the browser side for fullscreen case.
  if (fullscreen_ && fullscreen_proxy_) {
    fullscreen_proxy_->UpdateMetadata(GetMetadata());
  }
}

void WebMediaPlayerAndroid::seek(float seconds) {
  time_to_seek_ = seconds;
  if (media_player_.get()) {
    if (prepared_)
      SeekInternal(seconds);
  } else {
    LoadUrl(url_);
  }
}

bool WebMediaPlayerAndroid::supportsFullscreen() const {
  return true;
}

bool WebMediaPlayerAndroid::supportsSave() const {
  return false;
}

void WebMediaPlayerAndroid::setEndTime(float seconds) {
  // Deprecated.
}

void WebMediaPlayerAndroid::setRate(float rate) {
  if (rate != 1.0f) {
    NOTIMPLEMENTED();
  }
}

void WebMediaPlayerAndroid::setVolume(float volume) {
  if (prepared_)
    media_player_->SetVolume(volume, volume);
}

void WebMediaPlayerAndroid::setVisible(bool visible) {
  // Deprecated.
}

bool WebMediaPlayerAndroid::setAutoBuffer(bool autoBuffer) {
  NOTIMPLEMENTED();
  return false;
}

bool WebMediaPlayerAndroid::totalBytesKnown() {
  NOTIMPLEMENTED();
  return false;
}

bool WebMediaPlayerAndroid::hasVideo() const {
  // We don't know if the file has video until the player
  // is prepared. While it is not prepared we fall back on the
  // mime-type. There may be no mime-type on a redirect URL
  // so we conservatively assume it contains video (so that
  // supportsFullscreen will be true and entering fullscreen
  // will not fail).
  if (!prepared_) {
    GURL gurl(url_);
    if(!gurl.has_path())
      return false;
    std::string mime;
    if(!net::GetMimeTypeFromFile(FilePath(gurl.path()), &mime))
      return true;
    return mime.find("audio") == std::string::npos;
  }

  return natural_size_.width != 0 && natural_size_.height != 0;
}

bool WebMediaPlayerAndroid::hasAudio() const {
  // TODO(hclam): Query status of audio and return the actual value.
  return true;
}

bool WebMediaPlayerAndroid::paused() const {
  if (!prepared_)
    return !pending_play_event_;
  return !media_player_->IsPlaying();
}

bool WebMediaPlayerAndroid::seeking() const {
  return seeking_;
}

float WebMediaPlayerAndroid::duration() const {
  return duration_;
}

float WebMediaPlayerAndroid::currentTime() const {
  if (!prepared_ || seeking())
    return time_to_seek_;

  int current_time_ms = 0;
  media_player_->GetCurrentPosition(&current_time_ms);

  // Return the greater of the two values so that the current time is most
  // updated.
  return std::max(current_time_, current_time_ms / 1000.0f);
}

int WebMediaPlayerAndroid::dataRate() const {
  // Deprecated.
  return 0;
}

WebSize WebMediaPlayerAndroid::naturalSize() const {
  return natural_size_;
}

WebMediaPlayer::NetworkState WebMediaPlayerAndroid::networkState() const {
  return network_state_;
}

WebMediaPlayer::ReadyState WebMediaPlayerAndroid::readyState() const {
  return ready_state_;
}

const WebTimeRanges& WebMediaPlayerAndroid::buffered() {
  return time_ranges_;
}

float WebMediaPlayerAndroid::maxTimeSeekable() const {
  // TODO(hclam): If this stream is not seekable this should return 0.
  return duration();
}

unsigned long long WebMediaPlayerAndroid::bytesLoaded() const {
  return buffered_bytes_;
}

unsigned long long WebMediaPlayerAndroid::totalBytes() const {
  // Deprecated.
  return 0;
}

void WebMediaPlayerAndroid::setSize(const WebSize& size) {
  if (size == texture_size_) {
    return;
  }
  texture_size_ = size;

  WebVideoFrameImplAndroid* frame =
      static_cast<WebVideoFrameImplAndroid*>(video_frame_.get());
  frame->setWidth(texture_size_.width);
  frame->setHeight(texture_size_.height);
}

void WebMediaPlayerAndroid::paint(WebCanvas* canvas, const WebRect& rect) {
  // TODO(hclam): Painting to a canvas is used in WebGL and drawing a video
  // to a canvas. We need to read the video frame from texture and paint to
  // canvas.
}

bool WebMediaPlayerAndroid::hasSingleSecurityOrigin() const {
  return false;
}

WebMediaPlayer::MovieLoadType
    WebMediaPlayerAndroid::movieLoadType() const {
  // Deprecated.
  return WebMediaPlayer::Unknown;
}

float WebMediaPlayerAndroid::mediaTimeForTimeValue(float timeValue) const {
  return ConvertSecondsToTimestamp(timeValue).InSecondsF();
}

unsigned WebMediaPlayerAndroid::decodedFrameCount() const {
  NOTIMPLEMENTED();
  return 0;
}

unsigned WebMediaPlayerAndroid::droppedFrameCount() const {
  NOTIMPLEMENTED();
  return 0;
}

unsigned WebMediaPlayerAndroid::audioDecodedByteCount() const {
  NOTIMPLEMENTED();
  return 0;
}

unsigned WebMediaPlayerAndroid::videoDecodedByteCount() const {
  NOTIMPLEMENTED();
  return 0;
}

void WebMediaPlayerAndroid::OnPrepared() {
  WebKit::WebTimeRanges new_buffered(static_cast<size_t>(1));
  new_buffered[0].start = 0.0f;
  new_buffered[0].end = 0.0f;
  time_ranges_.swap(new_buffered);
  prepared_ = true;

  // Update the media duration first so that webkit will get the correct
  // duration when UpdateReadyState is called.
  float dur = duration_;
  int duration_ms = 0;
  media_player_->GetDuration(&duration_ms);
  duration_ = duration_ms / 1000.0f;

  GURL gurl(url_);
  if (gurl.SchemeIs("file"))
    UpdateNetworkState(WebMediaPlayer::Loaded);

  if (ready_state_ != WebMediaPlayer::HaveEnoughData) {
    UpdateReadyState(WebMediaPlayer::HaveMetadata);
    UpdateReadyState(WebMediaPlayer::HaveEnoughData);
  } else {
    // If the status is already set to HaveEnoughData, set it again to make sure
    // that Videolayerchromium will get created.
    UpdateReadyState(WebMediaPlayer::HaveEnoughData);
  }

  if (skip_preload_) {
    // In non-wifi mode, we have preset duration to 100 sec.
    // We have to update webkit about the new duration.
    if (duration_ != dur) {
      // Scale the time_to_seek_ according to the new duration.
      time_to_seek_ = time_to_seek_ * duration_ / 100.0f;
      client_->durationChanged();
    }
  }

  // If media player was recovered from a saved state, play all the pending
  // events.
  OnMediaPlayerSeek(time_to_seek_);

  if (pending_play_event_) {
    PlayInternal();
  }

  // Update the metadata on the browser side.
  if (fullscreen_ && fullscreen_proxy_) {
    fullscreen_proxy_->UpdateMetadata(GetMetadata());
  }

  pending_play_event_ = false;
}

void WebMediaPlayerAndroid::OnPlaybackComplete() {
  current_time_ = duration();
  client_->timeChanged();
}

void WebMediaPlayerAndroid::OnBufferingUpdate(int percentage) {
  if (time_ranges_.size() > 0) {
    time_ranges_[0].end = duration() * percentage / 100;
    // Implement a trick here to fake progress event, as WebKit checks
    // consecutive bytesLoaded() to see if any progress made.
    // See HTMLMediaElement::progressEventTimerFired.
    buffered_bytes_++;
  }
}

void WebMediaPlayerAndroid::OnSeekComplete() {
  seeking_ = false;
  DoTimeUpdate();

  UpdateReadyState(WebMediaPlayer::HaveEnoughData);

  client_->timeChanged();
}

void WebMediaPlayerAndroid::OnError(int error) {
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

void WebMediaPlayerAndroid::OnNotify(int msg, int ext1, int ext2) {
  if (fullscreen_ && fullscreen_proxy_) {
    fullscreen_proxy_->Notify(msg, ext1, ext2);
  }

  // Send message to the renderer.
  switch (msg) {
    case MEDIA_PREPARED:
      LOG(INFO) << "CLANK: MEDIA_PREPARED";
      OnPrepared();
      break;
    case MEDIA_PLAYBACK_COMPLETE:
      LOG(INFO) << "CLANK: MEDIA_PLAYBACK_COMPLETE";
      OnPlaybackComplete();
      break;
    case MEDIA_BUFFERING_UPDATE:
#if 0  // TODO(tedbo): This is pretty spammy.
      LOG(INFO) << "CLANK: MEDIA_BUFFERING_UPDATE: ext1: "
                << ext1 << ", ext2: " << ext2;
#endif
      OnBufferingUpdate(ext1);
      break;
    case MEDIA_SEEK_COMPLETE:
      LOG(INFO) << "CLANK: MEDIA_SEEK_COMPLETE";
      OnSeekComplete();
      break;
    case MEDIA_SET_VIDEO_SIZE:
      LOG(INFO) << "CLANK: MEDIA_SET_VIDEO_SIZE";
      UpdateNaturalSize(ext1, ext2);
      break;
    case MEDIA_INFO:
      LOG(INFO) << "CLANK: MEDIA_INFO";
      break;
    case MEDIA_ERROR:
      LOG(INFO) << "CLANK: MEDIA_ERROR: ext1: " << ext1;
      OnError(ext1);
      break;
  }
}

void WebMediaPlayerAndroid::DoTimeUpdate() {
  // It's possible that the timer fires after media_player_ has already
  // been freed, so check that we have a valid pointer first.
  if (!media_player_.get())
    return;

  int current_time_ms = 0;
  media_player_->GetCurrentPosition(&current_time_ms);
  current_time_ = current_time_ms / 1000.0f;

  if (fullscreen_ && fullscreen_proxy_) {
    fullscreen_proxy_->UpdateCurrentTime(current_time_);
  }
}

void WebMediaPlayerAndroid::OnMediaPlayerPlay() {
  play();

  // Inform webkit player state has changed
  client_->playbackStateChanged();
}

void WebMediaPlayerAndroid::OnMediaPlayerSeek(float seconds) {
  seek(seconds);
}

void WebMediaPlayerAndroid::OnMediaPlayerPause() {
  pause();

  // Inform webkit player state has changed
  client_->playbackStateChanged();
}

void WebMediaPlayerAndroid::OnExitFullscreen(bool release_media_player) {
  if (release_media_player) {
    ReleaseMediaResources();
  }

  client_->endFullscreen();       // This will trigger a call to exitFullscreen.
}

void WebMediaPlayerAndroid::UpdateNetworkState(
    WebMediaPlayer::NetworkState state) {
  network_state_ = state;
  client_->networkStateChanged();
}

void WebMediaPlayerAndroid::UpdateReadyState(
    WebMediaPlayer::ReadyState state) {
  ready_state_ = state;
  client_->readyStateChanged();
}

MediaMetadataAndroid* WebMediaPlayerAndroid::GetMetadata() {
  if (prepared_) {
    media_player_->GetMetadata(&can_pause_,
                               &can_seek_forward_,
                               &can_seek_backward_);
  }

  WebSize size = naturalSize();

  scoped_ptr<MediaMetadataAndroid> metadata(
      new MediaMetadataAndroid(size.width,
                               size.height,
                               duration(),
                               currentTime(),
                               paused(),
                               can_pause_,
                               can_seek_forward_,
                               can_seek_backward_));
  return metadata.release();
}

void WebMediaPlayerAndroid::UpdateNaturalSize(int width, int height) {
  natural_size_.width = width;
  natural_size_.height = height;
}

void WebMediaPlayerAndroid::ReleaseMediaResources() {
  // Pause the media player first.
  OnMediaPlayerPause();

  // Delete the proxy so we will not get notified afterwards
  proxy_->SetListener(NULL);
  // Now release the media player and surface texture, but not the texture_id
  // for screenshot purpose.
  if (media_player_.get()) {
    // Save the current media player status.
    time_to_seek_ = currentTime();
    duration_ = duration();
    media_player_->GetMetadata(&can_pause_,
                               &can_seek_forward_,
                               &can_seek_backward_);

    media_player_->SetListener(NULL);
    // Detaching the producer should unref all queued buffers except the one
    // currently associated with the GL texture (used as thumbnail when coming
    // back to this tab).
    media_player_->SetVideoSurface(0);
    media_player_->Release();
    media_player_.reset(NULL);
    needs_reestablish_peer_ = true;
  }
  prepared_ = false;
}

void WebMediaPlayerAndroid::SetVideoSurface(
    jobject j_surface) {
  if (media_player_.get())
    media_player_->SetVideoSurface(j_surface);
}

void WebMediaPlayerAndroid::InitializeMediaPlayer() {
  if (!media_player_.get()) {
    prepared_ = false;
    media_player_.reset(new MediaPlayerBridge());
    media_player_->SetWakeMode(kAndroidFullWakeLock);
    proxy_->SetListener(this);
    scoped_ptr<MediaPlayerListener> listener(new MediaPlayerListener(proxy_));
    media_player_->SetListener(listener.release());
  }
}

void WebMediaPlayerAndroid::LoadUrl(WebURL url) {
  if (!media_player_.get())
    InitializeMediaPlayer();

  media_player_->Reset();

  MediaPlayerBridge::HeadersMap headers_map;
  if (incognito_mode_) {
    headers_map.insert(std::make_pair(kHideUrlLogs, "true"));
  }
  if (cookie_jar_ != NULL) {
    WebURL first_party_url(owner_->document().firstPartyForCookies());
    std::string cookies = UTF16ToUTF8(cookie_jar_->cookies(url,
                                                           first_party_url));
    if (!cookies.empty()) {
      headers_map.insert(std::make_pair(kCookie, cookies));
    }
  }
  media_player_->SetDataSource(url.spec(), headers_map);

  manager_->RequestForMediaResources(player_id_);

  if (fullscreen_ && fullscreen_proxy_) {
    fullscreen_proxy_->EnterFullscreen(this);
  }
  media_player_->PrepareAsync();
}

void WebMediaPlayerAndroid::PlayInternal() {
  CHECK(prepared_);

  if (hasVideo() && !fullscreen_) {
    if (!stream_id_ && context_)
      CreateStreamTexture();

    if (needs_reestablish_peer_) {
      stream_texture_proxy_factory_->ReestablishPeer(stream_id_);
      needs_reestablish_peer_ = false;
    }
  }

  if (paused()) {
    media_player_->Start();
    if (fullscreen_)
      StartFullscreenTimer();
  }
}

void WebMediaPlayerAndroid::PauseInternal() {
  CHECK(prepared_);
  if (fullscreen_)
    StopFullscreenTimer();
  media_player_->Pause();
}

void WebMediaPlayerAndroid::SeekInternal(float seconds) {
  CHECK(prepared_);
  if (seconds == 0.0f && currentTime() == 0.0f) {
    client_->timeChanged();
    return;
  }
  seeking_ = true;
  media_player_->SeekTo(
      ConvertSecondsToTimestamp(seconds).InMilliseconds());
}

void WebMediaPlayerAndroid::CreateStreamTexture() {
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

void WebMediaPlayerAndroid::DestroyStreamTexture() {
  DCHECK(texture_id_ && stream_id_ && context_);
  if (context_->makeContextCurrent()) {
    context_->destroyStreamTextureCHROMIUM(texture_id_);
    context_->deleteTexture(texture_id_);
    texture_id_ = 0;
    stream_id_ = 0;
    context_->flush();
  }
}

void WebMediaPlayerAndroid::WillDestroyCurrentMessageLoop() {
  manager_ = NULL;
  main_loop_ = NULL;
}

// -------------------------------------------------------------------------
// Methods that are called on the compositor thread if useThreadedCompositor
// flag  is set.
WebVideoFrame* WebMediaPlayerAndroid::getCurrentFrame() {
  if (fullscreen_)
    return NULL;

  return video_frame_.get();
}

void WebMediaPlayerAndroid::putCurrentFrame(
    WebVideoFrame* web_video_frame) {
}

WebKit::WebStreamTextureProxy*
WebMediaPlayerAndroid::createStreamTextureProxy() {
  return stream_texture_proxy_factory_->CreateProxy(
      stream_id_, video_frame_->width(), video_frame_->height());
}

void WebMediaPlayerAndroid::StartFullscreenTimer() {
  if (!time_update_timer_.IsRunning())
    time_update_timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(kTimeUpdateInterval),
        this, &WebMediaPlayerAndroid::DoTimeUpdate);
}

void WebMediaPlayerAndroid::StopFullscreenTimer() {
  time_update_timer_.Stop();
}
// -------------------------------------------------------------------------

}  // namespace webkit_glue
