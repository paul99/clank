// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_SYNCABLE_ON_DISK_DIRECTORY_BACKING_STORE_H_
#define SYNC_SYNCABLE_ON_DISK_DIRECTORY_BACKING_STORE_H_

#include "base/file_path.h"
#include "sync/syncable/directory_backing_store.h"

namespace syncer {
namespace syncable {

// This is the concrete class that provides a useful implementation of
// DirectoryBackingStore.
class OnDiskDirectoryBackingStore : public DirectoryBackingStore {
 public:
  OnDiskDirectoryBackingStore(const std::string& dir_name,
                              const FilePath& backing_filepath);
  virtual ~OnDiskDirectoryBackingStore();
  virtual DirOpenResult Load(
      MetahandlesIndex* entry_bucket,
      Directory::KernelLoadInfo* kernel_load_info) OVERRIDE;

  // A helper function that will make one attempt to load the directory.
  // Unlike Load(), it does not attempt to recover from failure.
  DirOpenResult TryLoad(
      MetahandlesIndex* entry_bucket,
      Directory::KernelLoadInfo* kernel_load_info);

 protected:
  // Subclasses may override this to avoid a possible DCHECK.
  virtual void ReportFirstTryOpenFailure();

 private:
  FRIEND_TEST_ALL_PREFIXES(DirectoryBackingStoreTest, MinorCorruption);

  bool allow_failure_for_test_;
  FilePath backing_filepath_;
};

}  // namespace syncable
}  // namespace syncer

#endif  // SYNC_SYNCABLE_ON_DISK_DIRECTORY_BACKING_STORE_H_
