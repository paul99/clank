// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clank/native/framework/chrome/uma_record_action.h"

#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "content/browser/android/jni_helper.h"
#include "content/public/browser/user_metrics.h"
#include "jni/uma_record_action_jni.h"

namespace {

static void RecordNewTabOpened() {
  // Should be called when a new tab is opened:
  //  - From menu
  //  - From context menu
  //  - From tab strip
  //  - From Toolbar
  //  - From stack view toolbar
  content::RecordAction(content::UserMetricsAction("MobileNewTabOpened"));
}

static void RecordTabClosed() {
  // Should be called when a tab is closed:
  //  - From menu
  //  - From Tab strip
  //  - From stack view
  content::RecordAction(content::UserMetricsAction("MobileTabClosed"));
}

//First Run Experience Related
static const int MOBILE_FIRST_RUN_FINISH_STATES = 3;

enum {
  MOBILE_FIRST_RUN_SKIP_SIGN_IN,
  MOBILE_FIRST_RUN_SIGN_IN_SUCCESS,
  MOBILE_FIRST_RUN_SKIP_AFTER_FAIL,
};

static std::vector<int> GetAllFREFinishStates() {
  std::vector<int> states;
  for (int i = 0; i < MOBILE_FIRST_RUN_FINISH_STATES; i++)
    states.push_back(i);
  return states;
}

}  // namespace

static void RecordTabClobbered(JNIEnv*, jclass) {
  // Should be called on:
  //  - Any page navigation
  //  - Forward and back navagation
  //  - Omnibox rewrite
  //  - Link opened in same tab using context menu.
  content::RecordAction(content::UserMetricsAction("MobileTabClobbered"));
}

static void RecordTabSwitched(JNIEnv*, jclass) {
  // Should be called whenever the user switches to a different tab.
  content::RecordAction(content::UserMetricsAction("MobileTabSwitched"));
}

static void RecordSideSwipeFinished(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileSideSwipeFinished"));
}

static void RecordPageLoaded(JNIEnv*, jclass, jint num_tabs,
      jint num_incognito_tabs, jboolean is_desktop_user_agent) {
  // Should be called whenever a tab is selected.
  content::RecordAction(content::UserMetricsAction("MobilePageLoaded"));
  if (is_desktop_user_agent) {
    content::RecordAction(
        content::UserMetricsAction("MobilePageLoadedDesktopUserAgent"));
  }
  // Record how many tabs total are open.
  UMA_HISTOGRAM_CUSTOM_COUNTS("Tabs.TabCountPerLoad", num_tabs, 1, 200, 50);
  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "Tabs.IncognitoTabCountPerLoad", num_incognito_tabs, 1, 200, 50);
}

static void RecordPageLoadedWithKeyboard(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobilePageLoadedWithKeyboard"));
}

static void UmaRecordBack(JNIEnv* env, jclass clazz, jboolean fromMenu) {
  if (fromMenu) {
    content::RecordAction(content::UserMetricsAction("MobileMenuBack"));
  } else {
    content::RecordAction(content::UserMetricsAction("MobileToolbarBack"));
  }
  RecordTabClobbered(env, clazz);
}

static void UmaRecordSystemBack(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("SystemBack"));
}

static void UmaRecordSystemBackForNavigation(JNIEnv* env, jclass clazz) {
  content::RecordAction(content::UserMetricsAction("SystemBackForNavigation"));
  RecordTabClobbered(env, clazz);
}

static void UmaRecordForward(JNIEnv* env, jclass clazz, jboolean fromMenu) {
  if (fromMenu) {
    content::RecordAction(content::UserMetricsAction("MobileMenuForward"));
  } else {
    content::RecordAction(content::UserMetricsAction("MobileToolbarForward"));
  }
  RecordTabClobbered(env, clazz);
}

static void UmaRecordReload(JNIEnv*, jclass, jboolean fromMenu) {
  if (fromMenu) {
    content::RecordAction(content::UserMetricsAction("MobileMenuReload"));
  } else {
    content::RecordAction(content::UserMetricsAction("MobileToolbarReload"));
  }
}

// Menu actions.
static void RecordMenuNewTab(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileMenuNewTab"));
  RecordNewTabOpened();
}

static void RecordMenuCloseTab(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileMenuCloseTab"));
  RecordTabClosed();
}

static void RecordMenuCloseAllTabs(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileMenuCloseAllTabs"));
}

static void RecordMenuNewIncognitoTab(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileMenuNewIncognitoTab"));
  RecordNewTabOpened();
}

static void RecordMenuAllBookmarks(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileMenuAllBookmarks"));
}

static void RecordMenuOpenTabs(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileMenuOpenTabs"));
}

static void RecordMenuAddToBookmarks(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileMenuAddToBookmarks"));
}

static void RecordMenuShare(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileMenuShare"));
}

static void RecordMenuFindInPage(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileMenuFindInPage"));
}

static void RecordMenuFullscreen(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileMenuFullscreen"));
}

static void RecordMenuSettings(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileMenuSettings"));
}

static void RecordMenuFeedback(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileMenuFeedback"));
}

static void RecordMenuShow(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileMenuShow"));
}

// Toolbar and TabStrip actions
static void RecordToolbarPhoneNewTab(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileToolbarNewTab"));
  RecordNewTabOpened();
}

static void RecordToolbarPhoneTabStack(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileToolbarShowStackView"));
}

static void RecordTabStripNewTab(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileTabStripNewTab"));
  RecordNewTabOpened();
}

static void RecordTabStripCloseTab(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileTabStripCloseTab"));
  RecordTabClosed();
}

static void RecordToolbarToggleBookmark(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileToolbarToggleBookmark"));
}

static void RecordToolbarShowMenu(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileToolbarShowMenu"));
}

// Misc
static void RecordStackViewNewTab(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileToolbarStackViewNewTab"));
  RecordNewTabOpened();
}

static void RecordStackViewCloseTab(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileStackViewCloseTab"));
  RecordTabClosed();
}

static void RecordStackViewSwipeCloseTab(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileStackViewSwipeCloseTab"));
  RecordTabClosed();
}

static void RecordOmniboxSearch(JNIEnv* env, jclass clazz) {
  // Used for any kind of page load from the omnibox
  content::RecordAction(content::UserMetricsAction("MobileOmniboxSearch"));
  // Also counts as a page clobbered.
  RecordTabClobbered(env, clazz);
}

static void RecordOmniboxVoiceSearch(JNIEnv* env, jclass clazz) {
  // Used for voice search from the omnibox.
  content::RecordAction(content::UserMetricsAction("MobileOmniboxVoiceSearch"));
}

// Clank was opened from an external intent to view a url.
static void RecordReceivedExternalIntent(JNIEnv* env, jclass clazz) {
  content::RecordAction(
      content::UserMetricsAction("MobileReceivedExternalIntent"));
}

// NTP actions.
static void RecordNtpSectionMostVisited(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileNTPSwitchToMostVisited"));
}

static void RecordNtpSectionBookmarks(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileNTPSwitchToBookmarks"));
}

static void RecordNtpSectionOpenTabs(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileNTPSwitchToOpenTabs"));
}

static void RecordNtpSectionIncognito(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileNTPSwitchToIncognito"));
}

// Context Menu.
static void RecordContextMenuLink(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileContextMenuLink"));
}

static void RecordContextMenuImage(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileContextMenuImage"));
}

static void RecordContextMenuText(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileContextMenuText"));
}

static void RecordContextMenuOpenNewTab(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileContextMenuOpenLinkInNewTab"));
  RecordNewTabOpened();
}

static void RecordContextMenuOpenNewIncognitoTab(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileContextMenuOpenLinkInIncognito"));
  RecordNewTabOpened();
}

static void RecordContextMenuViewImage(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileContextMenuViewImage"));
}

static void RecordContextMenuCopyLinkAddress(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileContextMenuCopyLinkAddress"));
}

static void RecordContextMenuCopyLinkText(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileContextMenuCopyLinkText"));
}

static void RecordContextMenuSaveImage(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileContextMenuSaveImage"));
}

static void RecordContextMenuOpenImageInNewTab(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileContextMenuOpenImageInNewTab"));
}

//First Run Experience

static void RecordFreSkipSignIn(JNIEnv*, jclass) {
  UMA_HISTOGRAM_CUSTOM_ENUMERATION("MobileFre.FinishState",
                                   MOBILE_FIRST_RUN_SKIP_SIGN_IN,
                                   // Note the third argument is only
                                   // evaluated once, see macro
                                   // definition for details.
                                   GetAllFREFinishStates());
}

static void RecordFreSignInSuccessful(JNIEnv*, jclass) {
  UMA_HISTOGRAM_CUSTOM_ENUMERATION("MobileFre.FinishState",
                                   MOBILE_FIRST_RUN_SIGN_IN_SUCCESS,
                                   GetAllFREFinishStates());
}

static void RecordFreSignInAttempted(JNIEnv*, jclass) {
  UMA_HISTOGRAM_CUSTOM_ENUMERATION("MobileFre.FinishState",
                                   MOBILE_FIRST_RUN_SKIP_AFTER_FAIL,
                                   GetAllFREFinishStates());
}

static void RecordBeamCallbackSuccess(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileBeamCallbackSuccess"));
}

static void RecordBeamInvalidAppState(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileBeamInvalidAppState"));
}

static void RecordCrashUploadAttempt(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileBreakpadUploadAttempt"));
}

//Keyboard Shortcuts
static void RecordShortcutNewTab(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileShortcutNewTab"));
  RecordNewTabOpened();
}

static void RecordShortcutNewIncognitoTab(JNIEnv*, jclass) {
  content::RecordAction(
      content::UserMetricsAction("MobileShortcutNewIncognitoTab"));
  RecordNewTabOpened();
}

static void RecordShortcutAllBookmarks(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileShortcutAllBookmarks"));
}

static void RecordShortcutFindInPage(JNIEnv*, jclass) {
  content::RecordAction(content::UserMetricsAction("MobileShortcutFindInPage"));
}
// Register native methods
bool RegisterUmaRecordAction(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
