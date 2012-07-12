// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/infobar_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "chrome/browser/infobars/infobar.h"
#include "chrome/browser/infobars/infobar_delegate.h"
#include "chrome/browser/infobars/infobar_tab_helper.h"
#include "chrome/browser/tab_contents/auto_login_infobar_delegate.h"
#include "chrome/browser/tab_contents/confirm_infobar_delegate.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/common/chrome_notification_types.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "clank/native/framework/chrome/tab.h"
#include "content/browser/android/chrome_view.h"
#include "jni/info_bar_container_jni.h"
#include "ui/gfx/image/image.h"

using base::android::AttachCurrentThread;
using base::android::CheckException;
using base::android::ConvertUTF16ToJavaString;
using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;
using content::WebContents;

InfoBar::InfoBar(InfoBarDelegate* delegate)
    : container_(NULL),
      delegate_(delegate),
      closing_(false) {
}

InfoBar::~InfoBar() {
}

void InfoBar::set_java_infobar(const JavaRef<jobject>& java_info_bar) {
  DCHECK(java_info_bar_.is_null());
  java_info_bar_.Reset(java_info_bar);
}

void InfoBar::set_infobar_container(InfoBarContainerAndroid* container) {
   DCHECK(!container_);
   container_ = container;
 }

void InfoBar::OnConfirmClicked(JNIEnv* env, jobject obj, bool confirm) {
  DCHECK(delegate_->AsConfirmInfoBarDelegate());
  if (confirm)
    delegate_->AsConfirmInfoBarDelegate()->Accept();
  else
    delegate_->AsConfirmInfoBarDelegate()->Cancel();
  Close(true);
}

void InfoBar::OnAutoLogin(JNIEnv* env, jobject obj, jstring jurl) {
  DCHECK(delegate_);
  DCHECK(delegate_->AsXAutoLoginInfoBarDelegate());
  std::string url = base::android::ConvertJavaStringToUTF8(env, jurl);
  delegate_->AsXAutoLoginInfoBarDelegate()->Login(url);
}

ScopedJavaLocalRef<jstring> InfoBar::GetAutoLoginMessage(
    JNIEnv* env, jobject obj, jstring jaccount) {
  DCHECK(delegate_);
  DCHECK(delegate_->AsXAutoLoginInfoBarDelegate());
  string16 account = ConvertJavaStringToUTF16(env, jaccount);
  string16 message_text = delegate_->AsXAutoLoginInfoBarDelegate()->GetMessageText(account);
  return ConvertUTF16ToJavaString(env, message_text);
}

void InfoBar::OnInfoBarClosed(JNIEnv* env, jobject obj) {
  java_info_bar_.Reset();  // So we don't notify Java.
  Close(true);
}

// |remove_from_tab_contents_wrapper| is necessary when the infobar closing was
// triggered by native code. In that case, the removal happens as part of a
// notification (see the Observe method below), and attempting to remove the
// infobar while the TabContentsWrapper is already removing it is a bad idea,
// resulting in crashers.
void InfoBar::Close(bool remove_from_tab_contents_wrapper) {
  if (closing_)
    return;

  // Removing the infobar from the wrapper will end up calling Close again.
  // We use closing_ to prevent this.
  closing_ = true;
  if (delegate_) {
    if (remove_from_tab_contents_wrapper) {
      TabContentsWrapper* wrapper =
          TabContentsWrapper::GetCurrentWrapperForContents(
              container_->web_contents());
      wrapper->infobar_tab_helper()->RemoveInfoBar(delegate_);
    }
    if (!java_info_bar_.is_null()) {
      JNIEnv* env = AttachCurrentThread();
      Java_NativeInfoBar_dismiss(env, java_info_bar_.obj());
    }

    delegate_->InfoBarClosed();
    delegate_ = NULL;
  }
  delete this;
}

// InfoBarContainerAndroid
InfoBarContainerAndroid::InfoBarContainerAndroid(JNIEnv* env,
                                                 jobject obj,
                                                 WebContents* web_contents)
    : web_contents_(web_contents),
      weak_java_infobar_container_(env, obj) {
  DCHECK(web_contents);
  RegisterTabContentsNotifications(web_contents);
}

InfoBarContainerAndroid::~InfoBarContainerAndroid() {
  DCHECK(delegate_to_info_bar_map_.empty());
}

void InfoBarContainerAndroid::OnTabContentsReplaced(
    WebContents* old_tab_contents, WebContents* new_tab_contents) {
  DCHECK(old_tab_contents == web_contents_);
  web_contents_ = new_tab_contents;

  // The old tab contents is going to be destroyed, deregister its notifications
  // and remove any pending infobars it had.
  if (old_tab_contents) {
    UnregisterTabContentsNotifications(old_tab_contents);
    TabContentsWrapper* old_tab_contents_wrapper =
        TabContentsWrapper::GetCurrentWrapperForContents(old_tab_contents);
    RemovePendingInfoBars(old_tab_contents_wrapper);
  }

  // Now registers the new tab contents and create any pending infobar it may
  // already have.
  if (new_tab_contents) {
    RegisterTabContentsNotifications(new_tab_contents);
    TabContentsWrapper* new_tab_contents_wrapper =
        TabContentsWrapper::GetCurrentWrapperForContents(new_tab_contents);
    UpdateInfoBars(new_tab_contents_wrapper);
  }
}

void InfoBarContainerAndroid::RemovePendingInfoBars(
    TabContentsWrapper* tab_contents_wrapper) {
  while (!delegate_to_info_bar_map_.empty()) {
    DelegateToInfoBarMap::iterator i(delegate_to_info_bar_map_.begin());
    tab_contents_wrapper->infobar_tab_helper()->RemoveInfoBar(i->first);

    // TabContentsWrapper::RemoveInfoBar(), called above, posts a
    // TAB_CONTENTS_INFOBAR_REMOVED notification.  We (the
    // InfoBarContainerAndroid) listen for that notification and, as a
    // result, will both Close() the infobar and remove it from our
    // delegate_to_info_bar_map_.  Thus, after the call, "i" is
    // invalid and there is no need to Close().

    // TODO(jrg): tell bulach in case there are other similar issues
    // we need to work out.  When done delete the commented out code below.
    /*
    i->second->Close();
    delegate_to_info_bar_map_.erase(i);
    */
  }
}

void InfoBarContainerAndroid::UpdateInfoBars(
    TabContentsWrapper* tab_contents_wrapper) {
  for (size_t i = 0; i < tab_contents_wrapper->infobar_tab_helper()->infobar_count(); ++i) {
    InfoBarDelegate* delegate = tab_contents_wrapper->infobar_tab_helper()->GetInfoBarDelegateAt(i);
    AddInfoBar(delegate);
  }
}

void InfoBarContainerAndroid::RegisterTabContentsNotifications(
    WebContents* web_contents) {
  DCHECK(web_contents);
  registrar_.Add(this, chrome::NOTIFICATION_CHROME_VIEW_TAB_CONTENTS_REPLACED,
                 content::Source<WebContents>(web_contents));

  TabContentsWrapper* wrapper =
      TabContentsWrapper::GetCurrentWrapperForContents(web_contents);
  InfoBarTabHelper* infobar_helper = wrapper->infobar_tab_helper();

  content::Source<InfoBarTabHelper> source(infobar_helper);
  registrar_.Add(this, chrome::NOTIFICATION_TAB_CONTENTS_INFOBAR_ADDED,
                 source);
  registrar_.Add(this, chrome::NOTIFICATION_TAB_CONTENTS_INFOBAR_REMOVED,
                 source);
  registrar_.Add(this, chrome::NOTIFICATION_TAB_CONTENTS_INFOBAR_REPLACED,
                 source);
}

void InfoBarContainerAndroid::UnregisterTabContentsNotifications(
    WebContents* web_contents) {
  DCHECK(web_contents);
  registrar_.Remove(this,
                    chrome::NOTIFICATION_CHROME_VIEW_TAB_CONTENTS_REPLACED,
                    content::Source<WebContents>(web_contents));

  TabContentsWrapper* wrapper =
      TabContentsWrapper::GetCurrentWrapperForContents(web_contents);
  InfoBarTabHelper* infobar_helper = wrapper->infobar_tab_helper();

  content::Source<InfoBarTabHelper> source(infobar_helper);
  registrar_.Remove(this, chrome::NOTIFICATION_TAB_CONTENTS_INFOBAR_ADDED,
                    source);
  registrar_.Remove(this, chrome::NOTIFICATION_TAB_CONTENTS_INFOBAR_REMOVED,
                    source);
  registrar_.Remove(this, chrome::NOTIFICATION_TAB_CONTENTS_INFOBAR_REPLACED,
                    source);
}

void InfoBarContainerAndroid::Observe(int type,
                                      const content::NotificationSource& source,
                                      const content::NotificationDetails& details) {
  if (type == chrome::NOTIFICATION_CHROME_VIEW_TAB_CONTENTS_REPLACED) {
    OnTabContentsReplaced(content::Source<WebContents>(source).ptr(),
                          content::Details<WebContents>(details).ptr());
  } else if (type == chrome::NOTIFICATION_TAB_CONTENTS_INFOBAR_ADDED) {
    AddInfoBar(content::Details<InfoBarDelegate>(details).ptr());
  } else if (type == chrome::NOTIFICATION_TAB_CONTENTS_INFOBAR_REMOVED) {
    std::pair<InfoBarDelegate*, bool>* deleted_delegate_details =
        content::Details<std::pair<InfoBarDelegate*, bool> >(details).ptr();
    RemoveInfoBar(deleted_delegate_details->first);
  } else if (type == chrome::NOTIFICATION_TAB_CONTENTS_INFOBAR_REPLACED) {
    InfoBarReplacedDetails* delegates =
        content::Details<InfoBarReplacedDetails>(details).ptr();

    RemoveInfoBar(delegates->first);
    AddInfoBar(delegates->second);
  } else {
    NOTREACHED();
  }
}

void InfoBarContainerAndroid::AddInfoBar(InfoBarDelegate* delegate) {
  if (delegate->AsConfirmInfoBarDelegate()) {
    AddConfirmInfoBar(delegate);
  } else if (delegate->AsXAutoLoginInfoBarDelegate()) {
    AddAutoLoginInfoBar(delegate);
  } else {
    // TODO(bulach): CLANK: implement other types of InfoBars.
    // TODO(jrg): this will always print out WARNING_TYPE as an int.
    // Try and be more helpful.
    NOTIMPLEMENTED() << "CLANK: infobar type " << delegate->GetInfoBarType();
  }
}

void InfoBarContainerAndroid::AddConfirmInfoBar(InfoBarDelegate* delegate) {
  ConfirmInfoBarDelegate* confirm_info_bar_delegate =
      delegate->AsConfirmInfoBarDelegate();
  DCHECK(confirm_info_bar_delegate);
  InfoBar* info_bar = delegate->CreateInfoBar(NULL);
  info_bar->set_infobar_container(this);

  JNIEnv* env = AttachCurrentThread();
  const gfx::Image* original_icon = confirm_info_bar_delegate->GetIcon();
  ScopedJavaLocalRef<jobject> icon;
  if (original_icon && original_icon->GetNumberOfSkBitmaps()) {
    // Creates a java version of the bitmap and makes a copy of the pixels
    icon = ConvertToJavaBitmap(original_icon->GetSkBitmapAtIndex(0));
  }

  int buttons = confirm_info_bar_delegate->GetButtons();
  ScopedJavaLocalRef<jstring> ok_button_text;
  if ((buttons & ConfirmInfoBarDelegate::BUTTON_OK) ==
      ConfirmInfoBarDelegate::BUTTON_OK) {
    ok_button_text = ConvertUTF16ToJavaString(env,
        confirm_info_bar_delegate->GetButtonLabel(
            ConfirmInfoBarDelegate::BUTTON_OK));
  }

  ScopedJavaLocalRef<jstring> cancel_button_text;
  if ((buttons & ConfirmInfoBarDelegate::BUTTON_CANCEL) ==
      ConfirmInfoBarDelegate::BUTTON_CANCEL) {
    cancel_button_text = ConvertUTF16ToJavaString(env,
        confirm_info_bar_delegate->GetButtonLabel(
            ConfirmInfoBarDelegate::BUTTON_CANCEL));
  }

  ScopedJavaLocalRef<jstring> message_text =
      ConvertUTF16ToJavaString(env,
                               confirm_info_bar_delegate->GetMessageText());

  ScopedJavaLocalRef<jobject> java_infobar =
      Java_InfoBarContainer_showConfirmInfoBar(
          env,
          weak_java_infobar_container_.get(env).obj(),
          reinterpret_cast<jint>(info_bar),
          ok_button_text.obj(), cancel_button_text.obj(),
          message_text.obj(),
          icon.obj());

  info_bar->set_java_infobar(java_infobar);
  delegate_to_info_bar_map_[delegate] = info_bar;
}

void InfoBarContainerAndroid::AddAutoLoginInfoBar(InfoBarDelegate* delegate) {
  XAutoLoginInfoBarDelegate* auto_login_info_bar_delegate =
      delegate->AsXAutoLoginInfoBarDelegate();
  DCHECK(auto_login_info_bar_delegate);
  InfoBar* info_bar = delegate->CreateInfoBar(NULL);
  info_bar->set_infobar_container(this);

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> jinfobar_container =
      weak_java_infobar_container_.get(env);
  DCHECK(jinfobar_container.obj());

  ScopedJavaLocalRef<jstring> jrealm = ConvertUTF8ToJavaString(
      env, auto_login_info_bar_delegate->params().realm());
  ScopedJavaLocalRef<jstring> jaccount = ConvertUTF8ToJavaString(
      env, auto_login_info_bar_delegate->params().account());
  ScopedJavaLocalRef<jstring> jargs = ConvertUTF8ToJavaString(
      env, auto_login_info_bar_delegate->params().args());
  DCHECK(!jrealm.is_null() && !jaccount.is_null() && !jargs.is_null());

  ScopedJavaLocalRef<jstring> ok_button_text = ConvertUTF16ToJavaString(env,
      auto_login_info_bar_delegate->GetButtonLabel(
          ConfirmInfoBarDelegate::BUTTON_OK));
  ScopedJavaLocalRef<jstring> cancel_button_text = ConvertUTF16ToJavaString(env,
      auto_login_info_bar_delegate->GetButtonLabel(
          ConfirmInfoBarDelegate::BUTTON_CANCEL));

  ScopedJavaLocalRef<jobject> java_infobar =
      Java_InfoBarContainer_showAutoLoginInfoBar(
          env, jinfobar_container.obj(), reinterpret_cast<jint>(info_bar),
          jrealm.obj(), jaccount.obj(), jargs.obj(),
          ok_button_text.obj(), cancel_button_text.obj());

  if (!java_infobar.is_null()) {
    info_bar->set_java_infobar(java_infobar);
    delegate_to_info_bar_map_[delegate] = info_bar;
  } else {
    // This is not an error as such; there may not be auto-login-able
    // accounts to select from.
    VLOG(1) << "No auto-login-able accounts; infobar not shown.";
    info_bar->Close(false);  // will delete the info_bar object
  }
}

void InfoBarContainerAndroid::RemoveInfoBar(InfoBarDelegate* delegate) {
  DelegateToInfoBarMap::iterator i(delegate_to_info_bar_map_.find(delegate));
  if (i != delegate_to_info_bar_map_.end()) {
    i->second->Close(false);
    delegate_to_info_bar_map_.erase(i);
  }
}

void InfoBarContainerAndroid::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

// Clank implementation of cross-platform API
// TODO(bulach): CLANK: implement other types of InfoBars.
InfoBar* ConfirmInfoBarDelegate::CreateInfoBar(InfoBarTabHelper* owner) {
  return new InfoBar(this);
}

InfoBar* XAutoLoginInfoBarDelegate::CreateInfoBar(InfoBarTabHelper* owner) {
  return new InfoBar(this);
}

bool XAutoLoginInfoBarDelegate::ShouldShowInfoBar(
    WebContents* web_contents, const Params& autologin_params) {
  RenderWidgetHostViewAndroid* rwhv = static_cast<RenderWidgetHostViewAndroid*>(
      web_contents->GetRenderWidgetHostView());
  DCHECK(rwhv);

  ChromeView* chrome_view = rwhv->GetChromeView();
  // RWHVA holds a weak pointer to chrome view.
  if(chrome_view == NULL) {
    LOG(ERROR) << "Could not get chrome view from RenderWidgetHostViewAndroid";
    return false;
  }

  jobject java_chrome_view = chrome_view->GetJavaObject();

  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> jrealm =
      ConvertUTF8ToJavaString(env, autologin_params.realm());
  ScopedJavaLocalRef<jstring> jaccount =
      ConvertUTF8ToJavaString(env, autologin_params.account());
  ScopedJavaLocalRef<jstring> jargs =
      ConvertUTF8ToJavaString(env, autologin_params.args());
  DCHECK(!jrealm.is_null() && !jaccount.is_null() && !jargs.is_null());

  return Java_InfoBarContainer_shouldShowAutoLogin(env, java_chrome_view,
      jrealm.obj(), jaccount.obj(), jargs.obj());
}


// ----------------------------------------------------------------------------
// Native JNI methods
// ----------------------------------------------------------------------------

static int Init(JNIEnv* env, jobject obj, jobject chrome_view) {
  InfoBarContainerAndroid* infobar_container =
      new InfoBarContainerAndroid(
          env, obj,
          ChromeView::GetNativeChromeView(env, chrome_view)->web_contents());
  return reinterpret_cast<int>(infobar_container);
}

// Register native methods
bool RegisterInfoBarContainer(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
