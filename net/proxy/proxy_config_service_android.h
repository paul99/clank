// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_PROXY_PROXY_CONFIG_SERVICE_ANDROID_H_
#define NET_PROXY_PROXY_CONFIG_SERVICE_ANDROID_H_
#pragma once

#include "base/android/scoped_java_ref.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "base/string_piece.h"
#include "base/synchronization/lock.h"
#include "net/base/net_export.h"
#include "net/proxy/proxy_config.h"
#include "net/proxy/proxy_config_service.h"

#include <string>

namespace base {
class MessageLoopProxy;
}

namespace net {

class NET_EXPORT ProxyConfigServiceAndroid : public ProxyConfigService {
 public:
  // Delegate abstracts access to the platform.
  // Owned by ProxyConfigServiceAndroid.
  class Delegate {
   public:
    virtual ~Delegate() {}
    virtual void Start(ProxyConfigServiceAndroid* service) = 0;
    virtual void Stop() = 0;
    virtual std::string GetProperty(const std::string& property) const = 0;
  };

  static ProxyConfigService* Create();
  // CreateForTests takes ownership of the delegate.
  static ProxyConfigServiceAndroid* CreateForTests(Delegate* delegate);
  virtual ~ProxyConfigServiceAndroid();

  // Register JNI bindings.
  static bool RegisterProxyConfigService(JNIEnv* env);

  // ProxyConfigService:
  virtual void AddObserver(Observer* observer) OVERRIDE;
  virtual void RemoveObserver(Observer* observer) OVERRIDE;
  virtual ConfigAvailability GetLatestProxyConfig(ProxyConfig* config) OVERRIDE;

  // Called from the platform on any thread to signal that proxy settings have
  // changed. AddObserver() must have been called at least once before.
  void ProxySettingsChanged();

  // Called from Java to signal that the proxy settings have changed.
  void ProxySettingsChanged(JNIEnv*, jobject) { ProxySettingsChanged(); }

 private:
  class NotifyTask;

  explicit ProxyConfigServiceAndroid(Delegate* delegate);

  void ProxySettingsChangedCallback();
  void CancelNotifyTask();
  void ClearNotifyTask();
  bool OnObserverThread();
  void AssignObserverThread();

  // Accessed by the constructor, and from then on from the Observer thread.
  scoped_ptr<Delegate> delegate_;
  ObserverList<Observer> observers_;


  base::Lock notify_task_lock_;
  NotifyTask* notify_task_;  // Accessed from any thread, guarded by lock_.

  // The thread that this object lives on; set on first call to AddObserver().
  // Notifications from the platform (ProxySettingsChanged) will be bounced
  // over to this thread before calling back to the Observer. All other
  // methods should be called on this thread.
  scoped_refptr<base::MessageLoopProxy> observer_loop_;
};

} // namespace net

#endif // NET_PROXY_PROXY_CONFIG_SERVICE_ANDROID_H_
