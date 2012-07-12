// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/touch_point.h"

#include "base/debug/debugger.h"
#include "base/time.h"
#include "base/logging.h"

#include "jni/touch_point_jni.h"

void MaybeAddTouchPoint(JNIEnv* env, jobject pt, WebKit::WebTouchEvent& event) {
  using WebKit::WebTouchEvent;
  using WebKit::WebTouchPoint;

  WebTouchPoint::State state = static_cast<WebTouchPoint::State>(
      Java_TouchPoint_getState(env, pt));
  if (state == WebTouchPoint::StateUndefined)
    return;

  // When generating a cancel event from an event of a different type, the
  // touch points are out of sync, so we ensure the points are marked as
  // canceled as well.
  if (event.type == WebTouchEvent::TouchCancel)
    state = WebTouchPoint::StateCancelled;

  // Record the current number of points in the WebTouchEvent
  const int idx = event.touchesLength;
  DCHECK_LT(idx, WebKit::WebTouchEvent::touchesLengthCap);

  WebTouchPoint wtp;
  wtp.id = Java_TouchPoint_getId(env, pt);
  wtp.state = state;
  wtp.position.x = Java_TouchPoint_getX(env, pt);
  wtp.position.y = Java_TouchPoint_getY(env, pt);
  // TODO(joth): Raw event co-ordinates.
  wtp.screenPosition = wtp.position;
  wtp.force = Java_TouchPoint_getPressure(env, pt);

  // TODO(clank) WebKit stores touch point size as a pair of radii, which
  // are integers.  We receive touch point size from Android as a float
  // between 0 and 1.  We interpret 'size' as an elliptical area.  We convert
  // to a radius and then scale up to avoid truncating away all of the data.
  // W3C spec is for the radii to be in units of screen pixels.  Need to change.
  const static double PI = 3.1415926;
  const static double SCALE_FACTOR = 1024.0;
  const int radius =
    (int)((sqrt(Java_TouchPoint_getSize(env, pt)) / PI) * SCALE_FACTOR);
  wtp.radiusX = radius;
  wtp.radiusY = radius;
  // Since our radii are equal, a rotation angle doesn't mean anything.
  wtp.rotationAngle = 0.0;

  // Add the newly created WebTouchPoint to the event
  event.touches[idx] = wtp;
  ++(event.touchesLength);
}

void TouchPoint::BuildWebTouchEvent(JNIEnv* env, jint type, jlong time,
    jobjectArray pts, WebKit::WebTouchEvent& event) {
  using WebKit::WebTouchEvent;
  using WebKit::WebTouchPoint;

  event.type = static_cast<WebTouchEvent::Type>(type);
  event.timeStampSeconds =
      static_cast<double>(time) / base::Time::kMillisecondsPerSecond;
  int arrayLength = env->GetArrayLength(pts);
  // Loop until either all of the input points have been consumed or the output
  // array has been filled
  for (int i = 0; i < arrayLength; i++) {
    jobject pt = env->GetObjectArrayElement(pts, i);
    MaybeAddTouchPoint(env, pt, event);
    if (event.touchesLength >= event.touchesLengthCap)
      break;
  }
  DCHECK_GT(event.touchesLength, 0U);
}

// ----------------------------------------------------------------------------

static void RegisterConstants(JNIEnv* env) {
   Java_TouchPoint_initializeConstants(
       env,
       WebKit::WebTouchEvent::TouchStart,
       WebKit::WebTouchEvent::TouchMove,
       WebKit::WebTouchEvent::TouchEnd,
       WebKit::WebTouchEvent::TouchCancel,
       WebKit::WebTouchPoint::StateUndefined,
       WebKit::WebTouchPoint::StateReleased,
       WebKit::WebTouchPoint::StatePressed,
       WebKit::WebTouchPoint::StateMoved,
       WebKit::WebTouchPoint::StateStationary,
       WebKit::WebTouchPoint::StateCancelled);
}

bool RegisterTouchPoint(JNIEnv* env) {
  if (!base::android::HasClass(env, kTouchPointClassPath)) {
    DLOG(ERROR) << "Unable to find class TouchPoint!";
    return false;
  }

  if (RegisterNativesImpl(env) < 0)
    return false;

  RegisterConstants(env);

  return true;
}
