// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/support/platform_support.h"

// The AndroidRuntime.h includes the Android source tree jni.h, which has a
// different include guard than the NDK jni.h. We need to define the NDK
// include guard to prevent double-declaration issues.
#include <android_runtime/AndroidRuntime.h>
#include <dlfcn.h>

#define _JNI_H
// Undefine symbols that are redefined by base/logging.h
#undef LOG
#undef LOG_ASSERT

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/string16.h"
#include "base/string_piece.h"
#include "base/test/test_stub_android.h"
#include "content/browser/android/media_player_listener.h"
#include "content/browser/android/surface_texture_listener.h"
#include "googleurl/src/gurl.h"
#include "grit/webkit_resources.h"
#include "net/android/network_library.h"
#include "ui/base/resource/resource_bundle.h"
#include "webkit/glue/user_agent.h"
#include "webkit/support/test_webkit_platform_support.h"
#include "webkit/tools/test_shell/simple_resource_loader_bridge.h"

using base::android::GetClass;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;

namespace {
// An AndroidRuntime subclass to initialize the Android runtime when running
// unit tests.
// Note: we have to subclass because of a few pure virtual functions.
class TestRuntime : public android::AndroidRuntime {
 private:
  pthread_t android_thread_;
  pthread_mutex_t mutex_;
  pthread_cond_t cond_var_;

  ScopedJavaGlobalRef<jclass> class_loop_drt_;

  // This method is called from the thread we create to initialize the
  // Android runtime.
  static void InitThread(void* arg) {
    TestRuntime* instance = static_cast<TestRuntime*>(arg);
    instance->start("com.android.internal.os.RuntimeInit", "");
    NOTREACHED();
  }

 public:
  TestRuntime() : android::AndroidRuntime() {
    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_var_, NULL);

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    if (pthread_create(&android_thread_, &attr,
                       (void *(*)(void*)) InitThread, this) != 0) {
      LOG(ERROR) << "Failed to create AndroidRuntime thread";
    }
  }

  virtual void onVmCreated(JNIEnv* env) {
    // We have to get class here. See Android dir:
    // frameworks/base/cmds/app_process/app_main.cpp .
    class_loop_drt_.Reset(GetClass(env, "org/chromium/content/browser/LoopDRT"));
  }

  // Invoked by com.android.internal.os.RuntimeInit from Java once the runtime
  // has been initialized.
  virtual void onStarted() {
    // Wake up the other thread. We are ready for business.
    pthread_mutex_lock(&mutex_);
    pthread_cond_signal(&cond_var_);
    pthread_mutex_unlock(&mutex_);

    // Wait forever in Java message loop.
    callMain("org/chromium/content/browser/LoopDRT", class_loop_drt_.obj(), 0, NULL);
    NOTREACHED();
  }

  virtual void onExit(int code) {
    // We cannot simply call AndroidRuntime::onExit() here.
    // See BUG=6509516 to understand why.
    void* self;
    void* onExitSymbol;

    dlerror();
    self = dlopen(NULL, RTLD_LAZY);
    if (self == NULL) {
        // Humm, if we can't access the symbols in the current process,
        // there is no way we're going to solve this, so forget about it.
        return;
    }
    onExitSymbol = dlsym(self, "_ZN7android14AndroidRuntime4exitEi");
    if (onExitSymbol == NULL) {
        onExitSymbol = dlsym(self, "_ZN7android14AndroidRuntime6onExitEi");
    }
    if (onExitSymbol != NULL) {
        // HACK ATTACK!!!
        //
        // It is not possible to convert a simple memory address into a
        // C++ method pointer, because they don't have the same layout or size.
        //
        // On ARM, a memory address is a 32-bit value, while a C++ method
        // pointer is 64-bit. It actually contains an address + a vtable
        // offset/adjustment, and some of the address bits have special
        // meaning too.
        //
        // (See section 2.3 of the Itanium C++ ABI specification).
        //
        // The following is a hack that is not 100% compliant because the
        // C++ ABI does not mandate how a specific implementation implements
        // C++ method calls. In particular, the way the 'this' pointer is
        // passed is not specified, to allow optimizations (e.g. reserving
        // a special register to hold the value of 'this', instead of using
        // the first function parameter for it).
        //
        // On Android ARM and x86, GCC always implements method calls by
        // putting 'this' as the first function call parameter, so we can
        // actually mimic it by a C function call.
        //
        void (*onExitFunc)(void*, int) = reinterpret_cast<void (*)(void*,int)>(onExitSymbol);
        (*onExitFunc)(this,code);
    }
    // it's likely that this will never get called, but just be safe.
    dlclose(self);
  }

  // Wait for the runtime thread to be initialized.
  void Wait() {
    pthread_mutex_lock(&mutex_);
    pthread_cond_wait(&cond_var_, &mutex_);
    pthread_mutex_unlock(&mutex_);
  }
};

// Android OS Looper is needed to run tests related to media and
// AndroidNetworkLibrary.
static void InitAndroidOSLooper() {
  JavaVM* vm = NULL;
  jsize vm_count = 0;
  JNI_GetCreatedJavaVMs(&vm, sizeof(JavaVM*), &vm_count);
  CHECK(vm_count == 0); // VM must have not been created.

  TestRuntime* runtime = new TestRuntime();
  // Wait for the JavaVM to be initialized.
  runtime->Wait();
  // Register Java VM.
  base::android::InitVM(android::AndroidRuntime::getJavaVM());

  JNIEnv* env = base::android::AttachCurrentThread();
  CHECK(env);
  CHECK(RegisterMediaPlayerListener(env));
  CHECK(RegisterSurfaceTextureListener(env));
  CHECK(net::android::RegisterNetworkLibrary(env));

  // Init App Context.
  ScopedJavaLocalRef<jclass> clazz =
      GetClass(env, "org/chromium/content/browser/SandboxedProcessService");
  jmethodID j_get_app_context_method = GetStaticMethodID(
      env,
      clazz,
      "getContext",
      "()Landroid/content/Context;");
  ScopedJavaLocalRef<jobject> context(env,
      env->CallStaticObjectMethod(clazz.obj(), j_get_app_context_method));
  base::android::InitApplicationContext(context);
}

}  // namespace

namespace webkit_support {

void BeforeInitialize(bool unit_test_mode) {
  InitAndroidOSPathStub();
  webkit_glue::InitUserAgent("", "", "");
  // Set XML_CATALOG_FILES environment variable to blank to prevent libxml from
  // loading and complaining the non-exsistent /etc/xml/catalog file.
  setenv("XML_CATALOG_FILES", "", 0);
}

void AfterInitialize(bool unit_test_mode) {
  if (unit_test_mode)
    return;  // We don't have a resource pack when running the unit-tests.

  InitAndroidOSLooper();

  FilePath data_path;
  PathService::Get(base::DIR_EXE, &data_path);
  data_path = data_path.Append("DumpRenderTree.pak");
  ResourceBundle::InitSharedInstanceWithPakFile(data_path);

  // We enable file-over-http to bridge the file protocol to http protocol
  // in here, which can
  // (1) run the layout tests on android target device, but never need to
  // push the test files and corresponding resources to device, which saves
  // huge running time.
  // (2) still run non-http layout (tests not under LayoutTests/http) tests
  // via file protocol without breaking test environment / convention of webkit
  // layout tests, which are followed by current all webkit ports.
  SimpleResourceLoaderBridge::AllowFileOverHTTP(
      "third_party/WebKit/LayoutTests/",
      GURL("http://127.0.0.1:8000/all-tests/"));
}

void BeforeShutdown() {
  ResourceBundle::CleanupSharedInstance();
}

void AfterShutdown() {
  NOTIMPLEMENTED(); // TODO(zhenghao): Implement this function.
}

}  // namespace webkit_support

string16 TestWebKitPlatformSupport::GetLocalizedString(int message_id) {
  return ResourceBundle::GetSharedInstance().GetLocalizedString(message_id);
}

base::StringPiece TestWebKitPlatformSupport::GetDataResource(int resource_id) {
  FilePath resources_path;
  PathService::Get(base::DIR_EXE, &resources_path);
  resources_path = resources_path.Append("DumpRenderTree_resources");
  switch (resource_id) {
    case IDR_BROKENIMAGE: {
      CR_DEFINE_STATIC_LOCAL(std::string, broken_image_data, ());
      if (broken_image_data.empty()) {
        FilePath path = resources_path.Append("missingImage.gif");
        bool success = file_util::ReadFileToString(path, &broken_image_data);
        if (!success)
          LOG(FATAL) << "Failed reading: " << path.value();
      }
      return broken_image_data;
    }
    case IDR_TEXTAREA_RESIZER: {
      CR_DEFINE_STATIC_LOCAL(std::string, resize_corner_data, ());
      if (resize_corner_data.empty()) {
        FilePath path = resources_path.Append("textAreaResizeCorner.png");
        bool success = file_util::ReadFileToString(path, &resize_corner_data);
        if (!success)
          LOG(FATAL) << "Failed reading: " << path.value();
      }
      return resize_corner_data;
    }
  }

  return ResourceBundle::GetSharedInstance().GetRawDataResource(resource_id);
}
