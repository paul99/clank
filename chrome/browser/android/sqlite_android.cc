// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include <sqlite3.h>

// On Android we are using the system sqlite. The following fuctions have been
// added to Chrome's version of sqlite. Adding placeholders for now.
extern "C" void chromium_sqlite3_initialize_unix_sqlite3_file(sqlite3_file* file) {
  NOTIMPLEMENTED();
}

extern "C" int chromium_sqlite3_fill_in_unix_sqlite3_file(sqlite3_vfs* vfs, int fd,
                                               int dirfd, sqlite3_file* file,
                                               const char* fileName, int noLock) {
  NOTIMPLEMENTED();
  return -1;
}

extern "C" int chromium_sqlite3_get_reusable_file_handle(sqlite3_file* file,
                                              const char* fileName,
                                              int flags, int* fd) {
  NOTIMPLEMENTED();
  return -1;
}

extern "C" void chromium_sqlite3_update_reusable_file_handle(sqlite3_file* file,
                                                  int fd, int flags) {
  NOTIMPLEMENTED();
}

extern "C" void chromium_sqlite3_destroy_reusable_file_handle(sqlite3_file* file) {
  NOTIMPLEMENTED();
}
