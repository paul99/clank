// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/chrome_native_preferences.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/logging.h"
#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/memory/scoped_ptr.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/autofill/field_types.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browsing_data_remover.h"
#include "chrome/browser/browser_about_handler.h"
#include "chrome/browser/content_settings/host_content_settings_map.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/password_manager/password_store.h"
#include "chrome/browser/password_manager/password_store_consumer.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/search_engines/template_url_prepopulate_data.h"
#include "chrome/browser/search_engines/template_url_service.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/content_settings.h"
#include "chrome/common/content_settings_pattern.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_service.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_version_info.h"
#include "googleurl/src/gurl.h"
#include "jni/chrome_native_preferences_jni.h"
#include "webkit/forms/password_form.h"
#include "webkit/glue/webkit_glue.h"
#include "ui/base/l10n/l10n_util.h"
#include "base/file_util.h"
#include "webkit/glue/user_agent.h"
#include "webkit/plugins/npapi/plugin_list.h"
#include "webkit/plugins/webplugininfo.h"
#include "ui/base/resource/resource_bundle.h"
#include "v8/include/v8.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF16ToJavaString;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetClass;
using base::android::HasClass;
using base::android::ScopedJavaLocalRef;
using base::android::ToJavaArrayOfStrings;
using content::BrowserThread;

namespace {

struct FieldIds {
  // Field ids
  jfieldID search_engine;
  jfieldID accept_cookies_enabled;
  jfieldID remember_passwords_enabled;
  jfieldID autofill_enabled;
  jfieldID allow_location_enabled;
  jfieldID font_scale_factor;
  jfieldID force_enable_zoom;
  jfieldID minimum_font_size;
  jfieldID javascript_enabled;
  jfieldID allow_popups_enabled;
  jfieldID sync_account_name;
  jfieldID sync_setup_completed;
  jfieldID sync_suppress_start;
  jfieldID sync_everything;
  jfieldID sync_bookmarks;
  jfieldID sync_typed_urls;
  jfieldID sync_sessions;
  jfieldID user_agent;
  jfieldID autologin_enabled;
  jfieldID search_suggest_enabled;
  jfieldID network_prediction_enabled;
  jfieldID resolve_navigation_error_enabled;
  jfieldID remote_debugging_enabled;

  void Initialize(JNIEnv* env, const ScopedJavaLocalRef<jclass>& clazz);
};

void FieldIds::Initialize(JNIEnv* env,
                          const ScopedJavaLocalRef<jclass>& clazz) {
  search_engine = GetFieldID(env, clazz, "mSearchEngine", "I");
  accept_cookies_enabled = GetFieldID(env, clazz, "mAcceptCookiesEnabled", "Z");
  remember_passwords_enabled =
      GetFieldID(env, clazz, "mRememberPasswordsEnabled", "Z");
  autofill_enabled = GetFieldID(env, clazz, "mAutofillEnabled", "Z");
  allow_location_enabled = GetFieldID(env, clazz, "mAllowLocationEnabled", "Z");
  font_scale_factor = GetFieldID(env, clazz, "mFontScaleFactor", "F");
  force_enable_zoom = GetFieldID(env, clazz, "mForceEnableZoom", "Z");
  minimum_font_size = GetFieldID(env, clazz, "mMinimumFontSize", "I");
  javascript_enabled = GetFieldID(env, clazz, "mJavaScriptEnabled", "Z");
  allow_popups_enabled = GetFieldID(env, clazz, "mAllowPopupsEnabled", "Z");
  user_agent = GetFieldID(env, clazz, "mUserAgent", "Ljava/lang/String;");
  sync_suppress_start = GetFieldID(env, clazz, "mSyncSuppressStart", "Z");
  sync_account_name =
      GetFieldID(env, clazz, "mSyncAccountName", "Ljava/lang/String;");
  sync_setup_completed = GetFieldID(env, clazz, "mSyncSetupCompleted", "Z");
  sync_everything = GetFieldID(env, clazz, "mSyncEverything", "Z");
  sync_bookmarks = GetFieldID(env, clazz, "mSyncBookmarks", "Z");
  sync_typed_urls = GetFieldID(env, clazz, "mSyncTypedUrls", "Z");
  sync_sessions = GetFieldID(env, clazz, "mSyncSessions", "Z");
  autologin_enabled = GetFieldID(env, clazz, "mAutologinEnabled", "Z");
  search_suggest_enabled = GetFieldID(env, clazz, "mSearchSuggestEnabled", "Z");
  network_prediction_enabled =
      GetFieldID(env, clazz, "mNetworkPredictionEnabled", "Z");
  resolve_navigation_error_enabled =
      GetFieldID(env, clazz, "mResolveNavigationErrorEnabled", "Z");
  remote_debugging_enabled =
      GetFieldID(env, clazz, "mRemoteDebuggingEnabled", "Z");
}

struct FieldIds g_field_ids = {0};

// Keys for autofill profile maps.  Must be kept in sync with
// constants in ChromeNativePreferences.java.
const char kAutofillName[] = "name";
const char kAutofillCompany[] = "company";
const char kAutofillAddress1[] = "addr1";
const char kAutofillAddress2[] = "addr2";
const char kAutofillCity[] = "city";
const char kAutofillState[] = "state";
const char kAutofillZip[] = "zip";
const char kAutofillCountry[] = "country";
const char kAutofillPhone[] = "phone";
const char kAutofillEmail[] = "email";

// Keys for autofill credit card maps.  Must be kept in sync with
// constants in ChromeNativePreferences.java.
const char kAutofillObfuscated[] = "obfuscated";  // to send data up to java
const char kAutofillNumber[] = "number";  // to send data down to native
const char kAutofillMonth[] = "month";
const char kAutofillYear[] = "year";

// Keys for name/password maps.  Must be kept in sync with
// constants in ChromeNativePreferences.java.
const char kPasswordListURL[] = "url";
const char kPasswordListName[] = "name";
const char kPasswordListPassword[] = "password";

// Keys for popup exception maps.  Must be kept in sync with
// constants in ChromeNativePreferences.java.
const char kDisplayPattern[] = "displayPattern";
const char kSetting[] = "setting";

const char kGoogleChromeVersion[] = "google_chrome_version";
const char kApplicationVersion[] = "application_version";
const char kWebkitVersion[] = "webkit_version";
const char kJavaScriptVersion[] = "javascript_version";

const char kSearchEngineId[] = "searchEngineId";
const char kSearchEngineShortName[] = "searchEngineShortName";
const char kSearchEngineKeyword[] = "searchEngineKeyword";
const char kSearchEngineUrl[] = "searchEngineUrl";

// Used for saved name/password code.
// Based on browser/ui/webui/options/password_manager_handler.h
class PasswordManagerOwner {
 public:
  static PasswordManagerOwner* GetInstance();

  // Initiate a request for name/password data.
  void StartRequest(JNIEnv* env, jobject owner) {
    DCHECK(!owner_ || (owner == owner_));
    env_ = env;
    owner_ = env->NewWeakGlobalRef(owner);
    SetupJNICallbacks();

    password_list_.reset();
    password_exception_list_.reset();
    password_list_populater_.Populate();
    password_exception_list_populater_.Populate();
  }

  void StopRequest() {
    env_->DeleteWeakGlobalRef(owner_);
    owner_ = NULL;
    env_ = NULL;
  }

  void RemoveSavedPassword(size_t index) {
    DCHECK(index < password_list_.size());
    GetPasswordStore()->RemoveLogin(*password_list_[index]);
  }
  void RemovePasswordException(size_t index) {
    DCHECK(index < password_exception_list_.size());
    GetPasswordStore()->RemoveLogin(*password_exception_list_[index]);
  }

  PasswordStore* GetPasswordStore() {
    Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
    return profile->GetOriginalProfile()->GetPasswordStore(Profile::EXPLICIT_ACCESS);
  }

  // May not be valid if a request hasn't completed.
  int Passwords() { return password_list_.size(); }
  int Exceptions() { return password_exception_list_.size(); }

  const webkit::forms::PasswordForm& GetPassword(size_t index) {
    DCHECK(index < password_list_.size());
    return *password_list_[index];
  }
  const webkit::forms::PasswordForm& GetPasswordException(size_t index) {
    DCHECK(index < password_exception_list_.size());
    return *password_exception_list_[index];
  }

 protected:
  void SetPasswordList(
      const std::vector<webkit::forms::PasswordForm*>& result) {
    password_list_.insert(password_list_.end(),
                          result.begin(), result.end());
    VLOG(0) << "SetPasswordList: size=" << password_list_.size();
    if (owner_) {
      env_->CallVoidMethod(GetRealObject(env_, owner_).obj(),
                           passwords_available_method_,
                           (int)password_list_.size());
      CheckException(env_);
    }
  }
  void SetPasswordExceptionList(
      const std::vector<webkit::forms::PasswordForm*>& result) {
    password_exception_list_.insert(password_exception_list_.end(),
                          result.begin(), result.end());
    VLOG(0) << "SetPasswordExceptionList: size=" << password_list_.size();
    if (owner_) {
      env_->CallVoidMethod(GetRealObject(env_, owner_).obj(),
                           password_exceptions_available_method_,
                           (int)password_exception_list_.size());
      CheckException(env_);
    }
  }

 private:
  PasswordManagerOwner() : env_(NULL),
                           owner_(NULL),
                           passwords_available_method_(0),
                           password_exceptions_available_method_(0),
                           password_list_populater_(this),
                           password_exception_list_populater_(this) {}
  friend struct DefaultSingletonTraits<PasswordManagerOwner>;

  class PasswordListPopulater : public PasswordStoreConsumer {
   public:
    PasswordListPopulater(PasswordManagerOwner* owner)
        : owner_(owner),
          pending_login_query_(0) {}
    // Send an async request for login data.
    void Populate() {
      if (pending_login_query_)
        owner_->GetPasswordStore()->CancelRequest(pending_login_query_);
      pending_login_query_ = owner_->GetPasswordStore()->GetAutofillableLogins(this);
    }
    // Login data callback.
    virtual void OnPasswordStoreRequestDone(
        CancelableRequestProvider::Handle handle,
        const std::vector<webkit::forms::PasswordForm*>& result) {
      owner_->SetPasswordList(result);
      pending_login_query_ = 0;
    }

   private:
    PasswordManagerOwner* owner_;
    CancelableRequestProvider::Handle pending_login_query_;
  };

  class PasswordExceptionListPopulater : public PasswordStoreConsumer {
   public:
    PasswordExceptionListPopulater(PasswordManagerOwner* owner)
        : owner_(owner),
          pending_login_query_(0) {}
    // Send an async request for login exception data.
    void Populate() {
      if (pending_login_query_)
        owner_->GetPasswordStore()->CancelRequest(pending_login_query_);
      pending_login_query_ = owner_->GetPasswordStore()->GetBlacklistLogins(this);
    }
    // Login exception data callback.
    virtual void OnPasswordStoreRequestDone(
        CancelableRequestProvider::Handle handle,
        const std::vector<webkit::forms::PasswordForm*>& result) {
      owner_->SetPasswordExceptionList(result);
      pending_login_query_ = 0;
    }
   private:
    PasswordManagerOwner* owner_;
    CancelableRequestProvider::Handle pending_login_query_;
  };

  void SetupJNICallbacks() {
    DCHECK(env_ && owner_);
    ScopedJavaLocalRef<jclass> clazz = GetClass(env_,
        "com/google/android/apps/chrome/preferences/ChromeNativePreferences$PasswordListObserver");
    passwords_available_method_ = GetMethodID(
        env_, clazz, "passwordListAvailable", "(I)V");
    password_exceptions_available_method_ = GetMethodID(
        env_, clazz, "passwordExceptionListAvailable", "(I)V");
  }

  // For callbacks into Java when data is available.
  JNIEnv* env_;
  jobject owner_;
  jmethodID passwords_available_method_;
  jmethodID password_exceptions_available_method_;

  // Helper objects to post requests.
  PasswordListPopulater password_list_populater_;
  PasswordExceptionListPopulater password_exception_list_populater_;

  // Request data.
  ScopedVector<webkit::forms::PasswordForm> password_list_;
  ScopedVector<webkit::forms::PasswordForm> password_exception_list_;
};

PasswordManagerOwner* PasswordManagerOwner::GetInstance() {
  return Singleton<PasswordManagerOwner>::get();
}

// Used for autofill data.
PersonalDataManager* GetPersonalDataManager() {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  return PersonalDataManagerFactory::GetForProfile(profile);
}

// Convert a std::map<std::string, string16> to a java HashMap<String,String>.
// Used for autofill data (profile, credit cards).
ScopedJavaLocalRef<jobject> ConvertProfileMapToJavaMap(
    JNIEnv* env,
    const std::map<std::string, string16>& cmap) {
  ScopedJavaLocalRef<jclass> map_clazz = GetClass(env, "java/util/HashMap");
  jmethodID map_constructor = GetMethodID(env, map_clazz, "<init>", "()V");
  jmethodID map_put = GetMethodID(env, map_clazz, "put",
      "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

  jobject jmap = env->NewObject(map_clazz.obj(), map_constructor);
  DCHECK(jmap);
  CheckException(env);

  for (std::map<std::string, string16>::const_iterator i = cmap.begin();
       i != cmap.end(); i++) {
    ScopedJavaLocalRef<jstring> key = ConvertUTF8ToJavaString(env, i->first);
    ScopedJavaLocalRef<jstring> value = ConvertUTF16ToJavaString(env, i->second);
    // Assign the result to a ScopedJavaLocalRef so we don't leak the local ref.
    ScopedJavaLocalRef<jobject> result(env,
        env->CallObjectMethod(jmap, map_put, key.obj(), value.obj()));
    CheckException(env);
  }
  return ScopedJavaLocalRef<jobject>(env, jmap);
}

std::map<std::string, string16> ConvertJavaMapToProfileMap(JNIEnv* env,
                                                           jobject jmap) {
  std::map<std::string, string16> cmap;

  // Get a Set of all items.
  ScopedJavaLocalRef<jclass> map_clazz = GetClass(env, "java/util/HashMap");
  jmethodID map_entryset =
      GetMethodID(env, map_clazz, "entrySet", "()Ljava/util/Set;");
  ScopedJavaLocalRef<jobject> jset(env,
      env->CallObjectMethod(jmap, map_entryset));
  DCHECK(!jset.is_null());
  CheckException(env);

  // Get an iterator for this set.
  ScopedJavaLocalRef<jclass> set_clazz = GetClass(env, "java/util/Set");
  jmethodID set_iterator =
      GetMethodID(env, set_clazz, "iterator", "()Ljava/util/Iterator;");
  ScopedJavaLocalRef<jobject> iter(env,
      env->CallObjectMethod(jset.obj(), set_iterator));
  DCHECK(!iter.is_null());
  CheckException(env);

  // Loop over them.
  ScopedJavaLocalRef<jclass> iter_clazz = GetClass(env, "java/util/Iterator");
  jmethodID iter_hasnext = GetMethodID(env, iter_clazz, "hasNext", "()Z");
  jmethodID iter_next =
      GetMethodID(env, iter_clazz, "next", "()Ljava/lang/Object;");

  ScopedJavaLocalRef<jclass> map_entry_clazz =
      GetClass(env, "java/util/Map$Entry");
  jmethodID map_entry_getkey =
      GetMethodID(env, map_entry_clazz, "getKey", "()Ljava/lang/Object;");
  jmethodID map_entry_getvalue =
      GetMethodID(env, map_entry_clazz, "getValue", "()Ljava/lang/Object;");

  // Dump items into a std::map.
  while (env->CallBooleanMethod(iter.obj(), iter_hasnext)) {
    CheckException(env);
    ScopedJavaLocalRef<jobject> map_entry(env,
        env->CallObjectMethod(iter.obj(), iter_next));
    DCHECK(env->IsInstanceOf(map_entry.obj(), map_entry_clazz.obj()));
    ScopedJavaLocalRef<jstring> jkey(env,
        static_cast<jstring>(env->CallObjectMethod(map_entry.obj(),
                                                   map_entry_getkey)));
    ScopedJavaLocalRef<jstring> jvalue(env,
        static_cast<jstring>(env->CallObjectMethod(map_entry.obj(),
                                                   map_entry_getvalue)));
    // For profiles, jvalue can be null if not found in the contacts database.
    if (!jkey.is_null() && !jvalue.is_null() &&
        env->GetStringLength(jvalue.obj())) {
      cmap[ConvertJavaStringToUTF8(jkey)] = ConvertJavaStringToUTF16(jvalue);
    }
  }
  return cmap;
}

static bool GetBooleanForContentSettingsType(ContentSetting content_setting,
                                             ContentSettingsType content_setting_type) {
  switch(content_setting) {
    case CONTENT_SETTING_BLOCK:
      return false;
    case CONTENT_SETTING_ALLOW:
      return true;
    case CONTENT_SETTING_ASK:
    default:
      return true;
  }
}

static std::string GetStringForContentSettingsType(ContentSetting content_setting) {
  switch(content_setting) {
    case CONTENT_SETTING_BLOCK:
      return "block";
    case CONTENT_SETTING_ALLOW:
      return "allow";
    case CONTENT_SETTING_ASK:
      return "ask";
    case CONTENT_SETTING_SESSION_ONLY:
      return "session";
    case CONTENT_SETTING_NUM_SETTINGS:
      return "num_settings";
    case CONTENT_SETTING_DEFAULT:
    default:
      return "default";
  }
}

std::map<std::string, string16> GetExceptionForPage(
    const ContentSettingsPattern& pattern,
    ContentSetting setting) {
  std::map<std::string, string16> exception_map;
  exception_map[kDisplayPattern] = UTF8ToUTF16(pattern.ToString());
  exception_map[kSetting] = UTF8ToUTF16(GetStringForContentSettingsType(setting));
  return exception_map;
}

void ReturnAbsoluteExcecutablePathValue(std::string path_value) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> j_path_value =
      ConvertUTF8ToJavaString(env, path_value);
  Java_ChromeNativePreferences_setExecutablePathValue(env, j_path_value.obj());
}

void ReturnAbsoluteProfilePathValue(std::string path_value) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> j_path_value =
      ConvertUTF8ToJavaString(env, path_value);
  Java_ChromeNativePreferences_setProfilePathValue(env, j_path_value.obj());
}

void GetAbsolutePath(Profile* profile) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  FilePath executable_path = CommandLine::ForCurrentProcess()->GetProgram();
  if (file_util::AbsolutePath(&executable_path)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&ReturnAbsoluteExcecutablePathValue, executable_path.value()));
  }
  if (profile) {
    FilePath profile_path = profile->GetPath();
    if (file_util::AbsolutePath(&profile_path)) {
      BrowserThread::PostTask(
          BrowserThread::UI, FROM_HERE,
          base::Bind(&ReturnAbsoluteProfilePathValue, profile_path.value()));
    }
  }
}

}  // namespace

AutofillPersonalDataManagerObserver::AutofillPersonalDataManagerObserver(JNIEnv* env, jobject obj)
      : weak_chrome_native_preferences_(env, obj) {}

void AutofillPersonalDataManagerObserver::OnPersonalDataChanged() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jclass> clazz =
      GetClass(env, "com/google/android/apps/chrome/preferences/ChromeNativePreferences");
  jmethodID autofill_data_updated =
      GetMethodID(env, clazz, "autofillDataUpdated", "()V");
  env->CallVoidMethod(weak_chrome_native_preferences_.get(env).obj(), autofill_data_updated);
  CheckException(env);
}

void AutofillPersonalDataManagerObserver::DestroyPersonalDataManagerObserver(JNIEnv* env, jobject obj) {
   GetPersonalDataManager()->RemoveObserver(this);
   delete this;
}

AutofillPersonalDataManagerObserver::~AutofillPersonalDataManagerObserver() {}


void TemplateURLServiceLoadedObserver::Observe(int type,
                                               const content::NotificationSource& source,
                                               const content::NotificationDetails& details) {
  DCHECK(type == chrome::NOTIFICATION_TEMPLATE_URL_SERVICE_LOADED);
  JNIEnv* env = AttachCurrentThread();
  CheckException(env);
  ScopedJavaLocalRef<jclass> clazz =
      GetClass(env, "com/google/android/apps/chrome/preferences/ChromeNativePreferences");
  jmethodID template_url_service_loaded =
      GetMethodID(env, clazz, "templateURLServiceLoaded", "()V");
  env->CallVoidMethod(weak_chrome_native_preferences_.get(env).obj(), template_url_service_loaded);
  CheckException(env);
}

TemplateURLServiceLoadedObserver::TemplateURLServiceLoadedObserver(JNIEnv* env, jobject obj)
        : weak_chrome_native_preferences_(env, obj)  {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile);
  registrar_.Add(this, chrome::NOTIFICATION_TEMPLATE_URL_SERVICE_LOADED,
      content::Source<TemplateURLService>(template_url_service));
}

void TemplateURLServiceLoadedObserver::DestroyTemplateURLServiceLoadedObserver(JNIEnv* env, jobject obj) {
    delete this;
}

TemplateURLServiceLoadedObserver::~TemplateURLServiceLoadedObserver() {
}

// ----------------------------------------------------------------------------
// Native JNI methods
// ----------------------------------------------------------------------------
static jboolean IsTemplateURLServiceLoaded(JNIEnv* env, jobject obj) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile);
  return template_url_service->loaded();
}

static jint InitAutofillPersonalDataManagerObserver(JNIEnv* env, jobject obj) {
  AutofillPersonalDataManagerObserver* personal_data_manager_observer =
      new AutofillPersonalDataManagerObserver(env, obj);
  GetPersonalDataManager()->SetObserver(personal_data_manager_observer);
  return  reinterpret_cast<jint>(personal_data_manager_observer);
}

static jint InitTemplateURLServiceLoadedObserver(JNIEnv* env, jobject obj) {
  TemplateURLServiceLoadedObserver* observer =
      new TemplateURLServiceLoadedObserver(env, obj);
  return reinterpret_cast<jint>(observer);
}

static void InitPreferences(JNIEnv* env, jobject obj) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile);
  if (!template_url_service->loaded()) {
    template_url_service->Load();
  }
}

// Update search engine from native to Java.
static void UpdateSearchEngineInJava(JNIEnv* env, jobject obj) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile);
  std::vector<const TemplateURL*> model_urls =
      template_url_service->GetTemplateURLs();
  for (size_t i = 0; i < model_urls.size(); ++i) {
    if (template_url_service->GetDefaultSearchProvider() == model_urls[i]) {
      env->SetIntField(obj, g_field_ids.search_engine, i);
      break;
    }
  }
  CheckException(env);
}

static void EnsureConsistentGeolocationPreferences(Profile* profile) {
  // On Android, we use the kGeolocationEnabled flag to control geolocation on a
  // global basis, rather than the default geolocation host content setting,
  // which is only used if no previously stored more specific host exception
  // cannot be found.
  //
  // On Android, there is currently no UI to change this default setting, so it
  // needs to default to ASK. Additionally, for users that have previously set
  // the default to BLOCK, we set the preference to disable geolocation
  // globally.

  ContentSetting defaultSetting = profile->GetHostContentSettingsMap()->
      GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_GEOLOCATION, NULL);

  if (defaultSetting == CONTENT_SETTING_ASK) return;

  profile->GetHostContentSettingsMap()->SetDefaultContentSetting(
      CONTENT_SETTINGS_TYPE_GEOLOCATION, CONTENT_SETTING_ASK);

  if (defaultSetting == CONTENT_SETTING_BLOCK) {
    profile->GetPrefs()->SetBoolean(prefs::kGeolocationEnabled, false);
  }
}

// Get the preferences from the native to the Java.
static void Get(JNIEnv* env, jobject obj) {
  InitPreferences(env, obj);
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();

  UpdateSearchEngineInJava(env, obj);

  jboolean cookies_value = GetBooleanForContentSettingsType(
      profile->GetHostContentSettingsMap()->
      GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_COOKIES, NULL),
      CONTENT_SETTINGS_TYPE_COOKIES);
  env->SetBooleanField(obj, g_field_ids.accept_cookies_enabled, cookies_value);
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.remember_passwords_enabled,
                       prefs->GetBoolean(prefs::kPasswordManagerEnabled));
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.autofill_enabled,
                       prefs->GetBoolean(prefs::kAutofillEnabled));
  CheckException(env);

  EnsureConsistentGeolocationPreferences(profile);
  env->SetBooleanField(obj, g_field_ids.allow_location_enabled,
                       prefs->GetBoolean(prefs::kGeolocationEnabled));
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.javascript_enabled,
                       prefs->GetBoolean(prefs::kWebKitGlobalJavascriptEnabled));
  CheckException(env);

  env->SetFloatField(obj, g_field_ids.font_scale_factor,
      static_cast<float>(prefs->GetDouble(prefs::kWebKitFontScaleFactor)));
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.force_enable_zoom,
                       prefs->GetBoolean(prefs::kWebKitForceEnableZoom));
  CheckException(env);

  // MERGE(qinmin): Upstream uses kWebKitGlobalMinimumFontSize,
  // replaced our kWebKitMinimumFontSize with it.
  env->SetIntField(obj, g_field_ids.minimum_font_size,
                   prefs->GetInteger(prefs::kWebKitGlobalMinimumFontSize));
  CheckException(env);

  jboolean popups_value = GetBooleanForContentSettingsType(
        profile->GetHostContentSettingsMap()->
        GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS, NULL),
        CONTENT_SETTINGS_TYPE_POPUPS);
  env->SetBooleanField(obj, g_field_ids.allow_popups_enabled, popups_value);
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.sync_suppress_start,
                       prefs->GetBoolean(prefs::kSyncSuppressStart));
  CheckException(env);

  ScopedJavaLocalRef<jstring> username =
      ConvertUTF8ToJavaString(env, prefs->GetString(prefs::kGoogleServicesUsername));
  env->SetObjectField(obj, g_field_ids.sync_account_name, username.obj());
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.sync_setup_completed,
                       prefs->GetBoolean(prefs::kSyncHasSetupCompleted));
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.sync_everything,
                       prefs->GetBoolean(prefs::kSyncKeepEverythingSynced));
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.sync_bookmarks,
                       prefs->GetBoolean(prefs::kSyncBookmarks));
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.sync_typed_urls,
                       prefs->GetBoolean(prefs::kSyncTypedUrls));
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.sync_sessions,
                       prefs->GetBoolean(prefs::kSyncSessions));
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.autologin_enabled,
                       prefs->GetBoolean(prefs::kAutologinEnabled));
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.search_suggest_enabled,
                       prefs->GetBoolean(prefs::kSearchSuggestEnabled));
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.network_prediction_enabled,
                       prefs->GetBoolean(prefs::kNetworkPredictionEnabled));
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.resolve_navigation_error_enabled,
                       prefs->GetBoolean(prefs::kAlternateErrorPagesEnabled));
  CheckException(env);

  env->SetBooleanField(obj, g_field_ids.remote_debugging_enabled,
                       prefs->GetBoolean(prefs::kDevToolsRemoteEnabled));
  CheckException(env);

  GURL empty = GURL("");
  ScopedJavaLocalRef<jstring> str =
      ConvertUTF8ToJavaString(env, webkit_glue::GetUserAgent(empty));
  env->SetObjectField(obj, g_field_ids.user_agent, str.obj());
  CheckException(env);
}

static void SetUserAgent(JNIEnv* env, jobject obj, jstring str) {
  // empty "str" implies setting the user agent to the default.
  webkit_glue::SetUserAgent(ConvertJavaStringToUTF8(env, str), true);
}

static void StartPasswordListRequest(JNIEnv* env, jclass clazz, jobject owner) {
  PasswordManagerOwner::GetInstance()->StartRequest(env, owner);
}

static void StopPasswordListRequest(JNIEnv* env, jclass clazz) {
  PasswordManagerOwner::GetInstance()->StopRequest();
}

static jobject GetSavedNamePassword(JNIEnv* env, jclass clazz, int index) {
  std::map<std::string, string16> password_map;
  const webkit::forms::PasswordForm& form =
      PasswordManagerOwner::GetInstance()->GetPassword(index);
  password_map[kPasswordListURL] = UTF8ToUTF16(form.origin.spec());
  password_map[kPasswordListName] = form.username_value;
  password_map[kPasswordListPassword] = form.password_value;
  // OK to release, returning to Java.
  return ConvertProfileMapToJavaMap(env, password_map).Release();
}

static jobject GetSavedPasswordException(JNIEnv* env, jclass clazz, int index) {
  std::map<std::string, string16> password_map;
  const webkit::forms::PasswordForm& form =
      PasswordManagerOwner::GetInstance()->GetPasswordException(index);
  password_map[kPasswordListURL] = UTF8ToUTF16(form.origin.spec());
  // TODO(jrg): do I need anything else in here?
  // OK to release, returning to Java.
  return ConvertProfileMapToJavaMap(env, password_map).Release();
}

static void RemoveSavedNamePassword(JNIEnv* env, jclass clazz, int index) {
  PasswordManagerOwner::GetInstance()->RemoveSavedPassword(index);
}

static void RemoveSavedPasswordException(JNIEnv* env, jclass clazz, int index) {
  PasswordManagerOwner::GetInstance()->RemovePasswordException(index);
}

// Return an vector of GUIDs that index each profile.
static jobjectArray GetAutofillProfileGUIDs(JNIEnv* env, jobject obj) {
  PersonalDataManager* personal_data = GetPersonalDataManager();
  DCHECK(personal_data);
  const std::vector<AutofillProfile*>& profiles = personal_data->profiles();

  std::vector<std::string> guids;
  for (std::vector<AutofillProfile*>::const_iterator i = profiles.begin();
       i != profiles.end();
       i++) {
    guids.push_back((*i)->guid());
  }

  // OK to release, JNI binding.
  return ToJavaArrayOfStrings(env, guids).Release();
}

// Return a java Map representing the profile for the given GUID.
// Keys are specified by constants defined in ChromeNativePreferences.java.
static jobject GetAutofillProfile(JNIEnv* env, jobject obj, jstring jguid) {
  std::string guid = ConvertJavaStringToUTF8(env, jguid);
  PersonalDataManager* personal_data = GetPersonalDataManager();
  DCHECK(personal_data);

  std::map<std::string, string16> profile_map;
  AutofillProfile* profile = personal_data->GetProfileByGUID(guid);
  if (profile) {
    profile_map[kAutofillName] = profile->GetInfo(NAME_FULL);
    profile_map[kAutofillCompany] = profile->GetInfo(COMPANY_NAME);
    profile_map[kAutofillAddress1] = profile->GetInfo(ADDRESS_HOME_LINE1);
    profile_map[kAutofillAddress2] = profile->GetInfo(ADDRESS_HOME_LINE2);
    profile_map[kAutofillCity] = profile->GetInfo(ADDRESS_HOME_CITY);
    profile_map[kAutofillState] = profile->GetInfo(ADDRESS_HOME_STATE);
    profile_map[kAutofillZip] = profile->GetInfo(ADDRESS_HOME_ZIP);
    profile_map[kAutofillCountry] = profile->GetInfo(ADDRESS_HOME_COUNTRY);
    profile_map[kAutofillPhone] = profile->GetInfo(PHONE_HOME_WHOLE_NUMBER);
    profile_map[kAutofillEmail] = profile->GetInfo(EMAIL_ADDRESS);
  }

  // OK to release, returning to Java.
  return ConvertProfileMapToJavaMap(env, profile_map).Release();
}

// Add or modify a profile.  If |GUID| is an empty string, we are
// creating a new profile.  Else we are updating an existing profile.
// Always return the GUID for this profile; the GUID it may have just
// been created.
static jstring SetAutofillProfile(JNIEnv* env, jobject obj, jstring jguid,
                                  jobject jmap) {
  std::map<std::string, string16> profile_map =
      ConvertJavaMapToProfileMap(env, jmap);
  std::string guid = ConvertJavaStringToUTF8(env, jguid);

  VLOG(0) << "Setting profile " << guid;
  PersonalDataManager* personal_data = GetPersonalDataManager();
  DCHECK(personal_data);

  // If we have a GUID, update an existing profile.
  // Else use a new one.
  AutofillProfile* profile = NULL;
  scoped_ptr<AutofillProfile> new_profile;
  if (guid != "") {
    profile = personal_data->GetProfileByGUID(guid);
    if (!profile) {
      // Error case but can't return nothing.
      // OK to release, JNI binding.
      return ConvertUTF8ToJavaString(env, "").Release();
    }
    new_profile.reset(new AutofillProfile(*profile));
  } else {
    new_profile.reset(new AutofillProfile);
  }
  profile = new_profile.get();

  std::map<std::string, string16>::const_iterator i;
  std::map<std::string, string16>::const_iterator end = profile_map.end();

  if ((i = profile_map.find(kAutofillName)) != end)
    profile->SetInfo(NAME_FULL, i->second);
  if ((i = profile_map.find(kAutofillCompany)) != end)
    profile->SetInfo(COMPANY_NAME, i->second);
  if ((i = profile_map.find(kAutofillAddress1)) != end)
    profile->SetInfo(ADDRESS_HOME_LINE1, i->second);
  if ((i = profile_map.find(kAutofillAddress2)) != end)
    profile->SetInfo(ADDRESS_HOME_LINE2, i->second);
  if ((i = profile_map.find(kAutofillCity)) != end)
    profile->SetInfo(ADDRESS_HOME_CITY, i->second);
  if ((i = profile_map.find(kAutofillState)) != end)
    profile->SetInfo(ADDRESS_HOME_STATE, i->second);
  if ((i = profile_map.find(kAutofillZip)) != end)
    profile->SetInfo(ADDRESS_HOME_ZIP, i->second);
  if ((i = profile_map.find(kAutofillCountry)) != end)
    profile->SetInfo(ADDRESS_HOME_COUNTRY, i->second);
  if ((i = profile_map.find(kAutofillPhone)) != end)
    profile->SetInfo(PHONE_HOME_WHOLE_NUMBER, i->second);
  if ((i = profile_map.find(kAutofillEmail)) != end)
    profile->SetInfo(EMAIL_ADDRESS, i->second);

  if (guid == "") {
    personal_data->AddProfile(*profile);
  }
  else {
    personal_data->UpdateProfile(*profile);
  }
  // Return the GUID.  This may be different from what is passed in.
  // OK to release, JNI binding.
  return ConvertUTF8ToJavaString(env, profile->guid()).Release();
}

// Delete the autofill profile specified by |jguid|.
static void DeleteAutofillProfile(JNIEnv* env, jobject obj, jstring jguid) {
  std::string guid = ConvertJavaStringToUTF8(env, jguid);
  PersonalDataManager* personal_data = GetPersonalDataManager();
  DCHECK(personal_data);
  personal_data->RemoveProfile(guid);
}

// Return an vector of GUIDs that index each profile.
static jobjectArray GetAutofillCreditCardGUIDs(JNIEnv* env, jobject obj) {
  PersonalDataManager* personal_data = GetPersonalDataManager();
  DCHECK(personal_data);
  const std::vector<CreditCard*>& cards = personal_data->credit_cards();

  std::vector<std::string> guids;
  for (std::vector<CreditCard*>::const_iterator i = cards.begin();
       i != cards.end();
       i++) {
    guids.push_back((*i)->guid());
  }

  // OK to release, JNI binding.
  return ToJavaArrayOfStrings(env, guids).Release();
}

// Return a java Map representing the credit card for the given GUID.
// Keys are specified by constants defined in ChromeNativePreferences.java.
static jobject GetAutofillCreditCard(JNIEnv* env, jobject obj, jstring jguid) {
  std::string guid = ConvertJavaStringToUTF8(env, jguid);
  PersonalDataManager* personal_data = GetPersonalDataManager();
  DCHECK(personal_data);

  std::map<std::string, string16> card_map;
  CreditCard* card = personal_data->GetCreditCardByGUID(guid);
  if (card) {
    card_map[kAutofillName] = card->GetInfo(CREDIT_CARD_NAME);
    card_map[kAutofillObfuscated] = card->ObfuscatedNumber();
    // raw number is not passed up to java!
    card_map[kAutofillMonth] = card->GetInfo(CREDIT_CARD_EXP_MONTH);
    card_map[kAutofillYear] = card->GetInfo(CREDIT_CARD_EXP_4_DIGIT_YEAR);
  }

  // OK to release, returning to Java.
  return ConvertProfileMapToJavaMap(env, card_map).Release();
}

// Add or modify a credit card.  If |GUID| is an empty string, we are
// creating a new card.  Else we are updating an existing card.
// Always return the GUID for this card; the GUID it may have just
// been created.  Unlike profiles, we never send the actual credit
// card number up to java.  Thus, when updating card info, we must
// find the original card and pull it's number out.
static jstring SetAutofillCreditCard(JNIEnv* env, jobject obj, jstring jguid,
                                     jobject jmap) {
  std::map<std::string, string16> card_map =
      ConvertJavaMapToProfileMap(env, jmap);
  std::string guid = ConvertJavaStringToUTF8(env, jguid);

  VLOG(0) << "Setting credit card " << guid;
  PersonalDataManager* personal_data = GetPersonalDataManager();
  DCHECK(personal_data);

  // If we have a GUID, update an existing card.
  // Else use a new one.
  CreditCard* card = NULL;
  scoped_ptr<CreditCard> new_card;
  if (guid != "") {
    card = personal_data->GetCreditCardByGUID(guid);
    if (!card) {
      // Error case but can't return nothing.
      // OK to release, JNI binding.
      return ConvertUTF8ToJavaString(env, "").Release();
    }
    new_card.reset(new CreditCard(*card));
  } else {
    new_card.reset(new CreditCard);
  }
  card = new_card.get();

  std::map<std::string, string16>::const_iterator i;
  std::map<std::string, string16>::const_iterator end = card_map.end();

  if ((i = card_map.find(kAutofillName)) != end)
    card->SetInfo(CREDIT_CARD_NAME, i->second);
  // Obfuscated number goes up; full number comes down.
  // SetInfo(CREDIT_CARD_NUMBER...) is smart enough to ignore
  // obfuscated numbers should the java side accidentally send them
  // down.
  if ((i = card_map.find(kAutofillNumber)) != end)
      card->SetInfo(CREDIT_CARD_NUMBER, i->second);
  if ((i = card_map.find(kAutofillMonth)) != end)
    card->SetInfo(CREDIT_CARD_EXP_MONTH, i->second);
  if ((i = card_map.find(kAutofillYear)) != end)
    card->SetInfo(CREDIT_CARD_EXP_4_DIGIT_YEAR, i->second);

  if (guid == "") {
    personal_data->AddCreditCard(*card);
  }
  else {
    personal_data->UpdateCreditCard(*card);
  }

  // Return the GUID.  This may be different from what is passed in.
  // OK to release, JNI binding.
  return ConvertUTF8ToJavaString(env, card->guid()).Release();
}

// Delete the autofill credit card specified by |jguid|.
static void DeleteAutofillCreditCard(JNIEnv* env, jobject obj, jstring jguid) {
  std::string guid = ConvertJavaStringToUTF8(env, jguid);
  PersonalDataManager* personal_data = GetPersonalDataManager();
  DCHECK(personal_data);
  personal_data->RemoveCreditCard(guid);
}

/// --------------------------------------

namespace {

// Redirect a BrowsingDataRemover completion callback back into Java.
class ClearBrowsingDataObserver : public BrowsingDataRemover::Observer {
 public:
  // |obj| is expected to be the object passed into
  // ClearBrowsingData(); e.g. a ChromePreference.
  ClearBrowsingDataObserver(JNIEnv* env, jobject obj)
      : weak_chrome_native_preferences_(env, obj) {
  }

  virtual void OnBrowsingDataRemoverDone() {
    JNIEnv* env = AttachCurrentThread();
    ScopedJavaLocalRef<jclass> clazz =
        GetClass(env, "com/google/android/apps/chrome/preferences/ChromeNativePreferences");
    jmethodID data_cleared =
        GetMethodID(env, clazz, "browsingDataCleared", "()V");
    env->CallVoidMethod(weak_chrome_native_preferences_.get(env).obj(), data_cleared);
    CheckException(env);

    // Just as a BrowsingDataRemover deletes itself when done, we
    // delete ourself when done.  No need to remove ourself as an observer given
    // the lifetime of BrowsingDataRemover.
    delete this;
  }

 private:
  JavaObjectWeakGlobalRef weak_chrome_native_preferences_;
};
}  // namespace

static void ClearBrowsingData(JNIEnv* env, jobject obj, jboolean history, jboolean cache,
                              jboolean cookies_and_site_data, jboolean passwords,
                              jboolean form_data) {
  // BrowsingDataRemover deletes itself
  BrowsingDataRemover* browsing_data_remover = new BrowsingDataRemover(
      g_browser_process->profile_manager()->GetDefaultProfile()->GetOriginalProfile(),
      static_cast<BrowsingDataRemover::TimePeriod>(BrowsingDataRemover::EVERYTHING),
      base::Time());
  browsing_data_remover->AddObserver(new ClearBrowsingDataObserver(env, obj));

  int remove_mask = 0;
  if (history)
    remove_mask |= BrowsingDataRemover::REMOVE_HISTORY;
  if (cache)
    remove_mask |= BrowsingDataRemover::REMOVE_CACHE;
  if (cookies_and_site_data) {
    remove_mask |= BrowsingDataRemover::REMOVE_COOKIES;
    remove_mask |= BrowsingDataRemover::REMOVE_SITE_DATA;
  }
  if (passwords)
    remove_mask |= BrowsingDataRemover::REMOVE_PASSWORDS;
  if (form_data)
    remove_mask |= BrowsingDataRemover::REMOVE_FORM_DATA;
  browsing_data_remover->Remove(remove_mask);
}

static jobjectArray GetLocalizedSearchEngines(JNIEnv* env, jobject obj) {
  std::vector<TemplateURL*> prepopulated_urls;
  size_t default_search_index;
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  PrefService* prefs = profile->GetPrefs();
  TemplateURLPrepopulateData::GetPrepopulatedEngines(prefs,
                                                     &prepopulated_urls,
                                                     &default_search_index);

  ScopedJavaLocalRef<jclass> hashmap_clazz = GetClass(env, "java/util/HashMap");
  jobjectArray search_engine_array =
      env->NewObjectArray(prepopulated_urls.size(), hashmap_clazz.obj(), NULL);
  std::map<std::string, string16> search_engine_map;
  for (size_t i = 0; i < prepopulated_urls.size(); ++i) {
    scoped_ptr<TemplateURL> prepopulated_url(prepopulated_urls[i]);
    search_engine_map.clear();
    std::stringstream ss;
    ss << i;
    search_engine_map[kSearchEngineId] = UTF8ToUTF16(ss.str());
    search_engine_map[kSearchEngineShortName] = prepopulated_url.get()->short_name();
    search_engine_map[kSearchEngineKeyword] = prepopulated_url.get()->keyword();
    search_engine_map[kSearchEngineUrl] = prepopulated_url.get()->url()->DisplayURL();

    env->SetObjectArrayElement(search_engine_array, i,
        ConvertProfileMapToJavaMap(env, search_engine_map).obj());
    CheckException(env);
  }
  return search_engine_array;
}

static void SetSearchEngine(JNIEnv* env, jobject obj, jint selected_index) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile);
  std::vector<const TemplateURL*> model_urls =
      template_url_service->GetTemplateURLs();
  if (selected_index >= 0 &&
      selected_index < static_cast<int>(model_urls.size())) {
    template_url_service->SetDefaultSearchProvider(model_urls[selected_index]);
  } else {
    LOG(ERROR) << "Wrong index for search engine";
  }
}

static void SetAllowCookiesEnabled(JNIEnv* env, jobject obj, jboolean allow) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  HostContentSettingsMap* host_content_settings_map = profile->GetHostContentSettingsMap();
  host_content_settings_map->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_COOKIES,
      allow ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);
}

static void SetRememberPasswordsEnabled(JNIEnv* env, jobject obj, jboolean allow) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(
      prefs::kPasswordManagerEnabled, allow);
}

static void SetAutoFillEnabled(JNIEnv* env, jobject obj, jboolean allow) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(prefs::kAutofillEnabled, allow);
}

static void SetAllowLocationEnabled(JNIEnv* env, jobject obj, jboolean allow) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(prefs::kGeolocationEnabled, allow);
}

static void SetSyncSuppressStart(JNIEnv* env, jobject obj, jboolean enabled) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(prefs::kSyncSuppressStart, enabled);
}

static void SetJavaScriptEnabled(JNIEnv* env, jobject obj, jboolean enabled) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(prefs::kWebKitGlobalJavascriptEnabled, enabled);
}

static void SetFontScaleFactor(JNIEnv* env, jobject obj, jfloat scale) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetDouble(prefs::kWebKitFontScaleFactor, static_cast<double>(scale));
}

static void SetForceEnableZoom(JNIEnv* env, jobject obj,
                               jboolean forceEnableZoom) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(prefs::kWebKitForceEnableZoom, forceEnableZoom);
}

static void SetMinimumFontSize(JNIEnv* env, jobject obj, jint font_size) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetInteger(prefs::kWebKitGlobalMinimumFontSize, font_size);
  prefs->SetInteger(prefs::kWebKitGlobalMinimumLogicalFontSize, font_size);
}

static void SetAllowPopupsEnabled(JNIEnv* env, jobject obj, jboolean allow) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  HostContentSettingsMap* host_content_settings_map = profile->GetHostContentSettingsMap();
  host_content_settings_map->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS,
      allow ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);
}

static void SetAutologinEnabled(JNIEnv* env, jobject obj, jboolean autologinEnabled) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(prefs::kAutologinEnabled, autologinEnabled);
}

static void SetPopupException(JNIEnv* env, jobject obj, jstring pattern, jboolean allow) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  HostContentSettingsMap* host_content_settings_map = profile->GetHostContentSettingsMap();
  host_content_settings_map->SetContentSetting(
      ContentSettingsPattern::FromString(ConvertJavaStringToUTF8(env, pattern)),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_POPUPS,
      "",
      allow ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);
}

static void RemovePopupException(JNIEnv* env, jobject obj, jstring pattern) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  HostContentSettingsMap* host_content_settings_map = profile->GetHostContentSettingsMap();
  host_content_settings_map->SetContentSetting(
      ContentSettingsPattern::FromString(ConvertJavaStringToUTF8(env, pattern)),
      ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_POPUPS,
      "",
      CONTENT_SETTING_DEFAULT);
}

static jobjectArray GetPopupExceptions(JNIEnv* env, jobject obj) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
        GetOriginalProfile();
  HostContentSettingsMap* host_content_settings_map =
      profile->GetHostContentSettingsMap();
  ContentSettingsForOneType entries;
  host_content_settings_map->GetSettingsForOneType(
      CONTENT_SETTINGS_TYPE_POPUPS, "", &entries);
  ScopedJavaLocalRef<jclass> hashmap_clazz = GetClass(env, "java/util/HashMap");
  jobjectArray exceptions =
      env->NewObjectArray(entries.size(), hashmap_clazz.obj(), NULL);
  for (size_t i = 0; i < entries.size(); ++i) {
    // TODO(dtrainor): Do we want to expose the provider name here? (entries[i].c).  This
    // could be one of "policy", "extension", or "preference" as found in
    // chrome/browser/content_settings/host_content_settings_map.cc.
    env->SetObjectArrayElement(exceptions, i, ConvertProfileMapToJavaMap(env,
        GetExceptionForPage(entries[i].primary_pattern, entries[i].setting)).obj());
    CheckException(env);
  }
  return exceptions;
}


static void SetSearchSuggestEnabled(JNIEnv* env, jobject obj, jboolean enabled) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(prefs::kSearchSuggestEnabled, enabled);
}

static void SetNetworkPredictionEnabled(JNIEnv* env, jobject obj, jboolean enabled) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(prefs::kNetworkPredictionEnabled, enabled);
}

static void SetResolveNavigationErrorEnabled(JNIEnv* env, jobject obj, jboolean enabled) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(prefs::kAlternateErrorPagesEnabled, enabled);
}

static void SetRemoteDebuggingEnabled(JNIEnv* env, jobject obj, jboolean enabled) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(prefs::kDevToolsRemoteEnabled, enabled);
}

// Send all information about the different versions to Java.
// From browser_about_handler.cc
static jobject GetAboutVersionStrings(JNIEnv* env, jobject obj) {
  chrome::VersionInfo version_info;
  std::string browser_version = version_info.Version();
  std::string version_modifier =
      chrome::VersionInfo::GetVersionStringModifier();
  if (!version_modifier.empty())
    browser_version += " " + version_modifier;

#if !defined(GOOGLE_CHROME_BUILD)
  browser_version += " (";
  browser_version += version_info.LastChange();
  browser_version += ")";
#endif

  std::string js_version(v8::V8::GetVersion());
  std::string js_engine = "V8";

  std::string application;
#if defined(OS_ANDROID)
  application = AboutAndroidApp::GetAppLabel();
  application += " " + AboutAndroidApp::GetAppVersionName();
#endif

  std::map<std::string, string16> version_map;
  version_map[kGoogleChromeVersion] = UTF8ToUTF16(browser_version);
  version_map[kApplicationVersion] = UTF8ToUTF16(application);
  version_map[kWebkitVersion] = UTF8ToUTF16(webkit_glue::GetWebKitVersion());
  version_map[kJavaScriptVersion] = UTF8ToUTF16(js_engine + " " + js_version);

  // OK to release, returning to Java.
  return ConvertProfileMapToJavaMap(env, version_map).Release();
}

static void SetPathValuesForAboutChrome(JNIEnv* env, jobject obj) {
  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile()->
      GetOriginalProfile();
  BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
                          base::Bind(&GetAbsolutePath, profile));
}

static void SetCountryCodeAtInstall(JNIEnv* env, jobject obj, jstring country_code) {
  std::string code = ConvertJavaStringToUTF8(env, country_code);
  if (browser_defaults::kCountryCodeAtInstall == "" && code != "") {
    browser_defaults::kCountryCodeAtInstall = code;
  }
}

static jstring GetCountryCodeAtInstall(JNIEnv* env, jobject obj) {
  return ConvertUTF8ToJavaString(env, browser_defaults::kCountryCodeAtInstall).Release();
}

// Register native methods

bool RegisterChromeNativePreferences(JNIEnv* env) {
  if (!HasClass(env, kChromeNativePreferencesClassPath)) {
    DLOG(ERROR) << "Unable to find class ChromeNativePreferences!";
    return false;
  }
  ScopedJavaLocalRef<jclass> clazz = GetClass(env, kChromeNativePreferencesClassPath);
  g_field_ids.Initialize(env, clazz);

  return RegisterNativesImpl(env);
}
