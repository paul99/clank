// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/dalvik_heap_size.h"

#include <sys/system_properties.h>

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/string_number_conversions.h"

namespace base {
namespace android {

DalvikHeapSize::DalvikHeapSize() : heap_size_mb_(0) {
  char heap_size_str[PROP_VALUE_MAX];
  __system_property_get("dalvik.vm.heapsize", heap_size_str);
  heap_size_mb_ = ParseHeapSize(std::string(heap_size_str));
}

DalvikHeapSize::~DalvikHeapSize() {}

int DalvikHeapSize::HeapSizeMB() const {
  return heap_size_mb_;
}

int DalvikHeapSize::ParseHeapSize(const std::string& str) const {
  const int64 KB = 1024;
  const int64 MB = 1024 * KB;
  const int64 GB = 1024 * MB;
  CHECK_GT(str.size(), 0u);
  int64 factor = 1;
  size_t length = str.size();
  if (str[length - 1] == 'k') {
    factor = KB;
    length--;
  } else if (str[length - 1] == 'm') {
    factor = MB;
    length--;
  } else if (str[length - 1] == 'g') {
    factor = GB;
    length--;
  } else {
    CHECK('0' <= str[length - 1] && str[length - 1] <= '9');
  }
  int64 result = 0;
  bool parsed = base::StringToInt64(str.substr(0, length), &result);
  CHECK(parsed);
  result = result * factor / MB;
  // dalvik.vm.heapsize property is writable by user,
  // truncate it to reasonable value to avoid overflows later.
  result = std::min<int64>(std::max<int64>(32, result), 1024);
  return static_cast<int>(result);
}

}  // namespace android
}  // namespace base
