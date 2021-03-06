// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_NETWORK_CHANGE_NOTIFIER_ANDROID_H_
#define NET_BASE_NETWORK_CHANGE_NOTIFIER_ANDROID_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/synchronization/lock.h"
#include "net/base/network_change_notifier.h"

namespace net {
namespace android {

class NetworkChangeNotifierAndroid : public net::NetworkChangeNotifier {
 public:
  NetworkChangeNotifierAndroid();
  virtual ~NetworkChangeNotifierAndroid();

  void SetConnectivityOnline(bool is_online);

 private:
  // NetworkChangeNotifier:
  virtual bool IsCurrentlyOffline() const OVERRIDE;

  mutable base::Lock lock_;  // Protects the state below.
  bool is_connectivity_state_known_;
  bool is_online_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeNotifierAndroid);
};

}  // namespace android
}  // namespace net

#endif  // NET_BASE_NETWORK_CHANGE_NOTIFIER_ANDROID_H_
