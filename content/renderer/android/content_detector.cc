// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/android/content_detector.h"

#include "third_party/WebKit/Source/WebKit/chromium/public/android/WebDOMTextContentWalker.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/android/WebHitTestInfo.h"

using WebKit::WebDOMTextContentWalker;
using WebKit::WebRange;

ContentDetector::Result ContentDetector::FindTappedContent(
    const WebKit::WebHitTestInfo& hit_test) {
  std::string content_text;
  WebKit::WebRange range = FindContentRange(hit_test, &content_text);
  if (range.isNull())
    return Result();

  GURL intent_url = GetIntentURL(content_text);
  return Result(range, content_text, intent_url);
}

WebRange ContentDetector::FindContentRange(
    const WebKit::WebHitTestInfo& hit_test, std::string* content_text) {
  // As the content walker looks at maxLength/2 characters on either side of
  // the hit point, we need to double max content length here.
  WebDOMTextContentWalker content_walker(hit_test,
                                         GetMaximumContentLength() * 2);
  string16 content = content_walker.content();
  if (content.empty())
    return WebRange();

  size_t selected_offset = content_walker.hitOffsetInContent();
  for (size_t start_offset = 0; start_offset < content.length();) {
    size_t relative_start, relative_end;
    if (!FindContent(content.begin() + start_offset,
        content.end(), &relative_start, &relative_end, content_text)) {
      break;
    } else {
      size_t content_start = start_offset + relative_start;
      size_t content_end = start_offset + relative_end;
      DCHECK(content_end <= content.length());

      if (selected_offset >= content_start && selected_offset < content_end) {
        WebRange range = content_walker.contentOffsetsToRange(
            content_start, content_end);
        DCHECK(!range.isNull());
        return range;
      } else {
        start_offset += relative_end;
      }
    }
  }

  return WebRange();
}
