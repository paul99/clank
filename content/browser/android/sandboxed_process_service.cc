// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/sandboxed_process_service.h"

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/global_descriptors_posix.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop.h"
#include "base/message_loop_proxy.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/switches_android.h"
#include "chrome/common/url_constants.h"
#include "chrome/plugin/chrome_content_plugin_client.h"
#include "chrome/renderer/chrome_content_renderer_client.h"
#include "chrome/utility/chrome_content_utility_client.h"
#include "content/browser/android/android_native_window.h"
#include "content/browser/android/chrome_library_loader_hooks.h"
#include "content/browser/android/jni_helper.h"
#include "content/browser/android/surface_texture_peer.h"
#include "content/browser/android/user_agent.h"
#include "content/browser/android/v8_startup_data_mapper.h"
#include "content/common/chrome_descriptors.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/render_process.h"
#include "content/renderer/render_widget_fullscreen_pepper.h"
#include "content/public/common/main_function_params.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/render_view_visitor.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_descriptors.h"
#include "jni/sandboxed_process_service_jni.h"
#include "ppapi/proxy/proxy_module.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_switches.h"
#include "v8/include/v8.h"
#include "webkit/glue/user_agent.h"

#include "android/native_window_jni.h"  // Android include

#if defined(USE_LINUX_BREAKPAD)
#include "chrome/app/breakpad_posix.h"
#endif

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::GetMethodID;
using base::android::ScopedJavaLocalRef;
using base::WaitableEvent;
using content::RenderThread;
using content::RenderView;

extern int RendererMain(const content::MainFunctionParams&);
extern int PpapiPluginMain(const content::MainFunctionParams&);
extern int GpuMain(const content::MainFunctionParams&);
extern int UtilityMain(const content::MainFunctionParams&);

namespace {

class SurfaceTexturePeerRendererImpl : public SurfaceTexturePeer {
 public:
  // |service| is the instance of
  // org.chromium.content.browser.SandboxedProcessService.
  SurfaceTexturePeerRendererImpl(jobject service)
      : service_(service) {
  }

  virtual ~SurfaceTexturePeerRendererImpl() {
  }

  virtual void EstablishSurfaceTexturePeer(base::ProcessHandle pid,
                                           SurfaceTextureTarget type,
                                           jobject j_surface_texture,
                                           int primary_id,
                                           int secondary_id) {
    JNIEnv* env = base::android::AttachCurrentThread();
    Java_SandboxedProcessService_establishSurfaceTexturePeer(env, service_,
        pid, type, j_surface_texture, primary_id, secondary_id);
    CheckException(env);
  }

 private:
  // The instance of org.chromium.content.browser.SandboxedProcessService.
  jobject service_;

  DISALLOW_COPY_AND_ASSIGN(SurfaceTexturePeerRendererImpl);
};

base::LazyInstance<chrome::ChromeContentClient> g_chrome_content_client =
    LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<chrome::ChromeContentRendererClient> g_renderer_client =
    LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<chrome::ChromeContentPluginClient> g_plugin_client =
    LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<chrome::ChromeContentUtilityClient>
    g_chrome_content_utility_client = LAZY_INSTANCE_INITIALIZER;

void ConfigApplicationContext(JNIEnv* env, jclass clazz, jobject service) {
  ScopedJavaLocalRef<jclass> scoped_clazz(env, clazz);
  jmethodID j_get_app_context_method = GetMethodID(env, scoped_clazz,
      "getApplicationContext", "()Landroid/content/Context;");
  // We don't own the clazz object, we needed a scoped ref to call GetMethodID.
  scoped_clazz.Release();
  ScopedJavaLocalRef<jobject> context_obj(
      env, env->CallObjectMethod(service, j_get_app_context_method));
  base::android::InitApplicationContext(context_obj);
}

// Chrome actually uses the renderer code path for all of its sandboxed
// processes such as renderers, plugins, etc.
void InternalSandboxedProcessMain(int ipc_fd,
                                  int crash_fd,
                                  JNIEnv* env,
                                  jclass clazz,
                                  jobject service) {
  // Set up the IPC file descriptor mapping.
  base::GlobalDescriptors::GetInstance()->Set(kPrimaryIPCChannel, ipc_fd);
#if defined(USE_LINUX_BREAKPAD)
  if (crash_fd > 0) {
    base::GlobalDescriptors::GetInstance()->Set(kCrashDumpSignal, crash_fd);
  }
#endif

  // The command line should have already been copied over in JNI load.
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  DCHECK(command_line.GetCommandLineString().size());

  //SandboxInitWrapper sandbox_wrapper;
  ConfigApplicationContext(env, clazz, service);

#if defined(USE_LINUX_BREAKPAD)
  InitNonBrowserCrashReporter();
#endif
  SurfaceTexturePeer::InitInstance(new SurfaceTexturePeerRendererImpl(service));
  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);

  webkit_glue::InitUserAgent(GetUserAgentOSInfo(), GetUserAgentMobile(),
      GetUserAgentChromeVersionInfo());
  content::SetContentClient(g_chrome_content_client.Pointer());
  if (process_type == switches::kRendererProcess) {
    content::GetContentClient()->set_renderer(g_renderer_client.Pointer());
    // TODO(tedbo): This will not work when we turn on full sandboxing. We'll
    // need to pass the resource file descriptors so that they can be mmap'd.
    // TODO(tedbo): Do the utility processes need ResourceBundle?
    ResourceBundle::InitSharedInstanceWithLocale(
        command_line.GetSwitchValueASCII(switches::kLang));
  } else if (process_type == switches::kPpapiPluginProcess) {
    content::GetContentClient()->set_plugin(g_plugin_client.Pointer());
  } else if (process_type == switches::kUtilityProcess) {
    content::GetContentClient()->set_utility(
        g_chrome_content_utility_client.Pointer());
  }

  content::MainFunctionParams main_params(command_line);

  V8StartupDataMapper::GetInstance().ChildProcessMapData();

  // Register internal Chrome schemes so they'll be parsed correctly. This must
  // happen before we process any URLs with the affected schemes, and must be
  // done in all processes that work with these URLs (ie including renderers).
  chrome::RegisterChromeSchemes();

  LOG(INFO) << "Starting sandboxed process main entry point, cmdline="
            << CommandLine::ForCurrentProcess()->GetCommandLineString();
  if (process_type == switches::kRendererProcess) {
    RendererMain(main_params);
  } else if (process_type == switches::kPpapiPluginProcess) {
    PpapiPluginMain(main_params);
  } else if (process_type == switches::kGpuProcess) {
    GpuMain(main_params);
  } else if (process_type == switches::kUtilityProcess) {
    // In utility thread, code needs to get a V8 isolate from thread local
    // storage to run V8 functions. In Chrome, the default V8 isolate is
    // initialized by a static function called "EnsureDefaultIsolateAllocated"
    // in first thread (aka main thread) of the process. In utility process,
    // utility thread is running in the main thread. But on Android, the child
    // thread for the child process is NOT the thread when the process starts.
    // As the process is started by Android framework, the main thread is
    // handled by Java. We can't run our own message loop in the main thread
    // unless it is UI type, but the child thread for the child process uses the
    // default message loop, so we switch to create a worker thread to run the
    // Main method. In this case, the default V8 isolate was not set in the
    // worker thread in where utility thread runs because the V8 isolate static
    // initialization puts thing in the thread local storage of the main thread
    // in where the process does do static initialization instead of the worker
    // thread. We have to explicitly call v8 initialization in here, Otherwise,
    // the utility process will crash in V8 function call.
    v8::V8::Initialize();
    UtilityMain(main_params);
    // TODO(tedbo): Why do the utility processes exit from their main function
    // but are not killed by the browser via ChildProcessLauncher::Terminate()?
    // Since the browser does not clean up this utility process, we explicitly
    // _exit() here until that is fixed.
    LOG(INFO) << "Utility process dropped out of main; forcing exit(0).";
    _exit(0);
  } else {
    LOG(FATAL) << "Unknown process type: " << process_type;
    _exit(0);
  }

  // If we get here then we return and finish the main thread. The browser
  // should be aware of this and unbind() from the sandboxed service, which
  // will kill the whole sandboxed service process.
  LOG(INFO) << "Sandboxed process (type: " << process_type
            << ") dropping out of SandboxedProcessMain.";
  LibraryLoaderExitHook();
}

// Helper class to iterate over the RenderView objects and set the
// surface object on the view that matches the routing_id.
class SetVideoSurfaceVisitor : public content::RenderViewVisitor {
 public:
  SetVideoSurfaceVisitor(jobject surface, int routing_id, int player_id)
    : surface_(surface),
      routing_id_(routing_id),
      player_id_(player_id) {
  }

  virtual bool Visit(RenderView* render_view) {
    if (render_view->GetRoutingId() == routing_id_) {
      render_view->SetVideoSurface(surface_, player_id_);
      return false;
    }
    return true;
  }

 private:
  jobject surface_;
  int routing_id_;
  int player_id_;
};

void ReleaseSurface(jobject surface) {
  if (surface == NULL)
    return;

  JNIEnv* env = AttachCurrentThread();
  CHECK(env);

  jclass cls = env->FindClass("android/view/Surface");
  DCHECK(cls);

  jmethodID method = env->GetMethodID(cls, "release", "()V");
  DCHECK(method);

  env->CallVoidMethod(surface, method);
  DCHECK(env);

  env->DeleteLocalRef(cls);
}

void SetVideoSurface(jobject surface, jint routing_id, jint player_id) {
  DCHECK(RenderThread::Get() &&
         RenderThread::Get()->GetMessageLoop() == MessageLoop::current());
  SetVideoSurfaceVisitor visitor(surface, routing_id, player_id);
  RenderView::ForEach(&visitor);
  ReleaseSurface(surface);
  AttachCurrentThread()->DeleteGlobalRef(surface);
}

base::Lock g_registration_lock;
// We hold a reference to a message loop proxy which handles message loop
// destruction gracefully, which is important since we post tasks from
// an arbitrary binder thread while the main thread might be shutting down.
// Also, in single-process mode we have two ChildThread objects for render
// and gpu thread, so we need to store the msg loops separately.
scoped_refptr<base::MessageLoopProxy> g_render_thread;
scoped_refptr<base::MessageLoopProxy> g_gpu_thread;
NativeWindowCallback g_native_window_callback;

}  // namespace <anonymous>

void RegisterRenderThread(base::MessageLoopProxy* loop) {
  base::AutoLock lock(g_registration_lock);
  g_render_thread = loop;
}

void RegisterNativeWindowCallback(base::MessageLoopProxy* loop,
                                  NativeWindowCallback& callback) {
  base::AutoLock lock(g_registration_lock);
  g_gpu_thread = loop;
  g_native_window_callback = callback;
}

static void RunNativeWindowCallback(int32 routing_id,
                                    int32 renderer_id,
                                    ANativeWindow* native_window,
                                    WaitableEvent* completion) {
  g_native_window_callback.Run(
      routing_id, renderer_id, native_window, completion);
}

static void SandboxedProcessMain(JNIEnv* env,
                                 jclass clazz,
                                 jobject service,
                                 jint ipc_fd,
                                 jint crash_fd) {
  InternalSandboxedProcessMain(
      static_cast<int>(ipc_fd),
      static_cast<int>(crash_fd),
      env,
      clazz,
      service);
}

static void SandboxedProcessExit(JNIEnv* env, jclass clazz) {
  LOG(INFO) << "SandboxedProcessService: Force exiting sandboxed process.";
  _exit(0);
}

static void SetSurface(JNIEnv* env,
                       jclass clazz,
                       jint type,
                       jobject jsurface,
                       jint primary_id,
                       jint secondary_id) {
  SetSurfaceAsync(env, jsurface,
      static_cast<SurfaceTexturePeer::SurfaceTextureTarget>(type),
      primary_id, secondary_id, NULL);
}

void SetSurfaceAsync(JNIEnv* env,
                     jobject jsurface,
                     SurfaceTexturePeer::SurfaceTextureTarget type,
                     int primary_id,
                     int secondary_id,
                     WaitableEvent* completion) {
  base::AutoLock lock(g_registration_lock);

  ANativeWindow* native_window = NULL;

  if (jsurface &&
      type != SurfaceTexturePeer::SET_VIDEO_SURFACE_TEXTURE) {
    native_window = ANativeWindow_fromSurface(env, jsurface);
    ReleaseSurface(jsurface);
  }

  switch (type) {
    case SurfaceTexturePeer::SET_GPU_SURFACE_TEXTURE: {
      // This should only be sent as a reaction to the renderer
      // activating compositing. If the GPU process crashes, we expect this
      // to be resent after the new thread is initialized.
      DCHECK(g_gpu_thread);
      g_gpu_thread->PostTask(
          FROM_HERE,
          base::Bind(&RunNativeWindowCallback,
              primary_id,
              static_cast<uint32_t>(secondary_id),
              native_window,
              completion));
      // ANativeWindow_release will be called in SetNativeWindow()
      break;
    }
    case SurfaceTexturePeer::SET_VIDEO_SURFACE_TEXTURE: {
      jobject surface = env->NewGlobalRef(jsurface);
      DCHECK(g_render_thread);
      g_render_thread->PostTask(
          FROM_HERE,
          base::Bind(&SetVideoSurface,
                     surface,
                     primary_id,
                     secondary_id));
      break;
    }
  }
}

bool RegisterSandboxedProcessService(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
