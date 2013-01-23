// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/media_player_bridge.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/basictypes.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop_proxy.h"
#include "base/stringprintf.h"
#include "content/browser/android/media_player_listener.h"
#include "net/android/cookies_retriever.h"
#include "webkit/media/media_player_constants_android.h"
#include "media/base/media_player_bridge_manager.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetClass;
using base::android::GetMethodID;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

// Constants need to be kept in sync with android/media/MediaMetadata.java
static const jint kPauseAvailable = 1;
static const jint kSeekBackwardAvailable = 2;
static const jint kSeekForwardAvailable = 3;

// This needs to be kept in sync with android.os.PowerManager
static const int kAndroidFullWakeLock = 26;

static const int kTimeUpdateInterval = 250; // Time update happens every 250ms.
static const int kTemporaryDuration = 100000;  // 100 seconds.

MediaPlayerBridge::MediaPlayerBridge(
    int player_id,
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
    const TimeUpdateCB& time_update_cb)
    : media_error_cb_(media_error_cb),
      video_size_changed_cb_(video_size_changed_cb),
      buffering_update_cb_(buffering_update_cb),
      media_prepared_cb_(media_prepared_cb),
      playback_complete_cb_(playback_complete_cb),
      seek_complete_cb_(seek_complete_cb),
      time_update_cb_(time_update_cb),
      is_data_source_set_(false),
      player_id_(player_id),
      prepared_(false),
      pending_play_(false),
      pending_seek_(0),
      url_(url),
      first_party_for_cookies_(first_party_for_cookies),
      has_cookies_(false),
      hide_url_log_(hide_url_log),
      duration_(kTemporaryDuration),
      width_(0),
      height_(0),
      fullscreen_(false),
      can_pause_(true),
      can_seek_forward_(true),
      can_seek_backward_(true),
      manager_(manager),
      cookies_retriever_(cookies_retriever),
      listener_(new MediaPlayerListener(base::MessageLoopProxy::current(),
                                        AsWeakPtr())) {
}

MediaPlayerBridge::~MediaPlayerBridge() {
  Release();
}

void MediaPlayerBridge::InitializePlayer() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  j_class_.Reset(GetClass(env, "android/media/MediaPlayer"));

  jmethodID constructor = GetMethodID(env, j_class_, "<init>", "()V");
  ScopedJavaLocalRef<jobject> tmp(env,
      env->NewObject(j_class_.obj(), constructor));
  DCHECK(!tmp.is_null());
  j_media_player_.Reset(tmp);

  ScopedJavaLocalRef<jobject> j_listener(
      listener_->CreateMediaPlayerListener());

  // Set it as the various listeners.
  const char* listeners[] = {
      "OnBufferingUpdateListener",
      "OnCompletionListener",
      "OnErrorListener",
      "OnPreparedListener",
      "OnSeekCompleteListener",
      "OnVideoSizeChangedListener",
  };
  for (unsigned int i = 0; i < arraysize(listeners); ++i) {
    std::string signature = StringPrintf("(Landroid/media/MediaPlayer$%s;)V",
                                         listeners[i]);
    std::string method_name = StringPrintf("set%s", listeners[i]);
    jmethodID method = GetMethodID(env,
                                   j_class_,
                                   method_name.c_str(),
                                   signature.c_str());
    env->CallVoidMethod(j_media_player_.obj(), method, j_listener.obj());
    CheckException(env);
  }
  SetWakeMode(kAndroidFullWakeLock);
}

void MediaPlayerBridge::SetVideoSurface(jobject surface, bool fullscreen) {
  if (j_media_player_.is_null())
    Prepare();

  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method =
      GetMethodID(env, j_class_, "setSurface", "(Landroid/view/Surface;)V");
  env->CallVoidMethod(j_media_player_.obj(), method, surface);
  CheckException(env);

  fullscreen_ = fullscreen;
}

void MediaPlayerBridge::SetVideoSurfaceTexture(jobject surface_texture) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  ScopedJavaLocalRef<jclass> cls(GetClass(env, "android/view/Surface"));
  jmethodID constructor = GetMethodID(env, cls, "<init>",
      "(Landroid/graphics/SurfaceTexture;)V");
  ScopedJavaLocalRef<jobject> surface(env,
      env->NewObject(cls.obj(), constructor, surface_texture));
  SetVideoSurface(surface.obj(), false);

  // Release the surface object as it will never be reused.
  jmethodID method = GetMethodID(env, cls, "release", "()V");
  env->CallVoidMethod(surface.obj(), method);
  CheckException(env);
}

void MediaPlayerBridge::PrepareWithoutCookies() {
  cookies_retriever_->GetCookies(url_, first_party_for_cookies_,
      base::Bind(&MediaPlayerBridge::PrepareWithCookies, AsWeakPtr()));
}

void MediaPlayerBridge::PrepareWithCookies(const std::string& cookies) {
  cookies_ = cookies;
  has_cookies_ = true;

  JNIEnv* env = AttachCurrentThread();
  CHECK(env);
  // Create a Java String for the URL.
  ScopedJavaLocalRef<jstring> j_url_string =
      ConvertUTF8ToJavaString(env, url_);

  // Create the android.net.Uri object.
  ScopedJavaLocalRef<jclass> cls(GetClass(env, "android/net/Uri"));
  jmethodID method = GetStaticMethodID(env, cls,
      "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
  ScopedJavaLocalRef<jobject> j_uri(env,
      env->CallStaticObjectMethod(cls.obj(), method, j_url_string.obj()));

  // Create the java.util.Map.
  cls.Reset(GetClass(env, "java/util/HashMap"));
  jmethodID constructor = GetMethodID(env, cls, "<init>", "()V");
  ScopedJavaLocalRef<jobject> j_map(env,
      env->NewObject(cls.obj(), constructor));
  jmethodID put_method = GetMethodID(env, cls, "put",
      "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

  // Construct headers that needs to be sent with the url.
  HeadersMap headers;
  // For incognito mode, we need a header to hide url log.
  if (hide_url_log_)
    headers.insert(std::make_pair("x-hide-urls-from-log", "true"));
  // If cookies are present, add them in the header.
  if (!cookies_.empty())
    headers.insert(std::make_pair("Cookie", cookies_));

  // Fill the Map with the headers.
  for (HeadersMap::const_iterator iter = headers.begin();
       iter != headers.end(); ++iter) {
    ScopedJavaLocalRef<jstring> key = ConvertUTF8ToJavaString(env, iter->first);
    ScopedJavaLocalRef<jstring> value =
        ConvertUTF8ToJavaString(env, iter->second);
    ScopedJavaLocalRef<jobject> result(env,
        env->CallObjectMethod(j_map.obj(), put_method, key.obj(), value.obj()));
  }

  jobject j_context = base::android::GetApplicationContext();
  DCHECK(j_context);

  // Finally- Call the setDataSource method.
  jmethodID set_data_source =
      GetMethodID(env, j_class_, "setDataSource",
      "(Landroid/content/Context;Landroid/net/Uri;Ljava/util/Map;)V");
  DCHECK(set_data_source);
  env->CallVoidMethod(j_media_player_.obj(), set_data_source, j_context,
                      j_uri.obj(), j_map.obj());
  is_data_source_set_ = !base::android::ClearException(env);
  if (is_data_source_set_) {
    if (manager_)
      manager_->RequestMediaResources(this);
    CallVoidMethod("prepareAsync");
  } else
    media_error_cb_.Run(player_id_, MEDIA_ERROR_UNKNOWN);
}

void MediaPlayerBridge::Prepare() {
  if (j_media_player_.is_null())
    InitializePlayer();

  if (has_cookies_)
    PrepareWithCookies(cookies_);
  else
    PrepareWithoutCookies();
}

void MediaPlayerBridge::Start() {
  if (j_media_player_.is_null()) {
    pending_play_ = true;
    Prepare();
  } else {
    if (prepared_)
      StartInternal();
    else
      pending_play_ = true;
  }
}

void MediaPlayerBridge::Pause() {
  if (j_media_player_.is_null())
    pending_play_ = false;
  else {
    if (prepared_ && IsPlaying())
      PauseInternal();
    else
      pending_play_ = false;
  }
}

bool MediaPlayerBridge::IsPlaying() {
  if (!prepared_)
    return pending_play_;
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method = GetMethodID(env, j_class_, "isPlaying", "()Z");
  jint result = env->CallBooleanMethod(j_media_player_.obj(), method);
  CheckException(env);

  return static_cast<bool>(result);
}

int MediaPlayerBridge::GetVideoWidth() {
  if (!prepared_)
    return width_;
  return CallIntMethod("getVideoWidth");
}

int MediaPlayerBridge::GetVideoHeight() {
  if (!prepared_)
    return height_;
  return CallIntMethod("getVideoHeight");
}

void MediaPlayerBridge::SeekTo(int msec) {
  // Record the time to seek when OnMediaPrepared() is called.
  pending_seek_ = msec;

  if (j_media_player_.is_null())
    Prepare();
  else if (prepared_)
    SeekInternal(msec);
}

int MediaPlayerBridge::GetCurrentPosition() {
  if (!prepared_)
    return pending_seek_;
  return CallIntMethod("getCurrentPosition");
}

int MediaPlayerBridge::GetDuration() {
  if (!prepared_)
    return duration_;
  return CallIntMethod("getDuration");
}

void MediaPlayerBridge::Reset() {
  if (!j_media_player_.is_null())
    CallVoidMethod("reset");
}

void MediaPlayerBridge::Release() {
  if (!j_media_player_.is_null()) {
    if (prepared_)
      pending_seek_ = GetCurrentPosition();
    if (manager_)
      manager_->ReleaseMediaResources(this);
    prepared_ = false;
    pending_play_ = false;
    SetVideoSurface(NULL, false);
    CallVoidMethod("release");
    j_media_player_.Reset();
  }
}

void MediaPlayerBridge::SetVolume(float left_volume, float right_volume) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method = GetMethodID(env, j_class_, "setVolume", "(FF)V");
  DCHECK(method);
  jfloat j_left_volume = left_volume;
  jfloat j_right_volume = right_volume;
  env->CallVoidMethod(j_media_player_.obj(), method,
                      j_left_volume, j_right_volume);
  CheckException(env);
}

void MediaPlayerBridge::SetWakeMode(int mode) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jobject j_context = base::android::GetApplicationContext();
  DCHECK(j_context);
  jint j_mode = mode;
  jmethodID method = GetMethodID(env, j_class_,
      "setWakeMode", "(Landroid/content/Context;I)V");
  env->CallVoidMethod(j_media_player_.obj(), method, j_context, j_mode);
  CheckException(env);
}

void MediaPlayerBridge::GetMetadata(bool* can_pause,
                                    bool* can_seek_forward,
                                    bool* can_seek_backward) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method = GetMethodID(env, j_class_, "getMetadata",
      "(ZZ)Landroid/media/Metadata;");
  ScopedJavaLocalRef<jobject> j_metadata(env, env->CallObjectMethod(
      j_media_player_.obj(), method, JNI_FALSE, JNI_FALSE));
  CheckException(env);
  if (j_metadata.is_null())
    return;

  ScopedJavaLocalRef<jclass> cls(GetClass(env, "android/media/Metadata"));
  jmethodID get_boolean = GetMethodID(env, cls, "getBoolean", "(I)Z");
  *can_pause = env->CallBooleanMethod(j_metadata.obj(),
                                      get_boolean,
                                      kPauseAvailable);
  CheckException(env);
  *can_seek_backward = env->CallBooleanMethod(j_metadata.obj(),
                                              get_boolean,
                                              kSeekBackwardAvailable);
  CheckException(env);
  *can_seek_forward = env->CallBooleanMethod(j_metadata.obj(),
                                             get_boolean,
                                             kSeekForwardAvailable);
  CheckException(env);
}


void MediaPlayerBridge::DoTimeUpdate() {
  int current = GetCurrentPosition();
  time_update_cb_.Run(player_id_, current);
}

void MediaPlayerBridge::OnMediaError(int error_type) {
  media_error_cb_.Run(player_id_, error_type);
}

void MediaPlayerBridge::OnVideoSizeChanged(int width,
                                           int height) {
  width_ = width;
  height_ = height_;
  video_size_changed_cb_.Run(player_id_, width, height);
}

void MediaPlayerBridge::OnBufferingUpdate(int percent) {
  buffering_update_cb_.Run(player_id_, percent);
}

void MediaPlayerBridge::OnPlaybackComplete() {
  time_update_timer_.Stop();
  playback_complete_cb_.Run(player_id_);
}

void MediaPlayerBridge::OnSeekComplete() {
  seek_complete_cb_.Run(player_id_, GetCurrentPosition());
}

void MediaPlayerBridge::OnMediaPrepared() {
  if (j_media_player_.is_null())
    return;

  prepared_ = true;

  int dur = duration_;
  duration_ = GetDuration();

  if (duration_ != dur) {
    // Scale the |pending_seek_| according to the new duration.
    pending_seek_ = pending_seek_ * static_cast<float>(
        duration_ / kTemporaryDuration);
  }

  // If media player was recovered from a saved state, consume all the pending
  // events.
  SeekInternal(pending_seek_);

  if (pending_play_) {
    StartInternal();
    pending_play_ = false;
  }

  GetMetadata(&can_pause_, &can_seek_forward_, &can_seek_backward_);

  media_prepared_cb_.Run(player_id_, duration_);
}

void MediaPlayerBridge::StartInternal() {
  CallVoidMethod("start");
  if (!time_update_timer_.IsRunning()) {
    time_update_timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(kTimeUpdateInterval),
        this, &MediaPlayerBridge::DoTimeUpdate);
  }
}

void MediaPlayerBridge::PauseInternal() {
  CallVoidMethod("pause");
  time_update_timer_.Stop();
}

void MediaPlayerBridge::SeekInternal(int msec) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method = GetMethodID(env, j_class_, "seekTo", "(I)V");
  DCHECK(method);
  jint j_msec = msec;
  env->CallVoidMethod(j_media_player_.obj(), method, j_msec);
  CheckException(env);
}

// ---- JNI Helpers for repeated call patterns. ----

void MediaPlayerBridge::CallVoidMethod(std::string method_name) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method = GetMethodID(env, j_class_, method_name.c_str(), "()V");
  env->CallVoidMethod(j_media_player_.obj(), method);
  CheckException(env);
}

int MediaPlayerBridge::CallIntMethod(std::string method_name) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method = GetMethodID(env, j_class_, method_name.c_str(), "()I");
  jint j_result = env->CallIntMethod(j_media_player_.obj(), method);
  CheckException(env);

  return j_result;
}

