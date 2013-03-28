// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_MESSAGE_VIEW_H_
#define UI_MESSAGE_CENTER_MESSAGE_VIEW_H_

#include "ui/message_center/notification_list.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/slide_out_view.h"
#include "ui/views/view.h"

namespace views {
class ImageButton;
class ScrollView;
}

namespace message_center {

// Individual notifications constants.
const int kPaddingBetweenItems = 10;
const int kPaddingHorizontal = 18;
const int kWebNotificationButtonWidth = 32;
const int kWebNotificationIconSize = 40;
const int kWebNotificationWidth = 320;

// An abstract class that forms the basis of a view for a notification entry.
class MessageView : public views::SlideOutView,
                    public views::ButtonListener {
 public:
  MessageView(NotificationList::Delegate* list_delegate,
              const NotificationList::Notification& notification);

  virtual ~MessageView();

  // Creates the elements that make up the view layout. Must be called
  // immediately after construction.
  virtual void SetUpView() = 0;

  void set_scroller(views::ScrollView* scroller) { scroller_ = scroller; }

  // Overridden from views::View.
  virtual bool OnMousePressed(const ui::MouseEvent& event) OVERRIDE;

  // Overridden from ui::EventHandler.
  virtual void OnGestureEvent(ui::GestureEvent* event) OVERRIDE;

  // Overridden from ButtonListener.
  virtual void ButtonPressed(views::Button* sender,
                             const ui::Event& event) OVERRIDE;

 protected:
  MessageView();

  // Shows the menu for the notification.
  void ShowMenu(gfx::Point screen_location);

  // Overridden from views::SlideOutView.
  virtual void OnSlideOut() OVERRIDE;

  NotificationList::Delegate* list_delegate_;
  NotificationList::Notification notification_;
  views::ImageButton* close_button_;

  views::ScrollView* scroller_;

  DISALLOW_COPY_AND_ASSIGN(MessageView);
};

}  // namespace message_center

#endif // UI_MESSAGE_CENTER_MESSAGE_VIEW_H_
