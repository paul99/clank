// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/file_system/create_directory_operation.h"

#include "base/message_loop.h"
#include "base/threading/sequenced_worker_pool.h"
#include "chrome/browser/chromeos/drive/drive_cache.h"
#include "chrome/browser/chromeos/drive/drive_feed_loader.h"
#include "chrome/browser/chromeos/drive/drive_resource_metadata.h"
#include "chrome/browser/chromeos/drive/drive_scheduler.h"
#include "chrome/browser/chromeos/drive/drive_test_util.h"
#include "chrome/browser/chromeos/drive/drive_webapps_registry.h"
#include "chrome/browser/chromeos/drive/fake_free_disk_space_getter.h"
#include "chrome/browser/chromeos/drive/file_system/operation_observer.h"
#include "chrome/browser/google_apis/fake_drive_service.h"
#include "chrome/browser/google_apis/test_util.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace drive {
namespace file_system {

class CreateDirectoryOperationTest
    : public testing::Test, public OperationObserver {
 protected:
  CreateDirectoryOperationTest()
      : ui_thread_(content::BrowserThread::UI, &message_loop_) {
  }

  virtual void SetUp() OVERRIDE {
    scoped_refptr<base::SequencedWorkerPool> pool =
        content::BrowserThread::GetBlockingPool();
    blocking_task_runner_ =
        pool->GetSequencedTaskRunner(pool->GetSequenceToken());

    profile_.reset(new TestingProfile);

    fake_drive_service_.reset(new google_apis::FakeDriveService);
    fake_drive_service_->LoadResourceListForWapi("gdata/root_feed.json");
    fake_drive_service_->LoadAccountMetadataForWapi(
        "gdata/account_metadata.json");

    metadata_.reset(
        new DriveResourceMetadata(fake_drive_service_->GetRootResourceId()));

    scheduler_.reset(
        new DriveScheduler(profile_.get(), fake_drive_service_.get(), NULL));
    scheduler_->Initialize();

    drive_web_apps_registry_.reset(new DriveWebAppsRegistry);

    fake_free_disk_space_getter_.reset(new FakeFreeDiskSpaceGetter);
    cache_ = new DriveCache(
        DriveCache::GetCacheRootPath(profile_.get()),
        blocking_task_runner_,
        fake_free_disk_space_getter_.get());

    drive_feed_loader_.reset(new DriveFeedLoader(
        metadata_.get(), scheduler_.get(), drive_web_apps_registry_.get(),
        cache_, blocking_task_runner_));

    DriveFileError error = DRIVE_FILE_OK;
    drive_feed_loader_->ReloadFromServerIfNeeded(
        base::Bind(&test_util::CopyErrorCodeFromFileOperationCallback,
                   &error));
    cache_->RequestInitializeForTesting();
    google_apis::test_util::RunBlockingPoolTask();

    operation_.reset(
        new CreateDirectoryOperation(scheduler_.get(), metadata_.get(), this));
  }

  virtual void TearDown() OVERRIDE {
    operation_.reset();
    drive_feed_loader_.reset();

    cache_->Destroy();
    // The cache destruction requires to post a task to the blocking pool.
    google_apis::test_util::RunBlockingPoolTask();

    fake_free_disk_space_getter_.reset();
    drive_web_apps_registry_.reset();
    scheduler_.reset();
    metadata_.reset();
    fake_drive_service_.reset();
    profile_.reset();

    blocking_task_runner_ = NULL;
  }

  // Copy the result from FindFirstMissingParentDirectory().
  static void CopyResultFromFindFirstMissingParentDirectory(
      CreateDirectoryOperation
          ::FindFirstMissingParentDirectoryResult* out_result,
      const CreateDirectoryOperation
          ::FindFirstMissingParentDirectoryResult& result) {
    DCHECK(out_result);
    *out_result = result;
  }

  virtual void OnDirectoryChangedByOperation(
      const base::FilePath& directory_path) OVERRIDE {
    // Do nothing.
  }

  bool LoadRootFeedDocument(const std::string& filename) {
    return test_util::LoadChangeFeed(filename,
                                     drive_feed_loader_.get(),
                                     false,  // is_delta_feed
                                     0);
  }

  CreateDirectoryOperation* operation() const {
    return operation_.get();
  }

 private:
  MessageLoopForUI message_loop_;
  // The order of the test threads is important, do not change the order.
  // See also content/browser/browser_thread_impl.cc.
  content::TestBrowserThread ui_thread_;
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;

  scoped_ptr<TestingProfile> profile_;

  scoped_ptr<google_apis::FakeDriveService> fake_drive_service_;
  scoped_ptr<DriveResourceMetadata> metadata_;
  scoped_ptr<DriveScheduler> scheduler_;
  scoped_ptr<DriveWebAppsRegistry> drive_web_apps_registry_;
  scoped_ptr<FakeFreeDiskSpaceGetter> fake_free_disk_space_getter_;

  // The way to delete the DriveCache instance is a bit tricky, so here we use
  // a raw point. See TearDown method for how to delete it.
  DriveCache* cache_;
  scoped_ptr<DriveFeedLoader> drive_feed_loader_;

  scoped_ptr<CreateDirectoryOperation> operation_;
};

TEST_F(CreateDirectoryOperationTest, FindFirstMissingParentDirectory) {
  ASSERT_TRUE(LoadRootFeedDocument("gdata/root_feed.json"));

  CreateDirectoryOperation::FindFirstMissingParentDirectoryResult result;

  // Create directory in root.
  base::FilePath dir_path(FILE_PATH_LITERAL("drive/New Folder 1"));
  operation()->FindFirstMissingParentDirectory(
      dir_path,
      base::Bind(&CopyResultFromFindFirstMissingParentDirectory,
                 &result));
  google_apis::test_util::RunBlockingPoolTask();
  EXPECT_EQ(CreateDirectoryOperation::FIND_FIRST_FOUND_MISSING, result.error);
  EXPECT_EQ(base::FilePath(FILE_PATH_LITERAL("drive/New Folder 1")),
            result.first_missing_parent_path);
  EXPECT_EQ("fake_root", result.last_dir_resource_id);

  // Missing folders in subdir of an existing folder.
  base::FilePath dir_path2(FILE_PATH_LITERAL("drive/Directory 1/New Folder 2"));
  operation()->FindFirstMissingParentDirectory(
      dir_path2,
      base::Bind(&CopyResultFromFindFirstMissingParentDirectory,
                 &result));
  google_apis::test_util::RunBlockingPoolTask();
  EXPECT_EQ(CreateDirectoryOperation::FIND_FIRST_FOUND_MISSING, result.error);
  EXPECT_EQ(base::FilePath(FILE_PATH_LITERAL("drive/Directory 1/New Folder 2")),
            result.first_missing_parent_path);
  EXPECT_NE("fake_root", result.last_dir_resource_id);  // non-root dir.

  // Missing two folders on the path.
  base::FilePath dir_path3 =
      dir_path2.Append(FILE_PATH_LITERAL("Another Folder"));
  operation()->FindFirstMissingParentDirectory(
      dir_path3,
      base::Bind(&CopyResultFromFindFirstMissingParentDirectory,
                 &result));
  google_apis::test_util::RunBlockingPoolTask();
  EXPECT_EQ(CreateDirectoryOperation::FIND_FIRST_FOUND_MISSING, result.error);
  EXPECT_EQ(base::FilePath(FILE_PATH_LITERAL("drive/Directory 1/New Folder 2")),
            result.first_missing_parent_path);
  EXPECT_NE("fake_root", result.last_dir_resource_id);  // non-root dir.

  // Folders on top of an existing file.
  operation()->FindFirstMissingParentDirectory(
      base::FilePath(FILE_PATH_LITERAL("drive/File 1.txt/BadDir")),
      base::Bind(&CopyResultFromFindFirstMissingParentDirectory,
                 &result));
  google_apis::test_util::RunBlockingPoolTask();
  EXPECT_EQ(CreateDirectoryOperation::FIND_FIRST_FOUND_INVALID, result.error);

  // Existing folder.
  operation()->FindFirstMissingParentDirectory(
      base::FilePath(FILE_PATH_LITERAL("drive/Directory 1")),
      base::Bind(&CopyResultFromFindFirstMissingParentDirectory,
                 &result));
  google_apis::test_util::RunBlockingPoolTask();
  EXPECT_EQ(CreateDirectoryOperation::FIND_FIRST_DIRECTORY_ALREADY_PRESENT,
            result.error);
}

}  // namespace file_system
}  // namespace drive
