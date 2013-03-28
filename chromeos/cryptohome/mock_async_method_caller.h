// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_CRYPTOHOME_MOCK_ASYNC_METHOD_CALLER_H_
#define CHROMEOS_CRYPTOHOME_MOCK_ASYNC_METHOD_CALLER_H_

#include <string>

#include "base/basictypes.h"
#include "base/callback.h"
#include "chromeos/cryptohome/async_method_caller.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace cryptohome {

class MockAsyncMethodCaller : public AsyncMethodCaller {
 public:
  MockAsyncMethodCaller();
  virtual ~MockAsyncMethodCaller();

  void SetUp(bool success, MountError return_code);

  MOCK_METHOD3(AsyncCheckKey, void(const std::string& user_email,
                                   const std::string& passhash,
                                   Callback callback));
  MOCK_METHOD4(AsyncMigrateKey, void(const std::string& user_email,
                                     const std::string& old_hash,
                                     const std::string& new_hash,
                                     Callback callback));
  MOCK_METHOD4(AsyncMount, void(const std::string& user_email,
                                const std::string& passhash,
                                int flags,
                                Callback callback));
  MOCK_METHOD1(AsyncMountGuest, void(Callback callback));
  MOCK_METHOD2(AsyncRemove, void(const std::string& user_email,
                                 Callback callback));

 private:
  bool success_;
  MountError return_code_;

  void DoCallback(Callback callback);

  DISALLOW_COPY_AND_ASSIGN(MockAsyncMethodCaller);
};

}  // namespace cryptohome

#endif  // CHROMEOS_CRYPTOHOME_MOCK_ASYNC_METHOD_CALLER_H_
