// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/notification/notification_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/common/chrome_switches.h"

using extensions::Extension;

namespace utils = extension_function_test_utils;

namespace {

class NotificationApiTest : public ExtensionApiTest {
 public:
  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    ExtensionApiTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableExperimentalExtensionApis);
  }
};

}  // namespace

IN_PROC_BROWSER_TEST_F(NotificationApiTest, TestIdUsage) {
  // Create a new notification. A lingering output of this block is the
  // notification ID, which we'll use in later parts of this test.
  std::string notification_id;
  scoped_refptr<Extension> empty_extension(utils::CreateEmptyExtension());
  {
    scoped_refptr<extensions::NotificationCreateFunction>
        notification_function(
            new extensions::NotificationCreateFunction());

    notification_function->set_extension(empty_extension.get());
    notification_function->set_has_callback(true);

    scoped_ptr<base::Value> result(utils::RunFunctionAndReturnSingleResult(
        notification_function,
        "[\"\", "  // Empty string: ask API to generate ID
        "{"
        "\"type\": \"simple\","
        "\"iconUrl\": \"http://www.google.com/intl/en/chrome/assets/"
        "common/images/chrome_logo_2x.png\","
        "\"title\": \"Attention!\","
        "\"message\": \"Check out Cirque du Soleil\""
        "}]",
        browser(), utils::NONE));

    ASSERT_EQ(base::Value::TYPE_STRING, result->GetType());
    ASSERT_TRUE(result->GetAsString(&notification_id));
    ASSERT_TRUE(notification_id.length() > 0);
  }

  // Update the existing notification.
  {
    scoped_refptr<extensions::NotificationUpdateFunction>
        notification_function(
            new extensions::NotificationUpdateFunction());

    notification_function->set_extension(empty_extension.get());
    notification_function->set_has_callback(true);

    scoped_ptr<base::Value> result(utils::RunFunctionAndReturnSingleResult(
        notification_function,
        "[\"" + notification_id + "\", "
        "{"
        "\"type\": \"simple\","
        "\"iconUrl\": \"http://www.google.com/intl/en/chrome/assets/"
        "common/images/chrome_logo_2x.png\","
        "\"title\": \"Attention!\","
        "\"message\": \"Too late! The show ended yesterday\""
        "}]",
        browser(), utils::NONE));

    ASSERT_EQ(base::Value::TYPE_BOOLEAN, result->GetType());
    bool copy_bool_value = false;
    ASSERT_TRUE(result->GetAsBoolean(&copy_bool_value));
    ASSERT_TRUE(copy_bool_value);

    // TODO(miket): add a testing method to query the message from the
    // displayed notification, and assert it matches the updated message.
    //
    // TODO(miket): add a method to count the number of outstanding
    // notifications, and confirm it remains at one at this point.
  }

  // Update a nonexistent notification.
  {
    scoped_refptr<extensions::NotificationUpdateFunction>
        notification_function(
            new extensions::NotificationUpdateFunction());

    notification_function->set_extension(empty_extension.get());
    notification_function->set_has_callback(true);

    scoped_ptr<base::Value> result(utils::RunFunctionAndReturnSingleResult(
        notification_function,
        "[\"xxxxxxxxxxxx\", "
        "{"
        "\"type\": \"simple\","
        "\"iconUrl\": \"http://www.google.com/intl/en/chrome/assets/"
        "common/images/chrome_logo_2x.png\","
        "\"title\": \"!\","
        "\"message\": \"!\""
        "}]",
        browser(), utils::NONE));

    ASSERT_EQ(base::Value::TYPE_BOOLEAN, result->GetType());
    bool copy_bool_value = false;
    ASSERT_TRUE(result->GetAsBoolean(&copy_bool_value));
    ASSERT_FALSE(copy_bool_value);
  }

  // Delete a nonexistent notification.
  {
    scoped_refptr<extensions::NotificationDeleteFunction>
        notification_function(
            new extensions::NotificationDeleteFunction());

    notification_function->set_extension(empty_extension.get());
    notification_function->set_has_callback(true);

    scoped_ptr<base::Value> result(utils::RunFunctionAndReturnSingleResult(
        notification_function,
        "[\"xxxxxxxxxxx\"]", browser(), utils::NONE));

    ASSERT_EQ(base::Value::TYPE_BOOLEAN, result->GetType());
    bool copy_bool_value = false;
    ASSERT_TRUE(result->GetAsBoolean(&copy_bool_value));
    ASSERT_FALSE(copy_bool_value);
  }

  // Delete the notification we created.
  {
    scoped_refptr<extensions::NotificationDeleteFunction>
        notification_function(
            new extensions::NotificationDeleteFunction());

    notification_function->set_extension(empty_extension.get());
    notification_function->set_has_callback(true);

    scoped_ptr<base::Value> result(utils::RunFunctionAndReturnSingleResult(
        notification_function,
        "[\"" + notification_id + "\"]", browser(), utils::NONE));

    ASSERT_EQ(base::Value::TYPE_BOOLEAN, result->GetType());
    bool copy_bool_value = false;
    ASSERT_TRUE(result->GetAsBoolean(&copy_bool_value));
    ASSERT_TRUE(copy_bool_value);
  }
}

IN_PROC_BROWSER_TEST_F(NotificationApiTest, TestBaseFormatNotification) {
  scoped_refptr<extensions::NotificationCreateFunction>
      notification_create_function(
          new extensions::NotificationCreateFunction());
  scoped_refptr<Extension> empty_extension(utils::CreateEmptyExtension());

  notification_create_function->set_extension(empty_extension.get());
  notification_create_function->set_has_callback(true);

  scoped_ptr<base::Value> result(utils::RunFunctionAndReturnSingleResult(
      notification_create_function,
      "[\"\", "
      "{"
      "\"type\": \"base\","
      "\"iconUrl\": \"http://www.google.com/intl/en/chrome/assets/"
      "common/images/chrome_logo_2x.png\","
      "\"title\": \"Attention!\","
      "\"message\": \"Check out Cirque du Soleil\","
      "\"priority\": 1,"
      "\"timestamp\": \"Tue, 15 Nov 1994 12:45:26 GMT\","
      "\"secondIconUrl\": \"http://www.google.com/logos/2012/"
      "Day-Of-The-Dead-12-hp.jpg\","
      "\"unreadCount\": 42,"
      "\"buttonOneTitle\": \"Up\","
      "\"buttonTwoTitle\": \"Down\","
      "\"expandedMessage\": \"This is a longer expanded message.\","
      "\"imageUrl\": \"http://www.google.com/logos/2012/election12-hp.jpg\""
      "}]",
      browser(), utils::NONE));

  std::string notification_id;
  ASSERT_EQ(base::Value::TYPE_STRING, result->GetType());
  ASSERT_TRUE(result->GetAsString(&notification_id));
  ASSERT_TRUE(notification_id.length() > 0);
}

IN_PROC_BROWSER_TEST_F(NotificationApiTest, TestMultipleItemNotification) {
  scoped_refptr<extensions::NotificationCreateFunction>
      notification_create_function(
          new extensions::NotificationCreateFunction());
  scoped_refptr<Extension> empty_extension(utils::CreateEmptyExtension());

  notification_create_function->set_extension(empty_extension.get());
  notification_create_function->set_has_callback(true);

  scoped_ptr<base::Value> result(utils::RunFunctionAndReturnSingleResult(
      notification_create_function,
      "[\"\", "
      "{"
      "\"type\": \"multiple\","
      "\"iconUrl\": \"https://code.google.com/p/chromium/logo\","
      "\"title\": \"Multiple Item Notification Title\","
      "\"message\": \"Multiple item notification message.\","
      "\"items\": ["
      "  {\"title\": \"Brett Boe\","
      " \"message\": \"This is an important message!\"},"
      "  {\"title\": \"Carla Coe\","
      " \"message\": \"Just took a look at the proposal\"},"
      "  {\"title\": \"Donna Doe\","
      " \"message\": \"I see that you went to the conference\"},"
      "  {\"title\": \"Frank Foe\","
      " \"message\": \"I ate Harry's sandwich!\"},"
      "  {\"title\": \"Grace Goe\","
      " \"message\": \"I saw Frank steal a sandwich :-)\"}"
      "],"
      "\"priority\": 1,"
      "\"timestamp\": \"Fri, 16 Nov 2012 01:17:15 GMT\""
      "}]",
      browser(), utils::NONE));
  // TODO(dharcourt): [...], items = [{title: foo, message: bar}, ...], [...]

  std::string notification_id;
  ASSERT_EQ(base::Value::TYPE_STRING, result->GetType());
  ASSERT_TRUE(result->GetAsString(&notification_id));
  ASSERT_TRUE(notification_id.length() > 0);
}

IN_PROC_BROWSER_TEST_F(NotificationApiTest, TestEvents) {
  ASSERT_TRUE(RunExtensionTest("notification/api/events")) << message_;
}
