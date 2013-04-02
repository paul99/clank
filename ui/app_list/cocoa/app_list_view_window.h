// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

@interface AppListViewWindow : NSWindow {
}

- (id)initAsBubble;

- (void)setAppListView:(NSView*)view;

@end
