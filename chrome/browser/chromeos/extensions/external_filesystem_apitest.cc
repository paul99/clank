// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_file_value_serializer.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop_proxy.h"
#include "base/path_service.h"
#include "base/threading/worker_pool.h"
#include "base/values.h"
#include "chrome/browser/chromeos/drive/drive_file_system.h"
#include "chrome/browser/chromeos/drive/drive_system_service.h"
#include "chrome/browser/extensions/event_router.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_system.h"
#include "chrome/browser/extensions/extension_test_message_listener.h"
#include "chrome/browser/google_apis/fake_drive_service.h"
#include "chrome/browser/google_apis/gdata_wapi_parser.h"
#include "chrome/browser/google_apis/test_util.h"
#include "chrome/browser/google_apis/time_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "webkit/fileapi/external_mount_points.h"

using content::BrowserContext;
using extensions::Extension;

namespace {

// These should match the counterparts in remote.js.
// Also, the size of the file in |kTestRootFeed| has to be set to
// length of kTestFileContent string.
const char kTestFileContent[] = "hello, world!";

// Contains a folder named Folder that has a file File.aBc inside of it.
const char kTestRootFeed[] = "gdata/remote_file_system_apitest_root_feed.json";

// Flags used to run the tests with a COMPONENT extension.
const int kComponentFlags = ExtensionApiTest::kFlagEnableFileAccess |
                            ExtensionApiTest::kFlagLoadAsComponent;

// Helper class to wait for a background page to load or close again.
// TODO(tbarzic): We can probably share this with e.g.
//                lazy_background_page_apitest.
class BackgroundObserver {
 public:
  BackgroundObserver()
      : page_created_(chrome::NOTIFICATION_EXTENSION_BACKGROUND_PAGE_READY,
                      content::NotificationService::AllSources()),
        page_closed_(chrome::NOTIFICATION_EXTENSION_HOST_DESTROYED,
                     content::NotificationService::AllSources()) {
  }

  // TODO(tbarzic): Use this for file handlers in the rest of the tests
  // (instead of calling chrome.test.succeed in js).
  void WaitUntilLoaded() {
    page_created_.Wait();
  }

  void WaitUntilClosed() {
    page_closed_.Wait();
  }

 private:
  content::WindowedNotificationObserver page_created_;
  content::WindowedNotificationObserver page_closed_;
};

// Creates a cache representation of the test file with predetermined content.
void CreateFileWithContent(const base::FilePath& path,
                           const std::string& content) {
  int content_size = static_cast<int>(content.length());
  ASSERT_EQ(content_size,
            file_util::WriteFile(path, content.c_str(), content_size));
}

class FileSystemExtensionApiTest : public ExtensionApiTest {
 public:
  FileSystemExtensionApiTest() {}

  virtual ~FileSystemExtensionApiTest() {}

  virtual void SetUp() OVERRIDE {
    ASSERT_TRUE(tmp_dir_.CreateUniqueTempDir());
    mount_point_dir_ = tmp_dir_.path().Append("tmp");
    // Create the mount point.
    file_util::CreateDirectory(mount_point_dir_);

    // Create test files.
    ExtensionApiTest::SetUp();
  }

  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    ExtensionApiTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kEnableExperimentalExtensionApis);
  }

  // Adds a local mount point at at mount point /tmp.
  virtual void SetUpOnMainThread() OVERRIDE {
    BrowserContext::GetMountPoints(browser()->profile())->RegisterFileSystem(
        "tmp",
        fileapi::kFileSystemTypeNativeLocal,
        mount_point_dir_);
    ExtensionApiTest::SetUpOnMainThread();
  }

  bool RunFileBrowserHandlerTest(const std::string& test_page,
                                 const std::string& file_browser_name,
                                 const std::string& handler_name) {
    // If needed, load the test file handler.
    const Extension* file_handler = NULL;
    if (!handler_name.empty()) {
      BackgroundObserver page_complete;
      file_handler = LoadExtension(test_data_dir_.AppendASCII(handler_name));
      if (!file_handler)
        return false;
      page_complete.WaitUntilLoaded();
    }

    // Load test file browser.
    const Extension* file_browser = LoadExtensionAsComponent(
        test_data_dir_.AppendASCII(file_browser_name));
    if (!file_browser)
      return false;

    // Run the test.
    ResultCatcher catcher;
    GURL url = file_browser->GetResourceURL(test_page);
    ui_test_utils::NavigateToURL(browser(), url);
    return catcher.GetNextResult();
  }

 protected:
  base::FilePath mount_point_dir_;

 private:
  base::ScopedTempDir tmp_dir_;
};

class RestrictedFileSystemExtensionApiTest : public ExtensionApiTest {
 public:
  RestrictedFileSystemExtensionApiTest() {}

  virtual ~RestrictedFileSystemExtensionApiTest() {}

  virtual void SetUp() OVERRIDE {
    ASSERT_TRUE(tmp_dir_.CreateUniqueTempDir());
    mount_point_dir_ = tmp_dir_.path().Append("mount");
    // Create the mount point.
    file_util::CreateDirectory(mount_point_dir_);

    base::FilePath test_dir = mount_point_dir_.Append("test_dir");
    file_util::CreateDirectory(test_dir);

    base::FilePath test_file = test_dir.AppendASCII("test_file.foo");
    CreateFileWithContent(test_file, kTestFileContent);

    test_file = test_dir.AppendASCII("mutable_test_file.foo");
    CreateFileWithContent(test_file, kTestFileContent);

    test_file = test_dir.AppendASCII("test_file_to_delete.foo");
    CreateFileWithContent(test_file, kTestFileContent);

    test_file = test_dir.AppendASCII("test_file_to_move.foo");
    CreateFileWithContent(test_file, kTestFileContent);

    // Create test files.
    ExtensionApiTest::SetUp();
  }

  virtual void TearDown() OVERRIDE {
    ExtensionApiTest::TearDown();
  }

  virtual void SetUpOnMainThread() OVERRIDE {
    BrowserContext::GetMountPoints(browser()->profile())->RegisterFileSystem(
        "mount",
        fileapi::kFileSystemTypeRestrictedNativeLocal,
        mount_point_dir_);

    ExtensionApiTest::SetUpOnMainThread();
  }

 protected:
  base::ScopedTempDir tmp_dir_;
  base::FilePath mount_point_dir_;
};


class RemoteFileSystemExtensionApiTest : public ExtensionApiTest {
 public:
  RemoteFileSystemExtensionApiTest() : fake_drive_service_(NULL) {}

  virtual ~RemoteFileSystemExtensionApiTest() {}

  virtual void SetUp() OVERRIDE {
    // Set up cache root and documents service to be used when creating gdata
    // system service. This has to be done early on (before the browser is
    // created) because the system service instance is initialized very early
    // by FileBrowserEventRouter.
    base::FilePath tmp_dir_path;
    PathService::Get(base::DIR_TEMP, &tmp_dir_path);
    ASSERT_TRUE(test_cache_root_.CreateUniqueTempDirUnderPath(tmp_dir_path));

    drive::DriveSystemServiceFactory::SetFactoryForTest(
        base::Bind(&RemoteFileSystemExtensionApiTest::CreateDriveSystemService,
                   base::Unretained(this)));

    ExtensionApiTest::SetUp();
  }

 protected:
  // DriveSystemService factory function for this test.
  drive::DriveSystemService* CreateDriveSystemService(Profile* profile) {
    fake_drive_service_ = new google_apis::FakeDriveService;
    fake_drive_service_->LoadResourceListForWapi(
        kTestRootFeed);
    fake_drive_service_->LoadAccountMetadataForWapi(
        "gdata/account_metadata.json");

    return new drive::DriveSystemService(profile,
                                         fake_drive_service_,
                                         test_cache_root_.path(),
                                         NULL);
  }

  base::ScopedTempDir test_cache_root_;
  google_apis::FakeDriveService* fake_drive_service_;
};

IN_PROC_BROWSER_TEST_F(FileSystemExtensionApiTest, LocalFileSystem) {
  ASSERT_TRUE(RunFileBrowserHandlerTest("test.html", "local_filesystem", ""))
      << message_;
}

IN_PROC_BROWSER_TEST_F(FileSystemExtensionApiTest, FileBrowserTest) {
  ASSERT_TRUE(RunFileBrowserHandlerTest("read.html",
                                        "filebrowser_component",
                                        "filesystem_handler"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(FileSystemExtensionApiTest, FileBrowserTestLazy) {
  BackgroundObserver page_complete;
  const Extension* file_handler = LoadExtension(
      test_data_dir_.AppendASCII("filesystem_handler_lazy_background"));
  page_complete.WaitUntilClosed();

  ASSERT_TRUE(file_handler) << message_;

  ASSERT_TRUE(RunFileBrowserHandlerTest("read.html",
                                        "filebrowser_component",
                                        ""))
      << message_;
}

IN_PROC_BROWSER_TEST_F(FileSystemExtensionApiTest, FileBrowserTestWrite) {
  ASSERT_TRUE(RunFileBrowserHandlerTest("write.html",
                                        "filebrowser_component",
                                        "filesystem_handler_write"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(FileSystemExtensionApiTest,
                       FileBrowserTestWriteReadOnly) {
  ASSERT_FALSE(RunFileBrowserHandlerTest("write.html#def",
                                         "filebrowser_component",
                                         "filesystem_handler_write"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(FileSystemExtensionApiTest,
                       FileBrowserTestWriteComponent) {
  BackgroundObserver page_complete;
  const Extension* file_handler = LoadExtensionAsComponent(
      test_data_dir_.AppendASCII("filesystem_handler_write"));
  page_complete.WaitUntilLoaded();

  ASSERT_TRUE(file_handler) << message_;

  ASSERT_TRUE(RunFileBrowserHandlerTest("write.html",
                                        "filebrowser_component",
                                        ""))
      << message_;
}

IN_PROC_BROWSER_TEST_F(RestrictedFileSystemExtensionApiTest, Basic) {
  const Extension* file_browser = LoadExtensionAsComponent(
      test_data_dir_.AppendASCII("filebrowser_component"));

  ResultCatcher catcher;
  GURL url = file_browser->GetResourceURL("restricted.html");
  ui_test_utils::NavigateToURL(browser(), url);

  ASSERT_TRUE(catcher.GetNextResult()) << message_;
}

IN_PROC_BROWSER_TEST_F(RemoteFileSystemExtensionApiTest, RemoteMountPoint) {
  BackgroundObserver page_complete;
  ASSERT_TRUE(LoadExtension(test_data_dir_.AppendASCII("filesystem_handler")))
      << message_;
  page_complete.WaitUntilLoaded();

  EXPECT_TRUE(RunExtensionSubtest("filebrowser_component", "remote.html",
      kComponentFlags)) << message_;
}

IN_PROC_BROWSER_TEST_F(RemoteFileSystemExtensionApiTest, ContentSearch) {
  // Configure the drive service to return only one search result at a time
  // to simulate paginated searches.
  fake_drive_service_->set_default_max_results(1);
  EXPECT_TRUE(RunExtensionSubtest("filebrowser_component", "remote_search.html",
      kComponentFlags)) << message_;
}

IN_PROC_BROWSER_TEST_F(RemoteFileSystemExtensionApiTest, MetadataSearch) {
  EXPECT_TRUE(RunExtensionSubtest("filebrowser_component",
                                  "metadata_search.html",
                                  kComponentFlags)) << message_;
}

}  // namespace
