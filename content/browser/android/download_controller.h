// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This class pairs with DownloadController in Java side to forward requests
// from Chrome downloads to the current DownloadListener.
//
// Both classes are Singleton classes. C++ object owns Java object.
//
// Call sequence as of March 2011
// GET downloads:
// DownloadController::NewGetDownload() =>
// DownloadController.DownloadnewDownload() =>
// DownloadListener.onDownloadStart() /
// DownloadListener2.requestHttpGetDownload()
//
// POST downloads:
// DownloadController::NewPostDownload() =>
// DownloadController.newHttpPostDownload() =>
// DownloadListener2.onNewHttpPostDownload() =>
// *DownloadListener2 sets options*
// DownloadListener2.HttpPostDownloadOptions.done() =>
// DownloadController::PostDownloadOptionsReady()
//
// DownloadController::OnPostDownloadUpdated() =>
//   => DownloadController.httpPostDownloadUpdatesStart()
//   => DownloadController.httpPostDownloadUpdate() ...
//   => DownloadController.httpPostDownloadUpdatesFinish()
//
// DownloadController::OnPostDownloadCompleted() =>
// DownloadController.onHttpPostDownloadCompleted() =>
// DownloadListener.onHttpPostDownloadCompleted()

#ifndef CONTENT_BROWSER_ANDROID_DOWNLOAD_CONTROLLER_H_
#define CONTENT_BROWSER_ANDROID_DOWNLOAD_CONTROLLER_H_
#pragma once

#include <map>
#include <string>

#include "base/file_path.h"
#include "base/memory/singleton.h"
#include "base/synchronization/lock.h"
#include "base/tuple.h"
#include "content/browser/download/download_controller_android.h"
#include "content/browser/android/jni_helper.h"

struct DownloadInfoAndroid;

class DownloadController : public DownloadControllerAndroid {
 public:
  static DownloadController* GetInstance();

  // =========================================================
  // Methods called from Java DownloadController

  // Called when DownloadController Java object is instantiated.
  void Init(JNIEnv* env, jobject obj);

  // Called when Java application finished setting options for POST download.
  // Can be called from any thread.
  void PostDownloadOptionsReady(int download_id,
                                bool should_download,
                                std::string path);

  // DownloadControllerAndroid implementation. See DownloadControllerAndroid
  // for comments.
  virtual void NewGetDownload(DownloadInfoAndroid info) OVERRIDE;
  virtual void NewPostDownload(
      DownloadInfoAndroid info,
      const PostDownloadOptionsCallback& callback) OVERRIDE;
  virtual void UpdatePostDownloadPath(int download_id,
                                      FilePath new_path) OVERRIDE;
  virtual void OnPostDownloadUpdated(
      const ProgressUpdates& progress_updates) OVERRIDE;
  virtual void OnPostDownloadCompleted(
      int download_id, int64 total_bytes, bool successful) OVERRIDE;

 private:
  friend struct DefaultSingletonTraits<DownloadController>;
  DownloadController();
  virtual ~DownloadController();

  // Get the Java ChromeView object of the tab that initiated the download.
  // This may return NULL if the tab went away.
  jobject GetChromeView(DownloadInfoAndroid info);

  struct JavaObject;
  JavaObject* java_object_;

  // Creates Java object if it is not created already and returns it.
  JavaObject* java_object();

  // Stores DownloadInfoAndroid while Java application is determining the
  // download options. Mostly accessed on UI thread, except in
  // PostDownloadOptionsReady method.
  typedef Tuple2<DownloadInfoAndroid, PostDownloadOptionsCallback>
      DownloadOptions;
  typedef std::map<int, DownloadOptions> DownloadOptionsMap;

  // Wrapper class to ensure lock acquired for access to DownloadOptionsMap.
  class MutexMap {
   public:
     MutexMap();
     ~MutexMap();

     DownloadOptions Get(int download_id) const;
     void Set(int download_id, const DownloadOptions& options);
     void Erase(int download_id);
   private:
    DownloadOptionsMap download_options_map_;
    mutable base::Lock options_lock_;
  };

  MutexMap mutex_download_options_map_;
};

bool RegisterDownloadController(JNIEnv* env);

#endif  // CONTENT_BROWSER_ANDROID_DOWNLOAD_CONTROLLER_H_
