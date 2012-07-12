// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GEOLOCATION_LOCATION_PROVIDER_ANDROID_H_
#define CONTENT_BROWSER_GEOLOCATION_LOCATION_PROVIDER_ANDROID_H_

#include "base/memory/scoped_ptr.h"
#include "content/common/geoposition.h"
#include "content/browser/geolocation/location_provider.h"

class AndroidLocationApiAdapter;
struct Geoposition;

// Location provider for Android using the platform provider over JNI.
class LocationProviderAndroid : public LocationProviderBase {
 public:
  LocationProviderAndroid();
  virtual ~LocationProviderAndroid();

  // Called by the AndroidLocationApiAdapter.
  void NotifyNewGeoposition(const Geoposition& position);

  // LocationProvider.
  virtual bool StartProvider(bool high_accuracy) OVERRIDE;
  virtual void StopProvider() OVERRIDE;
  virtual void GetPosition(Geoposition* position) OVERRIDE;
  virtual void UpdatePosition() OVERRIDE;
  virtual void OnPermissionGranted(const GURL& requesting_frame) OVERRIDE;

 private:
  Geoposition last_position_;
};

#endif  // CONTENT_BROWSER_GEOLOCATION_LOCATION_PROVIDER_ANDROID_H_
