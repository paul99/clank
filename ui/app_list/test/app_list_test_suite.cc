// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/test/app_list_test_suite.h"

#include "ui/base/ui_base_paths.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/compositor_setup.h"
#include "ui/compositor/test/compositor_test_support.h"

namespace app_list {
namespace test {

AppListTestSuite::AppListTestSuite(int argc, char** argv)
    : base::TestSuite(argc, argv) {
}

void AppListTestSuite::Initialize() {
  base::TestSuite::Initialize();

  ui::RegisterPathProvider();

  // Force unittests to run using en-US so if we test against string
  // output, it'll pass regardless of the system language.
  ui::ResourceBundle::InitSharedInstanceWithLocale("en-US", NULL);

  ui::CompositorTestSupport::Initialize();
  ui::SetupTestCompositor();
}

void AppListTestSuite::Shutdown() {
  ui::CompositorTestSupport::Terminate();
  ui::ResourceBundle::CleanupSharedInstance();

  base::TestSuite::Shutdown();
}

}  // namespace test
}  // namespace app_list
