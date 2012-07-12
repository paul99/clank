// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/chrome_paths_internal.h"

#include "base/android/jni_android.h"
#include "base/android/path_utils.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"

namespace chrome {

void GetUserCacheDirectory(const FilePath& profile_dir, FilePath* result) {
  PathService::Get(base::DIR_CACHE, result);
}

bool GetDefaultUserDataDirectory(FilePath* result) {
  return PathService::Get(base::DIR_ANDROID_APP_DATA, result);
}

bool GetUserDocumentsDirectory(FilePath* result) {
  if (!GetDefaultUserDataDirectory(result))
    return false;
  *result = result->Append("Documents");
  return true;
}

bool GetUserDownloadsDirectory(FilePath* result) {
  if (!GetDefaultUserDataDirectory(result))
    return false;
  *result = result->Append("Downloads");
  return true;
}

bool GetUserDesktop(FilePath* result) {
  NOTREACHED() << "Android doesn't support GetUserDesktop";
  return false;
}

}  // namespace chrome
