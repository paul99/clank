// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/speech/speech_input_manager.h"

#include <map>
#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_restrictions.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/speech/chrome_speech_input_manager.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "content/browser/android/chrome_view.h"
#include "content/public/browser/browser_thread.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/browser/tab_contents/tab_contents_view_android.h"
#include "content/browser/speech/speech_recognizer.h"
#include "grit/generated_resources.h"
#include "jni/speech_input_manager_jni.h"
#include "media/audio/audio_manager.h"
#include "ui/base/l10n/l10n_util.h"

// megamerge: speech_input_manager.cc was moved from chrome to
// content.  we can't do the same thing with this file directly, since
// it references chrome components (e.g. grit/generated_resources.h).
// The TODO here is to refactor speech_input_manager_android.cc and
// split its content across chrome and content.  Note hacks to
// accomodate the lack of refactoring (e.g. profile_impl.cc
// referencing SpeechInputManager instead of
// ChromeSpeechInputManager.)

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetClass;
using base::android::GetMethodID;
using base::android::HasClass;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;
using content::WebContents;

namespace speech_input {

namespace {

// megamerge question: looks like we use this instead of ChromeSpeechInputManager.
// Is that correct, or did refactoring upstream make this a problem?
class SpeechInputManagerImpl : public SpeechInputManager {
 public:
  // These methods are invoked by the Java code via JNI wrapper functions below.
  void SetRecognitionResult(int session_id,
                            const content::SpeechInputResult& result);
  void DidCompleteRecording(int session_id);
  void DidCompleteRecognition(int session_id);

  // SpeechInputManager methods.
  virtual void StartRecognition(
      Delegate* delegate,
      int caller_id,
      int render_process_id,
      int render_view_id,
      const gfx::Rect& element_rect,
      const std::string& language,
      const std::string& grammar,
      const std::string& origin_url,
      net::URLRequestContextGetter* context_getter,
      SpeechInputPreferences* speech_input_prefs,
      AudioManager* audio_manager) OVERRIDE;
  virtual void CancelRecognition(int caller_id) OVERRIDE;
  virtual void StopRecording(int caller_id) OVERRIDE;
  virtual void CancelAllRequestsWithDelegate(
      SpeechInputManagerDelegate* delegate) OVERRIDE;
  virtual void GetRequestInfo(AudioManager* audio_manager,
                              bool* can_report_metrics,
                              std::string* request_info) OVERRIDE;
  virtual void ShowRecognitionRequested(int caller_id,
                                        int render_process_id,
                                        int render_view_id,
                                        const gfx::Rect& element_rect) OVERRIDE;
  virtual void ShowWarmUp(int caller_id) OVERRIDE;
  virtual void ShowRecognizing(int caller_id) OVERRIDE;
  virtual void ShowRecording(int caller_id) OVERRIDE;
  virtual void ShowInputVolume(int caller_id,
                               float volume,
                               float noise_volume) OVERRIDE;
  virtual void ShowMicError(int caller_id, MicError error) OVERRIDE;
  virtual void ShowRecognizerError(int caller_id,
                                   content::SpeechInputError error) OVERRIDE;
  virtual void DoClose(int caller_id) OVERRIDE;

  static SpeechInputManager* GetInstance();

 private:
  friend struct base::DefaultLazyInstanceTraits<SpeechInputManagerImpl>;

  struct JavaObject {
    jobject obj;
  };

  struct StartRecognitionParams {
    int session_id;
    int render_process_id;
    int render_view_id;
    gfx::Rect element_rect;
    std::string language;
    std::string grammar;
  };

  // Private constructor to enforce singleton.
  SpeechInputManagerImpl();
  virtual ~SpeechInputManagerImpl();

  void CancelRecognitionAndInformDelegate();

  // Creates and initialises necessary data to invoke the java
  // SpeechInputManager object.
  void CreateJavaSpeechInputManager();

  // Accessor to java_object_, initialises it if not done already.
  const JavaObject& GetJavaObject();

  void StartRecognitionInUIThread(StartRecognitionParams params);
  void CancelRecognitionInUIThread();
  void StopRecordingInUIThread();
  bool IsCurrentSession(int session_id) const;

  // Returns the java ChromeView object for the given tab, identified by
  // render process id and render view id.
  jobject GetChromeViewJavaObject(int render_process_id, int render_view_id);

  base::WeakPtrFactory<SpeechInputManagerImpl> task_factory_;

  // The following member variables are accessed only from the IO thread.
  int session_id_;  // incremented for each request.
  int caller_id_;
  SpeechInputManagerDelegate* delegate_;

  // This member variable is accessed only from the UI thread
  // (apart from initialisation).
  // Do NOT access this directly, use GetJavaObject() instead.
  JavaObject java_object_;
};

base::LazyInstance<SpeechInputManagerImpl> g_speech_input_manager_impl =
  LAZY_INSTANCE_INITIALIZER;

}  // namespace

}  // namespace speech_input

//---- Native wrapper methods invoked by SpeechInputManager java code. ----

using speech_input::SpeechInputManager;
using speech_input::ChromeSpeechInputManager;
using speech_input::SpeechInputManagerImpl;

static void SetRecognitionResults(JNIEnv* env, jobject obj, jint session_id,
                                  jobjectArray arr) {
  jsize len = env->GetArrayLength(static_cast<jarray>(arr));

  content::SpeechInputResult result;
  for (jsize i = 0; i < len; ++i) {
    jobject item = env->GetObjectArrayElement(arr, i);
    result.hypotheses[i].utterance = base::android::ConvertJavaStringToUTF16(
        env, static_cast<jstring>(item));
    env->DeleteLocalRef(item);
  }
  SpeechInputManagerImpl* speech_input_manager =
      static_cast<SpeechInputManagerImpl*>(SpeechInputManagerImpl::GetInstance());
  speech_input_manager->SetRecognitionResult(session_id, result);
}

static void DidCompleteRecording(JNIEnv* env, jobject obj, jint session_id) {
  SpeechInputManagerImpl* speech_input_manager =
      static_cast<SpeechInputManagerImpl*>(SpeechInputManagerImpl::GetInstance());
  speech_input_manager->DidCompleteRecording(session_id);
}

static void DidCompleteRecognition(JNIEnv* env, jobject obj, jint session_id) {
  SpeechInputManagerImpl* speech_input_manager =
      static_cast<SpeechInputManagerImpl*>(SpeechInputManagerImpl::GetInstance());
  speech_input_manager->DidCompleteRecognition(session_id);
}

namespace speech_input {

static const char* kClassPathName = "org/chromium/content/browser/SpeechInputManager";

bool RegisterSpeechInputManager(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

ChromeSpeechInputManager::SpeechInputRequest::SpeechInputRequest()
    : delegate(NULL),
      is_active(false) {
  NOTIMPLEMENTED() << "teramerge";
}

ChromeSpeechInputManager::SpeechInputRequest::~SpeechInputRequest() {
  NOTIMPLEMENTED() << "teramerge";
}

// TODO(teramerge): more cleanup needed from refactoring in:
// http://codereview.chromium.org/7838028
bool SpeechRecognizer::StartRecording() {
  NOTIMPLEMENTED() << "teramerge";
  return true;
}

// TODO(teramerge): more cleanup needed from refactoring in:
// http://codereview.chromium.org/7838028
void SpeechRecognizer::StopRecording() {
  NOTIMPLEMENTED() << "teramerge";
}

// TODO(teramerge): more cleanup needed from refactoring in:
// http://codereview.chromium.org/7838028
void SpeechRecognizer::CancelRecognition() {
  NOTIMPLEMENTED() << "teramerge";
}

// megamerge: remove once refactored
SpeechInputManager* ChromeSpeechInputManager::GetInstance() {
  return SpeechInputManagerImpl::GetInstance();
}

SpeechInputManager* SpeechInputManagerImpl::GetInstance() {
  return g_speech_input_manager_impl.Pointer();
}

// static
void SpeechInputManager::ShowAudioInputSettings(AudioManager* audio_manager) {
  NOTREACHED();
}

SpeechInputManagerImpl::SpeechInputManagerImpl()
    : ALLOW_THIS_IN_INITIALIZER_LIST(task_factory_(this)),
      session_id_(0),
      caller_id_(0) {
  java_object_.obj = NULL;
}

SpeechInputManagerImpl::~SpeechInputManagerImpl() {
  // Since this is a singleton object destructor gets called by the
  // AtExitManager in the main (UI) thread. The browser is shutting down so we
  // don't have to cleanup any ongoing session, it automatically gets teared
  // down.
  // TODO(satish): If and when we have a clean shutdown implemented, delete
  // the global ref for java_object_.obj here.
}

void SpeechInputManagerImpl::CreateJavaSpeechInputManager() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (java_object_.obj)
    return;

  JNIEnv* env = AttachCurrentThread();
  if (!env) {
    DLOG(ERROR) << "Unable to find JNIEnv";
    return;
  }

  if (!HasClass(env, kClassPathName)) {
    DLOG(ERROR) << "Unable to find class SpeechInputManager";
    return;
  }
  ScopedJavaLocalRef<jclass> clazz = GetClass(env, kClassPathName);
  jmethodID constructor = GetMethodID(env, clazz, "<init>", "()V");
  ScopedJavaLocalRef<jobject> manager(env,
      env->NewObject(clazz.obj(), constructor));
  CheckException(env);
  if (manager.is_null()) {
    DLOG(ERROR) << "Unable to create SpeechInputManager";
    return;
  }

  java_object_.obj = env->NewGlobalRef(manager.obj());
}

const SpeechInputManagerImpl::JavaObject&
SpeechInputManagerImpl::GetJavaObject() {
  if (!java_object_.obj)
    CreateJavaSpeechInputManager();
  DCHECK(java_object_.obj);
  return java_object_;
}

jobject SpeechInputManagerImpl::GetChromeViewJavaObject(int render_process_id,
                                                        int render_view_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  WebContents* web_contents = tab_util::GetWebContentsByID(render_process_id,
                                                           render_view_id);
  if (!web_contents)  // The tab was destroyed before we got this request.
    return NULL;

  TabContentsViewAndroid* view =
      static_cast<TabContentsViewAndroid*>(web_contents->GetView());
  DCHECK(view);
  return static_cast<ChromeView*>(view->GetNativeView())->GetJavaObject();
}

bool SpeechInputManagerImpl::IsCurrentSession(int session_id) const {
  return caller_id_ && session_id_ == session_id;
}

void SpeechInputManagerImpl::StartRecognition(
    Delegate* delegate,
    int caller_id,
    int render_process_id,
    int render_view_id,
    const gfx::Rect& element_rect,
    const std::string& language,
    const std::string& grammar,
    const std::string& origin_url,
    net::URLRequestContextGetter* context_getter,
    SpeechInputPreferences* speech_input_prefs,
    AudioManager* /* audio_manager */) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(caller_id_ != caller_id);
  // If we are currently recording audio for another caller, abort that cleanly.
  if (caller_id_ && session_id_)
    CancelRecognitionAndInformDelegate();

  ++session_id_;
  delegate_ = delegate;
  caller_id_ = caller_id;

  StartRecognitionParams params;
  params.session_id = session_id_;
  params.render_process_id = render_process_id;
  params.render_view_id = render_view_id;
  params.element_rect = element_rect;
  params.language = language;
  params.grammar = grammar;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE, base::Bind(
          &SpeechInputManagerImpl::StartRecognitionInUIThread,
          task_factory_.GetWeakPtr(), params));
}

void SpeechInputManagerImpl::StartRecognitionInUIThread(
    StartRecognitionParams params) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  jobject jchrome_view = GetChromeViewJavaObject(params.render_process_id,
                                                 params.render_view_id);
  if (!jchrome_view)  // Was the tab closed before we processed this request?
    return;

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jlanguage =
      ConvertUTF8ToJavaString(env, params.language);
  ScopedJavaLocalRef<jstring> jgrammar =
      ConvertUTF8ToJavaString(env, params.grammar);
  ScopedJavaLocalRef<jclass> rect_clazz =
      GetClass(env, "android/graphics/Rect");
  jmethodID rect_constructor =
      GetMethodID(env, rect_clazz, "<init>", "(IIII)V");
  ScopedJavaLocalRef<jobject> jrect(env,
                     env->NewObject(rect_clazz.obj(), rect_constructor,
                     params.element_rect.x(),
                     params.element_rect.y(),
                     params.element_rect.right(),
                     params.element_rect.bottom()));
  CheckException(env);

  Java_SpeechInputManager_startSpeechRecognition(env, GetJavaObject().obj,
      params.session_id, jchrome_view, jrect.obj(), jlanguage.obj(),
      jgrammar.obj());
}

void SpeechInputManagerImpl::CancelRecognition(int caller_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(caller_id_ == caller_id);
  if (caller_id_ != caller_id)
    return;

  caller_id_ = 0;
  delegate_ = NULL;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE, base::Bind(
          &SpeechInputManagerImpl::CancelRecognitionInUIThread, task_factory_.GetWeakPtr()));
}

void SpeechInputManagerImpl::CancelRecognitionInUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  JNIEnv* env = AttachCurrentThread();
  Java_SpeechInputManager_cancelSpeechRecognition(env, GetJavaObject().obj);
}

void SpeechInputManagerImpl::CancelAllRequestsWithDelegate(
    SpeechInputManagerDelegate* delegate) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (delegate_ == delegate)
    CancelRecognition(caller_id_);
}

void SpeechInputManagerImpl::StopRecording(int caller_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(caller_id_ == caller_id);
  if (caller_id_ != caller_id)
    return;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE, base::Bind(
          &SpeechInputManagerImpl::StopRecordingInUIThread, task_factory_.GetWeakPtr()));
}

void SpeechInputManagerImpl::GetRequestInfo(AudioManager* audio_manager,
                                            bool* can_report_metrics,
                                            std::string* request_info) {
  NOTIMPLEMENTED();
}

void SpeechInputManagerImpl::ShowRecognitionRequested(int caller_id,
    int render_process_id,
    int render_view_id,
    const gfx::Rect& element_rect) {
}

void SpeechInputManagerImpl::ShowWarmUp(int caller_id) {
  NOTIMPLEMENTED();
}

void SpeechInputManagerImpl::ShowRecognizing(int caller_id) {
  NOTIMPLEMENTED();
}

void SpeechInputManagerImpl::ShowRecording(int caller_id) {
  NOTIMPLEMENTED();
}

void SpeechInputManagerImpl::ShowInputVolume(int caller_id,
                                             float volume,
                                             float noise_volume) {
  NOTIMPLEMENTED();
}

void SpeechInputManagerImpl::ShowMicError(int caller_id, MicError error) {
  NOTIMPLEMENTED();
}

void SpeechInputManagerImpl::ShowRecognizerError(
    int caller_id, content::SpeechInputError error) {
  NOTIMPLEMENTED();
}

void SpeechInputManagerImpl::DoClose(int caller_id) {
  NOTIMPLEMENTED();
}

void SpeechInputManagerImpl::StopRecordingInUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  JNIEnv* env = AttachCurrentThread();
  Java_SpeechInputManager_stopSpeechRecording(env, GetJavaObject().obj);
}

void SpeechInputManagerImpl::SetRecognitionResult(
    int session_id, const content::SpeechInputResult& result) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE, base::Bind(
            &SpeechInputManagerImpl::SetRecognitionResult,
            task_factory_.GetWeakPtr(), session_id, result));
    return;
  }
  // Recognition cancelled before we got this response?
  if (!IsCurrentSession(session_id))
    return;
  delegate_->SetRecognitionResult(caller_id_, result);
}

void SpeechInputManagerImpl::DidCompleteRecording(int session_id) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE, base::Bind(
            &SpeechInputManagerImpl::DidCompleteRecording,
            task_factory_.GetWeakPtr(), session_id));
    return;
  }
  // Recognition cancelled before we got this response?
  if (!IsCurrentSession(session_id))
    return;
  delegate_->DidCompleteRecording(caller_id_);
}

void SpeechInputManagerImpl::DidCompleteRecognition(int session_id) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE, base::Bind(
            &SpeechInputManagerImpl::DidCompleteRecognition,
            task_factory_.GetWeakPtr(), session_id));
    return;
  }
  // Recognition cancelled before we got this response?
  if (!IsCurrentSession(session_id))
    return;
  delegate_->DidCompleteRecognition(caller_id_);
  delegate_ = NULL;
  caller_id_ = 0;
}

void SpeechInputManagerImpl::CancelRecognitionAndInformDelegate() {
  SpeechInputManagerDelegate* cur_delegate = delegate_;
  int caller_id = caller_id_;
  CancelRecognition(caller_id);
  cur_delegate->DidCompleteRecording(caller_id);
  cur_delegate->DidCompleteRecognition(caller_id);
}

}  // namespace speech_input
