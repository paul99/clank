// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVICE_ORIENTATION_DEVICE_ORIENTATION_ANDROID_H_
#define CHROME_BROWSER_DEVICE_ORIENTATION_DEVICE_ORIENTATION_ANDROID_H_
#pragma once

#include <jni.h>

#include "base/synchronization/lock.h"
#include "base/memory/scoped_ptr.h"
#include "content/browser/device_orientation/data_fetcher.h"
#include "content/browser/device_orientation/orientation.h"

namespace device_orientation {

/**
 * Android implementation of DeviceOrientation API.
 *
 * Android's SensorManager has a push API, whereas Chrome wants to pull data.
 * To fit them together, we store incoming sensor events in a 1-element buffer.
 * SensorManager calls SetOrientation() which pushes a new value (discarding the
 * previous value if any). Chrome calls GetOrientation() which reads the most
 * recent value. Repeated calls to GetOrientation() will return the same value.
 *
 * TODO(husky): Hoist the thread out of ProviderImpl and have the Java code own
 * it instead. That way we can have SensorManager post directly to our thread
 * rather than going via the UI thread.
 */
class DeviceOrientationAndroid : public DataFetcher {
 public:
  // Must be called at startup, before Create().
  static void Init(JNIEnv* env);

  // Factory function. We'll listen for events for the lifetime of this object.
  // Returns NULL on error.
  static DataFetcher* Create();

  virtual ~DeviceOrientationAndroid();

  // Implementation of DataFetcher.
  virtual bool GetOrientation(Orientation* orientation) OVERRIDE;

  void GotOrientation(JNIEnv*, jobject,
                      double alpha, double beta, double gamma);

 private:
  DeviceOrientationAndroid();

  // Wrappers for JNI methods.
  bool Start(int rateInMilliseconds);
  void Stop();

  // Value returned by GetOrientation.
  scoped_ptr<Orientation> current_orientation_;

  // 1-element buffer, written by SetOrientation, read by GetOrientation.
  base::Lock next_orientation_lock_;
  scoped_ptr<Orientation> next_orientation_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOrientationAndroid);
};

} // namespace device_orientation

#endif  // CHROME_BROWSER_DEVICE_ORIENTATION_DEVICE_ORIENTATION_ANDROID_H_
