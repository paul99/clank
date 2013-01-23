// Copyright (c) 2010 Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef CLIENT_LINUX_HANDLER_MINIDUMP_DESCRIPTOR_H_
#define CLIENT_LINUX_HANDLER_MINIDUMP_DESCRIPTOR_H_

#include <assert.h>
#include <stdio.h>

// The MinidumpDescriptor describes how to access a Minidump file, either
// with a path or with a file descriptor.

namespace google_breakpad {

class MinidumpDescriptor {
 public:
  MinidumpDescriptor() : path_(NULL), fd_(-1) {}

  explicit MinidumpDescriptor(const char* path) : path_(path), fd_(-1) {
    assert(path_);
  }

  explicit MinidumpDescriptor(const int fd) : path_(NULL), fd_(fd) {
    assert(fd != -1);
  }

  bool IsFD() const { return !path_; }

  int fd() const { return fd_; }
  void set_fd(const int fd) { fd_ = fd;}

  const char* path() const { return path_; }

 private:
  const char* path_;
  int fd_;
};

}  // namespace google_breakpad

#endif  // CLIENT_LINUX_HANDLER_MINIDUMP_DESCRIPTOR_H_
