// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ANDROID_CONTENT_DETECTOR_H_
#define CONTENT_RENDERER_ANDROID_CONTENT_DETECTOR_H_
#pragma once

#include "build/build_config.h"  // Needed for OS_ANDROID

#if defined(OS_ANDROID)

#include "base/string_util.h"
#include "googleurl/src/gurl.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebRange.h"

namespace WebKit {
class WebHitTestInfo;
}

// Base class for text-based content detectors.
class ContentDetector {
 public:

  // Holds the content detection results.
  struct Result {
    bool valid; // Flag indicating if the result is valid.
    WebKit::WebRange range; // Range describing the content boundaries.
    std::string text; // Processed text of the content.
    GURL intent_url; // URL of the intent that should process this content.

    Result() : valid(false) {}

    Result(const WebKit::WebRange& range,
           const std::string& text,
           const GURL& intent_url)
        : valid(true),
          range(range),
          text(text),
          intent_url(intent_url) {}
  };

  virtual ~ContentDetector() {}

  // Returns a WebKit range delimiting the contents found around the tapped
  // position. If no content is found a null range will be returned.
  Result FindTappedContent(const WebKit::WebHitTestInfo& hit_test);

 protected:
  // Parses the input string defined by the begin/end iterators returning true
  // if the desired content is found. The start and end positions relative to
  // the input iterators are returned in start_pos and end_pos.
  // The end position is assumed to be non-inclusive.
  virtual bool FindContent(const string16::const_iterator& begin,
                           const string16::const_iterator& end,
                           size_t* start_pos,
                           size_t* end_pos,
                           std::string* content_text) = 0;

  // Returns the intent URL that should process the content, if any.
  virtual GURL GetIntentURL(const std::string& content_text) = 0;

  // Returns the maximum length of text to be extracted around the tapped
  // position in order to search for content.
  virtual size_t GetMaximumContentLength() = 0;

  ContentDetector() {}
  WebKit::WebRange FindContentRange(const WebKit::WebHitTestInfo& hit_test,
                                    std::string* content_text);

  DISALLOW_COPY_AND_ASSIGN(ContentDetector);
};

#endif  // defined(OS_ANDROID)

#endif  // CONTENT_RENDERER_ANDROID_CONTENT_DETECTOR_H_
