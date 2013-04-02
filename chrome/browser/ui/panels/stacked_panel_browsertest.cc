// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/panels/base_panel_browser_test.h"
#include "chrome/browser/ui/panels/detached_panel_collection.h"
#include "chrome/browser/ui/panels/docked_panel_collection.h"
#include "chrome/browser/ui/panels/native_panel.h"
#include "chrome/browser/ui/panels/panel.h"
#include "chrome/browser/ui/panels/panel_manager.h"
#include "chrome/browser/ui/panels/stacked_panel_collection.h"
#include "content/public/test/test_utils.h"

class StackedPanelBrowserTest : public BasePanelBrowserTest {
};

// TODO(jianli): to be enabled for other platforms when grouping and snapping
// are supported.
#if defined(OS_WIN)

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest, CheckStackedPanelProperties) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create 2 stacked panels.
  StackedPanelCollection* stack = panel_manager->CreateStack();
  gfx::Rect panel1_initial_bounds = gfx::Rect(100, 50, 200, 150);
  Panel* panel1 = CreateStackedPanel("1", panel1_initial_bounds, stack);
  gfx::Rect panel2_initial_bounds = gfx::Rect(0, 0, 150, 100);
  Panel* panel2 = CreateStackedPanel("2", panel2_initial_bounds, stack);

  scoped_ptr<NativePanelTesting> panel1_testing(
      CreateNativePanelTesting(panel1));
  scoped_ptr<NativePanelTesting> panel2_testing(
      CreateNativePanelTesting(panel2));

  // Check that all 2 panels are in a stack.
  ASSERT_EQ(0, panel_manager->docked_collection()->num_panels());
  ASSERT_EQ(0, panel_manager->detached_collection()->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(2, stack->num_panels());
  EXPECT_EQ(stack, panel1->stack());
  EXPECT_EQ(stack, panel2->stack());

  // Check buttons.
  EXPECT_TRUE(panel1_testing->IsButtonVisible(panel::CLOSE_BUTTON));
  EXPECT_TRUE(panel2_testing->IsButtonVisible(panel::CLOSE_BUTTON));

  EXPECT_TRUE(panel1_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));
  EXPECT_FALSE(panel2_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));

  EXPECT_FALSE(panel1_testing->IsButtonVisible(panel::RESTORE_BUTTON));
  EXPECT_FALSE(panel2_testing->IsButtonVisible(panel::RESTORE_BUTTON));

  // Check bounds.
  gfx::Rect panel1_expected_bounds(panel1_initial_bounds);
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  gfx::Rect panel2_expected_bounds(panel1_expected_bounds.x(),
                                   panel1_expected_bounds.bottom(),
                                   panel1_expected_bounds.width(),
                                   panel2_initial_bounds.height());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());

  // Check other properties.
  EXPECT_FALSE(panel1->IsAlwaysOnTop());
  EXPECT_FALSE(panel2->IsAlwaysOnTop());

  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_FALSE(panel2->IsMinimized());

  EXPECT_EQ(panel::RESIZABLE_ALL_SIDES, panel1->CanResizeByMouse());
  EXPECT_EQ(panel::RESIZABLE_ALL_SIDES, panel2->CanResizeByMouse());

  EXPECT_EQ(panel::TOP_ROUNDED, panel1_testing->GetWindowCornerStyle());
  EXPECT_EQ(panel::BOTTOM_ROUNDED, panel2_testing->GetWindowCornerStyle());

  Panel::AttentionMode expected_attention_mode =
      static_cast<Panel::AttentionMode>(Panel::USE_PANEL_ATTENTION |
                                        Panel::USE_SYSTEM_ATTENTION);
  EXPECT_EQ(expected_attention_mode, panel1->attention_mode());
  EXPECT_EQ(expected_attention_mode, panel2->attention_mode());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest,
                       CheckMinimizedStackedPanelProperties) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create 2 stacked panels.
  StackedPanelCollection* stack = panel_manager->CreateStack();
  gfx::Rect panel1_initial_bounds = gfx::Rect(100, 50, 200, 150);
  Panel* panel1 = CreateStackedPanel("1", panel1_initial_bounds, stack);
  gfx::Rect panel2_initial_bounds = gfx::Rect(0, 0, 150, 100);
  Panel* panel2 = CreateStackedPanel("2", panel2_initial_bounds, stack);

  scoped_ptr<NativePanelTesting> panel1_testing(
      CreateNativePanelTesting(panel1));
  scoped_ptr<NativePanelTesting> panel2_testing(
      CreateNativePanelTesting(panel2));

  // Minimize these 2 panels.
  panel1->Minimize();
  WaitForBoundsAnimationFinished(panel1);
  panel2->Minimize();
  WaitForBoundsAnimationFinished(panel2);

  // Check that all 2 panels are in a stack.
  ASSERT_EQ(0, panel_manager->docked_collection()->num_panels());
  ASSERT_EQ(0, panel_manager->detached_collection()->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(2, stack->num_panels());
  EXPECT_EQ(stack, panel1->stack());
  EXPECT_EQ(stack, panel2->stack());

  // Check buttons.
  EXPECT_TRUE(panel1_testing->IsButtonVisible(panel::CLOSE_BUTTON));
  EXPECT_TRUE(panel2_testing->IsButtonVisible(panel::CLOSE_BUTTON));

  EXPECT_TRUE(panel1_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));
  EXPECT_FALSE(panel2_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));

  EXPECT_FALSE(panel1_testing->IsButtonVisible(panel::RESTORE_BUTTON));
  EXPECT_FALSE(panel2_testing->IsButtonVisible(panel::RESTORE_BUTTON));

  // Check bounds.
  gfx::Rect panel1_expected_bounds(panel1_initial_bounds);
  panel1_expected_bounds.set_height(panel1->TitleOnlyHeight());
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  gfx::Rect panel2_expected_bounds(panel1_expected_bounds.x(),
                                   panel1_expected_bounds.bottom(),
                                   panel1_expected_bounds.width(),
                                   panel2->TitleOnlyHeight());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());

  // Check other properties.
  EXPECT_FALSE(panel1->IsAlwaysOnTop());
  EXPECT_FALSE(panel2->IsAlwaysOnTop());

  EXPECT_TRUE(panel1->IsMinimized());
  EXPECT_TRUE(panel2->IsMinimized());

  EXPECT_EQ(panel::NOT_RESIZABLE, panel1->CanResizeByMouse());
  EXPECT_EQ(panel::NOT_RESIZABLE, panel2->CanResizeByMouse());

  EXPECT_EQ(panel::TOP_ROUNDED, panel1_testing->GetWindowCornerStyle());
  EXPECT_EQ(panel::BOTTOM_ROUNDED, panel2_testing->GetWindowCornerStyle());

  Panel::AttentionMode expected_attention_mode =
      static_cast<Panel::AttentionMode>(Panel::USE_PANEL_ATTENTION |
                                        Panel::USE_SYSTEM_ATTENTION);
  EXPECT_EQ(expected_attention_mode, panel1->attention_mode());
  EXPECT_EQ(expected_attention_mode, panel2->attention_mode());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest, ClickTitlebar) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create 2 stacked panels.
  StackedPanelCollection* stack = panel_manager->CreateStack();
  gfx::Rect panel1_initial_bounds = gfx::Rect(100, 50, 200, 150);
  Panel* panel1 = CreateStackedPanel("1", panel1_initial_bounds, stack);
  gfx::Rect panel2_initial_bounds = gfx::Rect(0, 0, 150, 100);
  Panel* panel2 = CreateStackedPanel("2", panel2_initial_bounds, stack);
  ASSERT_EQ(1, panel_manager->num_stacks());

  scoped_ptr<NativePanelTesting> panel1_testing(
      CreateNativePanelTesting(panel1));
  scoped_ptr<NativePanelTesting> panel2_testing(
      CreateNativePanelTesting(panel2));

  gfx::Point panel1_origin = panel2->GetBounds().origin();
  gfx::Point panel2_origin = panel2->GetBounds().origin();

  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_FALSE(panel2->IsMinimized());

  gfx::Rect panel1_expected_bounds(panel1_initial_bounds);
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  gfx::Rect panel2_expected_bounds(panel1_expected_bounds.x(),
                                   panel1_expected_bounds.bottom(),
                                   panel1_expected_bounds.width(),
                                   panel2_initial_bounds.height());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());

  // Clicking on P2's titlebar to collapse it.
  panel2_testing->PressLeftMouseButtonTitlebar(panel2_origin);
  panel2_testing->ReleaseMouseButtonTitlebar();
  WaitForBoundsAnimationFinished(panel2);
  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_TRUE(panel2->IsMinimized());
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  panel2_expected_bounds.set_height(panel2->TitleOnlyHeight());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());

  // Clicking on P2's titlebar to expand it.
  panel2_testing->PressLeftMouseButtonTitlebar(panel2_origin);
  panel2_testing->ReleaseMouseButtonTitlebar();
  WaitForBoundsAnimationFinished(panel2);
  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_FALSE(panel2->IsMinimized());
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  panel2_expected_bounds.set_height(panel2_initial_bounds.height());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());

  // Clicking on P2's titlebar with APPLY_TO_ALL modifier to collapse all
  // panels.
  panel2_testing->PressLeftMouseButtonTitlebar(panel2_origin,
                                               panel::APPLY_TO_ALL);
  panel2_testing->ReleaseMouseButtonTitlebar(panel::APPLY_TO_ALL);
  WaitForBoundsAnimationFinished(panel1);
  WaitForBoundsAnimationFinished(panel2);
  EXPECT_TRUE(panel1->IsMinimized());
  EXPECT_TRUE(panel2->IsMinimized());
  panel1_expected_bounds.set_height(panel1->TitleOnlyHeight());
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  panel2_expected_bounds.set_y(panel1_expected_bounds.bottom());
  panel2_expected_bounds.set_height(panel2->TitleOnlyHeight());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());

  // Clicking on P1's titlebar with APPLY_TO_ALL modifier to expand all
  // panels.
  panel1_testing->PressLeftMouseButtonTitlebar(panel1_origin,
                                               panel::APPLY_TO_ALL);
  panel1_testing->ReleaseMouseButtonTitlebar(panel::APPLY_TO_ALL);
  WaitForBoundsAnimationFinished(panel1);
  WaitForBoundsAnimationFinished(panel2);
  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_FALSE(panel2->IsMinimized());
  panel1_expected_bounds.set_height(panel1_initial_bounds.height());
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  panel2_expected_bounds.set_y(panel1_expected_bounds.bottom());
  panel2_expected_bounds.set_height(panel2_initial_bounds.height());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest, CallMinimizeAndRestoreApi) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create 2 stacked panels.
  StackedPanelCollection* stack = panel_manager->CreateStack();
  gfx::Rect panel1_initial_bounds = gfx::Rect(100, 50, 200, 150);
  Panel* panel1 = CreateStackedPanel("1", panel1_initial_bounds, stack);
  gfx::Rect panel2_initial_bounds = gfx::Rect(0, 0, 150, 100);
  Panel* panel2 = CreateStackedPanel("2", panel2_initial_bounds, stack);
  ASSERT_EQ(1, panel_manager->num_stacks());

  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_FALSE(panel2->IsMinimized());

  gfx::Rect panel1_expected_bounds(panel1_initial_bounds);
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  gfx::Rect panel2_expected_bounds(panel1_expected_bounds.x(),
                                   panel1_expected_bounds.bottom(),
                                   panel1_expected_bounds.width(),
                                   panel2_initial_bounds.height());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());

  // Collapsing P1 by calling its Minimize API.
  panel1->Minimize();
  WaitForBoundsAnimationFinished(panel1);
  EXPECT_TRUE(panel1->IsMinimized());
  EXPECT_FALSE(panel2->IsMinimized());
  panel1_expected_bounds.set_height(panel1->TitleOnlyHeight());
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  panel2_expected_bounds.set_y(panel1_expected_bounds.bottom());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());

  // Expanding P1 by calling its Restore API.
  panel1->Restore();
  WaitForBoundsAnimationFinished(panel1);
  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_FALSE(panel2->IsMinimized());
  panel1_expected_bounds.set_height(panel1_initial_bounds.height());
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  panel2_expected_bounds.set_y(panel1_expected_bounds.bottom());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest, MinimizeButtonVisibility) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create 3 stacked panels.
  StackedPanelCollection* stack = panel_manager->CreateStack();
  gfx::Rect panel1_initial_bounds = gfx::Rect(100, 50, 200, 150);
  Panel* panel1 = CreateStackedPanel("1", panel1_initial_bounds, stack);
  gfx::Rect panel2_initial_bounds = gfx::Rect(0, 0, 150, 100);
  Panel* panel2 = CreateStackedPanel("2", panel2_initial_bounds, stack);
  gfx::Rect panel3_initial_bounds = gfx::Rect(0, 0, 250, 120);
  Panel* panel3 = CreateStackedPanel("3", panel3_initial_bounds, stack);
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(3, stack->num_panels());

  scoped_ptr<NativePanelTesting> panel1_testing(
      CreateNativePanelTesting(panel1));
  scoped_ptr<NativePanelTesting> panel2_testing(
      CreateNativePanelTesting(panel2));
  scoped_ptr<NativePanelTesting> panel3_testing(
      CreateNativePanelTesting(panel3));

  // Only P1 shows minimize button.
  EXPECT_TRUE(panel1_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));
  EXPECT_FALSE(panel2_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));
  EXPECT_FALSE(panel3_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));

  // Drag P2 away to unstack from P1.
  // Expect only P2, top panel of the stack consisting P2 and P3, shows minimize
  // button.
  gfx::Point mouse_location(panel2->GetBounds().origin());
  panel2_testing->PressLeftMouseButtonTitlebar(mouse_location);
  gfx::Vector2d drag_delta_to_unstack(120, 50);
  panel2_testing->DragTitlebar(mouse_location + drag_delta_to_unstack);
  panel2_testing->FinishDragTitlebar();
  ASSERT_EQ(1, panel_manager->detached_collection()->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(2, stack->num_panels());

  EXPECT_FALSE(panel1_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));
  EXPECT_TRUE(panel2_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));
  EXPECT_FALSE(panel3_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));

  // Drag P1 to stack to the top edge of P2.
  // Expect only P1, top panel of the stack consisting P1, P2 and P3, shows
  // minimize button.
  gfx::Rect bounds1 = panel1->GetBounds();
  gfx::Rect bounds2 = panel2->GetBounds();
  mouse_location = bounds1.origin();
  panel1_testing->PressLeftMouseButtonTitlebar(mouse_location);
  gfx::Vector2d drag_delta_to_stack(bounds2.x() - bounds1.x(),
                                    bounds2.y() - bounds1.bottom());
  panel1_testing->DragTitlebar(mouse_location + drag_delta_to_stack);
  panel1_testing->FinishDragTitlebar();
  ASSERT_EQ(0, panel_manager->detached_collection()->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(3, stack->num_panels());

  EXPECT_TRUE(panel1_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));
  EXPECT_FALSE(panel2_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));
  EXPECT_FALSE(panel3_testing->IsButtonVisible(panel::MINIMIZE_BUTTON));

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest, DISABLED_ClickMinimizeButton) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create 2 stacked panels.
  StackedPanelCollection* stack = panel_manager->CreateStack();
  gfx::Rect panel1_initial_bounds = gfx::Rect(100, 50, 200, 150);
  Panel* panel1 = CreateStackedPanel("1", panel1_initial_bounds, stack);
  gfx::Rect panel2_initial_bounds = gfx::Rect(0, 0, 150, 100);
  Panel* panel2 = CreateStackedPanel("2", panel2_initial_bounds, stack);
  ASSERT_EQ(1, panel_manager->num_stacks());

  scoped_ptr<NativePanelTesting> panel1_testing(
      CreateNativePanelTesting(panel1));

  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_FALSE(panel2->IsMinimized());

  // Collapsing P1 by calling its Minimize API.
  panel1->OnMinimizeButtonClicked(panel::NO_MODIFIER);
  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_FALSE(panel2->IsMinimized());
  EXPECT_TRUE(panel1_testing->VerifySystemMinimizeState());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest, UngroupMinimizedPanels) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create 3 stacked panels.
  StackedPanelCollection* stack = panel_manager->CreateStack();
  gfx::Rect panel1_initial_bounds = gfx::Rect(100, 50, 200, 150);
  Panel* panel1 = CreateStackedPanel("1", panel1_initial_bounds, stack);
  gfx::Rect panel2_initial_bounds = gfx::Rect(0, 0, 150, 100);
  Panel* panel2 = CreateStackedPanel("2", panel2_initial_bounds, stack);
  gfx::Rect panel3_initial_bounds = gfx::Rect(0, 0, 250, 120);
  Panel* panel3 = CreateStackedPanel("3", panel3_initial_bounds, stack);
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(3, stack->num_panels());

  scoped_ptr<NativePanelTesting> panel2_testing(
      CreateNativePanelTesting(panel2));
  scoped_ptr<NativePanelTesting> panel3_testing(
      CreateNativePanelTesting(panel3));

  // Minimize these 3 panels.
  panel1->Minimize();
  WaitForBoundsAnimationFinished(panel1);
  panel2->Minimize();
  WaitForBoundsAnimationFinished(panel3);
  panel3->Minimize();
  WaitForBoundsAnimationFinished(panel3);

  EXPECT_TRUE(panel1->IsMinimized());
  EXPECT_TRUE(panel2->IsMinimized());
  EXPECT_TRUE(panel3->IsMinimized());

  gfx::Rect panel1_expected_bounds(panel1_initial_bounds);
  panel1_expected_bounds.set_height(panel1->TitleOnlyHeight());
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  gfx::Rect panel2_expected_bounds(panel1_expected_bounds.x(),
                                   panel1_expected_bounds.bottom(),
                                   panel1_expected_bounds.width(),
                                   panel2->TitleOnlyHeight());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());
  gfx::Rect panel3_expected_bounds(panel2_expected_bounds.x(),
                                   panel2_expected_bounds.bottom(),
                                   panel2_expected_bounds.width(),
                                   panel3->TitleOnlyHeight());
  EXPECT_EQ(panel3_expected_bounds, panel3->GetBounds());

  // Drag P2 away to unstack from P1.
  // Expect P2 and P3 are still stacked and minimized while P1 becomes detached
  // and expanded. The minimize button of P2 should become visible now.
  gfx::Point mouse_location(panel2->GetBounds().origin());
  panel2_testing->PressLeftMouseButtonTitlebar(mouse_location);
  gfx::Vector2d drag_delta(120, 50);
  panel2_testing->DragTitlebar(mouse_location + drag_delta);
  panel2_testing->FinishDragTitlebar();
  WaitForBoundsAnimationFinished(panel1);
  ASSERT_EQ(1, panel_manager->detached_collection()->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(2, stack->num_panels());

  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_TRUE(panel2->IsMinimized());
  EXPECT_TRUE(panel3->IsMinimized());

  panel1_expected_bounds.set_height(panel1_initial_bounds.height());
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  panel2_expected_bounds.Offset(drag_delta);
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());
  panel3_expected_bounds.Offset(drag_delta);
  EXPECT_EQ(panel3_expected_bounds, panel3->GetBounds());

  // Drag P3 away to unstack from P2.
  // Expect both panels become detached and expanded.
  mouse_location = panel3->GetBounds().origin();
  panel3_testing->PressLeftMouseButtonTitlebar(mouse_location);
  panel3_testing->DragTitlebar(mouse_location + drag_delta);
  panel3_testing->FinishDragTitlebar();
  WaitForBoundsAnimationFinished(panel2);
  WaitForBoundsAnimationFinished(panel3);
  ASSERT_EQ(3, panel_manager->detached_collection()->num_panels());
  ASSERT_EQ(0, panel_manager->num_stacks());

  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_FALSE(panel2->IsMinimized());
  EXPECT_FALSE(panel3->IsMinimized());

  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  panel2_expected_bounds.set_height(panel2_initial_bounds.height());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());
  panel3_expected_bounds.Offset(drag_delta);
  panel3_expected_bounds.set_height(panel3_initial_bounds.height());
  EXPECT_EQ(panel3_expected_bounds, panel3->GetBounds());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest,
                       AddNewPanelToStackWithMostPanels) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create one stack with 2 panels.
  StackedPanelCollection* stack1 = panel_manager->CreateStack();
  Panel* panel1 = CreateStackedPanel("1", gfx::Rect(200, 50, 200, 150), stack1);
  Panel* panel2 = CreateStackedPanel("2", gfx::Rect(0, 0, 150, 100), stack1);
  ASSERT_EQ(2, panel_manager->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(2, stack1->num_panels());

  // Create another stack with 3 panels.
  StackedPanelCollection* stack2 = panel_manager->CreateStack();
  Panel* panel3 = CreateStackedPanel("3", gfx::Rect(100, 50, 200, 150), stack2);
  Panel* panel4 = CreateStackedPanel("4", gfx::Rect(0, 0, 150, 100), stack2);
  Panel* panel5 = CreateStackedPanel("5", gfx::Rect(0, 0, 250, 120), stack2);
  ASSERT_EQ(5, panel_manager->num_panels());
  ASSERT_EQ(2, panel_manager->num_stacks());
  ASSERT_EQ(3, stack2->num_panels());

  // Create new panel. Expect that it will append to stack2 since it has most
  // panels.
  CreatePanelParams params("N", gfx::Rect(50, 50, 150, 100), SHOW_AS_INACTIVE);
  params.create_mode = PanelManager::CREATE_AS_DETACHED;
  Panel* new_panel = CreatePanelWithParams(params);
  EXPECT_EQ(6, panel_manager->num_panels());
  EXPECT_EQ(2, panel_manager->num_stacks());
  EXPECT_EQ(2, stack1->num_panels());
  EXPECT_EQ(4, stack2->num_panels());
  EXPECT_TRUE(stack2->HasPanel(new_panel));
  EXPECT_TRUE(new_panel == stack2->bottom_panel());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest,
                       AddNewPanelToRightMostStack) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create one stack with 2 panels.
  StackedPanelCollection* stack1 = panel_manager->CreateStack();
  Panel* panel1 = CreateStackedPanel("1", gfx::Rect(100, 50, 200, 150), stack1);
  Panel* panel2 = CreateStackedPanel("2", gfx::Rect(0, 0, 150, 100), stack1);
  ASSERT_EQ(2, panel_manager->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(2, stack1->num_panels());

  // Create another stack with 2 panels.
  StackedPanelCollection* stack2 = panel_manager->CreateStack();
  Panel* panel3 = CreateStackedPanel("3", gfx::Rect(200, 50, 200, 150), stack2);
  Panel* panel4 = CreateStackedPanel("4", gfx::Rect(0, 0, 150, 100), stack2);
  ASSERT_EQ(4, panel_manager->num_panels());
  ASSERT_EQ(2, panel_manager->num_stacks());
  ASSERT_EQ(2, stack2->num_panels());

  // Create new panel. Both stack1 and stack2 have same number of panels. Since
  // stack2 is right-most, new panel will be added to it.
  CreatePanelParams params("N", gfx::Rect(50, 50, 150, 100), SHOW_AS_INACTIVE);
  params.create_mode = PanelManager::CREATE_AS_DETACHED;
  Panel* new_panel = CreatePanelWithParams(params);
  EXPECT_EQ(5, panel_manager->num_panels());
  EXPECT_EQ(2, panel_manager->num_stacks());
  EXPECT_EQ(2, stack1->num_panels());
  EXPECT_EQ(3, stack2->num_panels());
  EXPECT_TRUE(stack2->HasPanel(new_panel));
  EXPECT_TRUE(new_panel == stack2->bottom_panel());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest,
                       AddNewPanelToTopMostStack) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create one stack with 2 panels.
  StackedPanelCollection* stack1 = panel_manager->CreateStack();
  Panel* panel1 = CreateStackedPanel("1", gfx::Rect(100, 90, 200, 150), stack1);
  Panel* panel2 = CreateStackedPanel("2", gfx::Rect(0, 0, 150, 100), stack1);
  ASSERT_EQ(2, panel_manager->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(2, stack1->num_panels());

  // Create another stack with 2 panels.
  StackedPanelCollection* stack2 = panel_manager->CreateStack();
  Panel* panel3 = CreateStackedPanel("3", gfx::Rect(100, 50, 200, 150), stack2);
  Panel* panel4 = CreateStackedPanel("4", gfx::Rect(0, 0, 150, 100), stack2);
  ASSERT_EQ(4, panel_manager->num_panels());
  ASSERT_EQ(2, panel_manager->num_stacks());
  ASSERT_EQ(2, stack2->num_panels());

  // Create new panel. Both stack1 and stack2 have same number of panels and
  // same right position. Since stack2 is top-most, new panel will be added to
  // it.
  CreatePanelParams params("N", gfx::Rect(50, 50, 150, 100), SHOW_AS_INACTIVE);
  params.create_mode = PanelManager::CREATE_AS_DETACHED;
  Panel* new_panel = CreatePanelWithParams(params);
  EXPECT_EQ(5, panel_manager->num_panels());
  EXPECT_EQ(2, panel_manager->num_stacks());
  EXPECT_EQ(2, stack1->num_panels());
  EXPECT_EQ(3, stack2->num_panels());
  EXPECT_TRUE(stack2->HasPanel(new_panel));
  EXPECT_TRUE(new_panel == stack2->bottom_panel());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest,
                       AddNewPanelToGroupWithRightMostDetachedPanel) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create 2 detached panels.
  Panel* panel1 = CreateDetachedPanel("1", gfx::Rect(200, 50, 250, 150));
  Panel* panel2 = CreateDetachedPanel("2", gfx::Rect(250, 100, 150, 100));
  ASSERT_EQ(2, panel_manager->num_panels());
  ASSERT_EQ(0, panel_manager->num_stacks());
  ASSERT_EQ(2, panel_manager->detached_collection()->num_panels());

  // Create new panel. Expect that new panel will stack to the bottom of panel2
  // since it is right-most.
  CreatePanelParams params("N", gfx::Rect(50, 50, 150, 100), SHOW_AS_INACTIVE);
  params.create_mode = PanelManager::CREATE_AS_DETACHED;
  Panel* new_panel = CreatePanelWithParams(params);
  EXPECT_EQ(3, panel_manager->num_panels());
  EXPECT_EQ(1, panel_manager->detached_collection()->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  StackedPanelCollection* stack = panel_manager->stacks().front();
  EXPECT_EQ(2, stack->num_panels());
  EXPECT_TRUE(panel2 == stack->top_panel());
  EXPECT_TRUE(new_panel == stack->bottom_panel());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest,
                       AddNewPanelToGroupWitTopMostDetachedPanel) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create 2 detached panels.
  Panel* panel1 = CreateDetachedPanel("1", gfx::Rect(200, 100, 100, 150));
  Panel* panel2 = CreateDetachedPanel("2", gfx::Rect(200, 50, 100, 100));
  ASSERT_EQ(2, panel_manager->num_panels());
  ASSERT_EQ(0, panel_manager->num_stacks());
  ASSERT_EQ(2, panel_manager->detached_collection()->num_panels());

  // Create new panel. Expect that new panel will stack to the bottom of panel2
  // since it is top-most.
  CreatePanelParams params("N", gfx::Rect(50, 50, 150, 100), SHOW_AS_INACTIVE);
  params.create_mode = PanelManager::CREATE_AS_DETACHED;
  Panel* new_panel = CreatePanelWithParams(params);
  EXPECT_EQ(3, panel_manager->num_panels());
  EXPECT_EQ(1, panel_manager->detached_collection()->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  StackedPanelCollection* stack = panel_manager->stacks().front();
  EXPECT_EQ(2, stack->num_panels());
  EXPECT_TRUE(panel2 == stack->top_panel());
  EXPECT_TRUE(new_panel == stack->bottom_panel());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest,
                       AddNewPanelToStackWithCollapseToFit) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create one stack with 3 panels.
  StackedPanelCollection* stack = panel_manager->CreateStack();
  Panel* panel1 = CreateStackedPanel("1", gfx::Rect(100, 50, 200, 120), stack);
  Panel* panel2 = CreateStackedPanel("2", gfx::Rect(0, 0, 150, 150), stack);
  Panel* panel3 = CreateStackedPanel("3", gfx::Rect(0, 0, 180, 120), stack);
  Panel* panel4 = CreateStackedPanel("4", gfx::Rect(0, 0, 170, 130), stack);
  ASSERT_EQ(4, panel_manager->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(4, stack->num_panels());

  // Activate panel1. The panels from most recent active to least recent active
  // are: P1 P4 P3 P2
  panel1->Activate();
  WaitForPanelActiveState(panel1, SHOW_AS_ACTIVE);

  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_FALSE(panel2->IsMinimized());
  EXPECT_FALSE(panel3->IsMinimized());
  EXPECT_FALSE(panel4->IsMinimized());

  // Create a panel. Expect the least recent active panel P2 gets minimized such
  // that there is enough space for new panel to append to the stack.
  CreatePanelParams params1("M", gfx::Rect(50, 50, 300, 110), SHOW_AS_INACTIVE);
  params1.create_mode = PanelManager::CREATE_AS_DETACHED;
  Panel* new_panel1 = CreatePanelWithParams(params1);
  EXPECT_EQ(5, panel_manager->num_panels());
  EXPECT_EQ(1, panel_manager->num_stacks());
  EXPECT_EQ(5, stack->num_panels());
  EXPECT_TRUE(new_panel1 == stack->bottom_panel());
  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_TRUE(panel2->IsMinimized());
  EXPECT_FALSE(panel3->IsMinimized());
  EXPECT_FALSE(panel4->IsMinimized());
  EXPECT_FALSE(new_panel1->IsMinimized());

  // Activate new_panel1. The panels from most recent active to least recent
  // active are: PM P1 P4 P3 P2*
  new_panel1->Activate();
  WaitForPanelActiveState(new_panel1, SHOW_AS_ACTIVE);

  // Create another panel. Expect P3 and P4 are minimized such that there is
  // enoush space for new panel to append to the stack.
  CreatePanelParams params2("N", gfx::Rect(50, 50, 300, 180), SHOW_AS_INACTIVE);
  params2.create_mode = PanelManager::CREATE_AS_DETACHED;
  Panel* new_panel2 = CreatePanelWithParams(params2);
  EXPECT_EQ(6, panel_manager->num_panels());
  EXPECT_EQ(1, panel_manager->num_stacks());
  EXPECT_EQ(6, stack->num_panels());
  EXPECT_TRUE(new_panel2 == stack->bottom_panel());
  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_TRUE(panel2->IsMinimized());
  EXPECT_TRUE(panel3->IsMinimized());
  EXPECT_TRUE(panel4->IsMinimized());
  EXPECT_FALSE(new_panel1->IsMinimized());
  EXPECT_FALSE(new_panel2->IsMinimized());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest,
                       AddNewPanelToGroupWithDetachedPanelWithCollapseToFit) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create 2 detached panels.
  Panel* panel1 = CreateDetachedPanel("1", gfx::Rect(300, 300, 200, 150));
  Panel* panel2 = CreateDetachedPanel("2", gfx::Rect(250, 310, 150, 150));
  ASSERT_EQ(2, panel_manager->num_panels());
  ASSERT_EQ(0, panel_manager->num_stacks());
  ASSERT_EQ(2, panel_manager->detached_collection()->num_panels());

  // Activate panel1 suhc that it will not get collapsed when the new panel
  // needs the space.
  panel1->Activate();
  WaitForPanelActiveState(panel1, SHOW_AS_ACTIVE);

  // Create new panel. Expect panel2 is minimized such that there is enough
  // space for new panel to append to panel2.
  CreatePanelParams params("N", gfx::Rect(50, 50, 300, 220), SHOW_AS_INACTIVE);
  params.create_mode = PanelManager::CREATE_AS_DETACHED;
  Panel* new_panel = CreatePanelWithParams(params);
  EXPECT_EQ(3, panel_manager->num_panels());
  EXPECT_EQ(1, panel_manager->detached_collection()->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  StackedPanelCollection* stack = panel_manager->stacks().front();
  EXPECT_EQ(2, stack->num_panels());
  EXPECT_TRUE(panel2 == stack->top_panel());
  EXPECT_TRUE(new_panel == stack->bottom_panel());
  EXPECT_FALSE(panel1->IsMinimized());
  EXPECT_TRUE(panel2->IsMinimized());
  EXPECT_FALSE(new_panel->IsMinimized());

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest,
                       AddNewPanelAsDetachedDueToNoPanelToGroupWith) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create one stack with 2 panels.
  StackedPanelCollection* stack = panel_manager->CreateStack();
  Panel* panel1 = CreateStackedPanel("1", gfx::Rect(100, 350, 200, 100), stack);
  Panel* panel2 = CreateStackedPanel("2", gfx::Rect(0, 0, 150, 100), stack);
  ASSERT_EQ(2, panel_manager->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(2, stack->num_panels());

  // Create 2 detached panels.
  Panel* panel3 = CreateDetachedPanel("3", gfx::Rect(300, 450, 200, 100));
  Panel* panel4 = CreateDetachedPanel("4", gfx::Rect(250, 150, 150, 200));
  ASSERT_EQ(4, panel_manager->num_panels());
  ASSERT_EQ(2, panel_manager->detached_collection()->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());

  // Create new panel. Expect that new panel has to be created as detached due
  // to that there is not enough space from any stack or detached panel.
  CreatePanelParams params("N", gfx::Rect(50, 50, 300, 300), SHOW_AS_INACTIVE);
  params.create_mode = PanelManager::CREATE_AS_DETACHED;
  Panel* new_panel = CreatePanelWithParams(params);
  EXPECT_EQ(5, panel_manager->num_panels());
  EXPECT_EQ(3, panel_manager->detached_collection()->num_panels());
  EXPECT_EQ(1, panel_manager->num_stacks());
  EXPECT_EQ(2, stack->num_panels());
  EXPECT_TRUE(panel_manager->detached_collection()->HasPanel(new_panel));

  panel_manager->CloseAll();
}

IN_PROC_BROWSER_TEST_F(StackedPanelBrowserTest, ClosePanels) {
  PanelManager* panel_manager = PanelManager::GetInstance();

  // Create 3 stacked panels.
  StackedPanelCollection* stack = panel_manager->CreateStack();
  gfx::Rect panel1_initial_bounds = gfx::Rect(100, 50, 200, 150);
  Panel* panel1 = CreateStackedPanel("1", panel1_initial_bounds, stack);
  gfx::Rect panel2_initial_bounds = gfx::Rect(0, 0, 150, 100);
  Panel* panel2 = CreateStackedPanel("2", panel2_initial_bounds, stack);
  gfx::Rect panel3_initial_bounds = gfx::Rect(0, 0, 250, 120);
  Panel* panel3 = CreateStackedPanel("3", panel3_initial_bounds, stack);
  ASSERT_EQ(3, panel_manager->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(3, stack->num_panels());

  gfx::Rect panel1_expected_bounds(panel1_initial_bounds);
  EXPECT_EQ(panel1_expected_bounds, panel1->GetBounds());
  gfx::Rect panel2_expected_bounds(panel1_expected_bounds.x(),
                                   panel1_expected_bounds.bottom(),
                                   panel1_expected_bounds.width(),
                                   panel2_initial_bounds.height());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());
  gfx::Rect panel3_expected_bounds(panel2_expected_bounds.x(),
                                   panel2_expected_bounds.bottom(),
                                   panel2_expected_bounds.width(),
                                   panel3_initial_bounds.height());
  EXPECT_EQ(panel3_expected_bounds, panel3->GetBounds());

  // Close P1. Expect that P2 and P3 should move up.
  CloseWindowAndWait(panel1);
  WaitForBoundsAnimationFinished(panel2);
  WaitForBoundsAnimationFinished(panel3);
  ASSERT_EQ(2, panel_manager->num_panels());
  ASSERT_EQ(1, panel_manager->num_stacks());
  ASSERT_EQ(2, stack->num_panels());
  EXPECT_EQ(PanelCollection::STACKED, panel2->collection()->type());
  EXPECT_EQ(PanelCollection::STACKED, panel3->collection()->type());

  panel2_expected_bounds.set_y(panel1_expected_bounds.y());
  EXPECT_EQ(panel2_expected_bounds, panel2->GetBounds());
  panel3_expected_bounds.set_y(panel2_expected_bounds.bottom());
  EXPECT_EQ(panel3_expected_bounds, panel3->GetBounds());

  // Close P2. Expect that P3 should become detached and move up.
  CloseWindowAndWait(panel2);
  WaitForBoundsAnimationFinished(panel3);
  ASSERT_EQ(1, panel_manager->num_panels());
  ASSERT_EQ(0, panel_manager->num_stacks());
  EXPECT_EQ(PanelCollection::DETACHED, panel3->collection()->type());

  panel3_expected_bounds.set_y(panel2_expected_bounds.y());
  EXPECT_EQ(panel3_expected_bounds, panel3->GetBounds());

  panel_manager->CloseAll();
}

#endif
