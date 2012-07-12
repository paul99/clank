// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_FAVICON_BG_COLOR_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_FAVICON_BG_COLOR_HANDLER_H_
#pragma once

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/favicon/favicon_service.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "third_party/skia/include/core/SkColor.h"

namespace base {
class ListValue;
}
class Profile;

// The handler for Javascript messages related to grabbing recommended
// favicon background colors.
//
// In Javascript if getRecommendedFaviconBgColor() is called with a string url
// and an integer id as parameters, the recommended background color for that
// favicon is returned through a call to recommendedFaviconBgColor().
//
// Examples:
// Calling into this handler from Javascript:
// chrome.send('getRecommendedFaviconBgColor', [ 'http://www.google.com', 2 ]);
//
// Handling the returning call in Javascript:
// function recommendedFaviconBgColor(index, color) {
//   var e = document.getElementById('some_prefix_' + index);
//   if (e) {
//     e.style.backgroundColor = color;
//   }
// }
class FaviconBgColorHandler : public content::WebUIMessageHandler {
 public:
  FaviconBgColorHandler(Profile* profile);
  virtual ~FaviconBgColorHandler();

  // WebUIMessageHandler override and implementation.
  virtual void RegisterMessages() OVERRIDE;

  void HandleGetRecommendedBgColor(const base::ListValue* args);

 private:

  // Kept so we can access the FaviconService that is owned by the profile.
  Profile* profile_;

  // Tracks each id with the corresponding FaviconService request
  // so that the Javascript can use the id to match the resulting color
  // with a specific request to this handler.
  CancelableRequestConsumerT<int, 0> cancelable_consumer_;

  // Sends the resulting |color| with the passed in |id| to Javascript.
  void SendBackColor(int id, SkColor color);

  // The callback that gets triggered when the FaviconService has data
  // available.
  void FaviconImageDataAvailable(FaviconService::Handle request_handle,
                                 history::FaviconData favicon);

  DISALLOW_COPY_AND_ASSIGN(FaviconBgColorHandler);
};
#endif  // CHROME_BROWSER_UI_WEBUI_FAVICON_BG_COLOR_HANDLER_H_
