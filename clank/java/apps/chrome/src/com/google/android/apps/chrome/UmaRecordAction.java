// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.google.android.apps.chrome;

import android.view.MenuItem;

/**
 * Static methods to record user actions.
 *
 * We have a different native method for each action as we want to use upstream scripts to
 * extract user command keys from code and show them in the dashboard.
 * See: chromium/src/content/browser/user_metrics.h for more details.
 */
public class UmaRecordAction {
    public static void back(boolean fromMenu) {
        nativeUmaRecordBack(fromMenu);
    }

    public static void systemBack() {
        nativeUmaRecordSystemBack();
    }

    public static void systemBackForNavigation() {
        nativeUmaRecordSystemBackForNavigation();
    }
    public static void forward(boolean fromMenu) {
        nativeUmaRecordForward(fromMenu);
    }

    public static void reload(boolean fromMenu) {
        nativeUmaRecordReload(fromMenu);
    }

    // Menu actions.
    public static void menuNewTab() {
        nativeRecordMenuNewTab();
    }

    public static void menuCloseTab() {
        nativeRecordMenuCloseTab();
    }

    public static void menuCloseAllTabs() {
        nativeRecordMenuCloseAllTabs();
    }

    public static void menuNewIncognitoTab() {
        nativeRecordMenuNewIncognitoTab();
    }

    public static void menuAllBookmarks() {
        nativeRecordMenuAllBookmarks();
    }

    public static void menuOpenTabs() {
        nativeRecordMenuOpenTabs();
    }

    public static void menuAddToBookmarks() {
        nativeRecordMenuAddToBookmarks();
    }
    public static void menuShare() {
        nativeRecordMenuShare();
    }

    public static void menuFindInPage() {
        nativeRecordMenuFindInPage();
    }
    public static void menuFullscreen() {
        nativeRecordMenuFullscreen();
    }
    public static void menuSettings() {
        nativeRecordMenuSettings();
    }
    public static void menuFeedback() {
        nativeRecordMenuFeedback();
    }
    public static void menuShow() {
        nativeRecordMenuShow();
    }

    // Toolbar and TabStrip actions.
    public static void toolbarPhoneNewTab() {
        nativeRecordToolbarPhoneNewTab();
    }

    public static void toolbarPhoneTabStack() {
        nativeRecordToolbarPhoneTabStack();
    }

    public static void tabStripNewTab() {
        nativeRecordTabStripNewTab();
    }

    public static void tabStripCloseTab() {
        nativeRecordTabStripCloseTab();
    }

    public static void toolbarToggleBookmark() {
        nativeRecordToolbarToggleBookmark();
    }

    public static void toolbarShowMenu() {
        nativeRecordToolbarShowMenu();
    }

    // Misc
    public static void stackViewNewTab() {
        nativeRecordStackViewNewTab();
    }

    public static void stackViewCloseTab() {
        nativeRecordStackViewCloseTab();
    }

    public static void stackViewSwipeCloseTab() {
        nativeRecordStackViewSwipeCloseTab();
    }

    public static void omniboxSearch() {
        nativeRecordOmniboxSearch();
    }

    public static void omniboxVoiceSearch() {
        nativeRecordOmniboxVoiceSearch();
    }

    public static void tabClobbered() {
        nativeRecordTabClobbered();
    }

    public static void tabSwitched() {
        nativeRecordTabSwitched();
    }
    public static void tabSideSwipeFinished() {
        nativeRecordSideSwipeFinished();
    }

    public static void pageLoaded(int numTabsOpen, int numIncognitoTabsOpen, boolean keyboard,
            boolean isDesktopUserAgent) {
        nativeRecordPageLoaded(numTabsOpen, numIncognitoTabsOpen, isDesktopUserAgent);
        if (keyboard) {
            nativeRecordPageLoadedWithKeyboard();
        }
    }

    public static void receivedExternalIntent() {
        nativeRecordReceivedExternalIntent();
    }

    // NTP actions.
    public static void ntpSectionMostVisited() {
        nativeRecordNtpSectionMostVisited();
    }

    public static void ntpSectionBookmarks() {
        nativeRecordNtpSectionBookmarks();
    }

    public static void ntpSectionOpenTabs() {
        nativeRecordNtpSectionOpenTabs();
    }

    public static void ntpSectionIncognito() {
        nativeRecordNtpSectionIncognito();
    }

    // Context Menu.
    public static void recordShowContextMenu(boolean isImage, boolean isAnchor,
            boolean isText) {
        if (isAnchor) {
            nativeRecordContextMenuLink();
        } else if (isImage) {
            nativeRecordContextMenuImage();
        } else if (isText) {
            nativeRecordContextMenuText();
        }
    }

    // First Run Experience

    public static void freSignInAttempted() {
        nativeRecordFreSignInAttempted();
    }

    public static void freSignInSuccessful() {
        nativeRecordFreSignInSuccessful();
    }

    public static void freSkipSignIn() {
        nativeRecordFreSkipSignIn();
    }

    public static void recordContextMenuItemSelected(MenuItem contextMenuItem) {
        switch (contextMenuItem.getItemId()) {
            case R.id.open_in_new_tab_context_menu_id:
                nativeRecordContextMenuOpenNewTab();
                break;
            case R.id.open_in_incognito_tab_context_menu_id:
                nativeRecordContextMenuOpenNewIncognitoTab();
                break;
            case R.id.open_image_context_menu_id:
                nativeRecordContextMenuViewImage();
                break;
            case R.id.copy_link_address_context_menu_id:
                nativeRecordContextMenuCopyLinkAddress();
                break;
            case R.id.copy_link_text_context_menu_id:
                nativeRecordContextMenuCopyLinkText();
                break;
            case R.id.save_image_context_menu_id:
                nativeRecordContextMenuSaveImage();
                break;
            case R.id.open_image_in_new_tab_context_menu_id:
                nativeRecordContextMenuOpenImageInNewTab();
        }
    }

    public static void beamCallbackSuccess() {
        nativeRecordBeamCallbackSuccess();
    }

    public static void beamInvalidAppState() {
        nativeRecordBeamInvalidAppState();
    }

    //Keyboard Shortcuts
    public static void shortcutNewTab() {
        nativeRecordShortcutNewTab();
    }

    public static void shortcutNewIncognitoTab() {
        nativeRecordShortcutNewIncognitoTab();
    }

    public static void shortcutFindInPage() {
        nativeRecordShortcutFindInPage();
    }

    public static void shortcutAllBookmarks() {
        nativeRecordShortcutAllBookmarks();
    }

    // Crash uploads
    public static void crashUploadAttempt() {
        nativeRecordCrashUploadAttempt();
    }

    // Native methods.
    private static native void nativeUmaRecordBack(boolean fromMenu);
    private static native void nativeUmaRecordSystemBack();
    private static native void nativeUmaRecordSystemBackForNavigation();
    private static native void nativeUmaRecordForward(boolean fromMenu);
    private static native void nativeUmaRecordReload(boolean fromMenu);

    // Menu
    private static native void nativeRecordMenuNewTab();
    private static native void nativeRecordMenuCloseTab();
    private static native void nativeRecordMenuCloseAllTabs();
    private static native void nativeRecordMenuNewIncognitoTab();
    private static native void nativeRecordMenuAllBookmarks();
    private static native void nativeRecordMenuOpenTabs();
    private static native void nativeRecordMenuAddToBookmarks();
    private static native void nativeRecordMenuShare();
    private static native void nativeRecordMenuFindInPage();
    private static native void nativeRecordMenuFullscreen();
    private static native void nativeRecordMenuSettings();
    private static native void nativeRecordMenuFeedback();
    private static native void nativeRecordMenuShow();

    // Toolbar and Tabstrip
    private static native void nativeRecordToolbarPhoneNewTab();
    private static native void nativeRecordToolbarPhoneTabStack();
    private static native void nativeRecordToolbarToggleBookmark();
    private static native void nativeRecordToolbarShowMenu();
    private static native void nativeRecordTabStripNewTab();
    private static native void nativeRecordTabStripCloseTab();

    // Misc
    private static native void nativeRecordStackViewNewTab();
    private static native void nativeRecordStackViewCloseTab();
    private static native void nativeRecordStackViewSwipeCloseTab();
    private static native void nativeRecordOmniboxSearch();
    private static native void nativeRecordOmniboxVoiceSearch();
    private static native void nativeRecordTabClobbered();
    private static native void nativeRecordTabSwitched();
    private static native void nativeRecordPageLoaded(int numTabsOpen, int numIncognitoTabsOpen,
        boolean isDesktopUserAgent);
    private static native void nativeRecordSideSwipeFinished();
    private static native void nativeRecordPageLoadedWithKeyboard();
    private static native void nativeRecordReceivedExternalIntent();

    // NTP
    private static native void nativeRecordNtpSectionMostVisited();
    private static native void nativeRecordNtpSectionBookmarks();
    private static native void nativeRecordNtpSectionOpenTabs();
    private static native void nativeRecordNtpSectionIncognito();

    // Context menu
    private static native void nativeRecordContextMenuLink();
    private static native void nativeRecordContextMenuImage();
    private static native void nativeRecordContextMenuText();
    private static native void nativeRecordContextMenuOpenNewTab();
    private static native void nativeRecordContextMenuOpenNewIncognitoTab();
    private static native void nativeRecordContextMenuViewImage();
    private static native void nativeRecordContextMenuCopyLinkAddress();
    private static native void nativeRecordContextMenuCopyLinkText();
    private static native void nativeRecordContextMenuSaveImage();
    private static native void nativeRecordContextMenuOpenImageInNewTab();

    // First Run Experience
    private static native void nativeRecordFreSignInAttempted();
    private static native void nativeRecordFreSignInSuccessful();
    private static native void nativeRecordFreSkipSignIn();

    // Android Beam
    private static native void nativeRecordBeamInvalidAppState();
    private static native void nativeRecordBeamCallbackSuccess();

    //Keyboard Shortcuts
    private static native void nativeRecordShortcutNewTab();
    private static native void nativeRecordShortcutNewIncognitoTab();
    private static native void nativeRecordShortcutAllBookmarks();
    private static native void nativeRecordShortcutFindInPage();

    // Crash uploads
    private static native void nativeRecordCrashUploadAttempt();
}
