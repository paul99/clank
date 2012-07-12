// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/proxy/proxy_config_service_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/message_loop_proxy.h"
#include "base/string_number_conversions.h"
#include "base/string_tokenizer.h"
#include "base/string_util.h"
#include "jni/proxy_change_listener_jni.h"
#include "net/proxy/proxy_config.h"

#include <sys/system_properties.h>

using base::android::AttachCurrentThread;
using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertJavaStringToUTF8;
using base::android::CheckException;
using base::android::ClearException;
using base::android::GetMethodID;

namespace net {

namespace {
// This class implements the link to Android's proxy broadcast mechanism.
class DelegateImpl : public ProxyConfigServiceAndroid::Delegate {
 public:
  DelegateImpl();
  // ProxyConfigServiceAndroid::Delegate:
  virtual void Start(ProxyConfigServiceAndroid* service) OVERRIDE;
  virtual void Stop() OVERRIDE;
  virtual std::string GetProperty(const std::string& property) const OVERRIDE;

 private:
  ProxyConfigServiceAndroid* service_;
  base::android::ScopedJavaGlobalRef<jobject>
      java_proxy_change_listener_android_;
};

DelegateImpl::DelegateImpl() : service_(NULL) {}

void DelegateImpl::Start(ProxyConfigServiceAndroid* service) {
  DCHECK(!service_);
  JNIEnv* env = AttachCurrentThread();
  service_ = service;
  if (java_proxy_change_listener_android_.is_null()) {
    java_proxy_change_listener_android_.Reset(
        Java_ProxyChangeListener_create(
            env, base::android::GetApplicationContext()));
    CHECK(!java_proxy_change_listener_android_.is_null());
  }
  Java_ProxyChangeListener_start(
      env,
      java_proxy_change_listener_android_.obj(),
      reinterpret_cast<jint>(service));
}

void DelegateImpl::Stop() {
  // The java object will be NULL if Stop() was already called (e.g. after a
  // Start()), or if Start() was never called. Stop() is called from the
  // destructor of ProxyConfigServiceAndroid.
  if (java_proxy_change_listener_android_.is_null())
    return;
  JNIEnv* env = AttachCurrentThread();
  Java_ProxyChangeListener_stop(env,
                                java_proxy_change_listener_android_.obj());
}

std::string DelegateImpl::GetProperty(const std::string& property) const {
  // Use Java System.getProperty to get configuration information.
  // Question: Conversion to/from UTF8 ok here?
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> str = ConvertUTF8ToJavaString(env, property);
  ScopedJavaLocalRef<jstring> result =
      Java_ProxyChangeListener_getProperty(env, str.obj());
  return result.is_null() ? "" : ConvertJavaStringToUTF8(env, result.obj());
}
}  // namespace

ProxyConfigService* ProxyConfigServiceAndroid::Create() {
  // For unit tests, our Java class will not be available.
  // TODO(jknotten): Remove this check once http://b/issue?id=5075273 is fixed.
  if (!g_ProxyChangeListener_clazz)
    return NULL;
  return new ProxyConfigServiceAndroid(new DelegateImpl());
}

ProxyConfigServiceAndroid* ProxyConfigServiceAndroid::CreateForTests(
    Delegate* delegate) {
  return new ProxyConfigServiceAndroid(delegate);
}

ProxyConfigServiceAndroid::~ProxyConfigServiceAndroid() {
  DCHECK(OnObserverThread());
  if (delegate_.get())
    delegate_->Stop();
  // Cancel the notify task last, as the java side may have been mid-way through
  // posting a new notify task when we requested it stop
  CancelNotifyTask();
}

bool ProxyConfigServiceAndroid::RegisterProxyConfigService(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void ProxyConfigServiceAndroid::AddObserver(Observer* observer) {
  DCHECK(OnObserverThread());
  if (observers_.size() == 0) {
    AssignObserverThread();
    delegate_->Start(this);
  }
  // Note that any callbacks from the delegate will result in a scheduled task
  // on this same thread, so there is no race between delegate_->Start() and
  // AddObserver().
  observers_.AddObserver(observer);
}

void ProxyConfigServiceAndroid::RemoveObserver(Observer* observer) {
  DCHECK(OnObserverThread());
  observers_.RemoveObserver(observer);
  if (observers_.size() == 0) {
    delegate_->Stop();
  }
}

namespace {

ProxyServer LookupOneProxy(
    const ProxyConfigServiceAndroid::Delegate& delegate,
    const std::string& host_property,
    const std::string& port_property,
    int default_port,
    ProxyServer::Scheme scheme) {
  std::string uri = delegate.GetProperty(host_property);
  if (uri.empty())
    return ProxyServer();
  std::string port = delegate.GetProperty(port_property);
  uri += ':';
  uri += port.empty() ? base::IntToString(default_port) : port;
  return ProxyServer::FromURI(uri, scheme);
}

ProxyServer LookupHttpProxy(
    const ProxyConfigServiceAndroid::Delegate& delegate,
    const std::string& prefix,
    int default_port) {
  ProxyServer server = LookupOneProxy(delegate,
                                      prefix + ".proxyHost",
                                      prefix + ".proxyPort",
                                      default_port,
                                      ProxyServer::SCHEME_HTTP);
  if (server.is_valid())
    return server;
  // Fallback to default proxy server.
  return LookupOneProxy(delegate, "proxyHost", "proxyPort", default_port,
                        ProxyServer::SCHEME_HTTP);
}

ProxyServer LookupSocksProxy(
    const ProxyConfigServiceAndroid::Delegate& delegate) {
  return LookupOneProxy(delegate, "socksProxyHost", "socksProxyPort", 1080,
                        ProxyServer::SCHEME_SOCKS5);
}

void AddBypassRules(
    const ProxyConfigServiceAndroid::Delegate& delegate,
    const std::string& scheme,
    ProxyBypassRules* bypass_rules) {
  // The format of a hostname pattern is a list of hostnames that are separated
  // by | and that use * as a wildcard. For example, setting the
  // http.nonProxyHosts property to *.android.com|*.kernel.org will cause
  // requests to http://developer.android.com to be made without a proxy.
  std::string non_proxy_hosts = delegate.GetProperty(scheme + ".nonProxyHosts");
  if (non_proxy_hosts.empty())
    return;
  StringTokenizer tokenizer(non_proxy_hosts, "|");
  while (tokenizer.GetNext()) {
    std::string token = tokenizer.token();
    std::string pattern;
    TrimWhitespaceASCII(token, TRIM_ALL, &pattern);
    if (pattern.empty())
      continue;
    // '?' is not one of the specified pattern characters above.
    DCHECK_EQ(std::string::npos, pattern.find('?'));
    bypass_rules->AddRuleForHostname(scheme, pattern, -1);
  }
}

// returns true iff a valid proxy was found.
bool GetProxyRules(
    const ProxyConfigServiceAndroid::Delegate& delegate,
    ProxyConfig::ProxyRules* rules) {
  // Note that Chrome supports connecting to a proxy via HTTP or HTTPS, but
  // Android system properties only allow the former. This is true regardless of
  // the type of requests being proxies (http/https/ftp), and regardless of the
  // proxy port. The default proxy port for https is 443, but we connect to the
  // proxy itself over HTTP.
  // See libcore/luni/src/main/java/java/net/ProxySelectorImpl.java for the
  // semantics we're trying to match.
  rules->type = ProxyConfig::ProxyRules::TYPE_PROXY_PER_SCHEME;
  rules->proxy_for_http = LookupHttpProxy(delegate, "http", 80);
  rules->proxy_for_https = LookupHttpProxy(delegate, "https", 443);
  rules->proxy_for_ftp = LookupHttpProxy(delegate, "ftp", 80);
  rules->fallback_proxy = LookupSocksProxy(delegate);
  rules->bypass_rules.Clear();
  AddBypassRules(delegate, "ftp", &rules->bypass_rules);
  AddBypassRules(delegate, "http", &rules->bypass_rules);
  AddBypassRules(delegate, "https", &rules->bypass_rules);
  return rules->proxy_for_http.is_valid() ||
      rules->proxy_for_https.is_valid() ||
      rules->proxy_for_ftp.is_valid() ||
      rules->fallback_proxy.is_valid();
};

void GetLatestProxyConfigInternal(
    const ProxyConfigServiceAndroid::Delegate& delegate,
    ProxyConfig* config) {
  if (!GetProxyRules(delegate, &config->proxy_rules()))
    *config = ProxyConfig::CreateDirect();
}

}  // namespace

ProxyConfigService::ConfigAvailability
ProxyConfigServiceAndroid::GetLatestProxyConfig(ProxyConfig* config) {
  DCHECK(OnObserverThread());
  if (!config)
    return ProxyConfigService::CONFIG_UNSET;
  GetLatestProxyConfigInternal(*delegate_.get(), config);
  return ProxyConfigService::CONFIG_VALID;
}

// Posted on observer_loop_. See header file for intended behaviour.
class ProxyConfigServiceAndroid::NotifyTask {
 public:
  NotifyTask(ProxyConfigServiceAndroid* service);
  ~NotifyTask();
  void Cancel();
  void Run();

 private:
  ProxyConfigServiceAndroid* service_;
};

ProxyConfigServiceAndroid::NotifyTask::NotifyTask(
    ProxyConfigServiceAndroid* service)
    : service_(service) {
}

ProxyConfigServiceAndroid::NotifyTask::~NotifyTask() {
  // Should be called from IO thread.
  if (service_) {
    // This could happen if the notify task is removed from the message loop
    // before it has been run (e.g. if the message loop is destroyed).
    service_->ClearNotifyTask();
  }
}

void ProxyConfigServiceAndroid::NotifyTask::Cancel() {
  // Should be called from IO thread.
  service_ = NULL;
}

void ProxyConfigServiceAndroid::NotifyTask::Run() {
  // Should be called from IO thread.
  if (service_) {
    service_->ProxySettingsChangedCallback();
    service_ = NULL;
  }
}

void ProxyConfigServiceAndroid::ProxySettingsChanged() {
  // Called on any thread.
  DCHECK(observer_loop_);  // Should have been assigned in AddObserver().
  NotifyTask* task = NULL;
  {
    base::AutoLock lock(notify_task_lock_);
    // If there is already a task scheduled, don't schedule another one.
    if (notify_task_)
      return;
    notify_task_ = task = new NotifyTask(this);
  }
  observer_loop_->PostTask(FROM_HERE,
                     base::Bind(&NotifyTask::Run, base::Owned(task)));
}

ProxyConfigServiceAndroid::ProxyConfigServiceAndroid(
    ProxyConfigServiceAndroid::Delegate* delegate)
    : delegate_(delegate),
      notify_task_(NULL) {
}

void ProxyConfigServiceAndroid::ProxySettingsChangedCallback() {
  DCHECK(OnObserverThread());
  // Allow another task to be scheduled (before running observers).
  ClearNotifyTask();
  ProxyConfig config;
  GetLatestProxyConfigInternal(*delegate_.get(), &config);
  FOR_EACH_OBSERVER(Observer, observers_,
                    OnProxyConfigChanged(config,
                                         ProxyConfigService::CONFIG_VALID));
}

void ProxyConfigServiceAndroid::ClearNotifyTask() {
  base::AutoLock lock(notify_task_lock_);
  notify_task_ = NULL;
}

void ProxyConfigServiceAndroid::CancelNotifyTask() {
  base::AutoLock lock(notify_task_lock_);
  if (notify_task_) {
    notify_task_->Cancel();
    notify_task_ = NULL;
  }
}

bool ProxyConfigServiceAndroid::OnObserverThread() {
  // Check that we're on the observer thread or the observer thread hasn't been
  // assigned yet. The observer thread is only assigned in AddObserver(), which,
  // in theory, might not be called.
  return !observer_loop_ || observer_loop_->BelongsToCurrentThread();
}

void ProxyConfigServiceAndroid::AssignObserverThread() {
  // Called from AddObserver. We assign observer_loop_ here under a lock so that
  // it is guaranteed that ProxySettingsChanged(), which is called from another
  // thread, will see the value of observer_loop_.
  if (!observer_loop_) {
    observer_loop_ = base::MessageLoopProxy::current();
  } else {
    DCHECK(OnObserverThread());
  }
}

} // namespace net
