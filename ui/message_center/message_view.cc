// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/message_view.h"

#include "grit/ui_resources.h"
#include "grit/ui_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/menu/menu_model_adapter.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/widget/widget.h"

namespace {

const int kCloseButtonSize = 29;
const int kCloseIconTopPadding = 5;
const int kCloseIconRightPadding = 5;

// ControlButtons are ImageButtons whose image can be padded within the button.
// This allows the creation of buttons like the notification close and expand
// buttons whose clickable areas extends beyond their image areas
// (<http://crbug.com/168822>) without the need to create and maintain
// corresponding resource images with alpha padding. In the future, this class
// will also allow for buttons whose touch areas extend beyond their clickable
// area (<http://crbug.com/168856>).
class ControlButton : public views::ImageButton {
 public:
  ControlButton(views::ButtonListener* listener);
  virtual ~ControlButton();

  // Overridden from views::ImageButton:
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE;

  // The SetPadding() method also sets the button's image alignment (positive
  // values yield left/top alignments, negative values yield right/bottom ones,
  // and zero values center/middle ones). ImageButton::SetImageAlignment() calls
  // will not affect ControlButton image alignments.
  void SetPadding(int horizontal_padding, int vertical_padding);

  void SetNormalImage(int resource_id);
  void SetHoveredImage(int resource_id);
  void SetPressedImage(int resource_id);

 protected:
  gfx::Point ComputePaddedImagePaintPosition(const gfx::ImageSkia& image);

 private:
  gfx::Insets padding_;

  DISALLOW_COPY_AND_ASSIGN(ControlButton);
};

ControlButton::ControlButton(views::ButtonListener* listener)
  : views::ImageButton(listener) {
}

ControlButton::~ControlButton() {
}

void ControlButton::SetPadding(int horizontal_padding, int vertical_padding) {
  padding_.Set(std::max(vertical_padding, 0),
               std::max(horizontal_padding, 0),
               std::max(-vertical_padding, 0),
               std::max(-horizontal_padding, 0));
}

void ControlButton::SetNormalImage(int resource_id) {
  SetImage(views::CustomButton::STATE_NORMAL,
           ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
               resource_id));
}

void ControlButton::SetHoveredImage(int resource_id) {
  SetImage(views::CustomButton::STATE_HOVERED,
           ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
               resource_id));
}

void ControlButton::SetPressedImage(int resource_id) {
  SetImage(views::CustomButton::STATE_PRESSED,
           ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
               resource_id));
}

gfx::Size ControlButton::GetPreferredSize() {
  return gfx::Size(kCloseButtonSize, kCloseButtonSize);
}

void ControlButton::OnPaint(gfx::Canvas* canvas) {
  // This is the same implementation as ImageButton::OnPaint except
  // that it calls ComputePaddedImagePaintPosition() instead of
  // ComputeImagePaintPosition(), in effect overriding that private method.
  View::OnPaint(canvas);
  gfx::ImageSkia image = GetImageToPaint();
  if (!image.isNull()) {
    gfx::Point position = ComputePaddedImagePaintPosition(image);
    if (!background_image_.isNull())
      canvas->DrawImageInt(background_image_, position.x(), position.y());
    canvas->DrawImageInt(image, position.x(), position.y());
    if (!overlay_image_.isNull())
      canvas->DrawImageInt(overlay_image_, position.x(), position.y());
  }
  OnPaintFocusBorder(canvas);
}

gfx::Point ControlButton::ComputePaddedImagePaintPosition(
    const gfx::ImageSkia& image) {
  gfx::Vector2d offset;
  gfx::Rect bounds = GetContentsBounds();
  bounds.Inset(padding_);

  if (padding_.left() == 0 && padding_.right() == 0)
    offset.set_x((bounds.width() - image.width()) / 2);  // Center align.
  else if (padding_.right() > 0)
    offset.set_x(bounds.width() - image.width());  // Right align.

  if (padding_.top() == 0 && padding_.bottom() == 0)
    offset.set_y((bounds.height() - image.height()) / 2);  // Middle align.
  else if (padding_.bottom() > 0)
    offset.set_y(bounds.height() - image.height());  // Bottom align.

  return bounds.origin() + offset;
}

} // namespace

namespace message_center {

// Menu constants
const int kTogglePermissionCommand = 0;
const int kToggleExtensionCommand = 1;
const int kShowSettingsCommand = 2;

// A dropdown menu for notifications.
class WebNotificationMenuModel : public ui::SimpleMenuModel,
                                 public ui::SimpleMenuModel::Delegate {
 public:
  WebNotificationMenuModel(NotificationList::Delegate* list_delegate,
                           const Notification& notification)
      : ALLOW_THIS_IN_INITIALIZER_LIST(ui::SimpleMenuModel(this)),
        list_delegate_(list_delegate),
        notification_(notification) {
    // Add 'disable notifications' menu item.
    if (!notification.extension_id.empty()) {
      AddItem(kToggleExtensionCommand,
              GetLabelForCommandId(kToggleExtensionCommand));
    } else if (!notification.display_source.empty()) {
      AddItem(kTogglePermissionCommand,
              GetLabelForCommandId(kTogglePermissionCommand));
    }
    // Add settings menu item.
    if (!notification.display_source.empty()) {
      AddItem(kShowSettingsCommand,
              GetLabelForCommandId(kShowSettingsCommand));
    }
  }

  virtual ~WebNotificationMenuModel() {
  }

  // Overridden from ui::SimpleMenuModel:
  virtual string16 GetLabelForCommandId(int command_id) const OVERRIDE {
    switch (command_id) {
      case kToggleExtensionCommand:
        return l10n_util::GetStringUTF16(IDS_MESSAGE_CENTER_EXTENSIONS_DISABLE);
      case kTogglePermissionCommand:
        return l10n_util::GetStringFUTF16(IDS_MESSAGE_CENTER_SITE_DISABLE,
                                          notification_.display_source);
      case kShowSettingsCommand:
        return l10n_util::GetStringUTF16(IDS_MESSAGE_CENTER_SETTINGS);
      default:
        NOTREACHED();
    }
    return string16();
  }

  // Overridden from ui::SimpleMenuModel::Delegate:
  virtual bool IsCommandIdChecked(int command_id) const OVERRIDE {
    return false;
  }

  virtual bool IsCommandIdEnabled(int command_id) const OVERRIDE {
    return false;
  }

  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) OVERRIDE {
    return false;
  }

  virtual void ExecuteCommand(int command_id) OVERRIDE {
    switch (command_id) {
      case kToggleExtensionCommand:
        list_delegate_->DisableNotificationByExtension(notification_.id);
        break;
      case kTogglePermissionCommand:
        list_delegate_->DisableNotificationByUrl(notification_.id);
        break;
      case kShowSettingsCommand:
        list_delegate_->ShowNotificationSettings(notification_.id);
        break;
      default:
        NOTREACHED();
    }
  }

 private:
  NotificationList::Delegate* list_delegate_;
  Notification notification_;

  DISALLOW_COPY_AND_ASSIGN(WebNotificationMenuModel);
};

MessageView::MessageView(
    NotificationList::Delegate* list_delegate,
    const Notification& notification)
    : list_delegate_(list_delegate),
      notification_(notification),
      scroller_(NULL) {
  ControlButton *close = new ControlButton(this);
  close->SetPadding(-kCloseIconRightPadding, kCloseIconTopPadding);
  close->SetNormalImage(IDR_NOTIFICATION_CLOSE);
  close->SetHoveredImage(IDR_NOTIFICATION_CLOSE_HOVER);
  close->SetPressedImage(IDR_NOTIFICATION_CLOSE_PRESSED);
  close_button_.reset(close);
}

MessageView::MessageView() {
}

MessageView::~MessageView() {
}

bool MessageView::OnMousePressed(const ui::MouseEvent& event) {
  if (event.flags() & ui::EF_RIGHT_MOUSE_BUTTON) {
    ShowMenu(event.location());
    return true;
  }
  list_delegate_->OnNotificationClicked(notification_.id);
  return true;
}

void MessageView::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP) {
    list_delegate_->OnNotificationClicked(notification_.id);
    event->SetHandled();
    return;
  }

  if (event->type() == ui::ET_GESTURE_LONG_PRESS) {
    ShowMenu(event->location());
    event->SetHandled();
    return;
  }

  SlideOutView::OnGestureEvent(event);
  // Do not return here by checking handled(). SlideOutView calls SetHandled()
  // even though the scroll gesture doesn't make no (or little) effects on the
  // slide-out behavior. See http://crbug.com/172991

  if (!event->IsScrollGestureEvent())
    return;

  if (scroller_)
    scroller_->OnGestureEvent(event);
  event->SetHandled();
}

void MessageView::ButtonPressed(views::Button* sender,
                                const ui::Event& event) {
  if (sender == close_button())
    list_delegate_->SendRemoveNotification(notification_.id);
}

void MessageView::ShowMenu(gfx::Point screen_location) {
  WebNotificationMenuModel menu_model(list_delegate_, notification_);
  if (menu_model.GetItemCount() == 0)
    return;

  views::MenuModelAdapter menu_model_adapter(&menu_model);
  views::MenuRunner menu_runner(menu_model_adapter.CreateMenu());

  views::View::ConvertPointToScreen(this, &screen_location);
  ignore_result(menu_runner.RunMenuAt(
      GetWidget()->GetTopLevelWidget(),
      NULL,
      gfx::Rect(screen_location, gfx::Size()),
      views::MenuItemView::TOPRIGHT,
      views::MenuRunner::HAS_MNEMONICS));
}

void MessageView::OnSlideOut() {
  list_delegate_->SendRemoveNotification(notification_.id);
}

}  // namespace message_center
