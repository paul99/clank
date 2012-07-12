// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/login/auto_login.h"

#include "base/bind.h"
#include "chrome/browser/tab_contents/auto_login_infobar_delegate.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace auto_login {

// We don't include chrome/browser/tab_contents headers in
// content/browser/renderer_host files, since that appears wrong in
// the dep graph.  Thus, we have a redirection through
// chrome/browser/ui/login to get to an infobar.  This is similar to
// the redirect through CreateLoginPrompt() in login_prompt.h.
void CreateAutoLogin(int render_process_id, int tab_contents_id,
                     const std::string& header_value) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&XAutoLoginInfoBarDelegate::Create,
                        render_process_id, tab_contents_id,
                        header_value));
}

}  // namespace auto_login
