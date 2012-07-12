// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/mapped_file.h"

#include <errno.h>
#if defined(OS_ANDROID)
#include <stdlib.h>
#else
#include <sys/mman.h>
#endif

#if defined(OS_ANDROID)
#include <algorithm>
#endif

#if defined(OS_ANDROID)
#include "base/debug/trace_event.h"
#endif
#include "base/file_path.h"
#include "base/logging.h"
#include "net/disk_cache/disk_cache.h"

namespace disk_cache {

void* MappedFile::Init(const FilePath& name, size_t size) {
  DCHECK(!init_);
  if (init_ || !File::Init(name))
    return NULL;

  if (!size)
    size = GetLength();

#if defined(OS_ANDROID)
  buffer_ = malloc(size);
  buffer_snapshot_ = malloc(size);
  DCHECK(buffer_);
  DCHECK(buffer_snapshot_);
  if (buffer_ && buffer_snapshot_) {
    Read(buffer_, size, 0);
    memcpy(buffer_snapshot_, buffer_, size);
  } else {
    buffer_ = buffer_snapshot_ = 0;
  }
#else
  buffer_ = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                 platform_file(), 0);
#endif

  init_ = true;
  DCHECK(reinterpret_cast<intptr_t>(buffer_) != -1);
  if (reinterpret_cast<intptr_t>(buffer_) == -1)
    buffer_ = 0;

  view_size_ = size;
  return buffer_;
}

bool MappedFile::Load(const FileBlock* block) {
  size_t offset = block->offset() + view_size_;
  return Read(block->buffer(), block->size(), offset);
}

bool MappedFile::Store(const FileBlock* block) {
  size_t offset = block->offset() + view_size_;
  return Write(block->buffer(), block->size(), offset);
}

#if defined(OS_ANDROID)
void MappedFile::AssertClean() {
  DCHECK(init_);
  DCHECK(buffer_);
  DCHECK(buffer_snapshot_);
  DCHECK_EQ(0, memcmp(buffer_snapshot_, buffer_, view_size_));
}

void MappedFile::Flush() {
  DCHECK(init_);
  DCHECK(buffer_);
  DCHECK(buffer_snapshot_);
  const char* buffer_ptr = static_cast<const char*>(buffer_);
  char* snapshot_ptr = static_cast<char*>(buffer_snapshot_);
  const size_t block_size = 4096;
  for (size_t offset = 0; offset < view_size_; offset += block_size) {
    size_t size = std::min(view_size_ - offset, block_size);
    if (memcmp(snapshot_ptr + offset, buffer_ptr + offset, size)) {
      memcpy(snapshot_ptr + offset, buffer_ptr + offset, size);
      Write(snapshot_ptr + offset, size, offset);
    }
  }
}
#endif

MappedFile::~MappedFile() {
  if (!init_)
    return;

#if defined(OS_ANDROID)
  if (buffer_ && buffer_snapshot_) {
    Flush();
  }
  free(buffer_);
  free(buffer_snapshot_);
#else
  if (buffer_) {
    int ret = munmap(buffer_, view_size_);
    DCHECK_EQ(0, ret);
  }
#endif
}

}  // namespace disk_cache
