// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "web_transform_animation_curve_impl.h"

#include "cc/keyframed_animation_curve.h"
#include "cc/timing_function.h"
#include "web_animation_curve_common.h"

namespace WebKit {

WebTransformAnimationCurve* WebTransformAnimationCurve::create()
{
    return new WebTransformAnimationCurveImpl();
}

WebTransformAnimationCurveImpl::WebTransformAnimationCurveImpl()
    : m_curve(cc::KeyframedTransformAnimationCurve::create())
{
}

WebTransformAnimationCurveImpl::~WebTransformAnimationCurveImpl()
{
}

WebAnimationCurve::AnimationCurveType WebTransformAnimationCurveImpl::type() const
{
    return WebAnimationCurve::AnimationCurveTypeTransform;
}

void WebTransformAnimationCurveImpl::add(const WebTransformKeyframe& keyframe)
{
    add(keyframe, TimingFunctionTypeEase);
}

void WebTransformAnimationCurveImpl::add(const WebTransformKeyframe& keyframe, TimingFunctionType type)
{
    m_curve->addKeyframe(cc::TransformKeyframe::create(keyframe.time, keyframe.value, createTimingFunction(type)));
}

void WebTransformAnimationCurveImpl::add(const WebTransformKeyframe& keyframe, double x1, double y1, double x2, double y2)
{
    m_curve->addKeyframe(cc::TransformKeyframe::create(keyframe.time, keyframe.value, cc::CubicBezierTimingFunction::create(x1, y1, x2, y2).PassAs<cc::TimingFunction>()));
}

WebTransformationMatrix WebTransformAnimationCurveImpl::getValue(double time) const
{
    return m_curve->getValue(time);
}

scoped_ptr<cc::AnimationCurve> WebTransformAnimationCurveImpl::cloneToAnimationCurve() const
{
    return m_curve->clone();
}

} // namespace WebKit
