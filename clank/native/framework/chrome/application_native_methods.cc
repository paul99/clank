// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/application_native_methods.h"

#include "base/android/jni_android.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "chrome/browser/android/select_file_dialog_android.h"
#include "clank/native/framework/chrome/android_sync_setup_flow_handler.h"
#include "clank/native/framework/chrome/autocomplete_bridge.h"
#include "clank/native/framework/chrome/browser_version.h"
#include "clank/native/framework/chrome/chrome_native_preferences.h"
#include "clank/native/framework/chrome/glui_functor_renderer.h"
#include "clank/native/framework/chrome/google_auto_login.h"
#include "clank/native/framework/chrome/google_location_settings_helper.h"
#include "clank/native/framework/chrome/gpu_profiler.h"
#include "clank/native/framework/chrome/infobar_android.h"
#include "clank/native/framework/chrome/instant_tab.h"
#include "clank/native/framework/chrome/location_bar.h"
#include "clank/native/framework/chrome/partner_bookmarks_shim.h"
#include "clank/native/framework/chrome/process_utils.h"
#include "clank/native/framework/chrome/sqlite_cursor.h"
#include "clank/native/framework/chrome/sync_setup_manager.h"
#include "clank/native/framework/chrome/tab.h"
#include "clank/native/framework/chrome/tab_model.h"
#include "clank/native/framework/chrome/uma_record_action.h"
#include "clank/native/framework/chrome/uma_session_stats.h"
#include "clank/native/framework/chrome/url_utilities.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;

static RegistrationMethod kRegistrationMethods[] = {
    { "AutocompleteBridge", RegisterAutocompleteBridge },
    { "BrowserVersion", RegisterBrowserVersion },
    { "ChromeNativePreferences", RegisterChromeNativePreferences },
    { "ChromeTabModel", RegisterTabModel },
    { "GLUIRender", RegisterGLUIFunctorRenderer },
    { "GoogleAutoLogin", RegisterGoogleAutoLogin },
    { "GoogleLocationSettingsHelper", RegisterGoogleLocationSettingsHelper },
    { "GpuProfiler", RegisterGpuProfiler },
    { "InfoBarContainer", RegisterInfoBarContainer },
    { "InstantTab", RegisterInstantTab },
    { "LocationBar", RegisterLocationBar },
    { "PartnerBookmarksShim", RegisterPartnerBookmarksShim },
    { "ProcessUtils", RegisterProcessUtils },
    { "SelectFileDialog", RegisterSelectFileDialog },
    { "SqliteCursor", RegisterSqliteCursor},
    { "SyncSetupFlowAdapter", RegisterSyncSetupFlowHandler },
    { "SyncSetupManager", RegisterSyncSetupManager },
    { "Tab", RegisterTab },
    { "UmaRecordAction", RegisterUmaRecordAction },
    { "UmaSessionStats", RegisterUmaSessionStats },
    { "UrlUtilities", RegisterURLUtilities },
};

bool RegisterApplicationNativeMethods(JNIEnv* env) {
  const RegistrationMethod* method = kRegistrationMethods;
  const RegistrationMethod* end = method + arraysize(kRegistrationMethods);
  while (method != end) {
    if (!method->func(env)) {
      DLOG(ERROR) << method->name << " failed registration!";
      return false;
    }
    method++;
  }

  return true;
}
