// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_V8_STARTUP_DATA_MAPPER_H_
#define CONTENT_BROWSER_ANDROID_V8_STARTUP_DATA_MAPPER_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"

namespace base {
template <typename Type>
struct DefaultLazyInstanceTraits;
}

namespace file_util {
class MemoryMappedFile;
}

class FilePath;

// Singleton to handle decompressing the V8 startup data and mapping it into
// each Chrome process.
class V8StartupDataMapper {
 public:
  // Returns the singleton instance (and initializes it if necessary).
  static V8StartupDataMapper& GetInstance();

  // Decompresses the V8 startup data if needed, cleans up old versions, and
  // maps it into the current process. This should be called by the browser
  // process before any children are created and before V8 is used.
  void BrowserProcessMapData();

  // Maps already-existing V8 startup data. This requires that the correct
  // version of the decompressed startup data is already present. This should be
  // called by child processes before V8 is used.
  void ChildProcessMapData();

 private:
  // Obtain an instance via GetInstance().
  V8StartupDataMapper();
  ~V8StartupDataMapper();
  friend struct base::DefaultLazyInstanceTraits<V8StartupDataMapper>;

  void DoMappings(const FilePath& startup_data_path, bool create_if_missing);

  scoped_array<file_util::MemoryMappedFile> mapped_files_;

  DISALLOW_COPY_AND_ASSIGN(V8StartupDataMapper);
};

#endif  // CONTENT_BROWSER_ANDROID_V8_STARTUP_DATA_MAPPER_H_
