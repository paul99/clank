// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/panel_layout_manager.h"

#include "ash/ash_switches.h"
#include "ash/launcher/launcher.h"
#include "ash/shell.h"
#include "ash/shell_window_ids.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/launcher_view_test_api.h"
#include "ash/test/test_launcher_delegate.h"
#include "ash/wm/window_util.h"
#include "base/basictypes.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window.h"
#include "ui/views/corewm/corewm_switches.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace internal {

using aura::test::WindowIsAbove;

class PanelLayoutManagerTest : public ash::test::AshTestBase {
 public:
  PanelLayoutManagerTest() {}
  virtual ~PanelLayoutManagerTest() {}

  virtual void SetUp() OVERRIDE {
    ash::test::AshTestBase::SetUp();
    ASSERT_TRUE(ash::test::TestLauncherDelegate::instance());

    launcher_view_test_.reset(new test::LauncherViewTestAPI(
        Launcher::ForPrimaryDisplay()->GetLauncherViewForTest()));
    launcher_view_test_->SetAnimationDuration(1);
  }

  aura::Window* CreateNormalWindow() {
    return CreateTestWindowInShellWithBounds(gfx::Rect());
  }

  aura::Window* CreatePanelWindow(const gfx::Rect& bounds) {
    aura::Window* window = CreateTestWindowInShellWithDelegateAndType(
        NULL,
        aura::client::WINDOW_TYPE_PANEL,
        0,
        bounds);
    ash::test::TestLauncherDelegate* launcher_delegate =
        ash::test::TestLauncherDelegate::instance();
    launcher_delegate->AddLauncherItem(window);
    PanelLayoutManager* manager =
        static_cast<PanelLayoutManager*>(GetPanelContainer()->layout_manager());
    manager->Relayout();
    return window;
  }

  aura::Window* GetPanelContainer() {
    return Shell::GetContainer(
        Shell::GetPrimaryRootWindow(),
        ash::internal::kShellWindowId_PanelContainer);
  }

  void GetCalloutWidget(views::Widget** widget) {
    PanelLayoutManager* manager =
        static_cast<PanelLayoutManager*>(GetPanelContainer()->layout_manager());
    ASSERT_TRUE(manager);
    ASSERT_TRUE(manager->callout_widget());
    *widget = manager->callout_widget();
  }

  void PanelsNotOverlapping(aura::Window* panel1, aura::Window* panel2) {
    // Waits until all launcher view animations are done.
    launcher_view_test()->RunMessageLoopUntilAnimationsDone();
    gfx::Rect window1_bounds = panel1->GetBoundsInRootWindow();
    gfx::Rect window2_bounds = panel2->GetBoundsInRootWindow();

    EXPECT_FALSE(window1_bounds.Intersects(window2_bounds));
  }

  // TODO(dcheng): This should be const, but GetScreenBoundsOfItemIconForWindow
  // takes a non-const Window. We can probably fix that.
  void IsPanelAboveLauncherIcon(aura::Window* panel) {
    // Waits until all launcher view animations are done.
    launcher_view_test()->RunMessageLoopUntilAnimationsDone();

    Launcher* launcher = Launcher::ForPrimaryDisplay();
    gfx::Rect icon_bounds = launcher->GetScreenBoundsOfItemIconForWindow(panel);
    ASSERT_FALSE(icon_bounds.IsEmpty());

    gfx::Rect window_bounds = panel->GetBoundsInRootWindow();

    // The horizontal bounds of the panel window should contain the bounds of
    // the launcher icon.
    EXPECT_LE(window_bounds.x(), icon_bounds.x());
    EXPECT_GE(window_bounds.right(), icon_bounds.right());
    EXPECT_EQ(launcher->widget()->GetWindowBoundsInScreen().y(),
              window_bounds.bottom());
  }

  void IsCalloutAboveLauncherIcon(aura::Window* panel) {
    // Flush the message loop, since callout updates use a delayed task.
    MessageLoop::current()->RunUntilIdle();
    views::Widget* widget = NULL;
    GetCalloutWidget(&widget);

    Launcher* launcher = Launcher::ForPrimaryDisplay();
    gfx::Rect icon_bounds = launcher->GetScreenBoundsOfItemIconForWindow(panel);
    ASSERT_FALSE(icon_bounds.IsEmpty());

    EXPECT_TRUE(widget->IsVisible());
    EXPECT_EQ(panel->GetBoundsInRootWindow().bottom(),
              widget->GetWindowBoundsInScreen().y());
    EXPECT_NEAR(icon_bounds.CenterPoint().x(),
                widget->GetWindowBoundsInScreen().CenterPoint().x(),
                1);
  }

  bool IsCalloutVisible() {
    views::Widget* widget = NULL;
    GetCalloutWidget(&widget);
    return widget->IsVisible();
  }

  test::LauncherViewTestAPI* launcher_view_test() {
    return launcher_view_test_.get();
  }

 private:
  scoped_ptr<test::LauncherViewTestAPI> launcher_view_test_;

  DISALLOW_COPY_AND_ASSIGN(PanelLayoutManagerTest);
};

// Tests that a created panel window is successfully added to the panel
// layout manager.
TEST_F(PanelLayoutManagerTest, AddOnePanel) {
  gfx::Rect bounds(0, 0, 201, 201);
  scoped_ptr<aura::Window> window(CreatePanelWindow(bounds));
  EXPECT_EQ(GetPanelContainer(), window->parent());
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(window.get()));
}

// Tests interactions between multiple panels
TEST_F(PanelLayoutManagerTest, MultiplePanelsAreAboveIcons) {
  gfx::Rect odd_bounds(0, 0, 201, 201);
  gfx::Rect even_bounds(0, 0, 200, 200);

  scoped_ptr<aura::Window> w1(CreatePanelWindow(odd_bounds));
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w1.get()));

  scoped_ptr<aura::Window> w2(CreatePanelWindow(even_bounds));
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w1.get()));
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w2.get()));

  scoped_ptr<aura::Window> w3(CreatePanelWindow(odd_bounds));
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w1.get()));
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w2.get()));
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w3.get()));
}

TEST_F(PanelLayoutManagerTest, MultiplePanelStacking) {
  gfx::Rect bounds(0, 0, 201, 201);
  scoped_ptr<aura::Window> w1(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w2(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w3(CreatePanelWindow(bounds));

  // Default stacking order.
  EXPECT_TRUE(WindowIsAbove(w3.get(), w2.get()));
  EXPECT_TRUE(WindowIsAbove(w2.get(), w1.get()));

  // Changing the active window should update the stacking order.
  wm::ActivateWindow(w1.get());
  launcher_view_test()->RunMessageLoopUntilAnimationsDone();
  EXPECT_TRUE(WindowIsAbove(w1.get(), w2.get()));
  EXPECT_TRUE(WindowIsAbove(w2.get(), w3.get()));

  wm::ActivateWindow(w2.get());
  launcher_view_test()->RunMessageLoopUntilAnimationsDone();
  EXPECT_TRUE(WindowIsAbove(w1.get(), w3.get()));
  EXPECT_TRUE(WindowIsAbove(w2.get(), w3.get()));
  EXPECT_TRUE(WindowIsAbove(w2.get(), w1.get()));

  wm::ActivateWindow(w3.get());
  EXPECT_TRUE(WindowIsAbove(w3.get(), w2.get()));
  EXPECT_TRUE(WindowIsAbove(w2.get(), w1.get()));
}

TEST_F(PanelLayoutManagerTest, MultiplePanelCallout) {
  gfx::Rect bounds(0, 0, 200, 200);
  scoped_ptr<aura::Window> w1(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w2(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w3(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w4(CreateNormalWindow());
  EXPECT_FALSE(IsCalloutVisible());
  wm::ActivateWindow(w1.get());
  EXPECT_NO_FATAL_FAILURE(IsCalloutAboveLauncherIcon(w1.get()));
  wm::ActivateWindow(w2.get());
  EXPECT_NO_FATAL_FAILURE(IsCalloutAboveLauncherIcon(w2.get()));
  wm::ActivateWindow(w3.get());
  EXPECT_NO_FATAL_FAILURE(IsCalloutAboveLauncherIcon(w3.get()));
  wm::ActivateWindow(w4.get());
  EXPECT_FALSE(IsCalloutVisible());
  wm::ActivateWindow(w3.get());
  EXPECT_NO_FATAL_FAILURE(IsCalloutAboveLauncherIcon(w3.get()));
  w3.reset();
  if (views::corewm::UseFocusController())
    EXPECT_NO_FATAL_FAILURE(IsCalloutAboveLauncherIcon(w2.get()));
  else
    EXPECT_FALSE(IsCalloutVisible());
}

// Tests removing panels.
TEST_F(PanelLayoutManagerTest, RemoveLeftPanel) {
  gfx::Rect bounds(0, 0, 201, 201);
  scoped_ptr<aura::Window> w1(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w2(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w3(CreatePanelWindow(bounds));

  // At this point, windows should be stacked with 1 < 2 < 3
  wm::ActivateWindow(w1.get());
  launcher_view_test()->RunMessageLoopUntilAnimationsDone();
  // Now, windows should be stacked 1 > 2 > 3
  w1.reset();
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w2.get()));
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w3.get()));
  EXPECT_TRUE(WindowIsAbove(w2.get(), w3.get()));
}

TEST_F(PanelLayoutManagerTest, RemoveMiddlePanel) {
  gfx::Rect bounds(0, 0, 201, 201);
  scoped_ptr<aura::Window> w1(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w2(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w3(CreatePanelWindow(bounds));

  // At this point, windows should be stacked with 1 < 2 < 3
  wm::ActivateWindow(w2.get());
  // Windows should be stacked 1 < 2 > 3
  w2.reset();
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w1.get()));
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w3.get()));
  EXPECT_TRUE(WindowIsAbove(w3.get(), w1.get()));
}

TEST_F(PanelLayoutManagerTest, RemoveRightPanel) {
  gfx::Rect bounds(0, 0, 201, 201);
  scoped_ptr<aura::Window> w1(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w2(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w3(CreatePanelWindow(bounds));

  // At this point, windows should be stacked with 1 < 2 < 3
  wm::ActivateWindow(w3.get());
  // Order shouldn't change.
  w3.reset();
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w1.get()));
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w2.get()));
  EXPECT_TRUE(WindowIsAbove(w2.get(), w1.get()));
}

TEST_F(PanelLayoutManagerTest, RemoveNonActivePanel) {
  gfx::Rect bounds(0, 0, 201, 201);
  scoped_ptr<aura::Window> w1(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w2(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w3(CreatePanelWindow(bounds));

  // At this point, windows should be stacked with 1 < 2 < 3
  wm::ActivateWindow(w2.get());
  // Windows should be stacked 1 < 2 > 3
  w1.reset();
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w2.get()));
  EXPECT_NO_FATAL_FAILURE(IsPanelAboveLauncherIcon(w3.get()));
  EXPECT_TRUE(WindowIsAbove(w2.get(), w3.get()));
}

TEST_F(PanelLayoutManagerTest, SplitView) {
  gfx::Rect bounds(0, 0, 90, 201);
  scoped_ptr<aura::Window> w1(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w2(CreatePanelWindow(bounds));

  EXPECT_NO_FATAL_FAILURE(PanelsNotOverlapping(w1.get(), w2.get()));
}

TEST_F(PanelLayoutManagerTest, FanWindows) {
  gfx::Rect bounds(0, 0, 201, 201);
  scoped_ptr<aura::Window> w1(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w2(CreatePanelWindow(bounds));
  scoped_ptr<aura::Window> w3(CreatePanelWindow(bounds));

  launcher_view_test()->RunMessageLoopUntilAnimationsDone();
  int window_x1 = w1->GetBoundsInRootWindow().x();
  int window_x2 = w2->GetBoundsInRootWindow().x();
  int window_x3 = w3->GetBoundsInRootWindow().x();
  Launcher* launcher = Launcher::ForPrimaryDisplay();
  int icon_x1 = launcher->GetScreenBoundsOfItemIconForWindow(w1.get()).x();
  int icon_x2 = launcher->GetScreenBoundsOfItemIconForWindow(w2.get()).x();
  EXPECT_EQ(window_x2 - window_x1, window_x3 - window_x2);
  int spacing = window_x2 - window_x1;
  EXPECT_GT(spacing, icon_x2 - icon_x1);
}

TEST_F(PanelLayoutManagerTest, MinimizeRestorePanel) {
  gfx::Rect bounds(0, 0, 201, 201);
  scoped_ptr<aura::Window> window(CreatePanelWindow(bounds));
  // Activate the window, ensure callout is visible.
  wm::ActivateWindow(window.get());
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(IsCalloutVisible());
  // Minimize the panel, callout should be hidden.
  window->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_MINIMIZED);
  RunAllPendingInMessageLoop();
  EXPECT_FALSE(IsCalloutVisible());
  // Restore the pantel; panel should not be activated by default and callout
  // should be hidden.
  window->SetProperty(aura::client::kShowStateKey, ui::SHOW_STATE_NORMAL);
  RunAllPendingInMessageLoop();
  EXPECT_FALSE(IsCalloutVisible());
  // Activate the window, ensure callout is visible.
  wm::ActivateWindow(window.get());
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(IsCalloutVisible());
}

}  // namespace internal
}  // namespace ash
