// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_IME_ADAPTER_ANDROID_H_
#define CONTENT_BROWSER_RENDERER_HOST_IME_ADAPTER_ANDROID_H_
#pragma once

#include "content/browser/android/jni_helper.h"

struct NativeWebKeyboardEvent;
class RenderWidgetHostViewAndroid;

// This class has a similar role as gtk_im_context_wrapper:
// it sits in between android IME and RenderWidgetHostViewAndroid, and manages
// input handling for text boxes and password fields.
//
// This class is in charge of dispatching key events from the java side
// and forward to renderer along with input method results via
// corresponding host view.
//
// This class is used solely by RenderWidgetHostViewAndroid.
// For more information, see http://b/issue?id=3248163
class ImeAdapterAndroid {
 public:
  explicit ImeAdapterAndroid(RenderWidgetHostViewAndroid* rwhva);
  ~ImeAdapterAndroid();

  // Called from java -> native
  // The java side is responsible to translate android KeyEvent various enums
  // and values into the corresponding WebKit::WebInputEvent.
  // TODO(bulach): we need the unmodified text to support accesskey.
  // See http://www.w3.org/TR/html5/editing.html#the-accesskey-attribute
  bool SendKeyEvent(JNIEnv* env, jobject,
                    jobject original_key_event,
                    int action, int meta_state,
                    long event_time, int key_code,
                    bool is_system_key, int unicode_text);
  // |event_type| is a value of WebInputEvent::Type.
  bool SendSyntheticKeyEvent(JNIEnv*,
                             jobject,
                             int event_type,
                             long timestamp_seconds,
                             int native_key_code,
                             int unicode_char);
  void SetComposingText(JNIEnv*, jobject, jstring text);
  void CommitText(JNIEnv*, jobject, jstring text);
  void AttachImeAdapter(JNIEnv*, jobject java_object);
  void ReplaceText(JNIEnv*, jobject, jstring text);
  void ClearFocus(JNIEnv*, jobject);
  void SetEditableSelectionOffsets(JNIEnv*, jobject, int start, int end);
  void SetComposingRegion(JNIEnv*, jobject, int start, int end);
  void DeleteSurroundingText(JNIEnv*, jobject, int before, int after);
  void Unselect(JNIEnv*, jobject);
  void SelectAll(JNIEnv*, jobject);
  void Cut(JNIEnv*, jobject);
  void Copy(JNIEnv*, jobject);
  void Paste(JNIEnv*, jobject);

  // Called from native -> java
  void CancelComposition();

 private:
  RenderWidgetHostViewAndroid* rwhva_;
  jobject java_ime_adapter_;
};

bool RegisterImeAdapter(JNIEnv* env);

#endif  // CONTENT_BROWSER_RENDERER_HOST_IME_ADAPTER_ANDROID_H_
