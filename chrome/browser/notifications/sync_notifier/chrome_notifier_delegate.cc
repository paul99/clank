// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/sync_notifier/chrome_notifier_delegate.h"

namespace notifier {
ChromeNotifierDelegate::ChromeNotifierDelegate(const std::string& id)
    : id_(id) {}

ChromeNotifierDelegate::~ChromeNotifierDelegate() {}

std::string ChromeNotifierDelegate::id() const {
  return id_;
}

content::RenderViewHost* ChromeNotifierDelegate::GetRenderViewHost() const {
    return NULL;
}

void ChromeNotifierDelegate::Close(bool by_user) {
  // TODO(petewil): implement
}

}  // namespace notifier
