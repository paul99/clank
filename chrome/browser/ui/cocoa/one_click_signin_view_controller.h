// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_ONE_CLICK_SIGNIN_VIEW_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_ONE_CLICK_SIGNIN_VIEW_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/callback.h"
#include "base/memory/scoped_nsobject.h"
#include "chrome/browser/ui/browser_window.h"

@class BrowserWindowController;
namespace content {
class WebContents;
}
@class HyperlinkTextView;

// View controller for the one-click signin confirmation UI.
@interface OneClickSigninViewController : NSViewController<NSTextViewDelegate> {
 @private
  IBOutlet NSTextField* messageTextField_;
  IBOutlet NSTextField* informativePlaceholderTextField_;
  IBOutlet NSButton* advancedLink_;
  IBOutlet NSButton* closeButton_;

  // Text fields don't work as well with embedded links as text views, but
  // text views cannot conveniently be created in IB. The xib file contains
  // a text field |informativePlaceholderTextField_| that's replaced by this
  // text view |promo_| in -awakeFromNib.
  scoped_nsobject<HyperlinkTextView> informativeTextView_;
  BrowserWindow::StartSyncCallback startSyncCallback_;
  base::Closure closeCallback_;
  content::WebContents* webContents_;
}

- (id)initWithNibName:(NSString*)nibName
          webContents:(content::WebContents*)webContents
         syncCallback:(const BrowserWindow::StartSyncCallback&)syncCallback
        closeCallback:(const base::Closure&)callback;

// Called before the view is closed.
- (void)viewWillClose;

// Starts sync and closes the bubble.
- (IBAction)ok:(id)sender;

- (IBAction)onClickUndo:(id)sender;

// Calls |advancedCallback_|.
- (IBAction)onClickAdvancedLink:(id)sender;

@end

#endif  // CHROME_BROWSER_UI_COCOA_ONE_CLICK_SIGNIN_VIEW_CONTROLLER_H_
