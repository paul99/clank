// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/ime_adapter_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "content/browser/android/ime_helper.h"
#include "content/browser/renderer_host/render_widget_host.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/common/view_messages.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "jni/ime_adapter_jni.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebCompositionUnderline.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF16;

bool RegisterImeAdapter(JNIEnv* env) {
  if (!RegisterNativesImpl(env)) {
    return false;
  }

  Java_ImeAdapter_initializeWebInputEvents(env,
                                           WebKit::WebInputEvent::RawKeyDown,
                                           WebKit::WebInputEvent::KeyUp,
                                           WebKit::WebInputEvent::Char,
                                           WebKit::WebInputEvent::ShiftKey,
                                           WebKit::WebInputEvent::AltKey,
                                           WebKit::WebInputEvent::ControlKey,
                                           WebKit::WebInputEvent::CapsLockOn,
                                           WebKit::WebInputEvent::NumLockOn);
  Java_ImeAdapter_initializeTextInputTypes(env,
                                           WebKit::WebTextInputTypeNone,
                                           WebKit::WebTextInputTypeText,
                                           WebKit::WebTextInputTypeTextArea,
                                           WebKit::WebTextInputTypePassword,
                                           WebKit::WebTextInputTypeSearch,
                                           WebKit::WebTextInputTypeURL,
                                           WebKit::WebTextInputTypeEmail,
                                           WebKit::WebTextInputTypeTelephone,
                                           WebKit::WebTextInputTypeNumber,
                                           WebKit::WebTextInputTypeDate,
                                           WebKit::WebTextInputTypeDateTime,
                                           WebKit::WebTextInputTypeDateTimeLocal,
                                           WebKit::WebTextInputTypeMonth,
                                           WebKit::WebTextInputTypeTime,
                                           WebKit::WebTextInputTypeWeek,
                                           WebKit::WebTextInputTypeContentEditable);
  return true;
}

ImeAdapterAndroid::ImeAdapterAndroid(RenderWidgetHostViewAndroid* rwhva)
    : rwhva_(rwhva),
      java_ime_adapter_(NULL) {
}

ImeAdapterAndroid::~ImeAdapterAndroid() {
  if (java_ime_adapter_) {
    JNIEnv* env = AttachCurrentThread();
    Java_ImeAdapter_detach(env, java_ime_adapter_);
    env->DeleteGlobalRef(java_ime_adapter_);
  }
}

bool ImeAdapterAndroid::SendSyntheticKeyEvent(JNIEnv*,
                                              jobject,
                                              int type,
                                              long timestamp,
                                              int key_code,
                                              int text) {
  NativeWebKeyboardEvent event(static_cast<WebKit::WebInputEvent::Type>(type),
                               0 /* modifiers */, timestamp, key_code, text,
                               false /* is_system_key */);
  rwhva_->SendKeyEvent(event);
  return true;
}

bool ImeAdapterAndroid::SendKeyEvent(JNIEnv* env, jobject,
                                     jobject original_key_event,
                                     int action, int modifiers,
                                     long event_time, int key_code,
                                     bool is_system_key, int unicode_char) {
  NativeWebKeyboardEvent event = NativeWebKeyboardEventFromKeyEvent(
      env, original_key_event, action, modifiers,
      event_time, key_code, is_system_key, unicode_char);
  rwhva_->SendKeyEvent(event);
  if (event.type == WebKit::WebInputEvent::RawKeyDown && event.text[0]) {
    // Send a Char event, but without an os_event since we don't want to
    // roundtrip back to java such synthetic event.
    NativeWebKeyboardEvent char_event(event);
    char_event.os_event = NULL;
    event.type = WebKit::WebInputEvent::Char;
    rwhva_->SendKeyEvent(event);
  }
  return true;
}

void ImeAdapterAndroid::SetComposingText(JNIEnv* env, jobject, jstring text) {
  RenderWidgetHost* rwh = rwhva_->GetRenderWidgetHost();
  if (!rwh)
    return;

  string16 text16 = ConvertJavaStringToUTF16(env, text);
  std::vector<WebKit::WebCompositionUnderline> underlines;
  underlines.push_back(
      WebKit::WebCompositionUnderline(0, text16.length(), SK_ColorBLACK,
                                      false));
  rwh->ImeSetComposition(text16, underlines, 0, text16.length());
}


void ImeAdapterAndroid::CommitText(JNIEnv* env, jobject, jstring text) {
  RenderWidgetHost* rwh = rwhva_->GetRenderWidgetHost();
  if (!rwh)
    return;

  string16 text16 = ConvertJavaStringToUTF16(env, text);
  rwh->ImeConfirmComposition(text16);
}

void ImeAdapterAndroid::AttachImeAdapter(JNIEnv* env, jobject java_object) {
  java_ime_adapter_ = AttachCurrentThread()->NewGlobalRef(java_object);
}

void ImeAdapterAndroid::CancelComposition() {
  Java_ImeAdapter_cancelComposition(AttachCurrentThread(), java_ime_adapter_);
}

void ImeAdapterAndroid::ReplaceText(JNIEnv* env, jobject, jstring text) {
  RenderWidgetHost* rwh = rwhva_->GetRenderWidgetHost();
  if (!rwh)
    return;

  string16 text16 = ConvertJavaStringToUTF16(env, text);
  rwh->Send(new ViewMsg_ReplaceAll(rwh->routing_id(), text16));
}

void ImeAdapterAndroid::ClearFocus(JNIEnv* env, jobject) {
  RenderWidgetHost* rwh = rwhva_->GetRenderWidgetHost();
  if (!rwh)
    return;

  rwh->Send(new ViewMsg_ClearFocusedNode(rwh->routing_id()));
}

void ImeAdapterAndroid::SetEditableSelectionOffsets(JNIEnv*, jobject,
                                                    int start, int end) {
  RenderWidgetHost* rwh = rwhva_->GetRenderWidgetHost();
  if (!rwh)
    return;

  rwh->Send(new ViewMsg_SetEditableSelectionOffsets(rwh->routing_id(),
                                                    start, end));
}

void ImeAdapterAndroid::SetComposingRegion(JNIEnv*, jobject,
                                           int start, int end) {
  RenderWidgetHost* rwh = rwhva_->GetRenderWidgetHost();
  if (!rwh)
    return;

  std::vector<WebKit::WebCompositionUnderline> underlines;
  underlines.push_back(
      WebKit::WebCompositionUnderline(0, end - start, SK_ColorBLACK, false));

  rwh->Send(new ViewMsg_SetComposingRegion(rwh->routing_id(),
                                           start, end, underlines));
}

void ImeAdapterAndroid::DeleteSurroundingText(JNIEnv*, jobject,
                                              int before, int after) {
  RenderWidgetHost* rwh = rwhva_->GetRenderWidgetHost();
  if (!rwh)
    return;

  rwh->Send(new ViewMsg_DeleteSurroundingText(rwh->routing_id(),
                                              before, after));
}

void ImeAdapterAndroid::Unselect(JNIEnv* env, jobject) {
  RenderWidgetHost* rwh = rwhva_->GetRenderWidgetHost();
  if (!rwh)
    return;

  rwh->Send(new ViewMsg_Unselect(rwh->routing_id()));
}

void ImeAdapterAndroid::SelectAll(JNIEnv* env, jobject) {
  RenderWidgetHost* rwh = rwhva_->GetRenderWidgetHost();
  if (!rwh)
    return;

  rwh->Send(new ViewMsg_SelectAll(rwh->routing_id()));
}

void ImeAdapterAndroid::Cut(JNIEnv* env, jobject) {
  RenderWidgetHost* rwh = rwhva_->GetRenderWidgetHost();
  if (!rwh)
    return;

  rwh->Send(new ViewMsg_Cut(rwh->routing_id()));
}

void ImeAdapterAndroid::Copy(JNIEnv* env, jobject) {
  RenderWidgetHost* rwh = rwhva_->GetRenderWidgetHost();
  if (!rwh)
    return;

  rwh->Send(new ViewMsg_Copy(rwh->routing_id()));
}

void ImeAdapterAndroid::Paste(JNIEnv* env, jobject) {
  RenderWidgetHost* rwh = rwhva_->GetRenderWidgetHost();
  if (!rwh)
    return;

  rwh->Send(new ViewMsg_Paste(rwh->routing_id()));
}
