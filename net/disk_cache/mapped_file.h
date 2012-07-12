// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// See net/disk_cache/disk_cache.h for the public interface of the cache.

#ifndef NET_DISK_CACHE_MAPPED_FILE_H_
#define NET_DISK_CACHE_MAPPED_FILE_H_
#pragma once

#include "net/base/net_export.h"
#include "net/disk_cache/disk_format.h"
#include "net/disk_cache/file.h"
#include "net/disk_cache/file_block.h"

class FilePath;

namespace disk_cache {

// This class implements a memory mapped file used to access block-files. The
// idea is that the header and bitmap will be memory mapped all the time, and
// the actual data for the blocks will be access asynchronously (most of the
// time).
class NET_EXPORT_PRIVATE MappedFile : public File {
 public:
  MappedFile() : File(true), init_(false) {}

  // Performs object initialization. name is the file to use, and size is the
  // ammount of data to memory map from th efile. If size is 0, the whole file
  // will be mapped in memory.
  void* Init(const FilePath& name, size_t size);

  void* buffer() const {
    return buffer_;
  }

  // Note: ScopedReadXXX may not be nested inside ScopedReadWrite as Flush does
  // not happen until the end.

  // Use this class to access the memory-mapped header without changing it.
  class ScopedReadOnly {
   public:
    explicit ScopedReadOnly(MappedFile* file) : file_(file) {
      file_->AssertClean();
    }
    // Can't make this const due to the loop idioms in block_files.cc -- we want
    // a ScopedReadOnly around the loop and a ScopedReadWrite inside the loop,
    // but the same non-const pointer is used in both places.
    void* buffer() {
      return file_->buffer_;
    }
    ~ScopedReadOnly() {
      file_->AssertClean();
    }
   private:
    MappedFile* file_;
  };

  // Use this class if you need to update the memory-mapped section.
  class ScopedReadWrite {
   public:
    explicit ScopedReadWrite(MappedFile* file) : file_(file) {
      file_->AssertClean();
    }
    void* buffer() {
      return file_->buffer_;
    }
    ~ScopedReadWrite() {
      file_->Flush();
    }
   private:
    MappedFile* file_;
  };

#if defined(OS_ANDROID)
  // Instead of calling mmap(), we read() the file in at creation time; when the
  // "memory-mapped" section is accessed, we use memcmp() to see if it changed
  // and write() it out if it has.

  // Assert that the memory-mapped section hasn't changed since the last Flush.
  void AssertClean();

  // Write any modified parts of the memory-mapped section to disk.
  void Flush();
#else
  // mmap() is available so these are no-ops.
  void AssertClean() {}
  void Flush() {}
#endif

  // Loads or stores a given block from the backing file (synchronously).
  bool Load(const FileBlock* block);
  bool Store(const FileBlock* block);

 private:
  virtual ~MappedFile();

  friend class ScopedReadOnly;
  friend class ScopedReadWrite;

  bool init_;
#if defined(OS_WIN)
  HANDLE section_;
#endif
  void* buffer_;  // Address of the memory mapped buffer.
  size_t view_size_;  // Size of the memory pointed by buffer_.

#if defined(OS_ANDROID)
  void* buffer_snapshot_;  // Original contents of the memory mapped buffer.
#endif

  DISALLOW_COPY_AND_ASSIGN(MappedFile);
};

}  // namespace disk_cache

#endif  // NET_DISK_CACHE_MAPPED_FILE_H_
