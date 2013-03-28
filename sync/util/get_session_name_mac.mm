// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/util/get_session_name_mac.h"

#import <SystemConfiguration/SCDynamicStoreCopySpecific.h>
#include <sys/sysctl.h>

#include "base/mac/scoped_cftyperef.h"
#include "base/string_util.h"
#include "base/sys_string_conversions.h"

namespace syncer {
namespace internal {

std::string GetHardwareModelName() {
  // Do not use NSHost currentHost, as it's very slow. http://crbug.com/138570
  SCDynamicStoreContext context = { 0, NULL, NULL, NULL };
  base::mac::ScopedCFTypeRef<SCDynamicStoreRef> store(
      SCDynamicStoreCreate(kCFAllocatorDefault, CFSTR("chrome_sync"),
                           NULL, &context));
  base::mac::ScopedCFTypeRef<CFStringRef> machine_name(
      SCDynamicStoreCopyLocalHostName(store.get()));
  if (machine_name.get())
    return base::SysCFStringRefToUTF8(machine_name.get());

  // Fall back to get computer name.
  base::mac::ScopedCFTypeRef<CFStringRef> computer_name(
      SCDynamicStoreCopyComputerName(store.get(), NULL));
  if (computer_name.get())
    return base::SysCFStringRefToUTF8(computer_name.get());

  // If all else fails, return to using a slightly nicer version of the
  // hardware model.
  char modelBuffer[256];
  size_t length = sizeof(modelBuffer);
  if (!sysctlbyname("hw.model", modelBuffer, &length, NULL, 0)) {
    for (size_t i = 0; i < length; i++) {
      if (IsAsciiDigit(modelBuffer[i]))
        return std::string(modelBuffer, 0, i);
    }
    return std::string(modelBuffer, 0, length);
  }
  return "Unknown";
}

}  // namespace internal
}  // namespace syncer
