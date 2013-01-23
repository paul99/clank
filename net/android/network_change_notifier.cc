// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/android/network_change_notifier.h"

#include "base/logging.h"

namespace net {
namespace android {

NetworkChangeNotifierAndroid::NetworkChangeNotifierAndroid()
    : is_connectivity_state_known_(false),
      is_online_(true) {}

NetworkChangeNotifierAndroid::~NetworkChangeNotifierAndroid() {}

void NetworkChangeNotifierAndroid::SetConnectivityOnline(bool is_online) {
  bool should_send_notification;
  {
    base::AutoLock auto_lock(lock_);
    should_send_notification = !is_connectivity_state_known_ ||
        is_online_ != is_online;
    is_connectivity_state_known_ = true;
    is_online_ = is_online;
  }
  if (should_send_notification) {
    VLOG(1) << "Got connectivity change: online? " << is_online;
    NotifyObserversOfOnlineStateChange();
  }
}

bool NetworkChangeNotifierAndroid::IsCurrentlyOffline() const {
  base::AutoLock auto_lock(lock_);
  return !is_online_;
}

}  // namespace android
}  // namespace net
