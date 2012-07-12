// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_MEDIA_PLAYER_BRIDGE_H_
#define CONTENT_BROWSER_ANDROID_MEDIA_PLAYER_BRIDGE_H_

#include <jni.h>
#include <map>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/memory/scoped_ptr.h"

class MediaPlayerBridge {
 public:
  class MediaPlayerListener {
   public:
    virtual void OnNotify(int msg, int ext1, int ext2) = 0;
    virtual ~MediaPlayerListener() {}
  };

  MediaPlayerBridge();
  ~MediaPlayerBridge();

  typedef std::map<std::string, std::string> HeadersMap;
  void SetDataSource(const std::string& url, const HeadersMap& headers);

  void SetVideoSurface(jobject surface);

  // Set the listener callback. This takes ownership of the callback
  // and will delete the old one if you set a new one. You may set
  // call this with NULL, in which case any existing callback will be
  // deleted.
  void SetListener(MediaPlayerListener* listener);

  void PrepareAsync();
  void Start();
  void Pause();
  void Stop();
  bool IsPlaying();
  void GetVideoWidth(int* w);
  void GetVideoHeight(int* h);
  void SeekTo(int msec);
  void GetCurrentPosition(int* msec);
  void GetDuration(int* msec);
  void Reset();
  void Release();
  void SetVolume(float leftVolume, float rightVolume);
  void GetMetadata(bool* can_pause,
                   bool* can_seek_forward,
                   bool* can_seek_backward);
  void SetWakeMode(int mode);

  // Called by the Java MediaPlayerListener
  void Notify(int msg, int ext1, int ext2);

 private:
  void CallVoidMethod(std::string method_name);
  void CallIntMethod(std::string method_name, int* result);

  scoped_ptr<MediaPlayerListener> listener_;

  // Java MediaPlayer class and instance.
  base::android::ScopedJavaGlobalRef<jclass> j_class_;
  base::android::ScopedJavaGlobalRef<jobject> j_media_player_;
};

#endif  // CONTENT_BROWSER_ANDROID_MEDIA_PLAYER_BRIDGE_H_
