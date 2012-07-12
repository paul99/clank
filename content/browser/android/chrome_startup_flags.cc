// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/chrome_startup_flags.h"

#include "base/string_split.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/common/content_switches.h"

namespace {

void SetCommandLineSwitch(const std::string& switch_string) {
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(switch_string))
    command_line->AppendSwitch(switch_string);
}

void SetPepperCommandLineFlags(std::string plugin_descriptor) {
  CommandLine* parsed_command_line = CommandLine::ForCurrentProcess();

  // Get the plugin info from the Java side. kRegisterPepperPlugins needs to be
  // added to the CommandLine before either PluginService::GetInstance() or
  // BrowserRenderProcessHost::AppendRendererCommandLine() is called to ensure
  // the plugin is available.
  // TODO(klobag) with the current implementation, the plugin can only be added
  // in the process start up time. Need to look into whether we can add the
  // plugin when the process is running.
  if (!plugin_descriptor.empty()) {
    // Usually the plugins are parsed by pepper_plugin_registry, but
    // flash needs two seperate command-line flags in order to work
    // when no mime-type is specified. The plugin string will look like this:
    // flash|others
    // Each plugin is specified like this:
    // path<#name><#description><#version>;mimetype
    std::vector<std::string> other_flash;
    base::SplitString(plugin_descriptor, '|', &other_flash);
    if (other_flash.size() == 2) {
      const std::string& other = other_flash[0];
      const std::string& flash = other_flash[1];
      parsed_command_line->AppendSwitchNative(
          switches::kRegisterPepperPlugins, other);
      std::vector<std::string> parts;
      base::SplitString(flash, ';', &parts);
      if (parts.size() >= 2) {
        std::vector<std::string> info;
        base::SplitString(parts[0], '#', &info);
        if (info.size() >= 4) {
          parsed_command_line->AppendSwitchNative(
              switches::kPpapiFlashPath, info[0]);
          parsed_command_line->AppendSwitchNative(
              switches::kPpapiFlashVersion, info[3]);
        }
      }
    }
  }
}

} // namespace

void SetAndroidCommandLineFlags(bool is_tablet,
                                const std::string& plugin_descriptor) {
  // May be called multiple times, to cover all possible program entry points.
  static bool already_initialized = false;
  if (already_initialized) return;
  already_initialized = true;

  CommandLine* parsed_command_line = CommandLine::ForCurrentProcess();

  // Set subflags for the --graphics-mode=XYZ omnibus flag.
  std::string graphics_mode;
  if (parsed_command_line->HasSwitch(switches::kGraphicsMode)) {
    graphics_mode = parsed_command_line->GetSwitchValueASCII(
        switches::kGraphicsMode);
  } else {
    // Default mode is threaded compositor mode
    graphics_mode = switches::kGraphicsModeValueCompositor;
    parsed_command_line->AppendSwitchNative(
        switches::kGraphicsMode, graphics_mode.c_str());
  }

  if (graphics_mode == switches::kGraphicsModeValueBasic) {
    // Intentionally blank.
  } else if (graphics_mode == switches::kGraphicsModeValueCompositor) {
    SetCommandLineSwitch(switches::kForceCompositingMode);
    SetCommandLineSwitch(switches::kEnableAcceleratedPlugins);
    SetCommandLineSwitch(switches::kEnableCompositingForFixedPosition);
    SetCommandLineSwitch(switches::kEnableThreadedCompositing);
    SetCommandLineSwitch(switches::kEnableThreadedAnimation);
    SetCommandLineSwitch(switches::kEnableAccelerated2dCanvas);
  } else {
    LOG(FATAL) << "Invalid --graphics-mode flag: " << graphics_mode;
  }

  // The "Synced Bookmarks" switch actually enables our Mobile Bookmarks folder.
  // Uses kCreateMobileBookmarksFolder from codereview.chromium.org/8786006.
  SetCommandLineSwitch(switches::kCreateMobileBookmarksFolder);

  // Turn on the new sync types by default.
  SetCommandLineSwitch(switches::kEnableSyncTabs);
  SetCommandLineSwitch(switches::kEnableSyncTabsForOtherClients);

  // Turn on autofill
  SetCommandLineSwitch(switches::kExternalAutofillPopup);

  // Tablet UI switch (used for using correct version of NTP HTML).
  if (is_tablet) {
    parsed_command_line->AppendSwitch(switches::kTabletUi);
  }

  // Load plugins out-of-process by default.
  // We always want flash out-of-process.
  parsed_command_line->AppendSwitch(switches::kPpapiOutOfProcess);

  // Run the GPU service as a thread in the browser instead of as a
  // standalone process.
  parsed_command_line->AppendSwitch(switches::kInProcessGPU);

  // Always fetch history thumbnails in the browser process
  parsed_command_line->AppendSwitch(switches::kEnableInBrowserThumbnailing);

  // Disable WebGL for now (See http://b/5634125)
  parsed_command_line->AppendSwitch(switches::kDisableExperimentalWebGL);

  // TODO(jcivelli): Enable the History Quick Provider and figure out
  //                 why it reports the wrong results for some pages.
  parsed_command_line->AppendSwitch(switches::kDisableHistoryQuickProvider);

  // Always used fixed layout (AKA overview mode).
  parsed_command_line->AppendSwitch(switches::kEnableFixedLayout);

  SetPepperCommandLineFlags(plugin_descriptor);
}
