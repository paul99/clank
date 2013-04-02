// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/display_manager.h"

#include "ash/display/display_controller.h"
#include "ash/screen_ash.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "base/format_macros.h"
#include "base/stringprintf.h"
#include "ui/aura/env.h"
#include "ui/aura/root_window.h"
#include "ui/aura/window_observer.h"
#include "ui/gfx/display_observer.h"
#include "ui/gfx/display.h"

namespace ash {
namespace internal {

using std::vector;
using std::string;

class DisplayManagerTest : public test::AshTestBase,
                           public gfx::DisplayObserver,
                           public aura::WindowObserver {
 public:
  DisplayManagerTest()
      : removed_count_(0U),
        root_window_destroyed_(false) {
  }
  virtual ~DisplayManagerTest() {}

  virtual void SetUp() OVERRIDE {
    AshTestBase::SetUp();
    Shell::GetScreen()->AddObserver(this);
    Shell::GetPrimaryRootWindow()->AddObserver(this);
  }
  virtual void TearDown() OVERRIDE {
    Shell::GetPrimaryRootWindow()->RemoveObserver(this);
    Shell::GetScreen()->RemoveObserver(this);
    AshTestBase::TearDown();
  }

  DisplayManager* display_manager() {
    return Shell::GetInstance()->display_manager();
  }
  const vector<gfx::Display>& changed() const { return changed_; }
  const vector<gfx::Display>& added() const { return added_; }

  string GetCountSummary() const {
    return StringPrintf("%"PRIuS" %"PRIuS" %"PRIuS,
                        changed_.size(), added_.size(), removed_count_);
  }

  void reset() {
    changed_.clear();
    added_.clear();
    removed_count_ = 0U;
    root_window_destroyed_ = false;
  }

  bool root_window_destroyed() const {
    return root_window_destroyed_;
  }

  const gfx::Display& FindDisplayForId(int64 id) {
    return display_manager()->FindDisplayForId(id);
  }

  // aura::DisplayObserver overrides:
  virtual void OnDisplayBoundsChanged(const gfx::Display& display) OVERRIDE {
    changed_.push_back(display);
  }
  virtual void OnDisplayAdded(const gfx::Display& new_display) OVERRIDE {
    added_.push_back(new_display);
  }
  virtual void OnDisplayRemoved(const gfx::Display& old_display) OVERRIDE {
    ++removed_count_;
  }

  // aura::WindowObserver overrides:
  virtual void OnWindowDestroying(aura::Window* window) OVERRIDE {
    ASSERT_EQ(Shell::GetPrimaryRootWindow(), window);
    root_window_destroyed_ = true;
  }

 private:
  vector<gfx::Display> changed_;
  vector<gfx::Display> added_;
  size_t removed_count_;
  bool root_window_destroyed_;

  DISALLOW_COPY_AND_ASSIGN(DisplayManagerTest);
};

TEST_F(DisplayManagerTest, NativeDisplayTest) {
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());

  // Update primary and add seconary.
  UpdateDisplay("100+0-500x500,0+501-400x400");
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0,0 500x500",
            display_manager()->GetDisplayAt(0)->bounds().ToString());

  EXPECT_EQ("1 1 0", GetCountSummary());
  EXPECT_EQ(display_manager()->GetDisplayAt(0)->id(), changed()[0].id());
  EXPECT_EQ(display_manager()->GetDisplayAt(1)->id(), added()[0].id());
  EXPECT_EQ("0,0 500x500", changed()[0].bounds().ToString());
  // Secondary display is on right.
  EXPECT_EQ("500,0 400x400", added()[0].bounds().ToString());
  EXPECT_EQ("0,501 400x400", added()[0].bounds_in_pixel().ToString());
  reset();

  // Delete secondary.
  UpdateDisplay("100+0-500x500");
  EXPECT_EQ("0 0 1", GetCountSummary());
  reset();

  // Change primary.
  UpdateDisplay("1+1-1000x600");
  EXPECT_EQ("1 0 0", GetCountSummary());
  EXPECT_EQ(display_manager()->GetDisplayAt(0)->id(), changed()[0].id());
  EXPECT_EQ("0,0 1000x600", changed()[0].bounds().ToString());
  reset();

  // Add secondary.
  UpdateDisplay("1+1-1000x600,1002+0-600x400");
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0 1 0", GetCountSummary());
  EXPECT_EQ(display_manager()->GetDisplayAt(1)->id(), added()[0].id());
  // Secondary display is on right.
  EXPECT_EQ("1000,0 600x400", added()[0].bounds().ToString());
  EXPECT_EQ("1002,0 600x400", added()[0].bounds_in_pixel().ToString());
  reset();

  // Secondary removed, primary changed.
  UpdateDisplay("1+1-800x300");
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());
  EXPECT_EQ("1 0 1", GetCountSummary());
  EXPECT_EQ(display_manager()->GetDisplayAt(0)->id(), changed()[0].id());
  EXPECT_EQ("0,0 800x300", changed()[0].bounds().ToString());
  reset();

  // # of display can go to zero when screen is off.
  const vector<gfx::Display> empty;
  display_manager()->OnNativeDisplaysChanged(empty);
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0 0 0", GetCountSummary());
  EXPECT_FALSE(root_window_destroyed());
  // Display configuration stays the same
  EXPECT_EQ("0,0 800x300",
            display_manager()->GetDisplayAt(0)->bounds().ToString());
  reset();

  // Connect to display again
  UpdateDisplay("100+100-500x400");
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());
  EXPECT_EQ("1 0 0", GetCountSummary());
  EXPECT_FALSE(root_window_destroyed());
  EXPECT_EQ("0,0 500x400", changed()[0].bounds().ToString());
  EXPECT_EQ("100,100 500x400", changed()[0].bounds_in_pixel().ToString());
  reset();

  // Go back to zero and wake up with multiple displays.
  display_manager()->OnNativeDisplaysChanged(empty);
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());
  EXPECT_FALSE(root_window_destroyed());
  reset();

  // Add secondary.
  UpdateDisplay("0+0-1000x600,1000+1000-600x400");
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0,0 1000x600",
            display_manager()->GetDisplayAt(0)->bounds().ToString());
  // Secondary display is on right.
  EXPECT_EQ("1000,0 600x400",
            display_manager()->GetDisplayAt(1)->bounds().ToString());
  EXPECT_EQ("1000,1000 600x400",
            display_manager()->GetDisplayAt(1)->bounds_in_pixel().ToString());
  reset();
}

// Test in emulation mode (use_fullscreen_host_window=false)
TEST_F(DisplayManagerTest, EmulatorTest) {
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());

  DisplayManager::CycleDisplay();
  // Update primary and add seconary.
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0 1 0", GetCountSummary());
  reset();

  DisplayManager::CycleDisplay();
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0 0 1", GetCountSummary());
  reset();

  DisplayManager::CycleDisplay();
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0 1 0", GetCountSummary());
  reset();
}

TEST_F(DisplayManagerTest, OverscanInsetsTest) {
  UpdateDisplay("0+0-500x500,0+501-400x400");
  reset();
  ASSERT_EQ(2u, display_manager()->GetNumDisplays());
  gfx::Display display1(*display_manager()->GetDisplayAt(0));
  gfx::Display display2(*display_manager()->GetDisplayAt(1));

  display_manager()->SetOverscanInsets(
      display2.id(), gfx::Insets(13, 12, 11, 10));
  std::vector<gfx::Display> changed_displays = changed();
  EXPECT_EQ(1u, changed_displays.size());
  EXPECT_EQ(display2.id(), changed_displays[0].id());
  EXPECT_EQ("0,0 500x500",
            display_manager()->GetDisplayAt(0)->bounds_in_pixel().ToString());
  EXPECT_EQ("12,514 378x376",
            display_manager()->GetDisplayAt(1)->bounds_in_pixel().ToString());

  // Make sure that SetOverscanInsets() is idempotent.
  display_manager()->SetOverscanInsets(display1.id(), gfx::Insets());
  display_manager()->SetOverscanInsets(
      display2.id(), gfx::Insets(13, 12, 11, 10));
  EXPECT_EQ("0,0 500x500",
            display_manager()->GetDisplayAt(0)->bounds_in_pixel().ToString());
  EXPECT_EQ("12,514 378x376",
            display_manager()->GetDisplayAt(1)->bounds_in_pixel().ToString());

  display_manager()->SetOverscanInsets(
      display2.id(), gfx::Insets(10, 11, 12, 13));
  EXPECT_EQ("0,0 500x500",
            display_manager()->GetDisplayAt(0)->bounds_in_pixel().ToString());
  EXPECT_EQ("11,511 376x378",
            display_manager()->GetDisplayAt(1)->bounds_in_pixel().ToString());

  // Recreate a new 2nd display. It won't apply the overscan inset because the
  // new display has a different ID.
  UpdateDisplay("0+0-500x500");
  UpdateDisplay("0+0-500x500,0+501-400x400");
  EXPECT_EQ("0,0 500x500",
            display_manager()->GetDisplayAt(0)->bounds_in_pixel().ToString());
  EXPECT_EQ("0,501 400x400",
            display_manager()->GetDisplayAt(1)->bounds_in_pixel().ToString());

  // Recreate the displays with the same ID.  It should apply the overscan
  // inset.
  UpdateDisplay("0+0-500x500");
  std::vector<gfx::Display> displays;
  displays.push_back(display1);
  displays.push_back(display2);
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ("0,0 500x500",
            display_manager()->GetDisplayAt(0)->bounds_in_pixel().ToString());
  EXPECT_EQ("11,511 376x378",
            display_manager()->GetDisplayAt(1)->bounds_in_pixel().ToString());

  // HiDPI but overscan display. The specified insets size should be doubled.
  UpdateDisplay("0+0-500x500");
  UpdateDisplay("0+0-500x500,0+501-400x400*2");
  display_manager()->SetOverscanInsets(
      display_manager()->GetDisplayAt(1)->id(), gfx::Insets(4, 5, 6, 7));
  EXPECT_EQ("0,0 500x500",
            display_manager()->GetDisplayAt(0)->bounds_in_pixel().ToString());
  EXPECT_EQ("10,509 376x380",
            display_manager()->GetDisplayAt(1)->bounds_in_pixel().ToString());
  EXPECT_EQ("188x190", display_manager()->GetDisplayAt(1)->size().ToString());

  // Make sure switching primary display applies the overscan offset only once.
  ash::Shell::GetInstance()->display_controller()->SetPrimaryDisplay(
      ScreenAsh::GetSecondaryDisplay());
  EXPECT_EQ("0,0 500x500",
            ScreenAsh::GetSecondaryDisplay().bounds_in_pixel().ToString());
  EXPECT_EQ("10,509 376x380", gfx::Screen::GetNativeScreen()->
            GetPrimaryDisplay().bounds_in_pixel().ToString());
}

TEST_F(DisplayManagerTest, ZeroOverscanInsets) {
  // Make sure the display change events is emitted for overscan inset changes.
  UpdateDisplay("0+0-500x500,0+501-400x400");
  ASSERT_EQ(2u, display_manager()->GetNumDisplays());
  int64 display2_id = display_manager()->GetDisplayAt(1)->id();

  reset();
  display_manager()->SetOverscanInsets(display2_id, gfx::Insets(0, 0, 0, 0));
  EXPECT_EQ(0u, changed().size());

  reset();
  display_manager()->SetOverscanInsets(display2_id, gfx::Insets(1, 0, 0, 0));
  EXPECT_EQ(1u, changed().size());
  EXPECT_EQ(display2_id, changed()[0].id());

  reset();
  display_manager()->SetOverscanInsets(display2_id, gfx::Insets(0, 0, 0, 0));
  EXPECT_EQ(1u, changed().size());
  EXPECT_EQ(display2_id, changed()[0].id());
}

TEST_F(DisplayManagerTest, TestDeviceScaleOnlyChange) {
  UpdateDisplay("1000x600");
  EXPECT_EQ(1,
            Shell::GetPrimaryRootWindow()->compositor()->device_scale_factor());
  EXPECT_EQ("1000x600",
            Shell::GetPrimaryRootWindow()->bounds().size().ToString());
  UpdateDisplay("1000x600*2");
  EXPECT_EQ(2,
            Shell::GetPrimaryRootWindow()->compositor()->device_scale_factor());
  EXPECT_EQ("500x300",
            Shell::GetPrimaryRootWindow()->bounds().size().ToString());
}

TEST_F(DisplayManagerTest, TestNativeDisplaysChanged) {
  const int64 internal_display_id =
      display_manager()->SetFirstDisplayAsInternalDisplayForTest();
  const gfx::Display native_display(internal_display_id,
                                    gfx::Rect(0, 0, 500, 500));
  const gfx::Display external_display(10, gfx::Rect(1, 1, 100, 100));

  EXPECT_EQ(1U, display_manager()->GetNumDisplays());
  std::string default_bounds =
      display_manager()->GetDisplayAt(0)->bounds().ToString();

  std::vector<gfx::Display> displays;
  // Primary disconnected.
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());
  EXPECT_EQ(default_bounds,
            display_manager()->GetDisplayAt(0)->bounds().ToString());

  // External connected while primary was disconnected.
  displays.push_back(external_display);
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());
  EXPECT_EQ(default_bounds,
            FindDisplayForId(internal_display_id).bounds().ToString());
  EXPECT_EQ("1,1 100x100",
            FindDisplayForId(10).bounds_in_pixel().ToString());

  // Primary connected, with different bounds.
  displays.clear();
  displays.push_back(native_display);
  displays.push_back(external_display);
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0,0 500x500",
            FindDisplayForId(internal_display_id).bounds().ToString());
  EXPECT_EQ("1,1 100x100",
            FindDisplayForId(10).bounds_in_pixel().ToString());

  // Turn off primary.
  displays.clear();
  displays.push_back(external_display);
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0,0 500x500",
            FindDisplayForId(internal_display_id).bounds().ToString());
  EXPECT_EQ("1,1 100x100",
            FindDisplayForId(10).bounds_in_pixel().ToString());

  // Emulate suspend.
  displays.clear();
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0,0 500x500",
            FindDisplayForId(internal_display_id).bounds().ToString());
  EXPECT_EQ("1,1 100x100",
            FindDisplayForId(10).bounds_in_pixel().ToString());

  // External display has disconnected then resumed.
  displays.push_back(native_display);
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0,0 500x500",
            FindDisplayForId(internal_display_id).bounds().ToString());

  // External display was changed during suspend.
  displays.push_back(external_display);
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());

  // suspend...
  displays.clear();
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());

  // and resume with different external display.
  displays.push_back(native_display);
  displays.push_back(gfx::Display(11, gfx::Rect(1, 1, 100, 100)));
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());
}

#if defined(OS_WIN)
// This test currently fails on Win8/Metro as it picks up the actual
// display size. http://crbug.com/154081
#define MAYBE_TestNativeDisplaysChangedNoInternal \
        DISABLED_TestNativeDisplaysChangedNoInternal
#else
#define MAYBE_TestNativeDisplaysChangedNoInternal \
        TestNativeDisplaysChangedNoInternal
#endif

TEST_F(DisplayManagerTest, MAYBE_TestNativeDisplaysChangedNoInternal) {
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());

  // Don't change the display info if all displays are disconnected.
  std::vector<gfx::Display> displays;
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());

  // Connect another display which will become primary.
  const gfx::Display external_display(10, gfx::Rect(1, 1, 100, 100));
  displays.push_back(external_display);
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ(1U, display_manager()->GetNumDisplays());
  EXPECT_EQ("1,1 100x100",
            FindDisplayForId(10).bounds_in_pixel().ToString());
  EXPECT_EQ("100x100",
            ash::Shell::GetPrimaryRootWindow()->GetHostSize().ToString());
}

TEST_F(DisplayManagerTest, EnsurePointerInDisplays) {
  UpdateDisplay("200x200,300x300");
  Shell::RootWindowList root_windows = Shell::GetAllRootWindows();

  aura::Env* env = aura::Env::GetInstance();

  // Set the initial position.
  root_windows[0]->MoveCursorTo(gfx::Point(350, 150));
  EXPECT_EQ("350,150", env->last_mouse_location().ToString());

  // A mouse pointer will be inside 2nd display.
  UpdateDisplay("300x300,200x200");
  EXPECT_EQ("350,150", env->last_mouse_location().ToString());

  // A mouse pointer will be outside of displays and move to the
  // center of 2nd display.
  UpdateDisplay("300x300,100x100");
  EXPECT_EQ("350,50", env->last_mouse_location().ToString());

  // 2nd display was disconnected, but the mouse pointer says in the
  // 1st display.
  UpdateDisplay("400x400");
  EXPECT_EQ("350,50", env->last_mouse_location().ToString());

  // 1st display's resolution has changed, and the mouse pointer is
  // now outside. Move the mouse pointer to the center of 1st display.
  UpdateDisplay("300x300");
  EXPECT_EQ("150,150", env->last_mouse_location().ToString());

  // Move the mouse pointer to the bottom of 1st display.
  root_windows[0]->MoveCursorTo(gfx::Point(150, 290));
  EXPECT_EQ("150,290", env->last_mouse_location().ToString());

  // The mouse pointer is outside and closest display is 1st one.
  UpdateDisplay("300x280,200x200");
  EXPECT_EQ("150,140", env->last_mouse_location().ToString());
}

TEST_F(DisplayManagerTest, EnsurePointerInDisplays_2ndOnLeft) {
  UpdateDisplay("200x200,300x300");
  Shell::RootWindowList root_windows = Shell::GetAllRootWindows();

  // Set the 2nd display on the left.
  DisplayController* display_controller =
      Shell::GetInstance()->display_controller();
  DisplayLayout layout = display_controller->default_display_layout();
  layout.position = DisplayLayout::LEFT;
  display_controller->SetDefaultDisplayLayout(layout);

  EXPECT_EQ("-300,0 300x300",
            ScreenAsh::GetSecondaryDisplay().bounds().ToString());

  aura::Env* env = aura::Env::GetInstance();

  // Set the initial position.
  root_windows[0]->MoveCursorTo(gfx::Point(-150, 150));
  EXPECT_EQ("-150,150", env->last_mouse_location().ToString());

  // A mouse pointer will be in 2nd display.
  UpdateDisplay("300x300,200x200");
  EXPECT_EQ("-150,150", env->last_mouse_location().ToString());

  // A mouse pointer will be outside of displays and move to the
  // center of 2nd display.
  UpdateDisplay("300x300,200x100");
  EXPECT_EQ("-100,50", env->last_mouse_location().ToString());

  // 2nd display was disconnected. Mouse pointer should move to
  // 1st display.
  UpdateDisplay("300x300");
  EXPECT_EQ("150,150", env->last_mouse_location().ToString());
}

TEST_F(DisplayManagerTest, NativeDisplaysChangedAfterPrimaryChange) {
  const int64 internal_display_id =
      display_manager()->SetFirstDisplayAsInternalDisplayForTest();
  const gfx::Display native_display(internal_display_id,
                                    gfx::Rect(0, 0, 500, 500));
  const gfx::Display secondary_display(10, gfx::Rect(1, 1, 100, 100));

  std::vector<gfx::Display> displays;
  displays.push_back(native_display);
  displays.push_back(secondary_display);
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ(2U, display_manager()->GetNumDisplays());
  EXPECT_EQ("0,0 500x500",
            FindDisplayForId(internal_display_id).bounds().ToString());
  EXPECT_EQ("500,0 100x100", FindDisplayForId(10).bounds().ToString());

  ash::Shell::GetInstance()->display_controller()->SetPrimaryDisplay(
      secondary_display);
  EXPECT_EQ("-500,0 500x500",
            FindDisplayForId(internal_display_id).bounds().ToString());
  EXPECT_EQ("0,0 100x100", FindDisplayForId(10).bounds().ToString());

  // OnNativeDisplaysChanged may change the display bounds.  Here makes sure
  // nothing changed if the exactly same displays are specified.
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ("-500,0 500x500",
            FindDisplayForId(internal_display_id).bounds().ToString());
  EXPECT_EQ("0,0 100x100", FindDisplayForId(10).bounds().ToString());
}

TEST_F(DisplayManagerTest, AutomaticOverscanInsets) {
  UpdateDisplay("200x200,400x400");

  std::vector<gfx::Display> displays;
  displays.push_back(*display_manager()->GetDisplayAt(0));
  displays.push_back(*display_manager()->GetDisplayAt(1));
  int64 id = displays[1].id();
  display_manager()->SetHasOverscanFlagForTest(id, true);

  display_manager()->OnNativeDisplaysChanged(displays);
  // It has overscan insets, although SetOverscanInsets() isn't called.
  EXPECT_EQ("11,211 380x380",
            display_manager()->GetDisplayAt(1)->bounds_in_pixel().ToString());

  // If custom overscan insets is specified, the specified value is used.
  display_manager()->SetOverscanInsets(id, gfx::Insets(5, 6, 7, 8));
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ("7,206 386x388",
            display_manager()->GetDisplayAt(1)->bounds_in_pixel().ToString());

  // Do not overscan even though it has 'has_overscan' flag, if the custom
  // insets is empty.
  display_manager()->SetOverscanInsets(id, gfx::Insets());
  display_manager()->OnNativeDisplaysChanged(displays);
  EXPECT_EQ("1,201 400x400",
            display_manager()->GetDisplayAt(1)->bounds_in_pixel().ToString());
}

}  // namespace internal
}  // namespace ash
