// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/js_modal_dialog_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/app_modal_dialogs/js_modal_dialog.h"
#include "content/browser/android/chrome_view.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/browser/tab_contents/tab_contents_view_android.h"
#include "jni/js_modal_dialog_jni.h"
#include "ui/base/javascript_message_type.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF16ToJavaString;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;

JSModalDialogAndroid::JSModalDialogAndroid(JNIEnv* env,
                                           JavaScriptAppModalDialog* dialog,
                                           gfx::NativeWindow parent)
    : dialog_(dialog),
      chrome_view_(env, parent->GetJavaObject()) {
}

JSModalDialogAndroid::~JSModalDialogAndroid() {
  JNIEnv* env = AttachCurrentThread();
  // In case the dialog is still displaying, tell it to close itself.
  // This can happen if you trigger a dialog but close the Tab before it's
  // shown, and then accept the dialog.
  if (!dialog_jobject_.is_null())
    Java_JSModalDialog_dismiss(env, dialog_jobject_.obj());
}

int JSModalDialogAndroid::GetAppModalDialogButtons() const {
  NOTIMPLEMENTED();
  return 0;
}

void JSModalDialogAndroid::ShowAppModalDialog() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  JNIEnv* env = AttachCurrentThread();

  // Keep a strong ref to the Chrome View while we make the call
  // to java to display the dialog.
  ScopedJavaLocalRef<jobject> chrome_view = chrome_view_.get(env);

  if (!chrome_view.obj()) {
    CancelAppModalDialog();
    return;
  }

  ScopedJavaLocalRef<jobject> dialog_object;
  ScopedJavaLocalRef<jstring> title =
      ConvertUTF16ToJavaString(env, dialog_->title());
  ScopedJavaLocalRef<jstring> message =
      ConvertUTF16ToJavaString(env, dialog_->message_text());

  switch (dialog_->javascript_message_type()) {
    case ui::JAVASCRIPT_MESSAGE_TYPE_ALERT: {
      dialog_object = Java_JSModalDialog_createAlertDialog(env,
          title.obj(), message.obj(),
          dialog_->display_suppress_checkbox());
      break;
    }
    case ui::JAVASCRIPT_MESSAGE_TYPE_CONFIRM: {
      dialog_object = Java_JSModalDialog_createConfirmDialog(env,
          title.obj(), message.obj(),
          dialog_->display_suppress_checkbox());
      break;
    }
    case ui::JAVASCRIPT_MESSAGE_TYPE_PROMPT: {
      ScopedJavaLocalRef<jstring> default_prompt_text =
          ConvertUTF16ToJavaString(env, dialog_->default_prompt_text());
      dialog_object = Java_JSModalDialog_createPromptDialog(env,
          title.obj(), message.obj(),
          dialog_->display_suppress_checkbox(), default_prompt_text.obj());
      break;
    }
    default:
      NOTREACHED();
  }

  // Keep a ref to the java side object until we get a confirm or cancel.
  dialog_jobject_.Reset(dialog_object);

  Java_JSModalDialog_showJSModalDialog(env, dialog_object.obj(),
                                       chrome_view.obj(),
                                       reinterpret_cast<jint>(this));
}

void JSModalDialogAndroid::ActivateAppModalDialog() {
  ShowAppModalDialog();
}

void JSModalDialogAndroid::CloseAppModalDialog() {
  CancelAppModalDialog();
}

void JSModalDialogAndroid::AcceptAppModalDialog() {
  string16 prompt_text;
  dialog_->OnAccept(prompt_text, false);
  delete this;
}

void JSModalDialogAndroid::DidAcceptAppModalDialog(
    JNIEnv* env, jobject, jstring prompt, bool should_suppress_js_dialogs) {
  string16 prompt_text = base::android::ConvertJavaStringToUTF16(env, prompt);
  dialog_->OnAccept(prompt_text, should_suppress_js_dialogs);
  delete this;
}

void JSModalDialogAndroid::CancelAppModalDialog() {
  dialog_->OnCancel(false);
  delete this;
}

void JSModalDialogAndroid::DidCancelAppModalDialog(
    JNIEnv* env, jobject, bool should_suppress_js_dialogs) {
  dialog_->OnCancel(should_suppress_js_dialogs);
  delete this;
}

// static
NativeAppModalDialog* NativeAppModalDialog::CreateNativeJavaScriptPrompt(
    JavaScriptAppModalDialog* dialog,
    gfx::NativeWindow parent_window) {
  return new JSModalDialogAndroid(AttachCurrentThread(),
                                  dialog, parent_window);
}

// JNI bindings
bool RegisterJSModalDialog(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
