// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/geolocation/location_provider_android.h"

#include "base/time.h"
#include "content/browser/geolocation/android_location_api_adapter.h"
#include "content/common/geoposition.h"

// LocationProviderAndroid
LocationProviderAndroid::LocationProviderAndroid() {
}

LocationProviderAndroid::~LocationProviderAndroid() {
  StopProvider();
}

void LocationProviderAndroid::NotifyNewGeoposition(
    const Geoposition& position) {
  last_position_ = position;
  UpdateListeners();
}

bool LocationProviderAndroid::StartProvider(bool high_accuracy) {
  return AndroidLocationApiAdapter::GetInstance()->Start(this, high_accuracy);
}

void LocationProviderAndroid::StopProvider() {
  AndroidLocationApiAdapter::GetInstance()->Stop();
}

void LocationProviderAndroid::GetPosition(Geoposition* position) {
  *position = last_position_;
}

void LocationProviderAndroid::UpdatePosition() {
  // Nothing to do here, android framework will call us back on new position.
}

void LocationProviderAndroid::OnPermissionGranted(
  const GURL& requesting_frame) {
  // Nothing to do here.
}

LocationProviderBase* NewSystemLocationProvider() {
  return new LocationProviderAndroid;
}
