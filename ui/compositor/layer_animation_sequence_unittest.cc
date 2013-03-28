// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/layer_animation_sequence.h"

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/compositor/layer_animation_delegate.h"
#include "ui/compositor/layer_animation_element.h"
#include "ui/compositor/test/test_layer_animation_delegate.h"
#include "ui/compositor/test/test_layer_animation_observer.h"
#include "ui/compositor/test/test_utils.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/transform.h"

namespace ui {

namespace {

// Check that the sequence behaves sanely when it contains no elements.
TEST(LayerAnimationSequenceTest, NoElement) {
  LayerAnimationSequence sequence;
  base::TimeTicks start_time;
  start_time += base::TimeDelta::FromSeconds(1);
  sequence.set_start_time(start_time);
  EXPECT_TRUE(sequence.IsFinished(start_time));
  EXPECT_TRUE(sequence.properties().size() == 0);
  LayerAnimationElement::AnimatableProperties properties;
  EXPECT_FALSE(sequence.HasCommonProperty(properties));
}

// Check that the sequences progresses the delegate as expected when it contains
// a single element.
TEST(LayerAnimationSequenceTest, SingleElement) {
  LayerAnimationSequence sequence;
  TestLayerAnimationDelegate delegate;
  float start = 0.0f;
  float middle = 0.5f;
  float target = 1.0f;
  base::TimeTicks start_time;
  base::TimeDelta delta = base::TimeDelta::FromSeconds(1);
  sequence.AddElement(
      LayerAnimationElement::CreateOpacityElement(target, delta));

  for (int i = 0; i < 2; ++i) {
    start_time += delta;
    sequence.set_start_time(start_time);
    delegate.SetOpacityFromAnimation(start);
    sequence.Progress(start_time, &delegate);
    EXPECT_FLOAT_EQ(start, delegate.GetOpacityForAnimation());
    sequence.Progress(start_time + base::TimeDelta::FromMilliseconds(500),
                      &delegate);
    EXPECT_FLOAT_EQ(middle, delegate.GetOpacityForAnimation());
    sequence.Progress(start_time + base::TimeDelta::FromMilliseconds(1000),
                      &delegate);
    EXPECT_FLOAT_EQ(target, delegate.GetOpacityForAnimation());
  }

  EXPECT_TRUE(sequence.properties().size() == 1);
  EXPECT_TRUE(sequence.properties().find(LayerAnimationElement::OPACITY) !=
              sequence.properties().end());
  EXPECT_TRUE(sequence.IsFinished(start_time + delta));
}

// Check that the sequences progresses the delegate as expected when it contains
// multiple elements. Note, see the layer animator tests for cyclic sequences.
TEST(LayerAnimationSequenceTest, MultipleElement) {
  LayerAnimationSequence sequence;
  TestLayerAnimationDelegate delegate;
  float start_opacity = 0.0f;
  float middle_opacity = 0.5f;
  float target_opacity = 1.0f;
  base::TimeTicks start_time;
  base::TimeDelta delta = base::TimeDelta::FromSeconds(1);
  sequence.AddElement(
      LayerAnimationElement::CreateOpacityElement(target_opacity, delta));

  // Pause bounds for a second.
  LayerAnimationElement::AnimatableProperties properties;
  properties.insert(LayerAnimationElement::BOUNDS);

  sequence.AddElement(
      LayerAnimationElement::CreatePauseElement(properties, delta));

  gfx::Transform start_transform, target_transform, middle_transform;
  start_transform.Rotate(-30.0);
  target_transform.Rotate(30.0);

  sequence.AddElement(
      LayerAnimationElement::CreateTransformElement(target_transform, delta));

  for (int i = 0; i < 2; ++i) {
    start_time += delta + delta + delta;
    sequence.set_start_time(start_time);
    delegate.SetOpacityFromAnimation(start_opacity);
    delegate.SetTransformFromAnimation(start_transform);

    sequence.Progress(start_time, &delegate);
    EXPECT_FLOAT_EQ(start_opacity, delegate.GetOpacityForAnimation());
    sequence.Progress(start_time + base::TimeDelta::FromMilliseconds(500),
                      &delegate);
    EXPECT_FLOAT_EQ(middle_opacity, delegate.GetOpacityForAnimation());
    sequence.Progress(start_time + base::TimeDelta::FromMilliseconds(1000),
                      &delegate);
    EXPECT_FLOAT_EQ(target_opacity, delegate.GetOpacityForAnimation());
    TestLayerAnimationDelegate copy = delegate;

    // In the middle of the pause -- nothing should have changed.
    sequence.Progress(start_time + base::TimeDelta::FromMilliseconds(1500),
                      &delegate);
    CheckApproximatelyEqual(delegate.GetBoundsForAnimation(),
                            copy.GetBoundsForAnimation());
    CheckApproximatelyEqual(delegate.GetTransformForAnimation(),
                            copy.GetTransformForAnimation());
    EXPECT_FLOAT_EQ(delegate.GetOpacityForAnimation(),
                    copy.GetOpacityForAnimation());


    sequence.Progress(start_time + base::TimeDelta::FromMilliseconds(2000),
                      &delegate);
    CheckApproximatelyEqual(start_transform,
                            delegate.GetTransformForAnimation());
    sequence.Progress(start_time + base::TimeDelta::FromMilliseconds(2500),
                      &delegate);
    CheckApproximatelyEqual(middle_transform,
                            delegate.GetTransformForAnimation());
    sequence.Progress(start_time + base::TimeDelta::FromMilliseconds(3000),
                      &delegate);
    CheckApproximatelyEqual(target_transform,
                            delegate.GetTransformForAnimation());
  }

  EXPECT_TRUE(sequence.properties().size() == 3);
  EXPECT_TRUE(sequence.properties().find(LayerAnimationElement::OPACITY) !=
              sequence.properties().end());
  EXPECT_TRUE(sequence.properties().find(LayerAnimationElement::TRANSFORM) !=
              sequence.properties().end());
  EXPECT_TRUE(sequence.properties().find(LayerAnimationElement::BOUNDS) !=
              sequence.properties().end());
  EXPECT_TRUE(sequence.IsFinished(start_time + delta + delta + delta));
}

// Check that a sequence can still be aborted if it has cycled many times.
TEST(LayerAnimationSequenceTest, AbortingCyclicSequence) {
  LayerAnimationSequence sequence;
  TestLayerAnimationDelegate delegate;
  float start_opacity = 0.0f;
  float target_opacity = 1.0f;
  base::TimeTicks start_time;
  base::TimeDelta delta = base::TimeDelta::FromSeconds(1);
  sequence.AddElement(
      LayerAnimationElement::CreateOpacityElement(target_opacity, delta));

  sequence.AddElement(
      LayerAnimationElement::CreateOpacityElement(start_opacity, delta));

  sequence.set_is_cyclic(true);

  delegate.SetOpacityFromAnimation(start_opacity);

  start_time += delta;
  sequence.set_start_time(start_time);
  sequence.Progress(start_time + base::TimeDelta::FromMilliseconds(101000),
                    &delegate);
  EXPECT_FLOAT_EQ(target_opacity, delegate.GetOpacityForAnimation());
  sequence.Abort();

  // Should be able to reuse the sequence after aborting.
  delegate.SetOpacityFromAnimation(start_opacity);
  start_time += base::TimeDelta::FromMilliseconds(101000);
  sequence.set_start_time(start_time);
  sequence.Progress(start_time + base::TimeDelta::FromMilliseconds(100000),
                    &delegate);
  EXPECT_FLOAT_EQ(start_opacity, delegate.GetOpacityForAnimation());
}

// Check that a sequence can be 'fast-forwarded' to the end and the target set.
// Also check that this has no effect if the sequence is cyclic.
TEST(LayerAnimationSequenceTest, SetTarget) {
  LayerAnimationSequence sequence;
  TestLayerAnimationDelegate delegate;
  float start_opacity = 0.0f;
  float target_opacity = 1.0f;
  base::TimeDelta delta = base::TimeDelta::FromSeconds(1);
  sequence.AddElement(
      LayerAnimationElement::CreateOpacityElement(target_opacity, delta));

  LayerAnimationElement::TargetValue target_value(&delegate);
  target_value.opacity = start_opacity;
  sequence.GetTargetValue(&target_value);
  EXPECT_FLOAT_EQ(target_opacity, target_value.opacity);

  sequence.set_is_cyclic(true);
  target_value.opacity = start_opacity;
  sequence.GetTargetValue(&target_value);
  EXPECT_FLOAT_EQ(start_opacity, target_value.opacity);
}

TEST(LayerAnimationSequenceTest, AddObserver) {
  base::TimeTicks start_time;
  base::TimeDelta delta = base::TimeDelta::FromSeconds(1);
  LayerAnimationSequence sequence;
  sequence.AddElement(
      LayerAnimationElement::CreateOpacityElement(1.0f, delta));
  for (int i = 0; i < 2; ++i) {
    start_time += delta;
    sequence.set_start_time(start_time);
    TestLayerAnimationObserver observer;
    TestLayerAnimationDelegate delegate;
    sequence.AddObserver(&observer);
    EXPECT_TRUE(!observer.last_ended_sequence());
    sequence.Progress(start_time + delta, &delegate);
    EXPECT_EQ(observer.last_ended_sequence(), &sequence);
    sequence.RemoveObserver(&observer);
  }
}

} // namespace

} // namespace ui
