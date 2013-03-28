// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_MESSAGE_CENTER_H_
#define UI_MESSAGE_CENTER_MESSAGE_CENTER_H_

#include "base/memory/scoped_ptr.h"
#include "ui/message_center/message_center_export.h"
#include "ui/message_center/notification_list.h"
#include "ui/notifications/notification_types.h"

namespace base {
class DictionaryValue;
}

// Class for managing the NotificationList. The client (e.g. Chrome) calls
// [Add|Remove|Update]Notification to create and update notifications in the
// list. It can also implement Delegate to receive callbacks when a
// notification is removed (closed), or clicked on.
// If a Host is provided, it will be informed when the notification list
// changes, and is expected to handle creating, showing, and hiding of any
// bubbles.

namespace message_center {

class MESSAGE_CENTER_EXPORT MessageCenter : public NotificationList::Delegate {
 public:
  // Class that hosts the message center.
  class MESSAGE_CENTER_EXPORT Host {
   public:
    // Called when the notification list has changed. |new_notification| will
    // be true if a notification was added or updated.
    virtual void MessageCenterChanged(bool new_notification) = 0;

   protected:
    virtual ~Host() {}
  };

  class MESSAGE_CENTER_EXPORT Delegate {
   public:
    // Called when the notification associated with |notification_id| is
    // removed (i.e. closed by the user).
    virtual void NotificationRemoved(const std::string& notification_id) = 0;

    // Request to disable the extension associated with |notification_id|.
    virtual void DisableExtension(const std::string& notification_id) = 0;

    // Request to disable notifications from the source of |notification_id|.
    virtual void DisableNotificationsFromSource(
        const std::string& notification_id) = 0;

    // Request to show the notification settings (|notification_id| is used
    // to identify the requesting browser context).
    virtual void ShowSettings(const std::string& notification_id) = 0;

    // Called when the notification body is clicked on.
    virtual void OnClicked(const std::string& notification_id) = 0;

    // Called when a button in a notification is clicked. |button_index|
    // indicates which button was clicked, zero-indexed (button one is 0,
    // button two is 1).
    //
    // TODO(miket): consider providing default implementations for the pure
    // virtuals above, to avoid changing so many files in disparate parts of
    // the codebase each time we enhance this interface.
    virtual void OnButtonClicked(const std::string& id, int button_index);

   protected:
    virtual ~Delegate() {}
  };

  // |host| is expected to manage any notification bubbles. It may be NULL.
  explicit MessageCenter(Host* host);

  virtual ~MessageCenter();

  // Called once to set the delegate.
  void SetDelegate(Delegate* delegate);

  // Informs the notification list whether the message center is visible.
  // This affects whether or not a message has been "read".
  void SetMessageCenterVisible(bool visible);

  // Accessors to notification_list_
  size_t NotificationCount() const;
  size_t UnreadNotificationCount() const;
  bool HasPopupNotifications() const;

  // Adds a new notification. |id| is a unique identifier, used to update or
  // remove notifications. |title| and |meesage| describe the notification text.
  // Use SetNotificationImage to set the icon image. If |extension_id| is
  // provided then 'Disable extension' will appear in a dropdown menu and the
  // id will be used to disable notifications from the extension. Otherwise if
  // |display_source| is provided, a menu item showing the source and allowing
  // notifications from that source to be disabled will be shown. All actual
  // disabling is handled by the Delegate.
  void AddNotification(ui::notifications::NotificationType type,
                       const std::string& id,
                       const string16& title,
                       const string16& message,
                       const string16& display_source,
                       const std::string& extension_id,
                       const base::DictionaryValue* optional_fields);

  // Updates an existing notification with id = old_id and set its id to new_id.
  // |optional_fields| can be NULL in case of no updates on those fields.
  void UpdateNotification(const std::string& old_id,
                          const std::string& new_id,
                          const string16& title,
                          const string16& message,
                          const base::DictionaryValue* optional_fields);

  // Removes an existing notification.
  void RemoveNotification(const std::string& id);

  // Sets the notification image.
  void SetNotificationImage(const std::string& id,
                            const gfx::ImageSkia& image);

  NotificationList* notification_list() { return notification_list_.get(); }

  // Overridden from NotificationList::Delegate.
  virtual void SendRemoveNotification(const std::string& id) OVERRIDE;
  virtual void SendRemoveAllNotifications() OVERRIDE;
  virtual void DisableNotificationByExtension(const std::string& id) OVERRIDE;
  virtual void DisableNotificationByUrl(const std::string& id) OVERRIDE;
  virtual void ShowNotificationSettings(const std::string& id) OVERRIDE;
  virtual void OnNotificationClicked(const std::string& id) OVERRIDE;
  virtual void OnQuietModeChanged(bool quiet_mode) OVERRIDE;
  virtual void OnButtonClicked(const std::string& id, int button_index)
      OVERRIDE;
  virtual NotificationList* GetNotificationList() OVERRIDE;

 private:
  scoped_ptr<NotificationList> notification_list_;
  Host* host_;
  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(MessageCenter);
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_MESSAGE_CENTER_H_
