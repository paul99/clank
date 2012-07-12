// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/website_settings_utils.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/file_path.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browsing_data_local_storage_helper.h"
#include "chrome/browser/content_settings/host_content_settings_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "jni/website_settings_utils_jni.h"
#include "webkit/quota/quota_manager.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetClass;
using base::android::GetMethodID;
using base::android::JavaRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;

static const char* kArrayListClass = "java/util/ArrayList";
static const char* kBooleanClass = "java/lang/Boolean";
static const char* kHashMapClass = "java/util/HashMap";

static HostContentSettingsMap* GetHostContentSettingsMap() {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  return profile->GetHostContentSettingsMap();
}

static jobject GetGeolocationOrigins(JNIEnv* env, jclass clazz) {
  ScopedJavaLocalRef<jclass> array_clazz = GetClass(env, kArrayListClass);
  jmethodID array_constructor = GetMethodID(env, array_clazz, "<init>", "()V");
  jmethodID array_add =
      GetMethodID(env, array_clazz, "add", "(Ljava/lang/Object;)Z");

  ScopedJavaLocalRef<jclass> geolocation_info_clazz =
      GetClass(env, kGeolocationInfoClassPath);
  jmethodID geolocation_info_constructor =
      GetMethodID(env, geolocation_info_clazz, "<init>", "()V");

  ScopedJavaLocalRef<jobject> jarray(
      env, env->NewObject(array_clazz.obj(), array_constructor));
  DCHECK(jarray.obj());

  ContentSettingsForOneType all_settings;
  HostContentSettingsMap* content_settings_map = GetHostContentSettingsMap();
  content_settings_map->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_GEOLOCATION,
      std::string(),
      &all_settings);
  ContentSetting default_content_setting = content_settings_map->
      GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_GEOLOCATION, NULL);

  // Now add all origins that have a non-default setting to the list.
  ContentSettingsForOneType::const_iterator i =
      all_settings.begin();
  for (; i != all_settings.end(); ++i) {
    if (i->setting == default_content_setting) continue;
    ScopedJavaLocalRef<jobject> jinfo(
        env, env->NewObject(geolocation_info_clazz.obj(),
                            geolocation_info_constructor));
    const std::string origin = i->primary_pattern.ToString();
    const std::string embedded = i->secondary_pattern.ToString();
    ScopedJavaLocalRef<jstring> jorigin = ConvertUTF8ToJavaString(env, origin);
    if (embedded.compare(origin) != 0) {
      ScopedJavaLocalRef<jstring> jembedded =
          ConvertUTF8ToJavaString(env, embedded);
      Java_GeolocationInfo_setInfo(
          env, jinfo.obj(), jorigin.obj(), jembedded.obj());
    } else {
      Java_GeolocationInfo_setInfo(
          env, jinfo.obj(), jorigin.obj(), NULL);
    }
    env->CallBooleanMethod(jarray.obj(), array_add, jinfo.obj());
  }
  return jarray.Release();
}

static jobject GetGeolocationSettingForOrigin(JNIEnv* env, jclass clazz,
    jstring origin, jstring embedder) {
  GURL url(ConvertJavaStringToUTF8(env, origin));
  GURL embedder_url(ConvertJavaStringToUTF8(env, embedder));
  ContentSetting setting = GetHostContentSettingsMap()->GetContentSetting(
      url,
      embedder_url,
      CONTENT_SETTINGS_TYPE_GEOLOCATION,
      std::string());

  bool allow;
  switch (setting) {
    case CONTENT_SETTING_ALLOW:
      allow = true;
      break;
    case CONTENT_SETTING_BLOCK:
      allow = false;
      break;
    default:
      return NULL;
  }
  ScopedJavaLocalRef<jclass> boolean_clazz = GetClass(env, kBooleanClass);
  jmethodID constructor =
      env->GetMethodID(boolean_clazz.obj(), "<init>", "(Z)V");
  ScopedJavaLocalRef<jobject> object(
      env, env->NewObject(boolean_clazz.obj(), constructor, allow));
  return object.Release();
}

static void SetGeolocationSettingForOrigin(JNIEnv* env, jclass clazz,
    jstring origin, jstring embedder, jobject value) {
  GURL url(ConvertJavaStringToUTF8(env, origin));
  GURL embedder_url(ConvertJavaStringToUTF8(env, embedder));
  ContentSetting setting = CONTENT_SETTING_DEFAULT;
  ScopedJavaLocalRef<jclass> boolean_clazz = GetClass(env, kBooleanClass);
  if (value != NULL &&
      env->IsInstanceOf(value, boolean_clazz.obj()) == JNI_TRUE) {
    jmethodID boolean_value =
        env->GetMethodID(boolean_clazz.obj(), "booleanValue", "()Z");
    DCHECK(boolean_value);
    setting = env->CallBooleanMethod(value, boolean_value) == JNI_TRUE ?
        CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK;
  }
  GetHostContentSettingsMap()->SetContentSetting(
      ContentSettingsPattern::FromURLNoWildcard(url),
      ContentSettingsPattern::FromURLNoWildcard(embedder_url),
      CONTENT_SETTINGS_TYPE_GEOLOCATION,
      std::string(),
      setting);
}

namespace {

class StorageInfoFetcher :
      public base::RefCountedThreadSafe<StorageInfoFetcher> {
 public:
  StorageInfoFetcher(quota::QuotaManager* quota_manager,
                     const JavaRef<jobject>& java_callback) :
      quota_manager_(quota_manager),
      java_callback_(java_callback) {}

  virtual ~StorageInfoFetcher() {}

  void Run() {
    // QuotaManager must be called on IO thread, but java_callback must then be
    // called back on UI thread.
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&StorageInfoFetcher::GetUsageInfo, this));
  }

 private:
  void GetUsageInfo() {
    // We will have no explicit owner as soon as we leave this method.
    AddRef();
    quota_manager_->GetUsageInfo(
        base::Bind(&StorageInfoFetcher::OnGetUsageInfo, this));
  }

  void OnGetUsageInfo(const quota::UsageInfoEntries& entries) {
    entries_.insert(entries_.begin(), entries.begin(), entries.end());
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&StorageInfoFetcher::InvokeCallback, this));
    Release();
  }

  void InvokeCallback() {
    ScopedJavaLocalRef<jclass> array_clazz = GetClass(env(), kArrayListClass);
    jmethodID array_constructor =
        GetMethodID(env(), array_clazz, "<init>", "()V");
    jmethodID array_add =
        GetMethodID(env(), array_clazz, "add", "(Ljava/lang/Object;)Z");

    ScopedJavaLocalRef<jclass> storage_info_clazz =
        GetClass(env(), kStorageInfoClassPath);
    jmethodID storage_info_constructor =
        GetMethodID(env(), storage_info_clazz, "<init>", "()V");

    ScopedJavaLocalRef<jobject> jarray(
        env(), env()->NewObject(array_clazz.obj(), array_constructor));
    DCHECK(jarray.obj());
    quota::UsageInfoEntries::const_iterator i;
    for (i = entries_.begin(); i != entries_.end(); ++i) {
      if (i->usage <= 0) continue;
      ScopedJavaLocalRef<jobject> jinfo(
          env(), env()->NewObject(storage_info_clazz.obj(),
                                  storage_info_constructor));
      ScopedJavaLocalRef<jstring> host =
          ConvertUTF8ToJavaString(env(), i->host);
      Java_StorageInfo_setInfo(
          env(), jinfo.obj(), host.obj(), i->type, i->usage);
      env()->CallBooleanMethod(jarray.obj(), array_add, jinfo.obj());
    }
    Java_StorageInfoReadyCallback_onStorageInfoReady(
        env(), java_callback_.obj(), jarray.obj());
  }

  JNIEnv* env() { return java_callback_.env(); }

  quota::QuotaManager* quota_manager_;
  ScopedJavaGlobalRef<jobject> java_callback_;
  quota::UsageInfoEntries entries_;

  DISALLOW_COPY_AND_ASSIGN(StorageInfoFetcher);
};

class StorageDataDeleter :
      public base::RefCountedThreadSafe<StorageDataDeleter> {
 public:
  StorageDataDeleter(quota::QuotaManager* quota_manager,
                     const std::string& host,
                     quota::StorageType type,
                     const JavaRef<jobject>& java_callback) :
      quota_manager_(quota_manager),
      host_(host),
      type_(type),
      java_callback_(java_callback) {}

  virtual ~StorageDataDeleter() {}

  void Run() {
    // QuotaManager must be called on IO thread, but java_callback must then be
    // called back on UI thread.  Grant ourself an extra reference to avoid
    // being deleted after DeleteHostData will return.
    AddRef();
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&quota::QuotaManager::DeleteHostData,
                   quota_manager_,
                   host_,
                   type_,
                   base::Bind(&StorageDataDeleter::OnHostDataDeleted,
                              this)));
  }

 private:
  void OnHostDataDeleted(quota::QuotaStatusCode) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&StorageDataDeleter::InvokeCallback, this));
    Release();
  }

  void InvokeCallback() {
    Java_StorageInfoClearedCallback_onStorageInfoCleared(
        env(), java_callback_.obj());
  }

  JNIEnv* env() { return java_callback_.env(); }

  quota::QuotaManager* quota_manager_;
  std::string host_;
  quota::StorageType type_;
  ScopedJavaGlobalRef<jobject> java_callback_;
};

class LocalStorageInfoReadyCallback {
 public:
  LocalStorageInfoReadyCallback(
      const ScopedJavaLocalRef<jobject>& java_callback) :
      java_callback_(java_callback) {}

  void OnLocalStorageModelInfoLoaded(
      const std::list<BrowsingDataLocalStorageHelper::LocalStorageInfo>&
          local_storage_info) {
    ScopedJavaLocalRef<jclass> map_clazz = GetClass(env(), kHashMapClass);
    jmethodID map_constructor = GetMethodID(env(), map_clazz, "<init>", "()V");
    jmethodID map_put = GetMethodID(env(), map_clazz, "put",
        "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    ScopedJavaLocalRef<jclass> local_storage_info_clazz =
        GetClass(env(), kLocalStorageInfoClassPath);
    jmethodID local_storage_info_constructor =
        GetMethodID(env(), local_storage_info_clazz, "<init>", "()V");

    ScopedJavaLocalRef<jobject> jmap(
        env(), env()->NewObject(map_clazz.obj(), map_constructor));
    DCHECK(jmap.obj());

    std::list<BrowsingDataLocalStorageHelper::LocalStorageInfo>::const_iterator i;
    for (i = local_storage_info.begin(); i != local_storage_info.end(); ++i) {
      ScopedJavaLocalRef<jobject> jinfo(
          env(), env()->NewObject(local_storage_info_clazz.obj(),
                                  local_storage_info_constructor));
      ScopedJavaLocalRef<jstring> origin =
          ConvertUTF8ToJavaString(env(), i->origin);
      ScopedJavaLocalRef<jstring> path =
          ConvertUTF8ToJavaString(env(), i->file_path.value());
      Java_LocalStorageInfo_setInfo(env(), jinfo.obj(), path.obj(), i->size);
      env()->CallObjectMethod(jmap.obj(), map_put, origin.obj(), jinfo.obj());
    }

    Java_LocalStorageInfoReadyCallback_onLocalStorageInfoReady(
        env(), java_callback_.obj(), jmap.obj());
    delete this;
  }

 private:
  JNIEnv* env() { return java_callback_.env(); }

  ScopedJavaGlobalRef<jobject> java_callback_;
};

}  // anonymous namespace

// TODO(jknotten): These methods should not be static. Instead we should
// expose a class to Java so that the fetch requests can be cancelled,
// and manage the lifetimes of the callback (and indirectly the helper
// by having a reference to it).

// The helper methods (StartFetching, DeleteLocalStorageFile, DeleteDatabase)
// are asynchronous. A "use after free" error is not possible because the
// helpers keep a reference to themselves for the duration of their tasks,
// which includes callback invocation.

static void FetchLocalStorageInfo(JNIEnv* env, jclass clazz,
    jobject java_callback) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  scoped_refptr<BrowsingDataLocalStorageHelper> local_storage_helper(
      new BrowsingDataLocalStorageHelper(profile));
  // local_storage_callback will delete itself when it is run.
  LocalStorageInfoReadyCallback* local_storage_callback =
      new LocalStorageInfoReadyCallback(
          ScopedJavaLocalRef<jobject>(env, java_callback));
  local_storage_helper->StartFetching(
      base::Bind(&LocalStorageInfoReadyCallback::OnLocalStorageModelInfoLoaded,
                 base::Unretained(local_storage_callback)));
}

static void FetchStorageInfo(JNIEnv* env, jclass clazz, jobject java_callback) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  scoped_refptr<StorageInfoFetcher> storage_info_fetcher(new StorageInfoFetcher(
      profile->GetQuotaManager(),
      ScopedJavaLocalRef<jobject>(env, java_callback)));
  storage_info_fetcher->Run();
}

static void ClearLocalStorageData(JNIEnv* env, jclass clazz, jstring jorigin) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  scoped_refptr<BrowsingDataLocalStorageHelper> local_storage_helper =
      new BrowsingDataLocalStorageHelper(profile);
  std::string origin = ConvertJavaStringToUTF8(env, jorigin);
  local_storage_helper->DeleteLocalStorageFile(FilePath(origin));
}

static quota::StorageType IntToStorageType(jint type) {
  if (type == quota::kStorageTypeTemporary)
    return quota::kStorageTypeTemporary;
  else if (type == quota::kStorageTypePersistent)
    return quota::kStorageTypePersistent;
  else
    return quota::kStorageTypeUnknown;
}

static void ClearStorageData(
    JNIEnv* env, jclass clazz, jstring jhost, jint type, jobject java_callback) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  std::string host = ConvertJavaStringToUTF8(env, jhost);
  scoped_refptr<StorageDataDeleter> storage_data_deleter(new StorageDataDeleter(
      profile->GetQuotaManager(),
      host,
      IntToStorageType(type),
      ScopedJavaLocalRef<jobject>(env, java_callback)));
  storage_data_deleter->Run();
}

// Register native methods
bool RegisterWebsiteSettingsUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
