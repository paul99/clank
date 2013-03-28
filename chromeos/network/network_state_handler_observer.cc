// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/network_state_handler_observer.h"

namespace chromeos {

NetworkStateHandlerObserver::NetworkStateHandlerObserver() {
}

NetworkStateHandlerObserver::~NetworkStateHandlerObserver() {
}

void NetworkStateHandlerObserver::NetworkManagerChanged() {
}

void NetworkStateHandlerObserver::NetworkListChanged(
    const NetworkStateList& networks) {
}

void NetworkStateHandlerObserver::DeviceListChanged() {
}

void NetworkStateHandlerObserver::ActiveNetworkChanged(
    const NetworkState* network) {
}

void NetworkStateHandlerObserver::ActiveNetworkStateChanged(
    const NetworkState* network) {
}

void NetworkStateHandlerObserver::NetworkServiceChanged(
    const NetworkState* network) {
}

}  // namespace chromeos
