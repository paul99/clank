// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/window/dialog_delegate.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "ui/base/ui_base_switches.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/text_button.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"
#include "ui/views/window/dialog_client_view.h"

namespace views {

namespace {

// Create a widget to host the dialog.
Widget* CreateDialogWidgetImpl(DialogDelegateView* dialog_delegate_view,
                               gfx::NativeWindow context,
                               gfx::NativeWindow parent) {
  views::Widget* widget = new views::Widget;
  views::Widget::InitParams params;
  params.delegate = dialog_delegate_view;
  if (DialogDelegate::UseNewStyle()) {
    // TODO(msw): Avoid Windows native controls or support dialog transparency
    //            with a separate border Widget, like BubbleDelegateView.
    params.transparent = views::View::get_use_acceleration_when_possible();
    params.remove_standard_frame = true;
  }
  params.context = context;
  params.parent = parent;
  params.top_level = true;
  widget->Init(params);
  return widget;
}

}  // namespace


////////////////////////////////////////////////////////////////////////////////
// DialogDelegate:

DialogDelegate::~DialogDelegate() {
}

// static
bool DialogDelegate::UseNewStyle() {
  static const bool use_new_style = CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableNewDialogStyle);
  return use_new_style;
}

int DialogDelegate::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL;
}

int DialogDelegate::GetDefaultDialogButton() const {
  if (GetDialogButtons() & ui::DIALOG_BUTTON_OK)
    return ui::DIALOG_BUTTON_OK;
  if (GetDialogButtons() & ui::DIALOG_BUTTON_CANCEL)
    return ui::DIALOG_BUTTON_CANCEL;
  return ui::DIALOG_BUTTON_NONE;
}

string16 DialogDelegate::GetDialogButtonLabel(ui::DialogButton button) const {
  // Empty string results in defaults for
  // ui::DIALOG_BUTTON_OK or ui::DIALOG_BUTTON_CANCEL.
  return string16();
}

bool DialogDelegate::IsDialogButtonEnabled(ui::DialogButton button) const {
  return true;
}

bool DialogDelegate::IsDialogButtonVisible(ui::DialogButton button) const {
  return true;
}

bool DialogDelegate::AreAcceleratorsEnabled(ui::DialogButton button) {
  return true;
}

View* DialogDelegate::GetExtraView() {
  return NULL;
}

bool DialogDelegate::GetSizeExtraViewHeightToButtons() {
  return false;
}

View* DialogDelegate::GetFootnoteView() {
  return NULL;
}

bool DialogDelegate::Cancel() {
  return true;
}

bool DialogDelegate::Accept(bool window_closing) {
  return Accept();
}

bool DialogDelegate::Accept() {
  return true;
}

View* DialogDelegate::GetInitiallyFocusedView() {
  // Focus the default button if any.
  const DialogClientView* dcv = GetDialogClientView();
  int default_button = GetDefaultDialogButton();
  if (default_button == ui::DIALOG_BUTTON_NONE)
    return NULL;

  if ((default_button & GetDialogButtons()) == 0) {
    // The default button is a button we don't have.
    NOTREACHED();
    return NULL;
  }

  if (default_button & ui::DIALOG_BUTTON_OK)
    return dcv->ok_button();
  if (default_button & ui::DIALOG_BUTTON_CANCEL)
    return dcv->cancel_button();
  return NULL;
}

DialogDelegate* DialogDelegate::AsDialogDelegate() {
  return this;
}

ClientView* DialogDelegate::CreateClientView(Widget* widget) {
  return new DialogClientView(widget, GetContentsView());
}

NonClientFrameView* DialogDelegate::CreateNonClientFrameView(Widget* widget) {
  return UseNewStyle() ? CreateNewStyleFrameView(widget) :
                         WidgetDelegate::CreateNonClientFrameView(widget);
}

// static
NonClientFrameView* DialogDelegate::CreateNewStyleFrameView(Widget* widget) {
  BubbleFrameView* frame = new BubbleFrameView(gfx::Insets(20, 20, 20, 20));
  const SkColor color = widget->GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_DialogBackground);
  frame->SetBubbleBorder(
      new BubbleBorder(BubbleBorder::FLOAT, BubbleBorder::SMALL_SHADOW, color));
  frame->SetTitle(widget->widget_delegate()->GetWindowTitle());
  frame->SetShowCloseButton(true);
  frame->set_can_drag(true);
  return frame;
}

const DialogClientView* DialogDelegate::GetDialogClientView() const {
  return GetWidget()->client_view()->AsDialogClientView();
}

DialogClientView* DialogDelegate::GetDialogClientView() {
  return GetWidget()->client_view()->AsDialogClientView();
}

ui::AccessibilityTypes::Role DialogDelegate::GetAccessibleWindowRole() const {
  return ui::AccessibilityTypes::ROLE_DIALOG;
}

////////////////////////////////////////////////////////////////////////////////
// DialogDelegateView:

DialogDelegateView::DialogDelegateView() {
  // A WidgetDelegate should be deleted on DeleteDelegate.
  set_owned_by_client();
}

DialogDelegateView::~DialogDelegateView() {}

// static
Widget* DialogDelegateView::CreateDialogWidget(DialogDelegateView* dialog,
                                               gfx::NativeWindow context,
                                               gfx::NativeWindow parent) {
  return CreateDialogWidgetImpl(dialog, context, parent);
}

void DialogDelegateView::DeleteDelegate() {
  delete this;
}

Widget* DialogDelegateView::GetWidget() {
  return View::GetWidget();
}

const Widget* DialogDelegateView::GetWidget() const {
  return View::GetWidget();
}

View* DialogDelegateView::GetContentsView() {
  return this;
}

}  // namespace views
