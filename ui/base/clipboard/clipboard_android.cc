// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/clipboard/clipboard.h"

#include <jni.h>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/size.h"

// Important note:
// The Android clipboard system only supports text format. So we use the
// Android system when some text is added or retreived from the system. For
// anything else, we currently store the value in some process wide static
// variable protected by a lock. So the clipboard will only work within the same
// process.

// Global contents map and its lock.
using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ClearException;
using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertJavaStringToUTF8;
using base::android::GetClass;
using base::android::GetMethodID;
using base::android::ScopedJavaLocalRef;

namespace ui {

namespace {
// The Android platform only supports the clipboard for text. So when non text
// data is used, we just store everything in the following global map. We use
// a Lock to make this thread safe.
typedef std::map<std::string, std::string> ClipboardMap;
ClipboardMap* g_map = NULL;
base::LazyInstance<base::Lock> g_map_lock = LAZY_INSTANCE_INITIALIZER;

// Various format we support
const char* const kPlainTextFormat = "text";
const char* const kHTMLFormat = "html";
const char* const kBitmapFormat = "bitmap";
const char* const kWebKitSmartPasteFormat = "webkit_smart";
const char* const kBookmarkFormat = "bookmark";
const char* const kMimeTypeWebCustomData = "chromium/x-web-custom-data";

}  // namespace

Clipboard::FormatType::FormatType() {
}

Clipboard::FormatType::FormatType(const std::string& native_format)
    : data_(native_format) {
}

Clipboard::FormatType::~FormatType() {
}

std::string Clipboard::FormatType::Serialize() const {
  return data_;
}

// static
Clipboard::FormatType Clipboard::FormatType::Deserialize(
    const std::string& serialization) {
  return FormatType(serialization);
}

bool Clipboard::FormatType::Equals(const FormatType& other) const {
  return data_ == other.data_;
}

// The clipboard object on the Android platform is simply wrapping the Java
// object for the text data format. For non text format, a global map is used.
Clipboard::Clipboard() : set_text_(NULL), has_text_(NULL), get_text_(NULL) {
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);

  // Get the context.
  // We have need a ScopedJavaLocalRef to clean-up the local ref if we have to
  // create the Context.
  jobject context = base::android::GetApplicationContext();
  ScopedJavaLocalRef<jobject> scoped_context;
  if (!context) {
    // Should be during testing only.
    // Get the ActivityThread class.
    ScopedJavaLocalRef<jclass> activity_thread_class =
        GetClass(env, "android/app/ActivityThread");

    // Try to get the current activity thread.
    jmethodID current_activity_method_id = GetMethodID(
        env, activity_thread_class,
        "currentActivityThread", "()Landroid/app/ActivityThread;");

    ScopedJavaLocalRef<jobject> current_activity(env,
        env->CallStaticObjectMethod(activity_thread_class.obj(),
                                    current_activity_method_id));
    ClearException(env);

    if (!current_activity.obj()) {
      // There is no current activity, create one.
      ScopedJavaLocalRef<jclass> looper_class =
          GetClass(env, "android/os/Looper");
      jmethodID prepare_method_id =
          GetStaticMethodID(env, looper_class, "prepareMainLooper", "()V");
      env->CallStaticVoidMethod(looper_class.obj(), prepare_method_id);
      CheckException(env);

      jmethodID system_main_method_id =
          GetStaticMethodID(env, activity_thread_class, "systemMain",
                            "()Landroid/app/ActivityThread;");

      current_activity.Reset(env, env->CallStaticObjectMethod(
          activity_thread_class.obj(), system_main_method_id));
      DCHECK(current_activity.obj());
      CheckException(env);
    }

    // Get the context.
    jmethodID get_system_context_id = GetMethodID(env, activity_thread_class,
       "getSystemContext", "()Landroid/app/ContextImpl;");
    scoped_context.Reset(env, env->CallObjectMethod(current_activity.obj(),
                                                    get_system_context_id));
    context = scoped_context.obj();
    DCHECK(context);
  }

  // Get the context class.
  ScopedJavaLocalRef<jclass> context_class =
      GetClass(env, "android/content/Context");
  // Get the system service method.
  jmethodID get_system_service = GetMethodID(env, context_class,
      "getSystemService",  "(Ljava/lang/String;)Ljava/lang/Object;");

  // Retrieve the system service.
  ScopedJavaLocalRef<jstring> service_name(env, env->NewStringUTF("clipboard"));
  clipboard_manager_.Reset(env, env->CallObjectMethod(context,
      get_system_service, service_name.obj()));
  ClearException(env);
  DCHECK(clipboard_manager_.obj());

  // Retain a few methods we'll keep using.
  ScopedJavaLocalRef<jclass> clipboard_class =
      GetClass(env, "android/text/ClipboardManager");
  set_text_ = GetMethodID(env, clipboard_class,
                          "setText", "(Ljava/lang/CharSequence;)V");
  has_text_ = GetMethodID(env, clipboard_class, "hasText", "()Z");
  get_text_ = GetMethodID(env, clipboard_class,
                          "getText", "()Ljava/lang/CharSequence;");

  // Will need to call toString as CharSequence is not always a String.
  ScopedJavaLocalRef<jclass> charsequence_class =
      GetClass(env, "java/lang/CharSequence");
  to_string_ = GetMethodID(env, charsequence_class,
                           "toString", "()Ljava/lang/String;");

  // Create the object map if we are the first clipboard
  g_map_lock.Get().Acquire();
  if (!g_map)
    g_map = new ClipboardMap;
  g_map_lock.Get().Release();
}

Clipboard::~Clipboard() {
}

// Main entry point used to write several values in the clipboard.
void Clipboard::WriteObjects(const ObjectMap& objects) {
  Clear();
  for (ObjectMap::const_iterator iter = objects.begin();
       iter != objects.end(); ++iter) {
    DispatchObject(static_cast<ObjectType>(iter->first), iter->second);
  }
}

void Clipboard::ClearInternalClipboard() const {
  base::AutoLock lock(g_map_lock.Get());
  g_map->clear();
}

void Clipboard::Clear() {
  JNIEnv* env = AttachCurrentThread();
  env->CallVoidMethod(clipboard_manager_.obj(), set_text_, NULL);
  ClearInternalClipboard();
}

void Clipboard::Set(const std::string& key, const std::string& value) {
  base::AutoLock lock(g_map_lock.Get());
  (*g_map)[key] = value;
}

void Clipboard::WriteText(const char* text_data, size_t text_len) {
  // Write the text in the Android Clipboard.
  JNIEnv* env = AttachCurrentThread();
  DCHECK(env);
  char buff[255];
  char *ptr;

  if (text_len < (sizeof(buff) - 1))
    ptr = buff;
  else
    ptr = new char[text_len+1];

  memcpy(ptr, text_data, text_len);
  ptr[text_len] = '\0';

  ScopedJavaLocalRef<jstring> str(env, env->NewStringUTF(ptr));
  DCHECK(str.obj() && !ClearException(env));

  env->CallVoidMethod(clipboard_manager_.obj(), set_text_, str.obj());

  if (ptr != buff)
    delete[] ptr;

  // Then write it in our internal data structure. We keep it there to check if
  // another app performed a copy. See ValidateInternalClipboard()
  Set(kPlainTextFormat, std::string(text_data, text_len));
}

void Clipboard::WriteHTML(const char* markup_data,
                          size_t markup_len,
                          const char* url_data,
                          size_t url_len) {
  static const char* html_prefix = "<meta http-equiv=\"content-type\" "
                                   "content=\"text/html; charset=utf-8\">";

  std::string value(html_prefix);
  value.append(std::string(markup_data, markup_len));
  Set(kHTMLFormat, value);
}

// Write an extra flavor that signifies WebKit was the last to modify the
// pasteboard. This flavor has no data.
void Clipboard::WriteWebSmartPaste() {
  Set(kWebKitSmartPasteFormat, std::string(""));
}

// All platforms use gfx::Size for size data but it is passed as a const char*
// Further, pixel_data is expected to be 32 bits per pixel
// Note: we implement this to pass all unit tests but it is currently unclear
// how some code would consume this.
void Clipboard::WriteBitmap(const char* pixel_data, const char* size_data) {
  const gfx::Size* size = reinterpret_cast<const gfx::Size*>(size_data);
  int bm_size = size->width() * size->height() * 4;
  int total_size = (sizeof(int) * 2) + bm_size;
  char* tmp = new char[total_size];

  char* p = tmp;
  int n = size->width();
  memcpy(p, (unsigned char*)&n, sizeof(int));
  p += sizeof(int);
  n = size->height();
  memcpy(p, (unsigned char*)&n, sizeof(int));
  p += sizeof(int);
  memcpy(p, pixel_data, bm_size);

  Set(kBitmapFormat, std::string(tmp, total_size));

  delete [] tmp;
}

// Note: according to other platforms implementations, this really writes the
// URL spec.
void Clipboard::WriteBookmark(const char* title_data, size_t title_len,
                              const char* url_data, size_t url_len) {
  Set(kBookmarkFormat, std::string(url_data, url_len));
}

void Clipboard::WriteData(const Clipboard::FormatType& format,
                          const char* data_data, size_t data_len) {
  Set(format.ToString(), std::string(data_data, data_len));
}

bool Clipboard::IsTextAvailableFromAndroid() const {
  JNIEnv* env = AttachCurrentThread();
  return env->CallBooleanMethod(clipboard_manager_.obj(), has_text_);
}

void Clipboard::ValidateInternalClipboard() const {
  JNIEnv* env = AttachCurrentThread();

  // First collect what text we currently have in our internal clipboard.
  bool has_internal_text;
  std::string internal_text;
  g_map_lock.Get().Acquire();
  ClipboardMap::const_iterator it = g_map->find(kPlainTextFormat);
  if (it != g_map->end()) {
    has_internal_text = true;
    internal_text = it->second;
  } else {
    has_internal_text = false;
  }
  g_map_lock.Get().Release();

  if (IsTextAvailableFromAndroid()) {
    // Make sure the text in the Android Clipboard matches what we think it
    // should be.
    ScopedJavaLocalRef<jstring> tmp_string(env,
        static_cast<jstring>(env->CallObjectMethod(
            env->CallObjectMethod(clipboard_manager_.obj(), get_text_),
            to_string_)));
    std::string android_text = ConvertJavaStringToUTF8(tmp_string);
    // If the android text doesn't match what we think it should be, our
    // internal representation is no longer valid.
    if (android_text.compare(internal_text))
      ClearInternalClipboard();
  } else {
    // If Android has no text but we have some internal text, our internal
    // representation is no longer valid.
    if (has_internal_text)
      ClearInternalClipboard();
  }
}

bool Clipboard::IsFormatAvailable(const Clipboard::FormatType& format,
                                  Clipboard::Buffer buffer) const {
  if (buffer != BUFFER_STANDARD)
    return false;

  if (!format.compare(kPlainTextFormat))
    return IsTextAvailableFromAndroid();

  ValidateInternalClipboard();

  base::AutoLock lock(g_map_lock.Get());
  return g_map->find(format.ToString()) != g_map->end();
}

void Clipboard::ReadAvailableTypes(Buffer buffer, std::vector<string16>* types,
                                   bool* contains_filenames) const {
  if (!types || !contains_filenames) {
    NOTREACHED();
    return;
  }

  // This is unimplemented on the other platforms (e.g. win, linux).
  // The NOTIMPLEMENTED is a warning to Clankers that we need to
  // implement this when the others get implemented.
  NOTIMPLEMENTED();

  types->clear();
  *contains_filenames = false;
}

void Clipboard::ReadText(Clipboard::Buffer buffer, string16* result) const {
  JNIEnv* env = AttachCurrentThread();

  result->clear();
  if (!env->CallBooleanMethod(clipboard_manager_.obj(), has_text_))
    return;

  ScopedJavaLocalRef<jobject> char_seq_text(env,
      env->CallObjectMethod(clipboard_manager_.obj(), get_text_));
  ScopedJavaLocalRef<jstring> tmp_string(env,
      static_cast<jstring>(env->CallObjectMethod(char_seq_text.obj(),
                                                 to_string_)));
  *result = ConvertJavaStringToUTF16(tmp_string);
}

void Clipboard::ReadAsciiText(Clipboard::Buffer buffer,
                              std::string* result) const {
  JNIEnv* env = AttachCurrentThread();

  result->clear();
  if (!env->CallBooleanMethod(clipboard_manager_.obj(), has_text_))
    return;

  ScopedJavaLocalRef<jobject> char_seq_text(env,
      env->CallObjectMethod(clipboard_manager_.obj(), get_text_));
  ScopedJavaLocalRef<jstring> tmp_string(env,
      static_cast<jstring>(env->CallObjectMethod(char_seq_text.obj(),
                                                 to_string_)));
  *result = ConvertJavaStringToUTF8(tmp_string);
}

// Note: |src_url| isn't really used. It is only implemented in Windows
void Clipboard::ReadHTML(Clipboard::Buffer buffer, string16* markup,
                         std::string* src_url, uint32* fragment_start,
                         uint32* fragment_end) const {
  markup->clear();
  if (src_url)
    src_url->clear();
  *fragment_start = 0;
  *fragment_end = 0;

  std::string input;

  ValidateInternalClipboard();

  g_map_lock.Get().Acquire();
  ClipboardMap::const_iterator it = g_map->find(kHTMLFormat);
  if (it != g_map->end())
    input = it->second;
  g_map_lock.Get().Release();

  if (input.empty())
    return;

  *fragment_end = static_cast<uint32>(input.length());

  UTF8ToUTF16(input.c_str(), input.length(), markup);
}

SkBitmap Clipboard::ReadImage(Buffer buffer) const {
  SkBitmap bmp;

  ValidateInternalClipboard();

  g_map_lock.Get().Acquire();
  ClipboardMap::const_iterator it = g_map->find(kBitmapFormat);
  if (it != g_map->end() && !it->second.empty()) {
    bmp.lockPixels();
    memcpy(bmp.getPixels(), it->second.c_str(), it->second.length());
    bmp.unlockPixels();
    // bmp.copyPixeslFrom was removed. Replaced it with the above which is
    // logically equivalent but untested.
    //bmp.copyPixelsFrom(it->second.c_str(), it->second.length());
    // TODO: We currently do not remember the width and height of the bitmap,
    // so we cannot create a SkBitmap config here.
    NOTIMPLEMENTED();
  }
  g_map_lock.Get().Release();

  return bmp;
}

void Clipboard::ReadData(const Clipboard::FormatType& format,
                         std::string* result) const {
  result->clear();

  // If ReadData is called for some text, just use the basic call
  if (!format.compare(kPlainTextFormat)) {
    ReadAsciiText(BUFFER_STANDARD, result);
    return;
  }

  ValidateInternalClipboard();

  g_map_lock.Get().Acquire();
  ClipboardMap::const_iterator it = g_map->find(format.ToString());
  if (it != g_map->end())
    result->assign(it->second);
  g_map_lock.Get().Release();
}

void Clipboard::ReadCustomData(Buffer buffer,
                               const string16& type,
                               string16* result) const {
  NOTIMPLEMENTED();
}

void Clipboard::ReadBookmark(string16* title, std::string* url) const {
  NOTIMPLEMENTED();
}

uint64 Clipboard::GetSequenceNumber(Clipboard::Buffer /* buffer */) {
  // TODO(): implement this. For now this interface will advertise
  // that the clipboard never changes. That's fine as long as we
  // don't rely on this signal.
  return 0;
}

// static
Clipboard::FormatType Clipboard::GetFormatType(
    const std::string& format_string) {
  return FormatType::Deserialize(format_string);
}

// static
const Clipboard::FormatType& Clipboard::GetPlainTextFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kPlainTextFormat));
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetPlainTextWFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kPlainTextFormat));
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetHtmlFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kHTMLFormat));
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetBitmapFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kBitmapFormat));
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetWebKitSmartPasteFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kWebKitSmartPasteFormat));
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetWebCustomDataFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kMimeTypeWebCustomData));
  return type;
}

} // namespace ui
