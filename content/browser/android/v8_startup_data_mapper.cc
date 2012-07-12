// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/v8_startup_data_mapper.h"

#if defined(V8_COMPRESS_STARTUP_DATA_BZ2)
#include <bzlib.h>
#endif

#include "base/file_util.h"
#include "base/lazy_instance.h"
#include "base/string_number_conversions.h"
#include "chrome/common/chrome_paths_internal.h"
#include "v8/include/v8.h"

namespace {

FilePath GetBaseStartupDataPath() {
  FilePath snapshot_path;
  chrome::GetUserCacheDirectory(FilePath(), &snapshot_path);
  return snapshot_path.Append(FILE_PATH_LITERAL("V8 Startup Data"));
}

FilePath GetStartupDataPath() {
  return GetBaseStartupDataPath().Append(v8::V8::GetVersion());
}

base::LazyInstance<V8StartupDataMapper> mapper_instance = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
V8StartupDataMapper& V8StartupDataMapper::GetInstance() {
  return mapper_instance.Get();
}

void V8StartupDataMapper::BrowserProcessMapData() {
#if defined(V8_COMPRESS_STARTUP_DATA_BZ2)
  DCHECK(v8::V8::GetCompressedStartupDataAlgorithm() ==
         v8::StartupData::kBZip2);
  FilePath base_startup_data_path(GetBaseStartupDataPath());
  bool success = file_util::CreateDirectory(base_startup_data_path);
  DCHECK(success);

  // Search for old versions and delete them
  FilePath::StringType v8_version = v8::V8::GetVersion();
  file_util::FileEnumerator enumerator(base_startup_data_path, false,
                                       file_util::FileEnumerator::DIRECTORIES);
  for (FilePath checking_path = enumerator.Next();
       !checking_path.empty();
       checking_path = enumerator.Next()) {
    if (checking_path.BaseName().value() != v8_version)
      file_util::Delete(checking_path, true);
  }

  FilePath startup_data_path = GetStartupDataPath();
  success = file_util::CreateDirectory(startup_data_path);
  DCHECK(success);
  DoMappings(startup_data_path, true);
#endif
}

void V8StartupDataMapper::ChildProcessMapData() {
#if defined(V8_COMPRESS_STARTUP_DATA_BZ2)
  DoMappings(GetStartupDataPath(), false);
#endif
}

V8StartupDataMapper::V8StartupDataMapper() {
}

V8StartupDataMapper::~V8StartupDataMapper() {
}

void V8StartupDataMapper::DoMappings(const FilePath& startup_data_path,
                                     bool create_if_missing) {
#if defined(V8_COMPRESS_STARTUP_DATA_BZ2)
  DCHECK(mapped_files_.get() == NULL);
  int compressed_data_count = v8::V8::GetCompressedStartupDataCount();
  mapped_files_.reset(new file_util::MemoryMappedFile[compressed_data_count]);
  scoped_array<v8::StartupData> compressed_data(
      new v8::StartupData[compressed_data_count]);
  v8::V8::GetCompressedStartupData(compressed_data.get());

  for (int i = 0; i < compressed_data_count; ++i) {
    unsigned int decompressed_size = compressed_data[i].raw_size;
    FilePath::StringType v8_blob_name(base::IntToString(i));
    FilePath v8_blob_path(startup_data_path.Append(v8_blob_name));

    if (create_if_missing) {
      int64 file_size;
      bool exists = file_util::GetFileSize(v8_blob_path, &file_size);
      if (!exists || file_size != decompressed_size) {
        scoped_array<char> decompressed(new char[decompressed_size]);
        int result = BZ2_bzBuffToBuffDecompress(
            decompressed.get(),
            &decompressed_size,
            const_cast<char*>(compressed_data[i].data),
            compressed_data[i].compressed_size,
            0, 1);
        DCHECK(result == BZ_OK);
        DCHECK(decompressed_size == compressed_data[i].raw_size);
        int written = file_util::WriteFile(v8_blob_path, decompressed.get(),
                                           decompressed_size);
        DCHECK(written == static_cast<int>(decompressed_size));
      }
    }

    bool mapped = mapped_files_[i].Initialize(v8_blob_path);
    DCHECK(mapped && mapped_files_[i].length() == decompressed_size);
    compressed_data[i].data =
        reinterpret_cast<const char*>(mapped_files_[i].data());
    compressed_data[i].raw_size = decompressed_size;
  }
  v8::V8::SetDecompressedStartupData(compressed_data.get());
#endif
}
