// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/web_contents_modal_dialog.h"
#include "chrome/browser/ui/web_contents_modal_dialog_manager.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/test/test_browser_thread.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::BrowserThread;

class WebContentsModalDialogManagerTest
    : public ChromeRenderViewHostTestHarness {
 public:
  WebContentsModalDialogManagerTest()
      : ChromeRenderViewHostTestHarness(),
        ui_thread_(BrowserThread::UI, &message_loop_) {
  }

  virtual void SetUp() {
    ChromeRenderViewHostTestHarness::SetUp();
    WebContentsModalDialogManager::CreateForWebContents(web_contents());
  }

 private:
  content::TestBrowserThread ui_thread_;
};

class WebContentsModalDialogCloseTest : public WebContentsModalDialog {
 public:
  explicit WebContentsModalDialogCloseTest(content::WebContents* web_contents)
      : web_contents_(web_contents) {
  }

  virtual void ShowWebContentsModalDialog() OVERRIDE {}
  virtual void CloseWebContentsModalDialog() OVERRIDE {
    WebContentsModalDialogManager* web_contents_modal_dialog_manager =
        WebContentsModalDialogManager::FromWebContents(web_contents_);
    web_contents_modal_dialog_manager->WillClose(this);
    close_count++;
  }
  virtual void FocusWebContentsModalDialog() OVERRIDE {}
  virtual void PulseWebContentsModalDialog() OVERRIDE {}
  virtual gfx::NativeWindow GetNativeWindow() OVERRIDE {
    NOTREACHED();
    return NULL;
  }
  virtual ~WebContentsModalDialogCloseTest() {}

  int close_count;
  content::WebContents* web_contents_;
};

TEST_F(WebContentsModalDialogManagerTest, WebContentsModalDialogs) {
  WebContentsModalDialogCloseTest window(web_contents());
  window.close_count = 0;
  WebContentsModalDialogManager* web_contents_modal_dialog_manager =
      WebContentsModalDialogManager::FromWebContents(web_contents());
  WebContentsModalDialogManager::TestApi test_api(
      web_contents_modal_dialog_manager);

  test_api.ResetNativeManager(NULL);

  const int kWindowCount = 4;
  for (int i = 0; i < kWindowCount; i++)
    web_contents_modal_dialog_manager->AddDialog(&window);
  EXPECT_EQ(window.close_count, 0);

  test_api.CloseAllDialogs();
  EXPECT_EQ(window.close_count, kWindowCount);
}
