// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/media_player_bridge.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/basictypes.h"
#include "base/logging.h"
#include "base/stringprintf.h"
#include "content/browser/android/media_player_listener.h"

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

MediaPlayerBridge::MediaPlayerBridge() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  j_class_.Reset(GetClass(env, "android/media/MediaPlayer"));

  jmethodID constructor = GetMethodID(env, j_class_, "<init>", "()V");
  ScopedJavaLocalRef<jobject> tmp(env,
      env->NewObject(j_class_.obj(), constructor));
  DCHECK(!tmp.is_null());
  j_media_player_.Reset(tmp);
}

MediaPlayerBridge::~MediaPlayerBridge() {
}

void MediaPlayerBridge::SetDataSource(
    const std::string& url,
    const std::map<std::string, std::string>& headers) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  // Create a Java String for the URL.
  ScopedJavaLocalRef<jstring> j_url_string = ConvertUTF8ToJavaString(env, url);

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
  DCHECK(!j_map.is_null());
  jmethodID put_method = GetMethodID(env, cls, "put",
      "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

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
  jmethodID set_data_source = GetMethodID(env, j_class_, "setDataSource",
      "(Landroid/content/Context;Landroid/net/Uri;Ljava/util/Map;)V");
  DCHECK(set_data_source);
  env->CallVoidMethod(j_media_player_.obj(), set_data_source, j_context,
                      j_uri.obj(), j_map.obj());
  CheckException(env);
}

void MediaPlayerBridge::SetVideoSurface(jobject surface) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method =
      GetMethodID(env, j_class_, "setSurface", "(Landroid/view/Surface;)V");
  env->CallVoidMethod(j_media_player_.obj(), method, surface);
  CheckException(env);
}

void MediaPlayerBridge::SetListener(MediaPlayerListener* listener) {
  listener_.reset(listener);

  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  ScopedJavaLocalRef<jobject> j_listener;
  if (listener)
    j_listener = CreateMediaPlayerListener(env, this);

  // Set it as the various listeners.
  const char* listeners[] = {
      "OnBufferingUpdateListener",
      "OnCompletionListener",
      "OnErrorListener",
      "OnInfoListener",
      "OnPreparedListener",
      "OnSeekCompleteListener",
      "OnVideoSizeChangedListener",
  };
  for (unsigned int i = 0; i < arraysize(listeners); ++i) {
    std::string signature = StringPrintf("(Landroid/media/MediaPlayer$%s;)V",
                                         listeners[i]);
    std::string method_name = StringPrintf("set%s", listeners[i]);
    jmethodID method = GetMethodID(env, j_class_, method_name.c_str(),
                                   signature.c_str());
    env->CallVoidMethod(j_media_player_.obj(), method, j_listener.obj());
    CheckException(env);
  }
}

void MediaPlayerBridge::PrepareAsync() {
  CallVoidMethod("prepareAsync");
}

void MediaPlayerBridge::Start() {
  CallVoidMethod("start");
}

void MediaPlayerBridge::Pause() {
  CallVoidMethod("pause");
}

void MediaPlayerBridge::Stop() {
  CallVoidMethod("stop");
}

bool MediaPlayerBridge::IsPlaying() {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method = GetMethodID(env, j_class_, "isPlaying", "()Z");
  jint result = env->CallBooleanMethod(j_media_player_.obj(), method);
  CheckException(env);

  return static_cast<bool>(result);
}

void MediaPlayerBridge::GetVideoWidth(int* w) {
  CallIntMethod("getVideoWidth", w);
}

void MediaPlayerBridge::GetVideoHeight(int* h) {
  CallIntMethod("getVideoHeight", h);
}

void MediaPlayerBridge::SeekTo(int msec) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method = GetMethodID(env, j_class_, "seekTo", "(I)V");
  DCHECK(method);
  jint j_msec = msec;
  env->CallVoidMethod(j_media_player_.obj(), method, j_msec);
  CheckException(env);
}

void MediaPlayerBridge::GetCurrentPosition(int* msec) {
  CallIntMethod("getCurrentPosition", msec);
}

void MediaPlayerBridge::GetDuration(int* msec) {
  CallIntMethod("getDuration", msec);
}

void MediaPlayerBridge::Reset() {
  CallVoidMethod("reset");
}

void MediaPlayerBridge::Release() {
  CallVoidMethod("release");
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

void MediaPlayerBridge::Notify(int msg, int ext1, int ext2) {
  if (listener_.get())
    listener_->OnNotify(msg, ext1, ext2);
}

void MediaPlayerBridge::GetMetadata(bool* can_pause,
                                    bool* can_seek_forward,
                                    bool* can_seek_backward) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method =
      GetMethodID(env, j_class_, "getMetadata", "(ZZ)Landroid/media/Metadata;");
  ScopedJavaLocalRef<jobject> j_metadata(env,
      env->CallObjectMethod(j_media_player_.obj(), method, JNI_FALSE, JNI_FALSE));
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

// ---- JNI Helpers for repeated call patterns. ----

void MediaPlayerBridge::CallVoidMethod(std::string method_name) {
  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method = GetMethodID(env, j_class_, method_name.c_str(), "()V");
  env->CallVoidMethod(j_media_player_.obj(), method);
  CheckException(env);
}

void MediaPlayerBridge::CallIntMethod(std::string method_name, int* result) {
  DCHECK(result);

  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jmethodID method = GetMethodID(env, j_class_, method_name.c_str(), "()I");
  jint j_result = env->CallIntMethod(j_media_player_.obj(), method);
  CheckException(env);

  *result = static_cast<int>(j_result);
}
