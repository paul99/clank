// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/chrome_shell_delegate.h"

#include "chrome/browser/ui/ash/caps_lock_delegate_views.h"

ash::CapsLockDelegate* ChromeShellDelegate::CreateCapsLockDelegate() {
  return new CapsLockDelegate();
}
