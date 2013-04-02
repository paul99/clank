// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/options/managed_user_set_passphrase_test.h"

#include "base/command_line.h"
#include "chrome/common/chrome_switches.h"

ManagedUserSetPassphraseTest::ManagedUserSetPassphraseTest() {
}

ManagedUserSetPassphraseTest::~ManagedUserSetPassphraseTest() {
}

void ManagedUserSetPassphraseTest::SetUpCommandLine(CommandLine* command_line) {
  command_line->AppendSwitch(switches::kManaged);
  command_line->AppendSwitch(switches::kEnableManagedUsers);
}
