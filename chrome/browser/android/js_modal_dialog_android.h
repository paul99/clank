// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_JS_MODAL_DIALOG_ANDROID_H_
#define CHROME_BROWSER_ANDROID_JS_MODAL_DIALOG_ANDROID_H_
#pragma once

#include "base/android/scoped_java_ref.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/ui/app_modal_dialogs/native_app_modal_dialog.h"
#include "content/browser/android/jni_helper.h"

class JavaScriptAppModalDialog;

class JSModalDialogAndroid : public NativeAppModalDialog {
 public:
  JSModalDialogAndroid(JNIEnv* env,
                       JavaScriptAppModalDialog* dialog,
                       gfx::NativeWindow parent);
  virtual ~JSModalDialogAndroid();

  // NativeAppModalDialog:
  virtual int GetAppModalDialogButtons() const OVERRIDE;
  virtual void ShowAppModalDialog() OVERRIDE;
  virtual void ActivateAppModalDialog() OVERRIDE;
  virtual void CloseAppModalDialog() OVERRIDE;
  virtual void AcceptAppModalDialog() OVERRIDE;
  virtual void CancelAppModalDialog() OVERRIDE;

  // Called when java confirms or cancels the dialog.
  void DidAcceptAppModalDialog(JNIEnv* env,
                               jobject,
                               jstring prompt_text,
                               bool suppress_js_dialogs);
  void DidCancelAppModalDialog(JNIEnv* env, jobject, bool suppress_js_dialogs);

 private:
  scoped_ptr<JavaScriptAppModalDialog> dialog_;
  base::android::ScopedJavaGlobalRef<jobject> dialog_jobject_;
  JavaObjectWeakGlobalRef chrome_view_;

  DISALLOW_COPY_AND_ASSIGN(JSModalDialogAndroid);
};

bool RegisterJSModalDialog(JNIEnv* env);

#endif  // CHROME_BROWSER_ANDROID_JS_MODAL_DIALOG_ANDROID_H_
