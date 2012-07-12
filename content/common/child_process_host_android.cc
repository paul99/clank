// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/child_process_host_impl.h"

#include "base/android/jni_android.h"
#include "base/android/path_utils.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/global_descriptors_posix.h"
#include "base/path_service.h"
#include "base/stringprintf.h"
#include "base/string_number_conversions.h"
#include "base/string_split.h"
#include "content/common/chrome_descriptors.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "ui/base/ui_base_switches.h"

namespace {
const char kPropertyWorkspaceEnvVariable[] = "ANDROID_PROPERTY_WORKSPACE";
}

namespace content {

// static
void ChildProcessHostImpl::AppendCommonChildCommandLine(CommandLine* cmd_line) {
  // The child process needs some essential information very early during its
  // life. We pass these as arguments
  FilePath file_path;
  PathService::Get(base::DIR_ANDROID_APP_DATA, &file_path);
  DCHECK(!file_path.empty());
  cmd_line->AppendSwitchPath(switches::kDataDirectory, file_path);
  // Pass on the browser locale.
  const std::string locale =
      content::GetContentClient()->browser()->GetApplicationLocale();
  cmd_line->AppendSwitchASCII(switches::kLang, locale);
  PathService::Get(base::DIR_CACHE, &file_path);
  DCHECK(!file_path.empty());
  cmd_line->AppendSwitchPath(switches::kCacheDirectory, file_path);
}

// static
void ChildProcessHostImpl::GetPropertyWorkspace(int* fd, int* size) {
  scoped_ptr<base::Environment> env_get(base::Environment::Create());
  std::string workspace_value;
  CHECK(env_get->GetVar(kPropertyWorkspaceEnvVariable, &workspace_value));
  CHECK(!workspace_value.empty());

  std::vector<std::string> values;
  base::SplitString(workspace_value, ',', &values);
  CHECK_EQ(values.size(), 2u);

  if (fd) {
    CHECK(base::StringToInt(values[0], fd));
  }
  if (size) {
    CHECK(base::StringToInt(values[1], size));
  }
}

// static
void ChildProcessHostImpl::FillChildEnvironment(base::environment_vector* env) {
  DCHECK(env != NULL);

  // Set up the environment variable used by Android to find the system
  // properties. The ChildProcessLauncher will map this processes fd to
  // the well defined kAndroidPropertyDescriptor for us.
  const int property_fd = kAndroidPropertyDescriptor +
      base::GlobalDescriptors::kBaseDescriptor;
  int property_size;
  GetPropertyWorkspace(NULL, &property_size);
  const std::string property_workspace =
      base::StringPrintf("%d,%d", property_fd, property_size);
  env->push_back(std::make_pair(kPropertyWorkspaceEnvVariable,
                                property_workspace));
}

}  // namespece content
