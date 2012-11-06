// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_pump_android.h"

#include <jni.h>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "jni/system_message_handler_jni.h"

using base::android::ScopedJavaLocalRef;

namespace {

const char kClassPathName[] = "org/chromium/base/SystemMessageHandler";

base::LazyInstance<base::android::ScopedJavaGlobalRef<jobject> >
    g_system_message_handler_obj = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// ----------------------------------------------------------------------------
// Native JNI methods called by Java.
// ----------------------------------------------------------------------------
// This method can not move to anonymous namespace as it has been declared as
// 'static' in system_message_handler_jni.h.
static jboolean DoRunLoopOnce(JNIEnv* env, jobject obj, jint native_delegate) {
  base::MessagePump::Delegate* delegate =
      reinterpret_cast<base::MessagePump::Delegate*>(native_delegate);
  DCHECK(delegate);
  // This is based on MessagePumpForUI::DoRunLoop() from desktop.
  // Note however that our system queue is handled in the java side.
  // In desktop we inspect and process a single system message and then
  // we call DoWork() / DoDelayedWork().
  // On Android, the java message queue may contain messages for other handlers
  // that will be processed before calling here again.
  bool more_work_is_plausible = delegate->DoWork();

  // This is the time when we need to do delayed work.
  base::TimeTicks delayed_work_time;
  more_work_is_plausible |= delegate->DoDelayedWork(&delayed_work_time);

  // This is a major difference between android and other platforms: since we
  // can't inspect it and process just one single message, instead we'll yeld
  // the callstack, and post a message to call us back soon.
  if (more_work_is_plausible)
    return true;

  more_work_is_plausible = delegate->DoIdleWork();
  if (!more_work_is_plausible && !delayed_work_time.is_null()) {
    // We only set the timer here as returning true would post a message.
    jlong millis =
        (delayed_work_time - base::TimeTicks::Now()).InMillisecondsRoundedUp();
    Java_SystemMessageHandler_setDelayedTimer(env, obj, millis);
  }
  return more_work_is_plausible;
}

namespace base {

MessagePumpForUI::MessagePumpForUI()
    : state_(NULL) {
}

MessagePumpForUI::~MessagePumpForUI() {
}

void MessagePumpForUI::Run(Delegate* delegate) {
  NOTREACHED() << "UnitTests should rely on MessagePumpForUIStub in"
      " test_stub_android.h";
}

void MessagePumpForUI::Start(Delegate* delegate) {
  state_ = new MessageLoop::AutoRunState(MessageLoop::current());

  DCHECK(g_system_message_handler_obj.Get().is_null());

  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);

  ScopedJavaLocalRef<jclass> clazz =
      base::android::GetClass(env, kClassPathName);
  jmethodID constructor =
      base::android::GetMethodID(env, clazz, "<init>", "(I)V");
  ScopedJavaLocalRef<jobject> client(env,
      env->NewObject(clazz.obj(), constructor, delegate));
  DCHECK(!client.is_null());
  base::android::CheckException(env);

  g_system_message_handler_obj.Get().Reset(client);
}

void MessagePumpForUI::Quit() {
  if (!g_system_message_handler_obj.Get().is_null()) {
    JNIEnv* env = base::android::AttachCurrentThread();
    DCHECK(env);

    Java_SystemMessageHandler_removeTimer(env,
        g_system_message_handler_obj.Get().obj());
    g_system_message_handler_obj.Get().Reset();
  }

  if (state_) {
    delete state_;
    state_ = NULL;
  }
}

void MessagePumpForUI::ScheduleWork() {
  if (g_system_message_handler_obj.Get().is_null())
    return;

  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);

  Java_SystemMessageHandler_setTimer(env,
      g_system_message_handler_obj.Get().obj());
}

void MessagePumpForUI::ScheduleDelayedWork(const TimeTicks& delayed_work_time) {
  if (g_system_message_handler_obj.Get().is_null())
    return;

  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(env);

  jlong millis =
      (delayed_work_time - base::TimeTicks::Now()).InMillisecondsRoundedUp();
  // Note that we're truncating to milliseconds as required by the java side,
  // even though delayed_work_time is microseconds resolution.
  Java_SystemMessageHandler_setDelayedTimer(env,
      g_system_message_handler_obj.Get().obj(), millis);
}

// Register native methods
bool RegisterSystemMessageHandler(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace base
