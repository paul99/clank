// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/chrome_library_loader_hooks.h"

#include <android/bitmap.h>
#include <vector>

#include "base/android/build_info.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/path_utils.h"
#include "base/command_line.h"
#include "base/debug/trace_event.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/string_tokenizer.h"
#include "base/string_util.h"
#include "base/tracked_objects.h"
#include "chrome/browser/android/js_modal_dialog_android.h"
#include "chrome/browser/speech/speech_input_manager_android.h"
#include "clank/native/framework/chrome/chrome_browser_provider.h"
#include "clank/native/framework/chrome/network_connectivity_receiver.h"
#include "clank/native/framework/chrome/website_settings_utils.h"
#include "clank/native/framework/third_party/etc1.h"
#include "content/browser/android/device_info.h"
#include "content/browser/android/browser_process_main.h"
#include "content/browser/android/chrome_video_view.h"
#include "content/browser/android/chrome_view.h"
#include "content/browser/android/chrome_view_client.h"
#include "content/browser/android/command_line.h"
#include "content/browser/android/download_controller.h"
#include "content/browser/android/media_player_listener.h"
#include "content/browser/android/remote_debugging_controller.h"
#include "content/browser/android/sandboxed_process_service.h"
#include "content/browser/android/surface_texture_listener.h"
#include "content/browser/android/touch_point.h"
#include "content/browser/android/trace_event_binding.h"
#include "content/browser/android/user_agent.h"
#include "content/browser/android/waitable_native_event.h"
#include "content/browser/android/web_settings.h"
#include "content/browser/android/sandboxed_process_launcher.h"
#include "content/browser/renderer_host/ime_adapter_android.h"
#include "content/public/common/content_switches.h"
#include "content/renderer/android/memory_threshold.h"
#include "net/android/network_library.h"
#include "net/proxy/proxy_config_service_android.h"

bool RegisterJniHelper(JNIEnv* env);

base::AtExitManager* g_at_exit_manager = NULL;

namespace base {
bool RegisterSystemMessageHandler(JNIEnv* env);
}

// The following arrays list the registration methods for all native methods
// required by either ChromeView or tests. They are registered in all
// processes. Native methods required only by the Clank application in the
// browser process should be registered in InitBrowserProcess().
//
// TODO(steveblock): Many of these native methods are required only in the
// browser process. We should consider moving them to InitBrowserProcess().
// This move will be easier after upstreaming, once some of the native methods
// have been moved to base/.

static RegistrationMethod kRegisteredMethods[] = {
  { "DeviceInfo", RegisterDeviceInfo },
  { "BrowserProcessMain", RegisterBrowserProcessMain },
  { "ChromeBrowserProvider", RegisterChromeBrowserProvider },
  { "ChromeHttpAuthHandler", RegisterChromeHttpAuthHandler },
  { "ChromeVideoView", RegisterChromeVideoView },
  { "ChromeView", RegisterChromeView },
  { "ChromeViewClient", RegisterChromeViewClient },
  { "CommandLine", RegisterCommandLine },
  { "DownloadController", RegisterDownloadController },
  { "ETC1", RegisterETC1 },
  { "JNIHelper", RegisterJniHelper },
  { "JSModalDialog", RegisterJSModalDialog },
  { "MediaPlayerListener", RegisterMediaPlayerListener },
  { "NetworkConnectivityReceiever", RegisterNetworkConnectivityReceiver },
  { "RegisterImeAdapter", RegisterImeAdapter },
  { "RemoteDebuggingController", RegisterRemoteDebuggingController },
  { "SandboxedProcessService", RegisterSandboxedProcessService },
  { "SpeechInputManager", speech_input::RegisterSpeechInputManager },
  { "SurfaceTextureListener", RegisterSurfaceTextureListener },
  { "TouchPoint", RegisterTouchPoint },
  { "TraceEvent", RegisterTraceEvent },
  { "UserAgent", RegisterUserAgent },
  { "WaitableNativeEvent", RegisterWaitableNativeEvent },
  { "WebSettings", RegisterWebSettings },
  { "WebsiteSettingsUtils", RegisterWebsiteSettingsUtils },
};

static RegistrationMethod kBaseRegisteredMethods[] = {
  { "BuildInfo", base::android::RegisterBuildInfo },
  { "PathUtils", base::android::RegisterPathUtils },
  { "SystemMessageHandler", base::RegisterSystemMessageHandler },
};

static RegistrationMethod kNetRegisteredMethods[] = {
  { "AndroidNetworkLibrary", net::android::RegisterNetworkLibrary},
  { "ProxyConfigService", net::ProxyConfigServiceAndroid::RegisterProxyConfigService},
};

static RegistrationMethod kContentRegisteredMethods[] = {
  { "MemoryThreshold", content::renderer::android::RegisterMemoryThreshold},
  { "SandboxedProcessLauncher",
      content::browser::android::RegisterSandboxedProcessLauncher },
};

// Return true if the registration of the given methods succeed.
static bool RegisterNativeMethods(JNIEnv* env,
                                  const RegistrationMethod* method,
                                  size_t count) {
  const RegistrationMethod* end = method + count;
  while (method != end) {
    if (!method->func(env) < 0) {
      DLOG(ERROR) << method->name << " failed registration!";
      return false;
    }
    method++;
  }
  return true;
}

jboolean LibraryLoaderEntryHook(JNIEnv* env, jclass clazz,
                                jobjectArray init_command_line) {
  // We need the Chrome AtExitManager to be created before we do any tracing or
  // logging.
  g_at_exit_manager = new base::AtExitManager();
  InitNativeCommandLineFromJavaArray(env, init_command_line);

  CommandLine* command_line = CommandLine::ForCurrentProcess();

  if (command_line->HasSwitch(switches::kTraceStartup)) {
    base::debug::TraceLog::GetInstance()->SetEnabled(
        command_line->GetSwitchValueASCII(switches::kTraceStartup));
  }

  // Can only use event tracing after setting up the command line.
  TRACE_EVENT0("jni", "JNI_OnLoad continuation");

  logging::InitLogging(NULL,
                       logging::LOG_ONLY_TO_SYSTEM_DEBUG_LOG,
                       logging::DONT_LOCK_LOG_FILE,
                       logging::DELETE_OLD_LOG_FILE,
                       logging::ENABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS);
  // To view log output with IDs and timestamps use "adb logcat -v threadtime".
  logging::SetLogItems(false,    // Process ID
                       false,    // Thread ID
                       false,    // Timestamp
                       false);   // Tick count
  VLOG(0) << "Chromium logging enabled: level = " << logging::GetMinLogLevel()
          << ", default verbosity = " << logging::GetVlogVerbosity();

  if (!RegisterNativeMethods(env, kBaseRegisteredMethods,
                             arraysize(kBaseRegisteredMethods)))
    return JNI_FALSE;

  if (!RegisterNativeMethods(env, kNetRegisteredMethods,
                             arraysize(kNetRegisteredMethods)))
    return JNI_FALSE;

  if (!RegisterNativeMethods(env, kRegisteredMethods,
                             arraysize(kRegisteredMethods)))
    return JNI_FALSE;

  if (!RegisterNativeMethods(env, kContentRegisteredMethods,
                             arraysize(kContentRegisteredMethods)))
    return JNI_FALSE;

  JavaVM* vm;
  env->GetJavaVM(&vm);
  base::android::InitVM(vm);
  return JNI_TRUE;
}

void LibraryLoaderExitHook() {
  delete g_at_exit_manager;
  g_at_exit_manager = NULL;
}
