// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "chrome/browser/shell_integration.h"

// static
bool ShellIntegration::CanSetAsDefaultBrowser() {
  NOTIMPLEMENTED();
  return false;
}

// static
bool ShellIntegration::SetAsDefaultBrowser() {
  NOTIMPLEMENTED();
  return false;
}

// static
bool ShellIntegration::SetAsDefaultProtocolClient(const std::string& protocol) {
  NOTIMPLEMENTED();
  return false;
}

// static
ShellIntegration::DefaultWebClientState ShellIntegration::IsDefaultBrowser() {
  NOTIMPLEMENTED();
  return UNKNOWN_DEFAULT_WEB_CLIENT;
}

//static
ShellIntegration::DefaultWebClientState
ShellIntegration::IsDefaultProtocolClient(const std::string& protocol) {
  NOTIMPLEMENTED();
  return UNKNOWN_DEFAULT_WEB_CLIENT;
}

// static
bool ShellIntegration::IsFirefoxDefaultBrowser() {
  return false;
}
