// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLANK_NATIVE_FRAMEWORK_CHROME_PAGE_INFO_VIEWER_H_
#define CLANK_NATIVE_FRAMEWORK_CHROME_PAGE_INFO_VIEWER_H_

#include "content/browser/android/jni_helper.h"

namespace content {
class WebContents;
}

class PageInfoViewer {
 public:
  static void Show(JNIEnv* env,
                   jobject context,
                   jobject java_chrome_view,
                   content::WebContents* web_contents);

 private:
  DISALLOW_COPY_AND_ASSIGN(PageInfoViewer);
};

#endif  // CLANK_NATIVE_FRAMEWORK_CHROME_PAGE_INFO_VIEWER_H_
