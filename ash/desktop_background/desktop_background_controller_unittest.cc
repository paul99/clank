// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/desktop_background/desktop_background_controller.h"

#include "ash/desktop_background/desktop_background_widget_controller.h"
#include "ash/shell.h"
#include "ash/shell_window_ids.h"
#include "ash/test/ash_test_base.h"
#include "ui/aura/root_window.h"

using aura::RootWindow;
using aura::Window;

namespace {

// Containers IDs used for tests.
const int kDesktopBackgroundId =
    ash::internal::kShellWindowId_DesktopBackgroundContainer;
const int kLockScreenBackgroundId =
    ash::internal::kShellWindowId_LockScreenBackgroundContainer;

// Returns number of child windows in a shell window container.
int ChildCountForContainer(int container_id) {
  RootWindow* root = ash::Shell::GetPrimaryRootWindow();
  Window* container = root->GetChildById(container_id);
  return static_cast<int>(container->children().size());
}

// Steps a widget's layer animation until it is completed. Animations must be
// enabled.
void RunAnimationForWidget(views::Widget* widget) {
  // Animations must be enabled for stepping to work.
  DCHECK(!ui::LayerAnimator::disable_animations_for_test());

  ui::Layer* layer = widget->GetNativeView()->layer();
  ui::LayerAnimator* animator = layer->GetAnimator();
  ui::AnimationContainerElement* element = layer->GetAnimator();
  // Multiple steps are required to complete complex animations.
  // TODO(vollick): This should not be necessary. crbug.com/154017
  while (animator->is_animating()) {
    base::TimeTicks step_time = animator->last_step_time();
    element->Step(step_time + base::TimeDelta::FromMilliseconds(1000));
  }
}

}  // namespace

namespace ash {
namespace internal {

class DesktopBackgroundControllerTest : public test::AshTestBase {
 public:
  DesktopBackgroundControllerTest() {}
  virtual ~DesktopBackgroundControllerTest() {}

  virtual void SetUp() OVERRIDE {
    test::AshTestBase::SetUp();
    // Ash shell initialization creates wallpaper. Reset it so we can manually
    // control wallpaper creation and animation in our tests.
    RootWindow* root = Shell::GetPrimaryRootWindow();
    root->SetProperty(kDesktopController,
        static_cast<DesktopBackgroundWidgetController*>(NULL));
    root->SetProperty(kAnimatingDesktopController,
        static_cast<AnimatingDesktopController*>(NULL));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DesktopBackgroundControllerTest);
};

TEST_F(DesktopBackgroundControllerTest, BasicReparenting) {
  DesktopBackgroundController* controller =
      Shell::GetInstance()->desktop_background_controller();
  controller->CreateEmptyWallpaper();

  // Wallpaper view/window exists in the desktop background container and
  // nothing is in the lock screen background container.
  EXPECT_EQ(1, ChildCountForContainer(kDesktopBackgroundId));
  EXPECT_EQ(0, ChildCountForContainer(kLockScreenBackgroundId));

  // Moving background to lock container should succeed the first time but
  // subsequent calls should do nothing.
  EXPECT_TRUE(controller->MoveDesktopToLockedContainer());
  EXPECT_FALSE(controller->MoveDesktopToLockedContainer());

  // One window is moved from desktop to lock container.
  EXPECT_EQ(0, ChildCountForContainer(kDesktopBackgroundId));
  EXPECT_EQ(1, ChildCountForContainer(kLockScreenBackgroundId));

  // Moving background to desktop container should succeed the first time.
  EXPECT_TRUE(controller->MoveDesktopToUnlockedContainer());
  EXPECT_FALSE(controller->MoveDesktopToUnlockedContainer());

  // One window is moved from lock to desktop container.
  EXPECT_EQ(1, ChildCountForContainer(kDesktopBackgroundId));
  EXPECT_EQ(0, ChildCountForContainer(kLockScreenBackgroundId));
}

TEST_F(DesktopBackgroundControllerTest, ControllerOwnership) {
  // We cannot short-circuit animations for this test.
  ui::LayerAnimator::set_disable_animations_for_test(false);

  // Create wallpaper and background view.
  DesktopBackgroundController* controller =
      Shell::GetInstance()->desktop_background_controller();
  controller->CreateEmptyWallpaper();

  // The new wallpaper is ready to start animating. kAnimatingDesktopController
  // holds the widget controller instance. kDesktopController will get it later.
  RootWindow* root = Shell::GetPrimaryRootWindow();
  EXPECT_TRUE(
      root->GetProperty(kAnimatingDesktopController)->GetController(false));

  // kDesktopController will receive the widget controller when the animation
  // is done.
  EXPECT_FALSE(root->GetProperty(kDesktopController));

  // Force the widget's layer animation to play to completion.
  RunAnimationForWidget(
      root->GetProperty(kAnimatingDesktopController)->GetController(false)->
              widget());

  // Ownership has moved from kAnimatingDesktopController to kDesktopController.
  EXPECT_FALSE(
      root->GetProperty(kAnimatingDesktopController)->GetController(false));
  EXPECT_TRUE(root->GetProperty(kDesktopController));
}

// Test for crbug.com/149043 "Unlock screen, no launcher appears". Ensure we
// move all desktop views if there are more than one.
TEST_F(DesktopBackgroundControllerTest, BackgroundMovementDuringUnlock) {
  // We cannot short-circuit animations for this test.
  ui::LayerAnimator::set_disable_animations_for_test(false);

  // Reset wallpaper state, see ControllerOwnership above.
  DesktopBackgroundController* controller =
      Shell::GetInstance()->desktop_background_controller();
  controller->CreateEmptyWallpaper();

  // Run wallpaper show animation to completion.
  RootWindow* root = Shell::GetPrimaryRootWindow();
  RunAnimationForWidget(
      root->GetProperty(kAnimatingDesktopController)->GetController(false)->
              widget());

  // User locks the screen, which moves the background forward.
  controller->MoveDesktopToLockedContainer();

  // Suspend/resume cycle causes wallpaper to refresh, loading a new desktop
  // background that will animate in on top of the old one.
  controller->CreateEmptyWallpaper();

  // In this state we have two desktop background views stored in different
  // properties. Both are in the lock screen background container.
  EXPECT_TRUE(
      root->GetProperty(kAnimatingDesktopController)->GetController(false));
  EXPECT_TRUE(root->GetProperty(kDesktopController));
  EXPECT_EQ(0, ChildCountForContainer(kDesktopBackgroundId));
  EXPECT_EQ(2, ChildCountForContainer(kLockScreenBackgroundId));

  // Before the wallpaper's animation completes, user unlocks the screen, which
  // moves the desktop to the back.
  controller->MoveDesktopToUnlockedContainer();

  // Ensure both desktop backgrounds have moved.
  EXPECT_EQ(2, ChildCountForContainer(kDesktopBackgroundId));
  EXPECT_EQ(0, ChildCountForContainer(kLockScreenBackgroundId));

  // Finish the new desktop background animation.
  RunAnimationForWidget(
      root->GetProperty(kAnimatingDesktopController)->GetController(false)->
              widget());

  // Now there is one desktop background, in the back.
  EXPECT_EQ(1, ChildCountForContainer(kDesktopBackgroundId));
  EXPECT_EQ(0, ChildCountForContainer(kLockScreenBackgroundId));
}

// Test for crbug.com/156542. Animating wallpaper should immediately finish
// animation and replace current wallpaper before next animation starts.
TEST_F(DesktopBackgroundControllerTest, ChangeWallpaperQuick) {
  // We cannot short-circuit animations for this test.
  ui::LayerAnimator::set_disable_animations_for_test(false);

  // Reset wallpaper state, see ControllerOwnership above.
  DesktopBackgroundController* controller =
      Shell::GetInstance()->desktop_background_controller();
  controller->CreateEmptyWallpaper();

  // Run wallpaper show animation to completion.
  RootWindow* root = Shell::GetPrimaryRootWindow();
  RunAnimationForWidget(
      root->GetProperty(kAnimatingDesktopController)->GetController(false)->
              widget());

  // Change to a new wallpaper.
  controller->CreateEmptyWallpaper();

  DesktopBackgroundWidgetController* animatingController =
      root->GetProperty(kAnimatingDesktopController)->GetController(false);
  EXPECT_TRUE(animatingController);
  EXPECT_TRUE(root->GetProperty(kDesktopController));

  // Change to another wallpaper before animation finished.
  controller->CreateEmptyWallpaper();

  // The animating controller should immediately move to desktop controller.
  EXPECT_EQ(animatingController, root->GetProperty(kDesktopController));

  // Cache the new animating controller.
  animatingController =
      root->GetProperty(kAnimatingDesktopController)->GetController(false);

  // Run wallpaper show animation to completion.
  RunAnimationForWidget(
      root->GetProperty(kAnimatingDesktopController)->GetController(false)->
              widget());

  EXPECT_TRUE(root->GetProperty(kDesktopController));
  EXPECT_FALSE(
      root->GetProperty(kAnimatingDesktopController)->GetController(false));
  // The desktop controller should be the last created animating controller.
  EXPECT_EQ(animatingController, root->GetProperty(kDesktopController));
}

}  // namespace internal
}  // namespace ash
