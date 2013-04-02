// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/notification/notification_api.h"

#include "base/callback.h"
#include "base/strings/string_number_conversions.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/event_names.h"
#include "chrome/browser/extensions/event_router.h"
#include "chrome/browser/extensions/extension_system.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/notifications/notification_ui_manager.h"
#include "chrome/common/extensions/extension.h"
#include "googleurl/src/gurl.h"
#include "ui/notifications/notification_types.h"

namespace extensions {

namespace {

const char kResultKey[] = "result";

class NotificationApiDelegate : public NotificationDelegate {
 public:
  NotificationApiDelegate(ApiFunction* api_function,
                          Profile* profile,
                          const std::string& extension_id,
                          const std::string& id)
      : api_function_(api_function),
        profile_(profile),
        extension_id_(extension_id),
        id_(id),
        scoped_id_(CreateScopedIdentifier(extension_id, id)) {
    DCHECK(api_function_);
  }

  // Given an extension id and another id, returns an id that is unique
  // relative to other extensions.
  static std::string CreateScopedIdentifier(const std::string& extension_id,
                                            const std::string& id) {
    return extension_id + "-" + id;
  }

  virtual void Display() OVERRIDE {
    scoped_ptr<ListValue> args(CreateBaseEventArgs());
    SendEvent(event_names::kOnNotificationDisplayed, args.Pass());
  }

  virtual void Error() OVERRIDE {
    scoped_ptr<ListValue> args(CreateBaseEventArgs());
    SendEvent(event_names::kOnNotificationError, args.Pass());
  }

  virtual void Close(bool by_user) OVERRIDE {
    scoped_ptr<ListValue> args(CreateBaseEventArgs());
    args->Append(Value::CreateBooleanValue(by_user));
    SendEvent(event_names::kOnNotificationClosed, args.Pass());
  }

  virtual void Click() OVERRIDE {
    scoped_ptr<ListValue> args(CreateBaseEventArgs());
    SendEvent(event_names::kOnNotificationClicked, args.Pass());
  }

  virtual void ButtonClick(int index) OVERRIDE {
    scoped_ptr<ListValue> args(CreateBaseEventArgs());
    args->Append(Value::CreateIntegerValue(index));
    SendEvent(event_names::kOnNotificationButtonClicked, args.Pass());
  }

  virtual std::string id() const OVERRIDE {
    return scoped_id_;
  }

  virtual content::RenderViewHost* GetRenderViewHost() const OVERRIDE {
    // We're holding a reference to api_function_, so we know it'll be valid as
    // long as we are, and api_function_ (as a UIThreadExtensionFunction)
    // listens to content::NOTIFICATION_RENDER_VIEW_HOST_DELETED and will
    // properly zero out its copy of render_view_host when the RVH goes away.
    return api_function_->render_view_host();
  }

 private:
  virtual ~NotificationApiDelegate() {}

  void SendEvent(const std::string& name, scoped_ptr<ListValue> args) {
    scoped_ptr<Event> event(new Event(name, args.Pass()));
    ExtensionSystem::Get(profile_)->event_router()->DispatchEventToExtension(
        extension_id_, event.Pass());
  }

  scoped_ptr<ListValue> CreateBaseEventArgs() {
    scoped_ptr<ListValue> args(new ListValue());
    args->Append(Value::CreateStringValue(id_));
    return args.Pass();
  }

  scoped_refptr<ApiFunction> api_function_;
  Profile* profile_;
  const std::string extension_id_;
  const std::string id_;
  const std::string scoped_id_;

  DISALLOW_COPY_AND_ASSIGN(NotificationApiDelegate);
};

}  // namespace

NotificationApiFunction::NotificationApiFunction() {
}

NotificationApiFunction::~NotificationApiFunction() {
}

void NotificationApiFunction::CreateNotification(
    const std::string& id,
    api::experimental_notification::NotificationOptions* options) {
  ui::notifications::NotificationType type =
      ui::notifications::StringToNotificationType(options->type);
  GURL icon_url(UTF8ToUTF16(options->icon_url));
  string16 title(UTF8ToUTF16(options->title));
  string16 message(UTF8ToUTF16(options->message));

  scoped_ptr<DictionaryValue> optional_fields(new DictionaryValue());

  // For all notification types.
  if (options->priority.get())
    optional_fields->SetInteger(ui::notifications::kPriorityKey,
                                *options->priority);
  if (options->timestamp.get())
    optional_fields->SetString(ui::notifications::kTimestampKey,
                               *options->timestamp);
  if (options->unread_count.get())
    optional_fields->SetInteger(ui::notifications::kUnreadCountKey,
                                *options->unread_count);
  if (options->button_one_title.get())
    optional_fields->SetString(ui::notifications::kButtonOneTitleKey,
                               UTF8ToUTF16(*options->button_one_title));
  if (options->button_one_icon_url.get())
    optional_fields->SetString(ui::notifications::kButtonOneIconUrlKey,
                               UTF8ToUTF16(*options->button_one_icon_url));
  // TODO(dharcourt): Fail if:
  //  (options->button_two_title.get() || options->button_two_icon_url.get()) &&
  //  !(options->button_one_title.get() || options->button_one_icon_url.get())
  if (options->button_two_title.get())
    optional_fields->SetString(ui::notifications::kButtonTwoTitleKey,
                               UTF8ToUTF16(*options->button_two_title));
  if (options->button_two_icon_url.get())
    optional_fields->SetString(ui::notifications::kButtonTwoIconUrlKey,
                               UTF8ToUTF16(*options->button_two_icon_url));
  if (options->expanded_message.get())
    optional_fields->SetString(ui::notifications::kExpandedMessageKey,
                               UTF8ToUTF16(*options->expanded_message));

  // For image notifications (type == 'image').
  // TODO(dharcourt): Fail if (type == 'image' && !options->image_url.get())
  // TODO(dharcourt): Fail if (type != 'image' && options->image_url.get())
  if (options->image_url.get())
    optional_fields->SetString(ui::notifications::kImageUrlKey,
                               UTF8ToUTF16(*options->image_url));

  // For list notifications (type == 'multiple').
  // TODO(dharcourt): Fail if (type == 'multiple' && !options->items.get())
  // TODO(dharcourt): Fail if (type != 'multiple' && options->items.get())
  if (options->items.get()) {
    base::ListValue* items = new base::ListValue();
    std::vector<
      linked_ptr<
        api::experimental_notification::NotificationItem> >::iterator i;
    for (i = options->items->begin(); i != options->items->end(); ++i) {
      base::DictionaryValue* item = new base::DictionaryValue();
      item->SetString(ui::notifications::kItemTitleKey,
                      UTF8ToUTF16(i->get()->title));
      item->SetString(ui::notifications::kItemMessageKey,
                      UTF8ToUTF16(i->get()->message));
      items->Append(item);
    }
    optional_fields->Set(ui::notifications::kItemsKey, items);
  }

  NotificationApiDelegate* api_delegate(new NotificationApiDelegate(
      this,
      profile(),
      extension_->id(),
      id));  // ownership is passed to Notification
  Notification notification(type, extension_->url(), icon_url, title, message,
                            WebKit::WebTextDirectionDefault,
                            string16(), UTF8ToUTF16(api_delegate->id()),
                            optional_fields.get(), api_delegate);

  g_browser_process->notification_ui_manager()->Add(notification, profile());
}

const char kNotificationPrefix[] = "extension.api.";

static uint64 next_id_ = 0;

NotificationCreateFunction::NotificationCreateFunction() {
}

NotificationCreateFunction::~NotificationCreateFunction() {
}

bool NotificationCreateFunction::RunImpl() {
  params_ = api::experimental_notification::Create::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params_.get());

  // If the caller provided a notificationId, use that. Otherwise, generate
  // one. Note that there's nothing stopping an app developer from passing in
  // arbitrary "extension.api.999" notificationIds that will collide with
  // future generated IDs. It doesn't seem necessary to try to prevent this; if
  // developers want to hurt themselves, we'll let them.
  const std::string extension_id(extension_->id());
  std::string notification_id;
  if (!params_->notification_id.empty())
    notification_id = params_->notification_id;
  else
    notification_id = kNotificationPrefix + base::Uint64ToString(next_id_++);

  CreateNotification(notification_id, &params_->options);

  SetResult(Value::CreateStringValue(notification_id));

  SendResponse(true);

  return true;
}

NotificationUpdateFunction::NotificationUpdateFunction() {
}

NotificationUpdateFunction::~NotificationUpdateFunction() {
}

bool NotificationUpdateFunction::RunImpl() {
  params_ = api::experimental_notification::Update::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params_.get());

  if (g_browser_process->notification_ui_manager()->
      DoesIdExist(NotificationApiDelegate::CreateScopedIdentifier(
          extension_->id(), params_->notification_id))) {
    CreateNotification(params_->notification_id, &params_->options);
    SetResult(Value::CreateBooleanValue(true));
  } else {
    SetResult(Value::CreateBooleanValue(false));
  }

  SendResponse(true);

  return true;
}

NotificationDeleteFunction::NotificationDeleteFunction() {
}

NotificationDeleteFunction::~NotificationDeleteFunction() {
}

bool NotificationDeleteFunction::RunImpl() {
  params_ = api::experimental_notification::Delete::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params_.get());

  bool cancel_result = g_browser_process->notification_ui_manager()->
      CancelById(NotificationApiDelegate::CreateScopedIdentifier(
          extension_->id(), params_->notification_id));

  SetResult(Value::CreateBooleanValue(cancel_result));
  SendResponse(true);

  return true;
}

}  // namespace extensions
