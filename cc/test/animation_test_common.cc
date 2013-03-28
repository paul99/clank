// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/animation_test_common.h"

#include "cc/keyframed_animation_curve.h"
#include "cc/layer.h"
#include "cc/layer_animation_controller.h"
#include "cc/layer_impl.h"
#include <public/WebTransformOperations.h>

using cc::ActiveAnimation;
using cc::AnimationCurve;
using cc::EaseTimingFunction;
using cc::FloatKeyframe;
using cc::KeyframedFloatAnimationCurve;
using cc::KeyframedTransformAnimationCurve;
using cc::TimingFunction;
using cc::TransformKeyframe;

namespace cc {

static int nextAnimationId = 0;

template <class Target>
int addOpacityTransition(Target& target, double duration, float startOpacity, float endOpacity, bool useTimingFunction)
{
    scoped_ptr<KeyframedFloatAnimationCurve> curve(KeyframedFloatAnimationCurve::create());

    scoped_ptr<TimingFunction> func;
    if (!useTimingFunction)
        func = EaseTimingFunction::create();
    if (duration > 0)
        curve->addKeyframe(FloatKeyframe::create(0, startOpacity, func.Pass()));
    curve->addKeyframe(FloatKeyframe::create(duration, endOpacity, scoped_ptr<cc::TimingFunction>()));

    int id = nextAnimationId++;

    scoped_ptr<ActiveAnimation> animation(ActiveAnimation::create(curve.PassAs<AnimationCurve>(), id, 0, ActiveAnimation::Opacity));
    animation->setNeedsSynchronizedStartTime(true);

    target.addAnimation(animation.Pass());
    return id;
}

template <class Target>
int addAnimatedTransform(Target& target, double duration, int deltaX, int deltaY)
{
    scoped_ptr<KeyframedTransformAnimationCurve> curve(KeyframedTransformAnimationCurve::create());

    if (duration > 0) {
        WebKit::WebTransformOperations startOperations;
        startOperations.appendTranslate(deltaX, deltaY, 0);
        curve->addKeyframe(TransformKeyframe::create(0, startOperations, scoped_ptr<cc::TimingFunction>()));
    }

    WebKit::WebTransformOperations operations;
    operations.appendTranslate(deltaX, deltaY, 0);
    curve->addKeyframe(TransformKeyframe::create(duration, operations, scoped_ptr<cc::TimingFunction>()));

    int id = nextAnimationId++;

    scoped_ptr<ActiveAnimation> animation(ActiveAnimation::create(curve.PassAs<AnimationCurve>(), id, 0, ActiveAnimation::Transform));
    animation->setNeedsSynchronizedStartTime(true);

    target.addAnimation(animation.Pass());
    return id;
}

FakeFloatAnimationCurve::FakeFloatAnimationCurve()
    : m_duration(1)
{
}

FakeFloatAnimationCurve::FakeFloatAnimationCurve(double duration)
    : m_duration(duration)
{
}

FakeFloatAnimationCurve::~FakeFloatAnimationCurve()
{
}

double FakeFloatAnimationCurve::duration() const
{
    return m_duration;
}

float FakeFloatAnimationCurve::getValue(double now) const
{
    return 0;
}

scoped_ptr<cc::AnimationCurve> FakeFloatAnimationCurve::clone() const
{
    return make_scoped_ptr(new FakeFloatAnimationCurve).PassAs<cc::AnimationCurve>();
}

FakeTransformTransition::FakeTransformTransition(double duration)
    : m_duration(duration)
{
}

FakeTransformTransition::~FakeTransformTransition()
{
}

double FakeTransformTransition::duration() const
{
    return m_duration;
}

WebKit::WebTransformationMatrix FakeTransformTransition::getValue(double time) const
{
    return WebKit::WebTransformationMatrix();
}

scoped_ptr<cc::AnimationCurve> FakeTransformTransition::clone() const
{
    return make_scoped_ptr(new FakeTransformTransition(*this)).PassAs<cc::AnimationCurve>();
}


FakeFloatTransition::FakeFloatTransition(double duration, float from, float to)
    : m_duration(duration)
    , m_from(from)
    , m_to(to)
{
}

FakeFloatTransition::~FakeFloatTransition()
{
}

double FakeFloatTransition::duration() const
{
    return m_duration;
}

float FakeFloatTransition::getValue(double time) const
{
    time /= m_duration;
    if (time >= 1)
        time = 1;
    return (1 - time) * m_from + time * m_to;
}

FakeLayerAnimationControllerClient::FakeLayerAnimationControllerClient()
    : m_opacity(0)
{
}

FakeLayerAnimationControllerClient::~FakeLayerAnimationControllerClient()
{
}

int FakeLayerAnimationControllerClient::id() const
{
    return 0;
}

void FakeLayerAnimationControllerClient::setOpacityFromAnimation(float opacity)
{
    m_opacity = opacity;
}

float FakeLayerAnimationControllerClient::opacity() const
{
    return m_opacity;
}

void FakeLayerAnimationControllerClient::setTransformFromAnimation(const gfx::Transform& transform)
{
    m_transform = transform;
}

const gfx::Transform& FakeLayerAnimationControllerClient::transform() const
{
    return m_transform;
}

scoped_ptr<cc::AnimationCurve> FakeFloatTransition::clone() const
{
    return make_scoped_ptr(new FakeFloatTransition(*this)).PassAs<cc::AnimationCurve>();
}

int addOpacityTransitionToController(cc::LayerAnimationController& controller, double duration, float startOpacity, float endOpacity, bool useTimingFunction)
{
    return addOpacityTransition(controller, duration, startOpacity, endOpacity, useTimingFunction);
}

int addAnimatedTransformToController(cc::LayerAnimationController& controller, double duration, int deltaX, int deltaY)
{
    return addAnimatedTransform(controller, duration, deltaX, deltaY);
}

int addOpacityTransitionToLayer(cc::Layer& layer, double duration, float startOpacity, float endOpacity, bool useTimingFunction)
{
    return addOpacityTransition(layer, duration, startOpacity, endOpacity, useTimingFunction);
}

int addOpacityTransitionToLayer(cc::LayerImpl& layer, double duration, float startOpacity, float endOpacity, bool useTimingFunction)
{
    return addOpacityTransition(*layer.layerAnimationController(), duration, startOpacity, endOpacity, useTimingFunction);
}

int addAnimatedTransformToLayer(cc::Layer& layer, double duration, int deltaX, int deltaY)
{
    return addAnimatedTransform(layer, duration, deltaX, deltaY);
}

int addAnimatedTransformToLayer(cc::LayerImpl& layer, double duration, int deltaX, int deltaY)
{
    return addAnimatedTransform(*layer.layerAnimationController(), duration, deltaX, deltaY);
}

}  // namespace cc
