// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/page_info_viewer.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "chrome/browser/page_info_model.h"
#include "chrome/browser/profiles/profile.h"
#include "content/browser/android/chrome_view.h"
#include "content/browser/cert_store.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"

using base::android::CheckException;
using base::android::ConvertUTF16ToJavaString;
using base::android::GetClass;
using base::android::GetMethodID;
using base::android::ScopedJavaLocalRef;
using content::WebContents;

namespace {

jobjectArray GetCertChain(JNIEnv* env, jobject obj, jobject view) {
  content::WebContents* contents =
      ChromeView::GetNativeChromeView(env, view)->web_contents();
  int cert_id = contents->GetController().GetActiveEntry()->GetSSL().cert_id;
  scoped_refptr<net::X509Certificate> cert;
  bool ok = CertStore::GetInstance()->RetrieveCert(cert_id, &cert);
  CHECK(ok);

  std::vector<std::string> cert_chain;
  cert->GetChainDEREncodedBytes(&cert_chain);

  // OK to release, JNI binding.
  return base::android::ToJavaArrayOfByteArray(env, cert_chain).Release();
}

}  // namespace

void PageInfoViewer::Show(JNIEnv* env,
                          jobject context,
                          jobject java_chrome_view,
                          WebContents* web_contents) {
  if (web_contents->GetController().GetActiveEntry() == NULL)
    return;

  ScopedJavaLocalRef<jclass> clazz =
      GetClass(env, "com/google/android/apps/chrome/PageInfoViewer");
  static JNINativeMethod kMethods[] = {
    { "nativeGetCertChain", "(Lorg/chromium/content/browser/ChromeView;)[[B",
        reinterpret_cast<void*>(GetCertChain) },
  };
  env->RegisterNatives(clazz.obj(), kMethods, arraysize(kMethods));

  jmethodID create = GetMethodID(env, clazz, "<init>",
                                 "(Landroid/content/Context;"
                                 "Lorg/chromium/content/browser/ChromeView;)V");
  ScopedJavaLocalRef<jobject> page_info_viewer(env,
      env->NewObject(clazz.obj(), create, context, java_chrome_view));

  jmethodID add_section = GetMethodID(env, clazz, "addSection",
      "(Landroid/graphics/Bitmap;Ljava/lang/String;Ljava/lang/String;)V");

  jmethodID set_certificate_viewer =
      GetMethodID(env, clazz, "setCertificateViewer", "(Ljava/lang/String;)V");

  jmethodID add_more_info_link =
      GetMethodID(env, clazz, "addMoreInfoLink", "()V");

  jmethodID add_divider = GetMethodID(env, clazz, "addDivider", "()V");

  PageInfoModel page_info_model(
      Profile::FromBrowserContext(web_contents->GetBrowserContext()),
      web_contents->GetURL(),
      web_contents->GetController().GetActiveEntry()->GetSSL(), true, NULL);

  for (int i = 0; i < page_info_model.GetSectionCount(); ++i) {
    PageInfoModel::SectionInfo section = page_info_model.GetSectionInfo(i);
    gfx::Image* icon_image = page_info_model.GetIconImage(section.icon_id);
    const SkBitmap* original_icon = *icon_image;
    // Creates a java version of the bitmap and makes a copy of the pixels
    ScopedJavaLocalRef<jobject> icon = ConvertToJavaBitmap(original_icon);

    ScopedJavaLocalRef<jstring> headline =
        ConvertUTF16ToJavaString(env, section.headline);
    ScopedJavaLocalRef<jstring> description =
        ConvertUTF16ToJavaString(env, section.description);
    env->CallVoidMethod(page_info_viewer.obj(), add_section, icon.obj(),
                        headline.obj(), description.obj());
    CheckException(env);

    string16 certificate_label = page_info_model.GetCertificateLabel();
    if (section.type == PageInfoModel::SECTION_INFO_IDENTITY &&
        !certificate_label.empty()) {
      ScopedJavaLocalRef<jstring> j_certificate_label =
          ConvertUTF16ToJavaString(env, certificate_label);
      env->CallVoidMethod(page_info_viewer.obj(), set_certificate_viewer,
                          j_certificate_label.obj());
      CheckException(env);
    }

    env->CallVoidMethod(page_info_viewer.obj(), add_divider);
    CheckException(env);
  }

  // TODO(bulach): add "what do these mean?" url link once we merge r65683.
  env->CallVoidMethod(page_info_viewer.obj(), add_more_info_link);
  CheckException(env);

  jmethodID show = GetMethodID(env, clazz, "show", "()V");
  env->CallVoidMethod(page_info_viewer.obj(), show);
  CheckException(env);
}
