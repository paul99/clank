// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/download_controller.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "content/browser/download/download_info_android.h"
#include "content/browser/download/download_types.h"
#include "chrome/browser/download/download_util.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "content/browser/android/chrome_view.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/public/browser/browser_thread.h"
#include "jni/download_controller_jni.h"
#include "googleurl/src/gurl.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetClass;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;
using content::WebContents;

// JNI methods
static void Init(JNIEnv* env, jobject obj) {
  DownloadController::GetInstance()->Init(env, obj);
}

static void PostDownloadOptionsReady(JNIEnv* env,
                                     jobject obj,
                                     int download_id,
                                     jboolean should_download,
                                     jstring jpath) {
  std::string path = base::android::ConvertJavaStringToUTF8(env, jpath);

  DownloadController::GetInstance()->PostDownloadOptionsReady(download_id,
                                                              should_download,
                                                              path);
}

namespace {

static const char* kDownloadControllerClassPathName =
    "org/chromium/content/browser/DownloadController";

}  // namespace

bool RegisterDownloadController(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

// DownloadController implementation

struct DownloadController::JavaObject {
  jweak obj;
  ScopedJavaLocalRef<jobject> Controller(JNIEnv* env) {
    return GetRealObject(env, obj);
  }
};


// static
DownloadControllerAndroid* DownloadControllerAndroid::GetInstance() {
  return DownloadController::GetInstance();
}

// static
DownloadController* DownloadController::GetInstance() {
  return Singleton<DownloadController>::get();
}

DownloadController::DownloadController()
    : java_object_(NULL) {
}

DownloadController::~DownloadController() {
  if (java_object_) {
    JNIEnv* env = AttachCurrentThread();
    env->DeleteWeakGlobalRef(java_object_->obj);
    delete java_object_;
    CheckException(env);
  }
}

// Initialize references to Java object.
void DownloadController::Init(JNIEnv* env, jobject obj) {
  java_object_ = new JavaObject;
  java_object_->obj = env->NewWeakGlobalRef(obj);
}

void DownloadController::PostDownloadOptionsReady(int download_id,
                                                  bool should_download,
                                                  std::string path) {
  // Note: Using mutex_download_options_map_ function may block UI thread,
  // so keep it short.
  DownloadOptions options = mutex_download_options_map_.Get(download_id);

  DownloadInfoAndroid info = options.a;
  info.should_download = should_download;
  info.save_info.file_path = FilePath(path);
  const PostDownloadOptionsCallback& callback = options.b;

  // Save the new options.
  base::Callback<void(DownloadInfoAndroid)> new_callback;
  mutex_download_options_map_.Set(
      download_id,
      MakeTuple(info, new_callback));

  callback.Run(info);
}

void DownloadController::NewGetDownload(DownloadInfoAndroid info) {
  JNIEnv* env = AttachCurrentThread();

  // Call newHttpGetDownload
  jobject view = GetChromeView(info);
  if (!view) {
    // The view went away. Can't proceed.
    LOG(ERROR) << "Download failed on URL:" << info.url.spec();
    return;
  }

  ScopedJavaLocalRef<jstring> jurl =
      ConvertUTF8ToJavaString(env, info.url.spec());
  ScopedJavaLocalRef<jstring> juser_agent =
      ConvertUTF8ToJavaString(env, info.user_agent);
  ScopedJavaLocalRef<jstring> jcontent_disposition =
      ConvertUTF8ToJavaString(env, info.content_disposition);
  ScopedJavaLocalRef<jstring> jmime_type =
      ConvertUTF8ToJavaString(env, info.mime_type);
  ScopedJavaLocalRef<jstring> jcookie =
      ConvertUTF8ToJavaString(env, info.cookie);
  ScopedJavaLocalRef<jstring> jreferer = ConvertUTF8ToJavaString(
      env, info.referrer_url.is_valid() ? info.referrer_url.spec() : "");

  Java_DownloadController_newHttpGetDownload(env,
    java_object()->Controller(env).obj(), view, jurl.obj(), juser_agent.obj(),
    jcontent_disposition.obj(), jmime_type.obj(), jcookie.obj(),
    jreferer.obj(), info.total_bytes);
}

void DownloadController::NewPostDownload(
    DownloadInfoAndroid info,
    const PostDownloadOptionsCallback& callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  JNIEnv* env = AttachCurrentThread();
  jobject view = GetChromeView(info);
  if(!view) {
    // The view went away. Can't proceed.
    return;
  }

  // Save info and callback
  int download_id = info.download_id;
  mutex_download_options_map_.Set(download_id, MakeTuple(info, callback));

  // Call newHttpPostDownload
  ScopedJavaLocalRef<jstring> jurl =
      ConvertUTF8ToJavaString(env, info.url.spec());
  ScopedJavaLocalRef<jstring> jcontent_disposition =
      ConvertUTF8ToJavaString(env, info.content_disposition);
  ScopedJavaLocalRef<jstring> jmime_type =
      ConvertUTF8ToJavaString(env, info.mime_type);

  Java_DownloadController_newHttpPostDownload(env,
      java_object()->Controller(env).obj(), view, download_id, jurl.obj(),
      jcontent_disposition.obj(), jmime_type.obj(), info.total_bytes);
}

void DownloadController::UpdatePostDownloadPath(
    int download_id, FilePath new_path) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  DownloadOptions options = mutex_download_options_map_.Get(download_id);
  options.a.save_info.file_path = new_path;
  mutex_download_options_map_.Set(download_id, options);
}

void DownloadController::OnPostDownloadUpdated(
    const ProgressUpdates& progress_updates) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  JNIEnv* env = AttachCurrentThread();

  Java_DownloadController_httpPostDownloadUpdatesStart(env,
      java_object()->Controller(env).obj());

  ProgressUpdates::const_iterator itr;
  for (itr = progress_updates.begin(); itr != progress_updates.end(); ++itr) {
    int download_id = itr->first;
    int64 bytes_so_far = itr->second;
    DownloadInfoAndroid info = mutex_download_options_map_.Get(download_id).a;

    // Call httpPostDownloadUpdate
    ScopedJavaLocalRef<jstring> jurl =
        ConvertUTF8ToJavaString(env, info.url.spec());
    ScopedJavaLocalRef<jstring> jcontent_disposition =
        ConvertUTF8ToJavaString(env, info.content_disposition);
    ScopedJavaLocalRef<jstring> jmime_type =
        ConvertUTF8ToJavaString(env, info.mime_type);
    ScopedJavaLocalRef<jstring> jpath =
        ConvertUTF8ToJavaString(env, info.save_info.file_path.value());

    Java_DownloadController_httpPostDownloadUpdate(env,
        java_object()->Controller(env).obj(), download_id, jurl.obj(),
        jcontent_disposition.obj(), jmime_type.obj(), jpath.obj(),
        info.total_bytes, bytes_so_far);
  }

  Java_DownloadController_httpPostDownloadUpdatesFinish(env,
    java_object()->Controller(env).obj());
}

void DownloadController::OnPostDownloadCompleted(
    int download_id, int64 total_bytes, bool successful) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  // Delete saved options.
  DownloadInfoAndroid info = mutex_download_options_map_.Get(download_id).a;
  mutex_download_options_map_.Erase(download_id);

  // Call onHttpPostDownloadCompleted
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jurl =
      ConvertUTF8ToJavaString(env, info.url.spec());
  ScopedJavaLocalRef<jstring> jcontent_disposition =
      ConvertUTF8ToJavaString(env, info.content_disposition);
  ScopedJavaLocalRef<jstring> jmime_type =
      ConvertUTF8ToJavaString(env, info.mime_type);
  ScopedJavaLocalRef<jstring> jpath =
      ConvertUTF8ToJavaString(env, info.save_info.file_path.value());

  Java_DownloadController_onHttpPostDownloadCompleted(env,
      java_object()->Controller(env).obj(), info.download_id, jurl.obj(),
      jcontent_disposition.obj(), jmime_type.obj(), jpath.obj(), total_bytes,
      successful);
}

jobject DownloadController::GetChromeView(DownloadInfoAndroid info) {
  WebContents* web_contents = tab_util::GetWebContentsByID(
      info.child_id, info.render_view_id);

  if (!web_contents) {
    LOG(ERROR) << "Failed to find tab for renderer_process_id: "
               << info.child_id << " and tab_contents_id: "
               << info.render_view_id;
    // Probably the tab is closed.
    return NULL;
  }

  RenderWidgetHostViewAndroid* rwhv = static_cast<RenderWidgetHostViewAndroid*>(
      web_contents->GetRenderWidgetHostView());
  if (!rwhv) {
    // RenderWidgetHostView can be null during setup or teardown of a tab.
    return NULL;
  }

  ChromeView* view = rwhv->GetChromeView();
  // RWHVA holds a weak pointer to chrome view.
  return view ? view->GetJavaObject() : NULL;
}

DownloadController::JavaObject* DownloadController::java_object() {
  if (!java_object_) {
    // Initialize Java DownloadController by calling
    // DownloadController.getInsatnce(), which will call InitNativeInstance()
    // if Java DownloadController is not instantiated already.
    JNIEnv* env = AttachCurrentThread();
    ScopedJavaLocalRef<jclass> clazz =
        GetClass(env, kDownloadControllerClassPathName);
    jmethodID get_instance = GetStaticMethodID(env, clazz, "getInstance",
        "()Lorg/chromium/content/browser/DownloadController;");
    ScopedJavaLocalRef<jobject> jobj(env,
        env->CallStaticObjectMethod(clazz.obj(), get_instance));
    CheckException(env);
  }

  DCHECK(java_object_);
  return java_object_;
}

DownloadController::MutexMap::MutexMap() {
}

DownloadController::MutexMap::~MutexMap() {
}

DownloadController::DownloadOptions DownloadController::MutexMap::Get(
    int download_id) const {
  base::AutoLock lock(options_lock_);
  DownloadOptionsMap::const_iterator itr =
    download_options_map_.find(download_id);
  DCHECK(itr != download_options_map_.end());
  return itr->second;
}

void DownloadController::MutexMap::Set(
    int download_id,
    const DownloadController::DownloadOptions& options) {
  base::AutoLock lock(options_lock_);
  download_options_map_[download_id] = options;
}

void DownloadController::MutexMap::Erase(int download_id) {
  base::AutoLock lock(options_lock_);
  download_options_map_.erase(download_id);
}
