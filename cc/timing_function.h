// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TIMING_FUNCTION_H_
#define CC_TIMING_FUNCTION_H_

#include "cc/animation_curve.h"
#include "cc/cc_export.h"
#include "third_party/skia/include/core/SkScalar.h"

namespace cc {

// See http://www.w3.org/TR/css3-transitions/.
class CC_EXPORT TimingFunction : public FloatAnimationCurve {
 public:
  virtual ~TimingFunction();

  // Partial implementation of FloatAnimationCurve.
  virtual double duration() const OVERRIDE;

 protected:
  TimingFunction();
};

class CC_EXPORT CubicBezierTimingFunction : public TimingFunction {
 public:
  static scoped_ptr<CubicBezierTimingFunction> create(double x1, double y1,
                                                      double x2, double y2);
  virtual ~CubicBezierTimingFunction();

  // Partial implementation of FloatAnimationCurve.
  virtual float getValue(double time) const OVERRIDE;
  virtual scoped_ptr<AnimationCurve> clone() const OVERRIDE;

 protected:
  CubicBezierTimingFunction(double x1, double y1, double x2, double y2);

  SkScalar x1_;
  SkScalar y1_;
  SkScalar x2_;
  SkScalar y2_;
};

class CC_EXPORT EaseTimingFunction {
 public:
  static scoped_ptr<TimingFunction> create();
};

class CC_EXPORT EaseInTimingFunction {
 public:
  static scoped_ptr<TimingFunction> create();
};

class CC_EXPORT EaseOutTimingFunction {
 public:
  static scoped_ptr<TimingFunction> create();
};

class CC_EXPORT EaseInOutTimingFunction {
 public:
  static scoped_ptr<TimingFunction> create();
};

}  // namespace cc

#endif  // CC_TIMING_FUNCTION_H_
