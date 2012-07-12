// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "chrome/browser/sync/glue/synced_window_delegate_registry.h"
#include "chrome/common/chrome_notification_types.h"
#include "clank/native/framework/chrome/tab.h"
#include "clank/native/framework/chrome/tab_model.h"
#include "content/browser/android/jni_helper.h"
#include "content/public/browser/notification_service.h"
#include "jni/chrome_view_list_adapter_jni.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::GetFieldID;
using base::android::HasField;
using base::android::ScopedJavaLocalRef;
using content::NotificationService;

struct TabModel::JavaObject {
  jweak obj_;
  ScopedJavaLocalRef<jobject> TabModel(JNIEnv* env) {
    return GetRealObject(env, obj_);
  }
};

TabModel::TabModel(JNIEnv* env, jobject obj) {
  java_object_ = new JavaObject;
  java_object_->obj_ = env->NewWeakGlobalRef(obj);
}

TabModel::~TabModel() {
  if (java_object_->obj_) {
    JNIEnv* env = AttachCurrentThread();
    env->DeleteWeakGlobalRef(java_object_->obj_);
    java_object_->obj_ = 0;
  }
  delete java_object_;
}

void TabModel::Destroy(JNIEnv* env, jobject obj) {
  browser_sync::SyncedWindowDelegateRegistry::Unregister(this);
  delete this;
}

// megamerge: Seems like GetWindowId() is obsolete and should be replaced with a call to GetSessionId()
int TabModel::GetWindowId(JNIEnv* env, jobject obj) {
  return session_id_.id();
}

bool TabModel::IsTabPinned(const browser_sync::SyncedTabDelegate* tab) const {
  return false;
}

SessionID::id_type TabModel::GetTabIdAt(int index) const {
  Tab* tab = GetNativeTab(index);
  return tab == NULL ? -1 : tab->id();
}

browser_sync::SyncedTabDelegate* TabModel::GetTabAt(int index) const {
  Tab* tab = GetNativeTab(index);
  return tab == NULL ? NULL : tab->synced_tab_delegate();
}

bool TabModel::HasWindow() const {
  return !is_incognito();
}

bool TabModel::is_incognito() const {
  JNIEnv* env = AttachCurrentThread();
  return Java_ChromeTabModel_isIncognito(
      env, java_object_->TabModel(env).obj());
}

void TabModel::CreateTab(content::WebContents* web_contents) const {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeTabModel_createTabWithNativeContents(
      env, java_object_->TabModel(env).obj(),
      reinterpret_cast<int>(web_contents));
}

SessionID::id_type TabModel::GetSessionId() const {
  return session_id_.id();
}

int TabModel::GetTabCount() const {
  JNIEnv* env = AttachCurrentThread();
  jint count = Java_ChromeTabModel_getCount(
      env, java_object_->TabModel(env).obj());
  return count;
}
int TabModel::GetActiveIndex() const {
  JNIEnv* env = AttachCurrentThread();
  jint index = Java_ChromeTabModel_index(
      env, java_object_->TabModel(env).obj());
  return index;
}

bool TabModel::IsApp() const {
  return false;
}

bool TabModel::IsTypeTabbed() const {
  return true;
}

bool TabModel::IsTypePopup() const {
  return false;
}

Tab* TabModel::GetNativeTab(int index) const {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj =
      Java_ChromeTabModel_getTab(env, java_object_->TabModel(env).obj(), index);
  if (obj.is_null()) {
    VLOG(0) << "Failed to get java tab";
    return NULL;
  }
  Tab* tab = Tab::GetNativeTab(env, obj.obj());
  if (!tab) {
    VLOG(0) << "Failed to get native tab";
    return NULL;
  }
  return tab;
}

bool TabModel::AreAllTabsLoaded() const {
  JNIEnv* env = AttachCurrentThread();
  return Java_ChromeTabModel_areAllTabsLoaded(
      env, java_object_->TabModel(env).obj());
}

void TabModel::BroadcastAllTabsLoaded(JNIEnv* env, jobject obj) const {
  NotificationService::current()->Notify(
      chrome::NOTIFICATION_ALL_TABS_LOADED,
      NotificationService::AllSources(),
      NotificationService::NoDetails());
}

void TabModel::OpenClearBrowsingData() const {
  JNIEnv* env = AttachCurrentThread();
  Java_ChromeTabModel_openClearBrowsingData(env,
      java_object_->TabModel(env).obj());
}

// ----------------------------------------------------------------------------
// Native JNI methods
// ----------------------------------------------------------------------------

static jint Init(JNIEnv* env, jobject obj) {
  TabModel* tab_model = new TabModel(env, obj);
  browser_sync::SyncedWindowDelegateRegistry::Register(tab_model);
  return reinterpret_cast<jint>(tab_model);
}

// Register native methods

bool RegisterTabModel(JNIEnv* env) {
  if (RegisterNativesImpl(env) < 0)
    return false;
  return true;
}
