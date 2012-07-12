// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_DALVIK_VM_HEAP_SIZE_H_
#define BASE_ANDROID_DALVIK_VM_HEAP_SIZE_H_

#include <string>

namespace base {
namespace android {

// A simple class to cache the heap size of the device.
class DalvikHeapSize {
 public:
  DalvikHeapSize();

  ~DalvikHeapSize();

  int HeapSizeMB() const;

 private:
  int ParseHeapSize(const std::string& str) const;
  int heap_size_mb_;
};

}  // namespace base
}  // namespace android

#endif  // BASE_ANDROID_DALVIK_VM_HEAP_SIZE_ANDROID_H_
