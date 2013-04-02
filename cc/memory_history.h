// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_MEMORY_HISTORY_H_
#define CC_MEMORY_HISTORY_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/time.h"
#include "cc/ring_buffer.h"

namespace cc {

// Maintains a history of memory for each frame
class MemoryHistory {
 public:
  static scoped_ptr<MemoryHistory> create();

  size_t HistorySize() const { return ring_buffer_.BufferSize(); }

  struct Entry {
  Entry()
      : total_budget_in_bytes(0),
        bytes_allocated(0),
        bytes_unreleasable(0),
        bytes_over(0) { }

      size_t total_budget_in_bytes;
      size_t bytes_allocated;
      size_t bytes_unreleasable;
      size_t bytes_over;
      size_t bytes_total() const {
          return bytes_allocated +
              bytes_unreleasable +
              bytes_over;
      }
  };

  // n = 0 returns the oldest and
  // n = HistorySize() - 1 the most recent paint time.
  Entry GetEntry(const size_t& n) const;

  void SaveEntry(const Entry& entry);
  void GetMinAndMax(size_t* min, size_t* max) const;

 private:
  MemoryHistory();

  RingBuffer<Entry, 80> ring_buffer_;

  DISALLOW_COPY_AND_ASSIGN(MemoryHistory);
};

}  // namespace cc

#endif  // CC_MEMORY_HISTORY_H_
