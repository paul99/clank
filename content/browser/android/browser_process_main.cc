// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/browser_process_main.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/at_exit.h"
#include "base/debug/debugger.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/command_line.h"
#include "base/metrics/histogram.h"
#include "base/path_service.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/system_monitor/system_monitor.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/plugin_prefs.h"
#include "chrome/browser/automation/automation_provider_list.h"
#include "chrome/browser/automation/testing_automation_provider_android.h"
#include "chrome/browser/browser_about_handler.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/browser_shutdown.h"
#include "chrome/browser/chrome_browser_main.h"
#include "chrome/browser/chrome_browser_main_android.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/metrics/histogram_synchronizer.h"
#include "chrome/browser/metrics/metrics_log.h"
#include "chrome/browser/metrics/metrics_service.h"
#include "chrome/browser/ui/webui/chrome_url_data_manager.h"
#include "chrome/browser/ui/webui/chrome_url_data_manager_backend.h"
#include "chrome/browser/extensions/extension_protocols.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/renderer/chrome_content_renderer_client.h"
#include "clank/native/framework/chrome/android_protocol_adapter.h"
#include "clank/native/framework/chrome/application_native_methods.h"
#include "content/browser/android/command_line.h"
#include "content/browser/android/chrome_library_loader_hooks.h"
#include "content/browser/android/chrome_startup_flags.h"
#include "content/browser/android/devtools_server.h"
#include "content/browser/android/surface_texture_peer.h"
#include "content/browser/android/user_agent.h"
#include "content/browser/android/v8_startup_data_mapper.h"
#include "content/browser/android/sandboxed_process_launcher.h"
#include "content/browser/browser_process_sub_thread.h"
#include "content/browser/browser_thread_impl.h"
#include "content/browser/device_orientation/device_orientation_android.h"
#include "content/browser/geolocation/android_location_api_adapter.h"
#include "content/browser/in_process_webkit/webkit_thread.h"
#include "content/browser/notification_service_impl.h"
#include "content/gpu/gpu_process.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/content_paths.h"
#include "content/public/browser/notification_service.h"
#include "jni/browser_process_main_jni.h"
#include "net/android/network_change_notifier_factory.h"
#include "net/base/cookie_monster.h"
#include "net/http/http_network_layer.h"
#include "net/http/http_stream_factory.h"
#include "net/socket/client_socket_pool.h"
#include "net/socket/client_socket_pool_manager.h"
#include "ui/base/resource/resource_bundle.h"
#include "webkit/glue/user_agent.h"

#if defined(USE_LINUX_BREAKPAD)
#include "chrome/app/breakpad_posix.h"
#endif

#include <sys/resource.h>

using base::android::CheckException;
using base::android::ConvertJavaStringToUTF8;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;

namespace {

class SurfaceTexturePeerBrowserImpl : public SurfaceTexturePeer {
 public:
  SurfaceTexturePeerBrowserImpl() {
  }

  virtual ~SurfaceTexturePeerBrowserImpl() {
  }

  virtual void EstablishSurfaceTexturePeer(base::ProcessHandle pid,
                                           SurfaceTextureTarget type,
                                           jobject j_surface_texture,
                                           int primary_id,
                                           int secondary_id) {
    JNIEnv* env = base::android::AttachCurrentThread();
    DCHECK(env);
    Java_BrowserProcessMain_establishSurfaceTexturePeer(env, pid, type,
        j_surface_texture, primary_id, secondary_id);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SurfaceTexturePeerBrowserImpl);
};

// This class is a observer notified when render process is closed. It will
// stop the sandboxed processes once get the notification.
class RendererProcessClosedObserver : public content::NotificationObserver {
 public:
  RendererProcessClosedObserver() {
  }

  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) {
    switch (type) {
      case content::NOTIFICATION_RENDERER_PROCESS_CLOSED: {
        base::ProcessHandle handle =
            content::Details<content::RenderProcessHost::RendererClosedDetails>(
                details)->handle;
        // Currently there is no way to detect whether the renderer process
        // closed as a normal termination or crashed. So always call
        // StopSandboxedProcess and it is ok as the non-existed pid will not do
        // any harm but triggers a warning.
        if (handle != base::kNullProcessHandle)
          content::browser::android::StopSandboxedProcess(handle);
        break;
      }
      default:
        NOTREACHED() << "Unexpected notification";
        break;
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(RendererProcessClosedObserver);
};

MessageLoop* g_ui_loop = NULL;
base::SystemMonitor* g_system_monitor = NULL;
content::BrowserThreadImpl* g_ui_thread = NULL;

content::NotificationService* g_main_notification_service = NULL;

// The registrar used to register/unregister render process closed observer.
content::NotificationRegistrar* g_registrar = NULL;

// The observer used for observing renderer closed notifications.
RendererProcessClosedObserver* g_renderer_process_closed_observer = NULL;

HistogramSynchronizer* g_histogram_synchronizer = NULL;
base::StatisticsRecorder* g_statistics_recorder = NULL;

base::LazyInstance<chrome::ChromeContentClient> g_chrome_content_client =
  LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<chrome::ChromeContentBrowserClient> g_browser_client =
  LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<chrome::ChromeContentRendererClient> g_renderer_client =
  LAZY_INSTANCE_INITIALIZER;

}  // namespace <anonymous>

// Declared in chrome/browser/chrome_browser_main_android.h and expected to be
// implemented by platform-specific code.
// TODO(yfriedman): Replace with BrowserMainParts hierarchy.
// See http://b/5632260
void DidEndMainMessageLoopImpl() {
  MetricsService* metrics = g_browser_process->metrics_service();
  if (metrics) {
    metrics->Stop();
  }
  if (g_histogram_synchronizer) {
    g_histogram_synchronizer->Release();
    g_histogram_synchronizer = NULL;
  }
  if (g_statistics_recorder) {
    delete g_statistics_recorder;
    g_statistics_recorder = NULL;
  }

  // browser_shutdown takes care of deleting g_browser_process
  // TODO(wangxianzhu): should conform to the new browser_shutdown protocol.
  browser_shutdown::ShutdownPostThreadsStop(
      browser_shutdown::ShutdownPreThreadsStop());
  delete g_main_notification_service;
  g_main_notification_service = NULL;
  delete g_ui_thread;
  g_ui_thread = NULL;
  delete g_system_monitor;
  g_system_monitor = NULL;
  delete g_ui_loop;
  g_ui_loop = NULL;
  LibraryLoaderExitHook();
  delete g_registrar;
  g_registrar = NULL;
  delete g_renderer_process_closed_observer;
  g_renderer_process_closed_observer = NULL;
}

namespace {

// Constants for accessing the shared preferences.  Must be kept in sync with
// the constants defined in ChromeBaseActivity.java.
const char kPrefsFileName[] = "ChromePrefs";
const char kPrefsCleanShutdownKey[] = "cleanShutdown";

bool GetIntSwitchValue(const CommandLine& parsed_command_line,
                       const char* switch_name,
                       int min,
                       int max,
                       int* result) {
  if (parsed_command_line.HasSwitch(switch_name)) {
    std::string string = parsed_command_line.GetSwitchValueASCII(switch_name);
    int value;
    if (!base::StringToInt(string, &value)) {
      LOG(WARNING) << "Could not parse " << switch_name << ": " << string;
      return false;
    }
    if (value < min || value > max) {
      LOG(WARNING) << switch_name << " is out of range [" << min << ","
                   << max << "]. Will be ignored.";
      return false;
    }
    *result = value;
    return true;
  }
  return false;
}

// Referenced browser_main_loop.cc.
void InitializeThreads() {
  DCHECK(g_browser_process);
  BrowserProcessImpl* browser_process =
      static_cast<BrowserProcessImpl*>(g_browser_process);
  browser_process->PreCreateThreads();

  base::Thread::Options default_options;
  base::Thread::Options io_message_loop_options;
  io_message_loop_options.message_loop_type = MessageLoop::TYPE_IO;

  // Start threads in the order they occur in the BrowserThread::ID
  // enumeration, except for BrowserThread::UI which is the main
  // thread.
  //
  // Must be size_t so we can increment it.
  for (size_t thread_id = BrowserThread::UI + 1;
       thread_id < BrowserThread::ID_COUNT;
       ++thread_id) {
    BrowserThread::ID id = static_cast<BrowserThread::ID>(thread_id);
    switch (thread_id) {
      case BrowserThread::WEBKIT_DEPRECATED:
        (new content::WebKitThread)->Initialize();
        break;
      case BrowserThread::FILE:
      case BrowserThread::CACHE:
      case BrowserThread::IO:
        (new content::BrowserProcessSubThread(id))->StartWithOptions(
            io_message_loop_options);
        break;
      case BrowserThread::DB:
      case BrowserThread::PROCESS_LAUNCHER:
      case BrowserThread::FILE_USER_BLOCKING:
        (new content::BrowserProcessSubThread(id))->StartWithOptions(
            default_options);
        break;
      default:
        NOTREACHED();
        break;
    }
  }

  browser_process->BrowserThreadsStarted();
}

// Copied from browser_main.cc
void InitializeNetworkOptions(const CommandLine& parsed_command_line) {
  if (parsed_command_line.HasSwitch(switches::kEnableFileCookies)) {
    // Enable cookie storage for file:// URLs.  Must do this before the first
    // Profile (and therefore the first CookieMonster) is created.
    net::CookieMonster::EnableFileScheme();
  }
  if (parsed_command_line.HasSwitch(switches::kIgnoreCertificateErrors)) {
    net::HttpStreamFactory::set_ignore_certificate_errors(true);
  }
  int value;
  if (GetIntSwitchValue(parsed_command_line, switches::kMaxSocketsPerGroup,
                        1, 99, &value)) {
    net::ClientSocketPoolManager::set_max_sockets_per_group(value);
  }
  if (GetIntSwitchValue(parsed_command_line, switches::kMaxSocketsPerPool,
                        1, 999, &value)) {
    net::ClientSocketPoolManager::set_max_sockets_per_pool(value);
  }
  if (GetIntSwitchValue(parsed_command_line,
                        switches::kMaxSocketsPerProxyServer,
                        1, 99, &value)) {
    net::ClientSocketPoolManager::set_max_sockets_per_proxy_server(value);
  }
  if (GetIntSwitchValue(parsed_command_line,
                        switches::kUnusedIdleSocketTimeoutSecs,
                        1, 1000, &value)) {
    net::ClientSocketPool::set_unused_idle_socket_timeout(
        base::TimeDelta::FromSeconds(value));
  }
  if (GetIntSwitchValue(parsed_command_line,
                        switches::kUsedIdleSocketTimeoutSecs,
                        1, 1000, &value)) {
    net::ClientSocketPool::set_used_idle_socket_timeout(
        base::TimeDelta::FromSeconds(value));
  }
}

// This captures the process startup done in ChromeMain() and BrowserMain(). We
// use this separate function to minimize the work needed to be done for Android
// to reduce the code size.
// TODO(yfriedman): Refactor to re-use the BrowserMainParts hierarchy instead of
// copying code out of BrowserMain. See http://b/5632260
void ChromeViewBrowserMain(
    int max_render_process_count, JNIEnv* env, jobject context) {
  if (g_browser_process)
    return;

  // The following are the minimum implementation of ChromeMain()
  const CommandLine& parsed_command_line = *CommandLine::ForCurrentProcess();
  if (parsed_command_line.HasSwitch(switches::kWaitForDebugger)) {
    LOG(ERROR) << "Browser waiting for GDB because flag "
               << switches::kWaitForDebugger << " was supplied.";
    base::debug::WaitForDebugger(24*60*60, false);
  }

  // Support gcov code coverage. In order for gcov to work, we need to
  // explicilty export GCOV_PREFIX to the environment.
  if (parsed_command_line.HasSwitch(switches::kGcovPrefix)) {
    const std::string gcov_prefix =
        parsed_command_line.GetSwitchValueASCII(switches::kGcovPrefix);
    setenv("GCOV_PREFIX", gcov_prefix.c_str(), 1);
  }
  if (parsed_command_line.HasSwitch(switches::kGcovPrefixStrip)) {
    const std::string gcov_prefix_strip =
        parsed_command_line.GetSwitchValueASCII(switches::kGcovPrefixStrip);
    setenv("GCOV_PREFIX", gcov_prefix_strip.c_str(), 1);
  }

  // Register internal Chrome schemes so they'll be parsed correctly. This must
  // happen before we process any URLs with the affected schemes, and must be
  // done in all processes that work with these URLs (i.e. including renderers).
  chrome::RegisterChromeSchemes();
  // Initialize SPDY
  net::HttpNetworkLayer::EnableSpdy("npn");

  // Initialize the content client which that code uses to talk to Chrome.
  content::SetContentClient(g_chrome_content_client.Pointer());
  content::GetContentClient()->set_browser(g_browser_client.Pointer());

  // Register the ui message loop by instantiating it.
  g_ui_loop = new MessageLoop(MessageLoop::TYPE_UI);

  g_system_monitor = new base::SystemMonitor;

  const char* kThreadName = "CrBrowserMain";
  base::PlatformThread::SetName(kThreadName);

  setpriority(PRIO_PROCESS, base::PlatformThread::CurrentId(), -2);

  g_ui_loop->set_thread_name(kThreadName);
  // Register the main thread by instantiating it, but don't call any methods.
  g_ui_thread = new content::BrowserThreadImpl(BrowserThread::UI,
                                               MessageLoop::current());

  g_main_notification_service = new NotificationServiceImpl();

  // Prior to any processing happening on the io thread, we create the
  // plugin service as it is predominantly used from the io thread,
  // but must be created on the main thread. The service ctor is
  // inexpensive and does not invoke the io_thread() accessor.
  content::PluginService::GetInstance()->Init();

  // Android UI message loop is managed by the Java side. Instead of calling
  // Run(), call Start() which ensures the native UI events appended to the
  // Java message loop.
  MessageLoopForUI::current()->Start();

  // Start tracing to a file if needed.
  if (base::debug::TraceLog::GetInstance()->IsEnabled())
    TraceController::GetInstance()->InitStartupTracing(parsed_command_line);

  // This must be set before the browser process is instantiated.
  net::NetworkChangeNotifier::SetFactory(
      new net::android::NetworkChangeNotifierFactory());
  BrowserProcessImpl* process = new BrowserProcessImpl(parsed_command_line);

  // Initialize the prefs of the local state.
  PrefService* local_state = process->local_state();
  ScopedJavaLocalRef<jstring> locale =
      Java_BrowserProcessMain_getDefaultLocale(env);
  local_state->RegisterStringPref(prefs::kApplicationLocale,
                                  ConvertJavaStringToUTF8(locale));

  std::string app_locale = ResourceBundle::InitSharedInstanceWithLocale(
      local_state->GetString(prefs::kApplicationLocale));
  process->SetApplicationLocale(app_locale);

  InitializeNetworkOptions(parsed_command_line);

  FilePath resources_pack_path;
  PathService::Get(chrome::FILE_RESOURCES_PACK, &resources_pack_path);
  ResourceBundle::AddDataPackToSharedInstance(resources_pack_path);

  int value;
  if (GetIntSwitchValue(parsed_command_line, switches::kRendererProcessLimit,
                        0, max_render_process_count, &value)) {
    max_render_process_count = value;
  }
  if (max_render_process_count <= 0 ||
      parsed_command_line.HasSwitch(switches::kSingleProcess)) {
    max_render_process_count = 0;
  }
  if (max_render_process_count == 0) {
    if (!parsed_command_line.HasSwitch(switches::kSingleProcess)) {
      // Need to ensure the command line flag is consistent as a lot of Chromium
      // internal code checks this directly, but it wouldn't normally get set
      // when we are implementing an embedded ContentView.
      CommandLine::ForCurrentProcess()->AppendSwitch(switches::kSingleProcess);
    }

    content::RenderProcessHost::set_run_renderer_in_process(true);
    content::GetContentClient()->set_renderer(g_renderer_client.Pointer());
    // See the comments in BrowserRenderProcessHost::Init(). As we can't have
    // in-process plugin, there is no need to set ChromeContentPluginClient.
  } else {
    max_render_process_count =
        std::min(max_render_process_count,
                 static_cast<int>(content::kMaxRendererProcessCount));
    // TODO(terror_merge): this method was changed to be a test method.
    //                     Figure-out if there is a better one to use instead.
    content::RenderProcessHost::SetMaxRendererProcessCountForTest(
        max_render_process_count);
  }

  // This must be done after we processed the kSingleProcess command line
  // parameter which is used in some threads like the WebKit thread.
  InitializeThreads();

  V8StartupDataMapper::GetInstance().BrowserProcessMapData();

  Profile* profile = g_browser_process->profile_manager()->GetDefaultProfile();

  if (parsed_command_line.HasSwitch(switches::kTestingChannelID)) {
    // Initialize testing automation provider.
    std::string testing_channel_id = parsed_command_line.GetSwitchValueASCII(
        switches::kTestingChannelID);
    scoped_refptr<TestingAutomationProvider> automation =
        new TestingAutomationProvider(profile);
    if (automation->InitializeChannel(testing_channel_id)) {
      // TODO: Add the logic about the expected count of tabs like that in
      // browser_init.cc. Now expect 1 because we force loading a blank page
      // at startup in proxy_launcher.cc.
      automation->SetExpectedTabCount(1);
      AutomationProviderList* list =
          g_browser_process->GetAutomationProviderList();
      DCHECK(list);
      list->AddProvider(automation);
    }
  }
}

// Handles initializing the metrics service, but starting the service is left
// up to the application.  Clank starts it up when in Main.onResume() through
// UmaSessionStats.updateMetricsServiceState().
void InitMetricsService() {
  // Should be called after the process is initialized.
  DCHECK(g_browser_process);

  // Singletons needed to be initialized for recording histograms.
  g_histogram_synchronizer = new HistogramSynchronizer();
  // HistogramSynchronizer is a refcounted object.
  g_histogram_synchronizer->AddRef();
  g_statistics_recorder = new base::StatisticsRecorder();

  // Create the metrics service. We want to record crashes during startup too.
  g_browser_process->metrics_service();

  // Set version extension to identify clank releases.
  // Example: 16.0.912.61-K88
  std::string app_version_extenstion =
      "-K" + AboutAndroidApp::GetAppVersionCode();
  MetricsLog::set_version_extension(app_version_extenstion);
}

}  // namespace <anonymous>

static void InitBrowserProcess(JNIEnv* env, jclass /* clazz */,
                               jobject context,
                               jint max_render_process_count,
                               jboolean host_is_chrome,
                               jboolean is_tablet,
                               jstring plugin_descriptor) {
  std::string plugin_str = ConvertJavaStringToUTF8(env, plugin_descriptor);
  SetAndroidCommandLineFlags(is_tablet, plugin_str);

  // This should be only called once per process. g_browser_process is set
  // through ChromeViewBrowserMain().
  CHECK(!g_browser_process);
  // PathService path providers must be registered before CrashReporting. In
  // particular chrome::DIR_CRASH_DUMP.
  chrome::RegisterPathProvider();
  content::RegisterPathProvider();
  // The application context must be initialized prior to the crash reporter
  // because we need access to context (and the location of the cache
  // directory) in order to initialize crash reporting. Yes this is some strange
  // coupling that should be fixed... later.
  ScopedJavaLocalRef<jobject> scoped_context(env, context);
  base::android::InitApplicationContext(scoped_context);
#if defined (USE_LINUX_BREAKPAD)
  InitCrashReporter();
#endif
  SurfaceTexturePeer::InitInstance(new SurfaceTexturePeerBrowserImpl());
  device_orientation::DeviceOrientationAndroid::Init(env);
  webkit_glue::InitUserAgent(GetUserAgentOSInfo(), GetUserAgentMobile(),
      GetUserAgentChromeVersionInfo());

  AndroidLocationApiAdapter::RegisterGeolocationService(env);
  AndroidProtocolAdapter::RegisterProtocols(env);

  ChromeViewBrowserMain(max_render_process_count, env, scoped_context.obj());
  if (host_is_chrome) {
    RegisterApplicationNativeMethods(env);
    // We need to register the native methods first as we rely on
    // BrowserVersion to be set up.
    InitMetricsService();
  }

  g_registrar = new content::NotificationRegistrar();
  g_renderer_process_closed_observer = new RendererProcessClosedObserver();
  g_registrar->Add(g_renderer_process_closed_observer,
                   content::NOTIFICATION_RENDERER_PROCESS_CLOSED,
                   content::NotificationService::AllSources());

  // Do not allow disk IO from the UI thread.
  // TODO(joth): See if this can be moved earlier in this function.
  base::ThreadRestrictions::SetIOAllowed(false);
}

static jboolean IsOfficialBuild(JNIEnv* env, jclass /* clazz */) {
#if defined(OFFICIAL_BUILD)
  return true;
#else
  return false;
#endif
}

bool RegisterBrowserProcessMain(JNIEnv* env) {
  RegisterDidEndMainMessageLoop(&DidEndMainMessageLoopImpl);
  return RegisterNativesImpl(env);
}
