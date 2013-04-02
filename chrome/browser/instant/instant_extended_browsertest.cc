// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/instant/instant_commit_type.h"
#include "chrome/browser/instant/instant_ntp.h"
#include "chrome/browser/instant/instant_overlay.h"
#include "chrome/browser/instant/instant_service.h"
#include "chrome/browser/instant/instant_service_factory.h"
#include "chrome/browser/instant/instant_test_utils.h"
#include "chrome/browser/ui/search/search.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"

class InstantExtendedTest : public InstantTestBase {
 protected:
  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE {
    chrome::search::EnableInstantExtendedAPIForTesting();
    ASSERT_TRUE(https_test_server_.Start());
    instant_url_ = https_test_server_.
        GetURL("files/instant_extended.html?strk=1&");
  }

  void FocusOmniboxAndWaitForInstantSupport() {
    content::WindowedNotificationObserver ntp_observer(
        chrome::NOTIFICATION_INSTANT_NTP_SUPPORT_DETERMINED,
        content::NotificationService::AllSources());
    content::WindowedNotificationObserver overlay_observer(
        chrome::NOTIFICATION_INSTANT_OVERLAY_SUPPORT_DETERMINED,
        content::NotificationService::AllSources());
    FocusOmnibox();
    ntp_observer.Wait();
    overlay_observer.Wait();
  }

  std::string GetOmniboxText() {
    return UTF16ToUTF8(omnibox()->GetText());
  }

  void SendDownArrow() {
    omnibox()->model()->OnUpOrDownKeyPressed(1);
    // Wait for JavaScript to run the key handler by executing a blank script.
    EXPECT_TRUE(ExecuteScript(std::string()));
  }

  void SendUpArrow() {
    omnibox()->model()->OnUpOrDownKeyPressed(-1);
    // Wait for JavaScript to run the key handler by executing a blank script.
    EXPECT_TRUE(ExecuteScript(std::string()));
  }
};

IN_PROC_BROWSER_TEST_F(InstantExtendedTest, ExtendedModeIsOn) {
  ASSERT_NO_FATAL_FAILURE(SetupInstant());
  EXPECT_TRUE(instant()->extended_enabled_);
}

// Test that Instant is preloaded when the omnibox is focused.
IN_PROC_BROWSER_TEST_F(InstantExtendedTest, OmniboxFocusLoadsInstant) {
  ASSERT_NO_FATAL_FAILURE(SetupInstant());

  // Explicitly unfocus the omnibox.
  EXPECT_TRUE(ui_test_utils::BringBrowserWindowToFront(browser()));
  ui_test_utils::ClickOnView(browser(), VIEW_ID_TAB_CONTAINER);

  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_TAB_CONTAINER));
  EXPECT_FALSE(omnibox()->model()->has_focus());

  // Delete any existing preview.
  instant()->overlay_.reset();
  EXPECT_FALSE(instant()->GetPreviewContents());

  // Refocus the omnibox. The InstantController should've preloaded Instant.
  FocusOmniboxAndWaitForInstantSupport();

  EXPECT_FALSE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_TAB_CONTAINER));
  EXPECT_TRUE(omnibox()->model()->has_focus());

  content::WebContents* preview_tab = instant()->GetPreviewContents();
  EXPECT_TRUE(preview_tab);

  // Check that the page supports Instant, but it isn't showing.
  EXPECT_TRUE(instant()->overlay_->supports_instant());
  EXPECT_FALSE(instant()->IsPreviewingSearchResults());
  EXPECT_TRUE(instant()->model()->mode().is_default());

  // Adding a new tab shouldn't delete or recreate the preview; otherwise,
  // what's the point of preloading?
  AddBlankTabAndShow(browser());
  EXPECT_EQ(preview_tab, instant()->GetPreviewContents());

  // Unfocusing and refocusing the omnibox should also preserve the preview.
  ui_test_utils::ClickOnView(browser(), VIEW_ID_TAB_CONTAINER);
  EXPECT_TRUE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_TAB_CONTAINER));

  FocusOmnibox();
  EXPECT_FALSE(ui_test_utils::IsViewFocused(browser(), VIEW_ID_TAB_CONTAINER));
  EXPECT_EQ(preview_tab, instant()->GetPreviewContents());
}

IN_PROC_BROWSER_TEST_F(InstantExtendedTest, InputShowsOverlay) {
  ASSERT_NO_FATAL_FAILURE(SetupInstant());

  // Focus omnibox and confirm overlay isn't shown.
  FocusOmniboxAndWaitForInstantSupport();
  content::WebContents* preview_tab = instant()->GetPreviewContents();
  EXPECT_TRUE(preview_tab);
  EXPECT_FALSE(instant()->IsPreviewingSearchResults());
  EXPECT_TRUE(instant()->model()->mode().is_default());

  // Typing in the omnibox should show the overlay.
  SetOmniboxTextAndWaitForInstantToShow("query");
  EXPECT_TRUE(instant()->model()->mode().is_search_suggestions());
  EXPECT_EQ(preview_tab, instant()->GetPreviewContents());
}

// Test that omnibox text is correctly set when overlay is committed with Enter.
IN_PROC_BROWSER_TEST_F(InstantExtendedTest, OmniboxTextUponEnterCommit) {
  ASSERT_NO_FATAL_FAILURE(SetupInstant());
  FocusOmniboxAndWaitForInstantSupport();

  // The page will autocomplete once we set the omnibox value.
  EXPECT_TRUE(ExecuteScript("suggestion = 'santa claus';"));

  // Set the text, and wait for suggestions to show up.
  SetOmniboxTextAndWaitForInstantToShow("santa");
  EXPECT_EQ(ASCIIToUTF16("santa"), omnibox()->GetText());

  // Test that the current suggestion is correctly set.
  EXPECT_EQ(ASCIIToUTF16(" claus"), omnibox()->GetInstantSuggestion());

  // Commit the search by pressing Enter.
  browser()->window()->GetLocationBar()->AcceptInput();

  // 'Enter' commits the query as it was typed.
  EXPECT_EQ(ASCIIToUTF16("santa"), omnibox()->GetText());

  // Suggestion should be cleared at this point.
  EXPECT_EQ(ASCIIToUTF16(""), omnibox()->GetInstantSuggestion());
}

// Test that omnibox text is correctly set when overlay is committed with focus
// lost.
IN_PROC_BROWSER_TEST_F(InstantExtendedTest, OmniboxTextUponFocusLostCommit) {
  ASSERT_NO_FATAL_FAILURE(SetupInstant());
  FocusOmniboxAndWaitForInstantSupport();

  // Set autocomplete text (grey text).
  EXPECT_TRUE(ExecuteScript("suggestion = 'johnny depp';"));

  // Set the text, and wait for suggestions to show up.
  SetOmniboxTextAndWaitForInstantToShow("johnny");
  EXPECT_EQ(ASCIIToUTF16("johnny"), omnibox()->GetText());

  // Test that the current suggestion is correctly set.
  EXPECT_EQ(ASCIIToUTF16(" depp"), omnibox()->GetInstantSuggestion());

  // Commit the overlay by lost focus (e.g. clicking on the page).
  instant()->CommitIfPossible(INSTANT_COMMIT_FOCUS_LOST);

  // Search term extraction should kick in with the autocompleted text.
  EXPECT_EQ(ASCIIToUTF16("johnny depp"), omnibox()->GetText());

  // Suggestion should be cleared at this point.
  EXPECT_EQ(ASCIIToUTF16(""), omnibox()->GetInstantSuggestion());
}

// This test simulates a search provider using the InstantExtended API to
// navigate through the suggested results and back to the original user query.
IN_PROC_BROWSER_TEST_F(InstantExtendedTest, NavigateSuggestionsWithArrowKeys) {
  ASSERT_NO_FATAL_FAILURE(SetupInstant());
  FocusOmniboxAndWaitForInstantSupport();

  SetOmniboxTextAndWaitForInstantToShow("hello");
  EXPECT_EQ("hello", GetOmniboxText());

  SendDownArrow();
  EXPECT_EQ("result 1", GetOmniboxText());
  SendDownArrow();
  EXPECT_EQ("result 2", GetOmniboxText());
  SendUpArrow();
  EXPECT_EQ("result 1", GetOmniboxText());
  SendUpArrow();
  EXPECT_EQ("hello", GetOmniboxText());
}

IN_PROC_BROWSER_TEST_F(InstantExtendedTest, NTPIsPreloaded) {
  // Setup Instant.
  ASSERT_NO_FATAL_FAILURE(SetupInstant());
  FocusOmniboxAndWaitForInstantSupport();

  // NTP contents should be preloaded.
  ASSERT_NE(static_cast<InstantNTP*>(NULL), instant()->ntp());
  content::WebContents* ntp_contents = instant()->ntp_->contents();
  EXPECT_TRUE(ntp_contents);
}

IN_PROC_BROWSER_TEST_F(InstantExtendedTest, PreloadedNTPIsUsedInNewTab) {
  // Setup Instant.
  ASSERT_NO_FATAL_FAILURE(SetupInstant());
  FocusOmniboxAndWaitForInstantSupport();

  // NTP contents should be preloaded.
  ASSERT_NE(static_cast<InstantNTP*>(NULL), instant()->ntp());
  content::WebContents* ntp_contents = instant()->ntp_->contents();
  EXPECT_TRUE(ntp_contents);

  // Open new tab. Preloaded NTP contents should have been used.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      GURL(chrome::kChromeUINewTabURL),
      NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB);
  content::WebContents* active_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(ntp_contents, active_tab);
}

IN_PROC_BROWSER_TEST_F(InstantExtendedTest, PreloadedNTPIsUsedInSameTab) {
  // Setup Instant.
  ASSERT_NO_FATAL_FAILURE(SetupInstant());
  FocusOmniboxAndWaitForInstantSupport();

  // NTP contents should be preloaded.
  ASSERT_NE(static_cast<InstantNTP*>(NULL), instant()->ntp());
  content::WebContents* ntp_contents = instant()->ntp_->contents();
  EXPECT_TRUE(ntp_contents);

  // Open new tab. Preloaded NTP contents should have been used.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      GURL(chrome::kChromeUINewTabURL),
      CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_NONE);
  content::WebContents* active_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(ntp_contents, active_tab);
}

IN_PROC_BROWSER_TEST_F(InstantExtendedTest, OmniboxHasFocusOnNewTab) {
  // Setup Instant.
  ASSERT_NO_FATAL_FAILURE(SetupInstant());
  FocusOmniboxAndWaitForInstantSupport();

  // Explicitly unfocus the omnibox.
  EXPECT_TRUE(ui_test_utils::BringBrowserWindowToFront(browser()));
  ui_test_utils::ClickOnView(browser(), VIEW_ID_TAB_CONTAINER);
  EXPECT_FALSE(omnibox()->model()->has_focus());

  // Open new tab. Preloaded NTP contents should have been used.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      GURL(chrome::kChromeUINewTabURL),
      NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB);

  // Omnibox should have focus.
  EXPECT_TRUE(omnibox()->model()->has_focus());
}

IN_PROC_BROWSER_TEST_F(InstantExtendedTest, OmniboxEmptyOnNewTabPage) {
  // Setup Instant.
  ASSERT_NO_FATAL_FAILURE(SetupInstant());
  FocusOmniboxAndWaitForInstantSupport();

  // Open new tab. Preloaded NTP contents should have been used.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      GURL(chrome::kChromeUINewTabURL),
      CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_NONE);

  // Omnibox should be empty.
  EXPECT_TRUE(omnibox()->GetText().empty());
}

IN_PROC_BROWSER_TEST_F(InstantExtendedTest, InputOnNTPDoesntShowOverlay) {
  ASSERT_NO_FATAL_FAILURE(SetupInstant());

  // Focus omnibox and confirm overlay isn't shown.
  FocusOmniboxAndWaitForInstantSupport();
  content::WebContents* preview_tab = instant()->GetPreviewContents();
  EXPECT_TRUE(preview_tab);
  EXPECT_FALSE(instant()->IsPreviewingSearchResults());
  EXPECT_TRUE(instant()->model()->mode().is_default());

  // Navigate to the NTP.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      GURL(chrome::kChromeUINewTabURL),
      CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_NONE);

  // Typing in the omnibox should not show the overlay.
  SetOmniboxText("query");
  EXPECT_FALSE(instant()->IsPreviewingSearchResults());
  EXPECT_TRUE(instant()->model()->mode().is_default());
}

IN_PROC_BROWSER_TEST_F(InstantExtendedTest, ProcessIsolation) {
  // Prior to setup no render process is dedicated to Instant.
  InstantService* instant_service =
        InstantServiceFactory::GetForProfile(browser()->profile());
  ASSERT_NE(static_cast<InstantService*>(NULL), instant_service);
  EXPECT_EQ(0, instant_service->GetInstantProcessCount());

  // Setup Instant.
  ASSERT_NO_FATAL_FAILURE(SetupInstant());
  FocusOmniboxAndWaitForInstantSupport();

  // Now there should be a registered Instant render process.
  EXPECT_LT(0, instant_service->GetInstantProcessCount());

  // And the Instant overlay and ntp should live inside it.
  content::WebContents* preview = instant()->GetPreviewContents();
  EXPECT_TRUE(instant_service->IsInstantProcess(
      preview->GetRenderProcessHost()->GetID()));
  content::WebContents* ntp_contents = instant()->ntp_->contents();
  EXPECT_TRUE(instant_service->IsInstantProcess(
      ntp_contents->GetRenderProcessHost()->GetID()));

  // Navigating to the NTP should use the Instant render process.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(),
      GURL(chrome::kChromeUINewTabURL),
      CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_NONE);
  content::WebContents* active_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(instant_service->IsInstantProcess(
      active_tab->GetRenderProcessHost()->GetID()));

  // Navigating elsewhere should not use the Instant render process.
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUIAboutURL));
  EXPECT_FALSE(instant_service->IsInstantProcess(
      active_tab->GetRenderProcessHost()->GetID()));
}

// Verification of fix for BUG=176365.  Ensure that each Instant WebContents in
// a tab uses a new BrowsingInstance, to avoid conflicts in the
// NavigationController.
IN_PROC_BROWSER_TEST_F(InstantExtendedTest, UnrelatedSiteInstance) {
  // Setup Instant.
  ASSERT_NO_FATAL_FAILURE(SetupInstant());
  FocusOmniboxAndWaitForInstantSupport();

  // Check that the uncommited ntp page and uncommited preview have unrelated
  // site instances.
  // TODO(sreeram): |ntp_| is going away, so this check can be removed in the
  // future.
  content::WebContents* preview = instant()->GetPreviewContents();
  content::WebContents* ntp_contents = instant()->ntp_->contents();
  EXPECT_FALSE(preview->GetSiteInstance()->IsRelatedSiteInstance(
      ntp_contents->GetSiteInstance()));

  // Type a query and hit enter to get a results page.  The preview becomes the
  // active tab.
  SetOmniboxTextAndWaitForInstantToShow("hello");
  EXPECT_EQ("hello", GetOmniboxText());
  browser()->window()->GetLocationBar()->AcceptInput();
  content::WebContents* first_active_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(first_active_tab, preview);
  scoped_refptr<content::SiteInstance> first_site_instance =
      first_active_tab->GetSiteInstance();
  EXPECT_FALSE(first_site_instance->IsRelatedSiteInstance(
      ntp_contents->GetSiteInstance()));

  // Navigating elsewhere gets us off of the commited page.  The next
  // query will give us a new |preview| which we will then commit.
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUIAboutURL));

  // Show and commit the new preview.
  SetOmniboxTextAndWaitForInstantToShow("hello again");
  EXPECT_EQ("hello again", GetOmniboxText());
  browser()->window()->GetLocationBar()->AcceptInput();
  content::WebContents* second_active_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_NE(first_active_tab, second_active_tab);
  scoped_refptr<content::SiteInstance> second_site_instance =
      second_active_tab->GetSiteInstance();
  EXPECT_NE(first_site_instance, second_site_instance);
  EXPECT_FALSE(first_site_instance->IsRelatedSiteInstance(
      second_site_instance));
}
