// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/select_file_dialog_android.h"


#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/logging.h"
#include "base/string_split.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "content/browser/android/chrome_view.h"
#include "content/public/common/file_chooser_params.h"
#include "jni/select_file_dialog_jni.h"

using base::android::CheckException;
using base::android::ConvertUTF16ToJavaString;
using base::android::GetClass;
using base::android::GetMethodID;
using base::android::GetMethodIDFromClassName;
using base::android::ScopedJavaLocalRef;

static const char* kSelectFileDialogClassPathName =
    "org/chromium/content/browser/SelectFileDialog";
static const char* kChromeViewClassPathName =
    "org/chromium/content/browser/ChromeView";

SelectFileDialogImpl::SelectFileDialogImpl(Listener* listener)
  : SelectFileDialog(listener),
  is_running_(false) {
}

void SelectFileDialogImpl::OnFileSelected(JNIEnv* env,
                                          jobject,
                                          jstring filepath) {
  if (listener_) {
    std::string path = base::android::ConvertJavaStringToUTF8(env, filepath);
    listener_->FileSelected(FilePath(path), 0, NULL);
  }

  is_running_ = false;
}

void SelectFileDialogImpl::OnFileNotSelected(JNIEnv*, jobject) {
  if (listener_)
    listener_->FileSelectionCanceled(NULL);

  is_running_ = false;
}

bool SelectFileDialogImpl::IsRunning(gfx::NativeWindow) const {
  return is_running_;
}

void SelectFileDialogImpl::ListenerDestroyed() {
  listener_ = 0;
}

void SelectFileDialogImpl::SelectFileImpl(
    SelectFileDialog::Type type,
    const string16& title,
    const FilePath& default_path,
    const SelectFileDialog::FileTypeInfo* file_types,
    int file_type_index,
    const std::string& default_extension,
    gfx::NativeWindow owning_window,
    void* params) {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jclass> vector_clazz = GetClass(env, "java/util/Vector");
  jmethodID vector_ctor = GetMethodID(env, vector_clazz, "<init>", "()V");
  jmethodID vector_add =
      GetMethodID(env, vector_clazz, "add", "(Ljava/lang/Object;)Z");
  ScopedJavaLocalRef<jobject> file_type_vector(env,
      env->NewObject(vector_clazz.obj(), vector_ctor));
  ScopedJavaLocalRef<jstring> capture_value;

  if (params != NULL) {
    content::FileChooserParams* file_chooser_params =
        reinterpret_cast<content::FileChooserParams*>(params);
    capture_value = ConvertUTF16ToJavaString(env,
        StringToLowerASCII(file_chooser_params->capture));

    std::vector<string16> accept_types(file_chooser_params->accept_types);
    std::vector<string16>::iterator it = accept_types.begin();
    for (; it != accept_types.end(); ++it) {
      string16 accept_type = *it;
      ScopedJavaLocalRef<jstring> file_type =
          ConvertUTF16ToJavaString(env, StringToLowerASCII(accept_type));
      env->CallBooleanMethod(file_type_vector.obj(), vector_add,
                             file_type.obj());
      CheckException(env);
    }
  }

  ScopedJavaLocalRef<jclass> select_file_dialog_clazz =
      GetClass(env, kSelectFileDialogClassPathName);
  jmethodID select_file_dialog_ctor = GetMethodID(env, select_file_dialog_clazz,
      "<init>", "(ILjava/util/Vector;Ljava/lang/String;)V");
  ScopedJavaLocalRef<jobject> select_file(env,
      env->NewObject(select_file_dialog_clazz.obj(),
                     select_file_dialog_ctor,
                     reinterpret_cast<jint>(this),
                     file_type_vector.obj(),
                     capture_value.obj()));

  ScopedJavaLocalRef<jobject> chrome_view(env,
      static_cast<ChromeView*>(owning_window)->GetJavaObject());
  jmethodID show_select_file_dialog = GetMethodIDFromClassName(env,
      kChromeViewClassPathName,
      "showSelectFileDialog",
      "(Lorg/chromium/content/browser/SelectFileDialog;)V");
  env->CallVoidMethod(chrome_view.obj(), show_select_file_dialog,
                      select_file.obj());
  CheckException(env);
  is_running_ = true;
}

// static
SelectFileDialog* SelectFileDialog::Create(Listener* listener) {
  return new SelectFileDialogImpl(listener);
}

bool SelectFileDialogImpl::HasMultipleFileTypeChoicesImpl() {
  NOTIMPLEMENTED();
  return false;
}

bool RegisterSelectFileDialog(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
